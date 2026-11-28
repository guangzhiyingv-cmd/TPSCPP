#pragma once

#include "Curves/CurveFloat.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TPSCharacterMovementComponent.generated.h"

UCLASS()
class TPSCPP_API UTPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UTPSCharacterMovementComponent();

protected:
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime,
		FRotator& DeltaRotation) const override;

	/** Maximum grounded turn rate (degrees per second) by remaining yaw angle (degrees). */
	UPROPERTY(EditDefaultsOnly, Category = "Character Movement|Ground Turning")
	FRuntimeFloatCurve GroundTurnRate;
};
