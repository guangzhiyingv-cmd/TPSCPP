#include "Animation/IrisLandingPose.h"

#include "Character/TPSCPPCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"

namespace
{
TAutoConsoleVariable<int32> CVarShortFallReach(TEXT("tps.IrisShortFallReach"), 0,
	TEXT("Apply Iris shoulder-aim short-fall contact and preserve the two grip transforms."));

// A rigid two-bone chain. End orientation is preserved unless the landing shoe rolls onto its sole.
struct FLimb
{
	FCompactPoseBoneIndex EndIndex, MidIndex, RootIndex, ParentIndex;
	FTransform End, Mid, Root, Parent;
	FVector EndWorld, MidWorld, RootWorld;
	double Length;

	FLimb(FCompactPoseBoneIndex Tip, const FCompactPose& Output, FCSPose<FCompactPose>& Pose, const FTransform& Mesh)
		: EndIndex(Tip), MidIndex(Output.GetParentBoneIndex(Tip)), RootIndex(Output.GetParentBoneIndex(MidIndex)),
		  ParentIndex(Output.GetParentBoneIndex(RootIndex)), End(Pose.GetComponentSpaceTransform(Tip)),
		  Mid(Pose.GetComponentSpaceTransform(MidIndex)), Root(Pose.GetComponentSpaceTransform(RootIndex)),
		  Parent(Pose.GetComponentSpaceTransform(ParentIndex)), EndWorld(Mesh.TransformPosition(End.GetLocation())),
		  MidWorld(Mesh.TransformPosition(Mid.GetLocation())), RootWorld(Mesh.TransformPosition(Root.GetLocation())),
		  Length(FVector::Distance(EndWorld, MidWorld) + FVector::Distance(MidWorld, RootWorld)) {}

	void Solve(FCompactPose& Output, const FTransform& Mesh, const FVector& Delta)
	{
		AnimationCore::SolveTwoBoneIK(Root, Mid, End, Mid.GetLocation(),
			End.GetLocation() + Mesh.InverseTransformVector(Delta), false, 1., 1.);
		Output[RootIndex] = Root.GetRelativeTransform(Parent);
		Output[MidIndex] = Mid.GetRelativeTransform(Root);
		Output[EndIndex] = End.GetRelativeTransform(Mid);
	}
};
}

