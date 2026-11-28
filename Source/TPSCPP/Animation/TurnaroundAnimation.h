#pragma once

#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimMetaData.h"
#include "Animation/AnimNodeReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TurnaroundAnimation.generated.h"

class UAnimSequence;

USTRUCT()
struct FTurnaroundFootSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector Ankle = FVector::ZeroVector;

	UPROPERTY(EditAnywhere)
	FVector Ball = FVector::ZeroVector;

	FVector ContactPoint(float ToeWeight) const;
};

// Component-space track for the incoming support foot, sampled only over the entry window.
UCLASS()
class TPSCPP_API UTurnaroundEntryData : public UAnimMetaData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Entry")
	float NominalEntryTime = 0.f;

	UPROPERTY(EditAnywhere, Category="Entry")
	float BrakeTime = 0.f;

	UPROPERTY(EditAnywhere, Category="Entry")
	float FirstSampleTime = 0.f;

	UPROPERTY(EditAnywhere, Category="Entry")
	float SampleInterval = 1.f / 60.f;

	UPROPERTY(EditAnywhere, Category="Entry")
	TArray<FTurnaroundFootSample> Samples;

	// Source root velocity in approach space (+X forward), scaled to the authored gameplay approach speed.
	UPROPERTY(EditAnywhere, Category="Warping")
	TArray<FVector2D> TravelVelocity;

	UPROPERTY(EditAnywhere, Category="Warping", meta=(Units="s", ClampMin="0.001"))
	float TravelSampleInterval = 1.f / 60.f;
};

USTRUCT(BlueprintType)
struct FTurnaroundClip
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="Turnaround")
	TObjectPtr<UAnimSequence> Sequence;

	UPROPERTY(EditDefaultsOnly, Category="Turnaround", meta=(Units="s", ClampMin="0"))
	float ExitTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Turnaround", meta=(Units="s", ClampMin="0"))
	float CycleEntryTime = 0.f;

	// Fractional corrections available only after the plant, as the clip runs out in its new direction.
	UPROPERTY(EditDefaultsOnly, Category="Turnaround|Run Out", meta=(ClampMin="0", ClampMax="0.5"))
	float MaxRunOutRateAdjustment = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Turnaround|Run Out", meta=(ClampMin="0", ClampMax="0.5"))
	float ExtraRunOutStrideAdjustment = 0.f;

	// Stride multiplier for the outgoing support step. The node interpolates toward this target.
	UPROPERTY(EditDefaultsOnly, Category="Turnaround|Run Out", meta=(ClampMin="1", ClampMax="1.9"))
	float ExitStrideScale = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Turnaround|Run Out", meta=(Units="s", ClampMin="0"))
	float ExitStrideLeadTime = 0.f;
};

USTRUCT(BlueprintType)
struct FTurnaroundSide
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="Turnaround")
	FTurnaroundClip LeftFootLow;

	UPROPERTY(EditDefaultsOnly, Category="Turnaround")
	FTurnaroundClip RightFootLow;
};

UCLASS()
class TPSCPP_API UTurnaroundAnimationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Animation|Turnaround", meta=(BlueprintThreadSafe))
	static void MatchTurnaroundCycle(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
};
