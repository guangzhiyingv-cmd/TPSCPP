#include "Components/LedgePresentationComponent.h"

#include "Character/TPSCPPCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "PrimitiveSceneDesc.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

static TAutoConsoleVariable<int32> CVarShoulderStepDown(
	TEXT("tps.ShoulderStepDown"), 1, TEXT("Smooth grounded step downs during shoulder aim (0 or 1)."));

// Submit absolute draw transforms after the normal end-of-frame component updates.
// Collision bodies, retargeting inputs, sockets and authoritative weapon traces keep their true positions.
class FLedgeViewExtension final : public FWorldSceneViewExtension
{
public:
	FLedgeViewExtension(const FAutoRegister& AutoRegister, UWorld* World, ATPSCPPCharacter* InCharacter)
		: FWorldSceneViewExtension(AutoRegister, World), Character(InCharacter) {}

	float Height = 0.f;

	void SetupViewFamily(FSceneViewFamily&) override {}
	void SetupView(FSceneViewFamily&, FSceneView&) override {}

	void BeginRenderViewFamily(FSceneViewFamily& Family) override
	{
		USkeletalMeshComponent* Body = Character.IsValid() ? Character->GetCustomMesh() : nullptr;
		TArray<TWeakObjectPtr<USkinnedMeshComponent>> Current;
		if (Body && Height != 0.f && Family.Scene == GetWorld()->Scene)
		{
			auto Submit = [&](USkinnedMeshComponent* Mesh)
			{
				if (!Mesh || !Mesh->IsRegistered() || !Mesh->GetSceneProxy()) return;
				FPrimitiveSceneDesc Draw;
				Draw.PrimitiveUObject = Mesh;
				Draw.PrimitiveSceneData = &Mesh->GetSceneData();
				Draw.RenderMatrix = Mesh->GetRenderMatrix();
				const FVector Offset(0., 0., Height);
				Draw.RenderMatrix.SetOrigin(Draw.RenderMatrix.GetOrigin() + Offset);
				Draw.Bounds = Mesh->Bounds;
				Draw.Bounds.Origin += Offset;
				Draw.LocalBounds = Mesh->GetLocalBounds();
				Draw.AttachmentRootPosition = Mesh->GetActorPositionForRenderer() + Offset;
				Family.Scene->UpdatePrimitiveTransform(&Draw);
				Current.Add(Mesh);
			};
			Submit(Body);
			TArray<USceneComponent*> Children;
			Body->GetChildrenComponents(true, Children);
			for (USceneComponent* Child : Children) Submit(Cast<USkinnedMeshComponent>(Child));
		}
		for (const auto& Previous : Presented)
		{
			if (!Current.Contains(Previous)) Restore(Previous.Get());
		}
		Presented = MoveTemp(Current);
	}

	void Clear()
	{
		Height = 0.f;
		for (const auto& Previous : Presented) Restore(Previous.Get());
		Presented.Reset();
	}

protected:
	bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
	{
		return Height != 0.f && FWorldSceneViewExtension::IsActiveThisFrame_Internal(Context);
	}

private:
	static void Restore(USkinnedMeshComponent* Mesh)
	{
		if (Mesh && Mesh->IsRegistered()) Mesh->MarkRenderTransformDirty();
	}
	TWeakObjectPtr<ATPSCPPCharacter> Character;
	TArray<TWeakObjectPtr<USkinnedMeshComponent>> Presented;
};

void ULedgePresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = CastChecked<ATPSCPPCharacter>(GetOwner());
	if (GetNetMode() != NM_DedicatedServer)
	{
		Character->OnCharacterMovementUpdated.AddDynamic(this, &ThisClass::OnMovementUpdated);
		Character->GetCameraBoom()->AddTickPrerequisiteComponent(Character->GetCharacterMovement());
	}
}

void ULedgePresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Character)
	{
		Character->OnCharacterMovementUpdated.RemoveDynamic(this, &ThisClass::OnMovementUpdated);
		StanceOffset = 0.f;
		SetHeight(0.f);
	}
	ViewExtension.Reset();
	Super::EndPlay(Reason);
}

void ULedgePresentationComponent::OnMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector)
{
	if (DeltaSeconds <= 0.f || Character->bClientUpdating) return;
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FFindFloorResult& Floor = Movement->CurrentFloor;
	const FVector Location = Character->GetActorLocation();
	const bool bGrounded = Movement->MovementMode == MOVE_Walking && Floor.IsWalkableFloor();
	const bool bEligible = bGrounded && Character->GetAimState() == EAimState::Shoulder
		&& RecoveryTime > 0.f && CVarShoulderStepDown.GetValueOnGameThread() != 0;
	const bool bContinuous = OldLocation.Equals(LastLocation, 0.1);
	const double Drop = OldLocation.Z - Location.Z;
	const double FloorZ = Floor.HitResult.ImpactPoint.Z;
	if (!bEligible || !bContinuous)
	{
		InitialHeight = 0.f;
	}
	else if (bWasGrounded && Drop >= MinimumDrop && Drop <= Movement->MaxStepHeight
		&& LastFloorZ - FloorZ >= MinimumDrop && Movement->Velocity.SizeSquared2D() > 1.)
	{
		InitialHeight = FMath::Min(PresentationHeight + float(Drop), Movement->MaxStepHeight);
		Elapsed = 0.f;
	}
	bWasGrounded = bEligible;
	LastLocation = Location;
	LastFloorZ = FloorZ;
	if (InitialHeight > 0.f)
	{
		Elapsed += DeltaSeconds;
		const float Alpha = FMath::Min(Elapsed / RecoveryTime, 1.f);
		SetHeight(InitialHeight * (1.f - FMath::SmoothStep(0.f, 1.f, Alpha)));
		if (Alpha >= 1.f) InitialHeight = 0.f;
	}
	else if (PresentationHeight > 0.f)
	{
		SetHeight(0.f);
	}
}

void ULedgePresentationComponent::SetHeight(float Height)
{
	PresentationHeight = Height;
	const float CameraHeight = Character->IsLocallyControlled() ? Height : 0.f;
	Character->GetCameraBoom()->TargetOffset.Z += CameraHeight - AppliedCameraHeight;
	AppliedCameraHeight = CameraHeight;
	UpdateDrawHeight();
}

void ULedgePresentationComponent::SetStanceOffset(float Offset)
{
	if (StanceOffset != Offset)
	{
		StanceOffset = Offset;
		UpdateDrawHeight();
	}
}

void ULedgePresentationComponent::UpdateDrawHeight()
{
	const float Height = PresentationHeight + StanceOffset;
	if (Height != 0.f)
	{
		if (!ViewExtension)
		{
			ViewExtension = FSceneViewExtensions::NewExtension<FLedgeViewExtension>(GetWorld(), Character.Get());
		}
		ViewExtension->Height = Height;
	}
	else if (ViewExtension)
	{
		ViewExtension->Clear();
	}
}