void FIrisLandingPose::Prepare(const ATPSCPPCharacter* Character, const FIrisShortFallSettings& InSettings, float DeltaSeconds)
{
	if (!Character || !CVarShortFallReach.GetValueOnGameThread())
	{
		if (bWasWalking || bTracking || Alpha > 0.) *this = {};
		return;
	}
	Settings = InSettings;
	const auto* Movement = Character->GetCharacterMovement();
	const bool bWalking = Movement->MovementMode == MOVE_Walking;
	const bool bFalling = Movement->IsFalling();
	const FVector Center = Character->GetActorLocation();
	const double HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const bool bContinuous = FVector::DistSquared(Center, LastCenter) <= FMath::Square(HalfHeight);
	const bool bBegin = bWasWalking && bFalling;
	bWasWalking = bWalking;
	LastCenter = Center;
	if (!bContinuous)
	{
		bTracking = false;
		Alpha = 0.;
		return;
	}
	if (bBegin)
	{
		// A new walk-off chooses the contact pose. Aim and camera transitions can continue during that step.
		bTracking = Character->GetAimState() == EAimState::Shoulder && Movement->Velocity.Z <= 0.;
		Support = INDEX_NONE;
		bPlanted = false;
		bHasBase = false;
		GroundTime = 0.;
	}
	if ((!bWalking && !bFalling) || (bFalling && Movement->Velocity.Z > 0.)) bTracking = false;
	if (!bTracking)
	{
		Alpha = FMath::Max(0., Alpha - DeltaSeconds / Settings.ReleaseTime);
		bHasBase = false;
		return;
	}
	bGrounded = bWalking;
	if (bFalling)
	{
		const double Gravity = -Movement->GetGravityZ();
		const double Speed = FMath::Max(0., -Movement->Velocity.Z);
		const double Depth = HalfHeight + Speed * Settings.MaximumFallTime + 0.5 * Gravity * FMath::Square(Settings.MaximumFallTime);
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IrisLanding), false, Character);
		if (Gravity <= 0. || !Character->GetWorld()->LineTraceSingleByChannel(Hit, Center,
			Center - FVector(0., 0., Depth + 1.), ECC_Visibility, Query) || !Movement->IsWalkable(Hit))
		{
			bTracking = false;
			Alpha = FMath::Max(0., Alpha - DeltaSeconds / Settings.ReleaseTime);
			return;
		}
		Gap = FMath::Max(0., Center.Z - HalfHeight - Hit.ImpactPoint.Z);
		const double TimeToLand = (FMath::Sqrt(Speed * Speed + 2. * Gravity * Gap) - Speed) / Gravity;
		if (bBegin && TimeToLand > Settings.MaximumFallTime)
		{
			bTracking = false;
			Alpha = 0.;
			return;
		}
		Alpha = 1. - FMath::SmoothStep(0., double(Settings.PreparationTime), TimeToLand);
		FloorPoint = Hit.ImpactPoint;
		FloorNormal = Hit.ImpactNormal;
	}
	else
	{
		const auto& Floor = Movement->CurrentFloor;
		const auto* CurrentBase = Floor.HitResult.GetComponent();
		bHasBase = CurrentBase && Floor.IsWalkableFloor();
		if (CurrentBase != Base.Get()) bPlanted = false;
		Base = CurrentBase;
		if (bHasBase)
		{
			BaseTransform = CurrentBase->GetComponentTransform();
			FloorPoint = Floor.HitResult.ImpactPoint;
			FloorNormal = Floor.HitResult.ImpactNormal;
		}
		GroundTime += DeltaSeconds;
		Alpha = FMath::Min(Alpha, 1. - FMath::SmoothStep(double(Settings.PlantTime),
			double(Settings.PlantTime + Settings.ReleaseTime), GroundTime));
		Gap = 0.;
		bTracking = Alpha > 0.;
	}
}

