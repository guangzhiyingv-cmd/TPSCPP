#pragma once

#include "Animation/AnimInstance.h"
#include "TurnaroundAnimation.h"
#include "TurnaroundAnimLayer.generated.h"

USTRUCT(BlueprintType)
struct FTurnaroundPose
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Transient)
	TObjectPtr<UAnimSequence> Sequence;

	UPROPERTY(BlueprintReadOnly, Transient)
	float Time = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient)
	float RootYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient)
	float OrientationAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient)
	float StrideScale = 1.f;

	UPROPERTY(BlueprintReadOnly, Transient)
	FVector StrideDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Transient)
	float WarpAlpha = 0.f;

	double Elapsed = 0.0;
	float EntryYaw = 0.f;
	float ApproachYaw = 0.f;
	float NominalTime = 0.f;
	float EntryOffset = 0.f;
	float BrakeInterval = 0.f;
	float ExitTime = 0.f;
	float CycleEntry = 0.f;
	float MaxRunOutRateAdjustment = 0.f;
	float ExtraRunOutStrideAdjustment = 0.f;
	float ExitStrideScale = 1.f;
	float ExitStrideLeadTime = 0.f;
};

UCLASS()
class TPSCPP_API UTurnaroundAnimLayer : public UAnimInstance
{
	GENERATED_BODY()

public:
	bool StartTurn(bool bSprint, bool bLeft, bool bLeftFootLow, bool bRestart, double Elapsed, float MeshYaw, float ApproachYaw,
		const FMarkerSyncAnimPosition& Phase, const FTurnaroundFootSample& Foot, const FVector& ContactVelocity,
		float ToeWeight, float PhaseTolerance, float VelocityWeightSeconds);
	bool CanReuseOtherPose() const;
	void CaptureRelinkPose();
	void Advance(float DeltaSeconds, const FTransform& MeshTransform, const FVector& WorldVelocity);
	const FTurnaroundPose& CurrentPose() const { return bUseTurnB ? TurnB : TurnA; }

protected:
	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround")
	FTurnaroundSide LeftTurn;

	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround")
	FTurnaroundSide RightTurn;

	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround|Sprint")
	FTurnaroundSide SprintLeftTurn;

	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround|Sprint")
	FTurnaroundSide SprintRightTurn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Anim Set|Turnaround|Sprint")
	TObjectPtr<UAnimSequence> SprintCarryPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Anim Set|Turnaround|Sprint", meta=(Units="s", ClampMin="0"))
	float SprintCarryTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Turnaround")
	float SprintCarryWeight = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround|Warping", meta=(ClampMin="0", ClampMax="90", Units="deg"))
	float MaxOrientationAngle = 20.f;

	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround|Warping", meta=(ClampMin="0", ClampMax="0.9"))
	float MaxStrideAdjustment = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category="Anim Set|Turnaround|Warping", meta=(ClampMin="1", Units="cm/s"))
	float WarpSpeedThreshold = 80.f;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Turnaround")
	FTurnaroundPose TurnA;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Turnaround")
	FTurnaroundPose TurnB;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Turnaround")
	bool bUseTurnB = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Turnaround")
	bool bUseRelinkPose = false;
};
