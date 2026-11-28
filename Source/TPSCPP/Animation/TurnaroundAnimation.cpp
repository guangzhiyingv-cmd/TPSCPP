#include "Animation/TurnaroundAnimation.h"

#include "Animation/TPSCPPAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/TurnaroundAnimLayer.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Character/TPSCPPCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

FVector FTurnaroundFootSample::ContactPoint(float ToeWeight) const
{
	// Project a heel point behind the ankle. Ball is the metatarsal joint of the mannequin.
	const FVector Heel = Ankle + 0.35f * (Ankle - Ball);
	return FMath::Lerp(Heel, Ball, ToeWeight);
}

namespace
{
FTurnaroundFootSample ReadFoot(const USkeletalMeshComponent& Mesh, FName AnkleBone, FName BallBone)
{
	FTurnaroundFootSample Foot;
	Foot.Ankle = Mesh.GetSocketTransform(AnkleBone, RTS_Component).GetLocation();
	Foot.Ball = Mesh.GetSocketTransform(BallBone, RTS_Component).GetLocation();
	return Foot;
}

bool HasTurnaroundWeight(const UAnimInstance& Main)
{
	static const FName LocomotionMachine(TEXT("LocomotionSM"));
	static const FName TurnaroundState(TEXT("Turnaround"));
	const FAnimNode_StateMachine* Machine = Main.GetStateMachineInstanceFromName(LocomotionMachine);
	const int32 Index = Machine ? Machine->GetStateIndex(TurnaroundState) : INDEX_NONE;
	return Index != INDEX_NONE && FAnimWeight::IsRelevant(Machine->GetStateWeight(Index));
}
}

void UTPSCPPAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	bTurnaroundActive = false;
	bHadGroundedSample = false;
	bAwaitingTurnSide = false;
	PlayingLayer.Reset();
	HandoffElapsed = -1.f;
}