void FIrisLandingPose::Apply(FCompactPose& Output, FCSPose<FCompactPose>& Pose, const FTransform& Mesh,
	const FVector& HeelInAnkle, const FVector& ForefootInToe)
{
	const auto& Bones = Output.GetBoneContainer();
	auto Index = [&](FName Name) { return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(Name))); };
	const FCompactPoseBoneIndex Toes[] = { Index(TEXT("Left-toe")), Index(TEXT("Right-toe")) };
	const FCompactPoseBoneIndex Wrists[] = { Index(TEXT("Left-wrist")), Index(TEXT("Right-wrist")) };
	const auto Hips = Index(TEXT("Hips"));
	if (!Hips.IsValid() || !Toes[0].IsValid() || !Toes[1].IsValid() || !Wrists[0].IsValid() || !Wrists[1].IsValid()) return;
	FLimb Legs[] = { {Output.GetParentBoneIndex(Toes[0]), Output, Pose, Mesh}, {Output.GetParentBoneIndex(Toes[1]), Output, Pose, Mesh} };
	FLimb Arms[] = { {Wrists[0], Output, Pose, Mesh}, {Wrists[1], Output, Pose, Mesh} };
	FVector Heel[2], Fore[2];
	for (int32 I = 0; I < 2; ++I)
	{
		const FVector Side(I == 0 ? 1. : -1., 1., 1.);
		Heel[I] = Mesh.TransformPosition(Legs[I].End.TransformPosition(HeelInAnkle * Side));
		Fore[I] = Mesh.TransformPosition(Pose.GetComponentSpaceTransform(Toes[I]).TransformPosition(ForefootInToe * Side));
	}
	if (Support == INDEX_NONE)
	{
		Support = Mesh.TransformPosition(Pose.GetComponentSpaceTransform(Toes[0]).GetLocation()).Z
			< Mesh.TransformPosition(Pose.GetComponentSpaceTransform(Toes[1]).GetLocation()).Z ? 0 : 1;
	}
	if (bGrounded)
	{
		// Roll the landing shoe toward the local floor as weight arrives on the forefoot.
		const FVector Sole = (Fore[Support] - Heel[Support]).GetSafeNormal();
		const FVector Flat = FVector::VectorPlaneProject(Sole, FloorNormal).GetSafeNormal();
		if (!Flat.IsNearlyZero())
		{
			FLimb& Leg = Legs[Support];
			const FQuat Roll = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Sole, Flat),
				Alpha * FMath::SmoothStep(0., double(Settings.PlantTime), GroundTime));
			Leg.End.SetRotation(Mesh.GetRotation().Inverse() * Roll * Mesh.GetRotation() * Leg.End.GetRotation());
			Heel[Support] = Leg.EndWorld + Roll.RotateVector(Heel[Support] - Leg.EndWorld);
			Fore[Support] = Leg.EndWorld + Roll.RotateVector(Fore[Support] - Leg.EndWorld);
		}
	}
	auto Distance = [&](const FVector& P) { return FVector::DotProduct(P - FloorPoint, FloorNormal) / FloorNormal.Z; };
	const FLimb& Lead = Legs[Support];
	const double SupportGap = FMath::Min(Distance(Heel[Support]), Distance(Fore[Support]));
	// Leave a small knee bend and use the pelvis only for the height the leg cannot supply.
	double Needed = SupportGap - Gap + Lead.RootWorld.Z - Lead.EndWorld.Z - 0.96 * Lead.Length;
	for (const FLimb& Arm : Arms)
	{
		const double MaxVertical = FMath::Sqrt(FMath::Max(0., Arm.Length * Arm.Length - FVector::DistSquared2D(Arm.EndWorld, Arm.RootWorld)));
		Needed = FMath::Min(Needed, MaxVertical - (Arm.EndWorld.Z - Arm.RootWorld.Z));
	}
	const double Lowering = Alpha * FMath::Clamp(Needed, 0., double(Settings.MaximumPelvisDrop));
	if (Lowering > 0.)
	{
		const FVector Up(0., 0., Lowering);
		for (FLimb& Arm : Arms) Arm.Solve(Output, Mesh, Up);
		FTransform Pelvis = Pose.GetComponentSpaceTransform(Hips);
		Pelvis.AddToTranslation(-Mesh.InverseTransformVector(Up));
		Output[Hips] = Pelvis.GetRelativeTransform(Pose.GetComponentSpaceTransform(Output.GetParentBoneIndex(Hips)));
	}
	for (int32 I = 0; I < 2; ++I)
	{
		FLimb& Leg = Legs[I];
		const bool bHeel = Distance(Heel[I]) < Distance(Fore[I]);
		const FVector& Low = bHeel ? Heel[I] : Fore[I];
		const double Lower = FMath::Max(0., Distance(Low) - Lowering - Gap - (I == Support ? 0. : Settings.SwingFootClearance));
		const double Vertical = FMath::Clamp(Leg.RootWorld.Z - Leg.EndWorld.Z + Lower, -Leg.Length, 0.995 * Leg.Length);
		const double MaxHorizontal = FMath::Sqrt(FMath::Max(0., Leg.Length * Leg.Length - Vertical * Vertical));
		FVector Horizontal = Leg.EndWorld - Leg.RootWorld;
		Horizontal.Z = 0.;
		FVector Target = Leg.RootWorld + Horizontal.GetClampedToMaxSize(MaxHorizontal) - FVector(0., 0., Vertical);
		if (bGrounded && I == Support && bHasBase)
		{
			if (!bPlanted)
			{
				PlantPoint = BaseTransform.InverseTransformPosition(Low + Target - Leg.EndWorld);
				bPlantHeel = bHeel;
				bPlanted = true;
			}
			const FVector Marker = (bPlantHeel ? Heel[I] : Fore[I]) + Target - Leg.EndWorld;
			const FVector Anchor = BaseTransform.TransformPosition(PlantPoint);
			const FVector Slip = Anchor - Marker;
			Target.X += Slip.X;
			Target.Y += Slip.Y;
			Target.Z -= (Slip.X * FloorNormal.X + Slip.Y * FloorNormal.Y) / FloorNormal.Z;
		}
		Leg.Solve(Output, Mesh, Alpha * (Target - Leg.EndWorld));
	}
}
