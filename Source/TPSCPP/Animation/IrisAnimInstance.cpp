#include "Animation/IrisAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/IrisLandingPose.h"
#include "Character/TPSCPPCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/LedgePresentationComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"

namespace
{
DEFINE_LOG_CATEGORY_STATIC(LogIrisFootContact, Log, All);
TAutoConsoleVariable<int32> CVarLedgeFeet(
	TEXT("tps.IrisLedgeFeet"), 1, TEXT("Adjust Iris's support foot on grounded shoulder-aim step downs."));
TAutoConsoleVariable<int32> CVarLedgeWeightTransfer(
	TEXT("tps.IrisLedgeWeightTransfer"), 1, TEXT("Lower Iris's stance before a grounded shoulder-aim step down."));

class FIrisAnimProxy final : public FAnimInstanceProxy
{
public:
	explicit FIrisAnimProxy(UIrisAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	void PreEvaluateAnimation(UAnimInstance* Instance) override
	{
		FAnimInstanceProxy::PreEvaluateAnimation(Instance);
		const auto* Character = Cast<ATPSCPPCharacter>(Instance->TryGetPawnOwner());
		const auto* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		const auto* Iris = CastChecked<UIrisAnimInstance>(Instance);
		Settings = Iris->LedgeFootSettings;
		Landing.Prepare(Character && Character->GetCustomMesh() == Instance->GetSkelMeshComponent() ? Character : nullptr,
			Iris->ShortFallSettings, GetDeltaSeconds());
		ApproachFraction = -1.;
		AirborneRise = 0.;
		if (!Movement || Character->GetCustomMesh() != Instance->GetSkelMeshComponent())
		{
			ForgetSupport();
			StanceOffset = 0.;
			return;
		}

		const FVector Center = Character->GetActorLocation();
		if (StanceOffset != 0. && FVector::DistSquared(Center, LastCenter)
			> FMath::Square(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()))
		{
			ForgetSupport();
			StanceOffset = 0.;
		}
		if (Movement->IsFalling() && StanceOffset != 0.)
		{
			AirborneRise = FMath::Max(0., Center.Z - LastCenter.Z);
		}
		LastCenter = Center;
		if (!CVarLedgeFeet.GetValueOnGameThread() || Character->GetAimState() != EAimState::Shoulder
			|| Movement->MovementMode != MOVE_Walking || !Movement->CurrentFloor.IsWalkableFloor())
		{
			ForgetSupport();
			DrawOffset = Presentation.IsValid() ? Presentation->GetPresentationHeight() : 0.f;
			return;
		}

		World = Character->GetWorld();
		const FHitResult& Support = Movement->CurrentFloor.HitResult;
		const double Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
		if (const auto* Upper = UpperComponent.Get())
		{
			UpperPoint = Upper->GetComponentTransform().TransformPosition(UpperLocalPoint);
		}
		bool bFoundLedge = false;
		// A flat floor normally hits directly below the capsule. Only probe a displaced support contact.
		if (FVector::DistSquared2D(Support.ImpactPoint, Center) > 0.25 && Support.GetComponent()
			&& (Movement->Velocity.SizeSquared2D() > 1. || UpperComponent.IsValid()))
		{
			Params = FCollisionQueryParams(SCENE_QUERY_STAT(IrisLedgeFoot), false, Character);
			FHitResult Below;
			const FVector End(Center.X, Center.Y, Support.ImpactPoint.Z - Settings.MaximumLedgeHeight - 1.);
			if (World->LineTraceSingleByChannel(Below, Center, End, ECC_Visibility, Params) && Movement->IsWalkable(Below))
			{
				const double Height = Support.ImpactPoint.Z - Below.ImpactPoint.Z;
				if (Height >= Settings.MinimumLedgeHeight && Height <= Settings.MaximumLedgeHeight)
				{
					if (UpperComponent != Support.Component) ForgetSupport();
					bFoundLedge = true;
					UpperComponent = Support.Component;
					UpperPoint = Support.ImpactPoint;
					UpperLocalPoint = Support.GetComponent()->GetComponentTransform().InverseTransformPosition(UpperPoint);
					LowerZ = Below.ImpactPoint.Z;
					const FVector Outward = (Center - UpperPoint).GetSafeNormal2D();
					if (CVarLedgeWeightTransfer.GetValueOnGameThread() && Radius > 0.
						&& FVector::DotProduct(Movement->Velocity, Outward) > 1.)
					{
						ApproachFraction = FMath::Clamp(FVector::Dist2D(Center, UpperPoint) / Radius, 0., 1.);
					}
					UE_LOG(LogIrisFootContact, VeryVerbose, TEXT("Upper surface acquired, drop %.2f"), Height);
				}
			}
		}
		if (!bFoundLedge && (FVector::DistSquared2D(Center, UpperPoint) > FMath::Square(Radius * 3.)
			|| FMath::Abs(Support.ImpactPoint.Z - LowerZ) > 3.))
		{
			ForgetSupport();
		}
		if (UpperComponent.IsValid() || StanceOffset != 0.)
		{
			if (!Presentation.IsValid()) Presentation = Character->FindComponentByClass<ULedgePresentationComponent>();
			DrawOffset = Presentation.IsValid() ? Presentation->GetPresentationHeight() : 0.f;
			WalkableZ = Movement->GetWalkableFloorZ();
		}
	}

	void PostEvaluate(UAnimInstance* Instance) override
	{
		FAnimInstanceProxy::PostEvaluate(Instance);
		if (auto* Component = Presentation.Get()) Component->SetStanceOffset(StanceOffset);
	}

	void Uninitialize(UAnimInstance* Instance) override
	{
		if (auto* Component = Presentation.Get()) Component->SetStanceOffset(0.f);
		Presentation.Reset();
		Landing = {};
		ForgetSupport();
		StanceOffset = 0.;
		FAnimInstanceProxy::Uninitialize(Instance);
	}

	bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* Root) override
	{
		const bool bLedge = UpperComponent.IsValid() || StanceOffset != 0.;
		if ((!bLedge && !Landing.IsActive()) || Root != GetRootNode())
		{
			return false;
		}
		EvaluateAnimationNode_WithRoot(Output, Root);
		FCSPose<FCompactPose> Pose;
		Pose.InitPose(Output.Pose);
		if (bLedge)
		{
			UpdateStance(Output.Pose.GetBoneContainer(), Pose);
			AdjustFoot(Output.Pose, Pose, TEXT("Left-toe"), 1., LeftAdjustment);
			AdjustFoot(Output.Pose, Pose, TEXT("Right-toe"), -1., RightAdjustment);
		}
		if (Landing.IsActive())
		{
			if (bLedge) Pose.InitPose(Output.Pose);
			Landing.Apply(Output.Pose, Pose, GetComponentTransform(), Settings.HeelInAnkle, Settings.ForefootInToe);
		}
		return true;
	}

private:
	void UpdateStance(const FBoneContainer& Bones, FCSPose<FCompactPose>& Pose)
	{
		const auto Hips = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(TEXT("Hips"))));
		if (!Hips.IsValid()) return;
		const double PelvisZ = GetComponentTransform().TransformPosition(Pose.GetComponentSpaceTransform(Hips).GetLocation()).Z + DrawOffset;
		if (!bApproaching && ApproachFraction >= 0.)
		{
			StartPelvisZ = PelvisZ;
			if (StanceOffset == 0.) LastPelvisZ = PelvisZ;
		}
		bApproaching = ApproachFraction >= 0.;
		const double Drop = LastPelvisZ - PelvisZ;
		LastPelvisZ = PelvisZ;
		double Target = 0.;
		if (bApproaching)
		{
			const double ExpectedDescent = (UpperPoint.Z - LowerZ) * Settings.AnticipatedDropFraction
				* FMath::SmoothStep(0., 1., ApproachFraction);
			Target = FMath::Clamp(StartPelvisZ - ExpectedDescent - PelvisZ, -double(Settings.MaximumStanceLowering), 0.);
		}
		// Use the incoming pose's actual descent to take up the bend without adding an upward kick.
		const double DownBudget = FMath::Max(0., Settings.StanceLoweringSpeed * GetDeltaSeconds() - Drop);
		double UpBudget = FMath::Max(0., Settings.StanceRecoverySpeed * GetDeltaSeconds() + Drop + AirborneRise);
		if (AirborneRise > 0.)
		{
			// Let the legs extend into takeoff while limiting how much extra height is restored in one update.
			UpBudget = FMath::Min(UpBudget, double(Settings.StanceLoweringSpeed * GetDeltaSeconds()));
		}
		StanceOffset = FMath::Clamp(Target, StanceOffset - DownBudget, StanceOffset + UpBudget);
	}

	void ForgetSupport()
	{
		UpperComponent.Reset();
		bApproaching = false;
		LeftAdjustment = RightAdjustment = FVector::ZeroVector;
	}

	bool SampleUpper(const FVector& Point, double& SurfaceZ) const
	{
		if (!UpperComponent.IsValid()) return false;
		FHitResult Hit;
		const FVector Start(Point.X, Point.Y, UpperPoint.Z + 4.);
		const FVector End(Point.X, Point.Y, UpperPoint.Z - 4.);
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
			&& Hit.Component == UpperComponent && Hit.ImpactNormal.Z >= WalkableZ && !Hit.bStartPenetrating)
		{
			SurfaceZ = Hit.ImpactPoint.Z;
			return true;
		}
		return false;
	}

	void AdjustFoot(FCompactPose& Output, FCSPose<FCompactPose>& Pose, FName ToeName, double Mirror, FVector& Previous) const
	{
		const FBoneContainer& Bones = Output.GetBoneContainer();
		const FCompactPoseBoneIndex Toe = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(ToeName)));
		if (!Toe.IsValid()) return;
		const auto Ankle = Output.GetParentBoneIndex(Toe);
		const auto Knee = Output.GetParentBoneIndex(Ankle);
		const auto Thigh = Output.GetParentBoneIndex(Knee);
		const auto Hip = Output.GetParentBoneIndex(Thigh);
		const FVector Side(Mirror, 1., 1.);
		FTransform End = Pose.GetComponentSpaceTransform(Ankle);
		const FVector DrawTranslation(0., 0., DrawOffset);
		auto WorldPoint = [&](const FTransform& Bone, const FVector& Offset)
		{
			return GetComponentTransform().TransformPosition(Bone.TransformPosition(Offset * Side)) + DrawTranslation;
		};
		const FVector Heel = WorldPoint(End, Settings.HeelInAnkle);
		const FVector Fore = WorldPoint(Pose.GetComponentSpaceTransform(Toe), Settings.ForefootInToe);
		double HeelZ = 0., ForeZ = 0.;
		const bool bHeel = SampleUpper(Heel, HeelZ);
		const bool bFore = SampleUpper(Fore, ForeZ);
		double Lift = FMath::Max(bHeel ? HeelZ - Heel.Z : 0., bFore ? ForeZ - Fore.Z : 0.);
		FVector Clearance = FVector::ZeroVector;
		const FVector& SupportedEnd = bHeel ? Heel : Fore;
		if (Lift > 0. && bHeel != bFore)
		{
			// Measure the supported length of the sole instead of pinning the last point on the lip.
			const FVector& Off = bHeel ? Fore : Heel;
			double Inside = 0., Outside = 1., Height;
			for (int32 I = 0; I < 4; ++I)
			{
				const double Mid = 0.5 * (Inside + Outside);
				if (SampleUpper(FMath::Lerp(SupportedEnd, Off, Mid), Height)) Inside = Mid;
				else Outside = Mid;
			}
			const double Coverage = 0.5 * (Inside + Outside);
			FVector Travel = Off - SupportedEnd;
			Travel.Z = 0.;
			const double ExitDistance = Outside * Travel.Size() + 0.75;
			if (Coverage < 0.65 && ExitDistance <= Settings.MaximumEdgeClearance && Lift > ExitDistance)
			{
				Clearance = Travel.GetSafeNormal() * ExitDistance;
			}
		}
		const double MaxRelease = Settings.ReleaseSpeed * GetDeltaSeconds();
		FVector OldClearance(Previous.X, Previous.Y, 0.);
		Clearance = OldClearance + (Clearance - OldClearance).GetClampedToMaxSize(2. * MaxRelease);
		double SurfaceZ;
		if (bHeel != bFore && !Clearance.IsNearlyZero() && !SampleUpper(SupportedEnd + Clearance, SurfaceZ))
		{
			// Start lowering only after the shoe has cleared the vertical face.
			Lift = 0.;
		}
		Lift = FMath::Clamp<double>(Lift, 0., Settings.MaximumFootLift);
		if (!(bHeel && bFore)) Lift = FMath::Max(Lift, Previous.Z - MaxRelease);
		Previous = FVector(Clearance.X, Clearance.Y, Lift);
		const FVector AnkleAdjustment = Previous - FVector(0., 0., StanceOffset);
		if (AnkleAdjustment.IsNearlyZero()) return;
		UE_LOG(LogIrisFootContact, VeryVerbose, TEXT("%s lift %.2f"), *ToeName.ToString(), Lift);

		FTransform Root = Pose.GetComponentSpaceTransform(Thigh);
		FTransform Joint = Pose.GetComponentSpaceTransform(Knee);
		const FVector Delta = GetComponentTransform().InverseTransformVector(AnkleAdjustment);
		AnimationCore::SolveTwoBoneIK(Root, Joint, End, Joint.GetLocation(), End.GetLocation() + Delta, false, 1., 1.);
		Output[Thigh] = Root.GetRelativeTransform(Pose.GetComponentSpaceTransform(Hip));
		Output[Knee] = Joint.GetRelativeTransform(Root);
		Output[Ankle] = End.GetRelativeTransform(Joint);
	}

	FIrisLedgeFootSettings Settings;
	FIrisLandingPose Landing;
	const UWorld* World = nullptr;
	FCollisionQueryParams Params;
	TWeakObjectPtr<UPrimitiveComponent> UpperComponent;
	TWeakObjectPtr<ULedgePresentationComponent> Presentation;
	FVector UpperPoint = FVector::ZeroVector;
	FVector UpperLocalPoint = FVector::ZeroVector;
	FVector LeftAdjustment = FVector::ZeroVector;
	FVector RightAdjustment = FVector::ZeroVector;
	FVector LastCenter = FVector::ZeroVector;
	double LowerZ = 0.;
	double ApproachFraction = -1.;
	double StartPelvisZ = 0.;
	double LastPelvisZ = 0.;
	double StanceOffset = 0.;
	bool bApproaching = false;
	double AirborneRise = 0.;
	float DrawOffset = 0.f;
	float WalkableZ = 1.f;
};
}

FAnimInstanceProxy* UIrisAnimInstance::CreateAnimInstanceProxy()
{
	return new FIrisAnimProxy(this);
}

void UIrisAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)
{
	delete static_cast<FIrisAnimProxy*>(Proxy);
}