void UTPSCPPAnimInstance::UpdateTurnaround(const ATPSCPPCharacter* Character, float DeltaSeconds)
{
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	bCameraFacingLocomotion = Move && !Move->bOrientRotationToMovement;
	const bool bEligible = Move && Move->IsMovingOnGround() && !bCameraFacingLocomotion;
	UTurnaroundAnimLayer* Layer = Cast<UTurnaroundAnimLayer>(GetLinkedAnimLayerInstanceByGroup(TEXT("ItemAnimLayers")));
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (PlayingLayer.Get() != Layer)
	{
		if (Layer && HasTurnaroundWeight(*this))
		{
			Layer->CaptureRelinkPose();
		}
		bTurnaroundActive = false;
		bAwaitingTurnSide = false;
		bHadGroundedSample = false;
		HandoffElapsed = -1.f;
		PlayingLayer = Layer;
	}
	if (!bEligible || !Layer)
	{
		if (Move && Layer && Layer->CurrentPose().Sequence && HasTurnaroundWeight(*this))
		{
			Layer->Advance(DeltaSeconds, Mesh->GetComponentTransform(), Move->Velocity);
		}
		bHadGroundedSample = false;
		bAwaitingTurnSide = false;
		bTurnaroundActive = false;
		HandoffElapsed = -1.f;
		return;
	}

	const FVector Desired = Move->GetCurrentAcceleration().GetSafeNormal2D();
	const float MeshYaw = GetSkelMeshComponent()->GetComponentRotation().Yaw;
	const float ActorYaw = Character->GetActorRotation().Yaw;
	FTurnaroundFootSample Feet[2];
	Feet[0] = ReadFoot(*Mesh, TEXT("foot_l"), TEXT("ball_l"));
	Feet[1] = ReadFoot(*Mesh, TEXT("foot_r"), TEXT("ball_r"));
	const bool bPreviousTurnRelevant = Layer->CurrentPose().Sequence && HasTurnaroundWeight(*this);
	if (bTurnaroundActive || bPreviousTurnRelevant)
	{
		Layer->Advance(DeltaSeconds, Mesh->GetComponentTransform(), Move->Velocity);
	}
	if (bAwaitingTurnSide)
	{
		PendingElapsed += DeltaSeconds;
	}

	if (HandoffElapsed >= 0.f)
	{
		HandoffElapsed += DeltaSeconds;
		// The state machine owns the blend duration, including editor-authored adjustments.
		if (!bPreviousTurnRelevant)
		{
			HandoffElapsed = -1.f;
		}
	}

	const float ReversalDot = FMath::Cos(FMath::DegreesToRadians(ReversalAngle));
	const bool bRunningReversal = PreviousVelocity.SizeSquared2D() >= FMath::Square(MinimumApproachSpeed)
		&& FVector::DotProduct(PreviousVelocity.GetSafeNormal2D(), Desired) <= ReversalDot;
	const bool bRepeat = bTurnaroundActive && FVector::DotProduct(TurnaroundDirection, Desired) <= ReversalDot;
	if (!bAwaitingTurnSide && bHadGroundedSample && !Desired.IsNearlyZero() && (bRunningReversal || bRepeat)
		&& (!bTurnaroundActive || bRepeat))
	{
		bEnteredOnLeftFoot = Feet[0].Ankle.Z <= Feet[1].Ankle.Z;
		const int32 FootIndex = bEnteredOnLeftFoot ? 0 : 1;
		EntryFoot = Feet[FootIndex];
		// Flat or heel-first support uses the rear of the shoe; a lifted heel shifts weight toward the ball.
		EntryToeWeight = FMath::Clamp((EntryFoot.Ankle.Z - EntryFoot.Ball.Z - 5.f) / 7.f, 0.f, 1.f);
		const FVector PoseDelta = EntryFoot.ContactPoint(EntryToeWeight) - PreviousFeet[FootIndex].ContactPoint(EntryToeWeight);
		EntryContactVelocity = PoseDelta / FMath::Max(DeltaSeconds, UE_SMALL_NUMBER)
			+ Mesh->GetComponentQuat().UnrotateVector(PreviousVelocity - Move->Velocity);
		EntryPhase = GetSyncGroupPosition(TEXT("Locomotion"));
		EntryActorYaw = PreviousActorYaw;
		EntryMeshYaw = PreviousMeshYaw;
		PendingDirection = Desired;
		ApproachYaw = PreviousVelocity.IsNearlyZero() ? EntryActorYaw : PreviousVelocity.Rotation().Yaw;
		PendingElapsed = 0.0;
		bAwaitingTurnSide = true;
	}

	if (bAwaitingTurnSide)
	{
		// Network smoothing may still rotate the mesh in the old direction on a listen server.
		const float Turn = FMath::FindDeltaAngleDegrees(EntryActorYaw, ActorYaw);
		if (FVector::DotProduct(Desired, PendingDirection) <= 0.5f)
		{
			bAwaitingTurnSide = false;
		}
		else if (FMath::Abs(Turn) >= 2.f && (!(bTurnaroundActive || bPreviousTurnRelevant) || Layer->CanReuseOtherPose()))
		{
			bAwaitingTurnSide = false;
			if (Layer->StartTurn(Character->IsSprinting(), Turn < 0.f, bEnteredOnLeftFoot, bTurnaroundActive || bPreviousTurnRelevant,
				PendingElapsed, EntryMeshYaw, ApproachYaw, EntryPhase, EntryFoot, EntryContactVelocity, EntryToeWeight,
				EntryPhaseTolerance, EntryVelocityWeightSeconds))
			{
				Layer->Advance(0.f, Mesh->GetComponentTransform(), Move->Velocity);
				TurnaroundDirection = PendingDirection;
				CycleEntryTime = Layer->CurrentPose().CycleEntry;
				HandoffElapsed = -1.f;
				bTurnaroundActive = true;
			}
		}
		else if (FMath::Abs(Turn) < 2.f && FMath::Abs(FMath::FindDeltaAngleDegrees(ActorYaw, Desired.Rotation().Yaw)) < 2.f)
		{
			bAwaitingTurnSide = false;
		}
	}
	if (bTurnaroundActive && !bAwaitingTurnSide)
	{
		const bool bContinues = FVector::DotProduct(Desired, TurnaroundDirection) > 0.5f;
		if (!bContinues || Layer->CurrentPose().Elapsed >= Layer->CurrentPose().ExitTime)
		{
			bTurnaroundActive = false;
			HandoffElapsed = bContinues ? 0.f : -1.f;
		}
	}
	PreviousFeet[0] = Feet[0];
	PreviousFeet[1] = Feet[1];
	PreviousActorYaw = ActorYaw;
	PreviousVelocity = Move->Velocity;
	PreviousMeshYaw = MeshYaw;
	bHadGroundedSample = true;
}

bool UTPSCPPAnimInstance::GetTurnaroundCycleTime(float PlayRate, float& Time) const
{
	if (HandoffElapsed < 0.f)
	{
		return false;
	}
	Time = CycleEntryTime + HandoffElapsed * PlayRate;
	return true;
}

void UTurnaroundAnimationLibrary::MatchTurnaroundCycle(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	const FAnimationUpdateContext* Update = Context.GetContext();
	FAnimNode_SequencePlayer* Player = Node.GetAnimNodePtr<FAnimNode_SequencePlayer>();
	if (!Update || !Player)
	{
		return;
	}
	const UTPSCPPAnimInstance* Main = Cast<UTPSCPPAnimInstance>(Update->AnimInstanceProxy->GetSkelMeshComponent()->GetAnimInstance());
	float Time;
	if (Main && Main->GetTurnaroundCycleTime(Player->GetPlayRate(), Time))
	{
		Player->SetAccumulatedTime(Time);
	}
}
