#pragma once

#include "Animation/AnimInstance.h"
#include "IrisAnimInstance.generated.h"

USTRUCT()
struct FIrisLedgeFootSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Ledge", meta = (Units = "cm", ClampMin = "0"))
	float MinimumLedgeHeight = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ledge", meta = (Units = "cm", ClampMin = "0"))
	float MaximumLedgeHeight = 65.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ledge", meta = (Units = "cm", ClampMin = "0"))
	float MaximumFootLift = 26.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ledge", meta = (Units = "cm", ClampMin = "0"))
	float MaximumEdgeClearance = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ledge", meta = (Units = "cm/s", ClampMin = "1"))
	float ReleaseSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weight Transfer", meta = (Units = "cm", ClampMin = "0"))
	float MaximumStanceLowering = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weight Transfer", meta = (ClampMin = "0", ClampMax = "1"))
	float AnticipatedDropFraction = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Weight Transfer", meta = (Units = "cm/s", ClampMin = "0"))
	float StanceLoweringSpeed = 210.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weight Transfer", meta = (Units = "cm/s", ClampMin = "1"))
	float StanceRecoverySpeed = 24.f;

	// The landmarks use the local spaces of the left ankle and toe. The right foot mirrors X.
	UPROPERTY(EditDefaultsOnly, Category = "Sole")
	FVector HeelInAnkle = FVector(0.36, -2.60, 9.46);

	UPROPERTY(EditDefaultsOnly, Category = "Sole")
	FVector ForefootInToe = FVector(-2.00, -0.20, 3.91);
};

USTRUCT()
struct FIrisShortFallSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (Units = "s", ClampMin = "0"))
	float MaximumFallTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (Units = "s", ClampMin = "0.01"))
	float PreparationTime = 0.21f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (Units = "s", ClampMin = "0.01"))
	float PlantTime = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (Units = "s", ClampMin = "0.01"))
	float ReleaseTime = 0.13f;

	UPROPERTY(EditDefaultsOnly, Category = "Pose", meta = (Units = "cm", ClampMin = "0"))
	float MaximumPelvisDrop = 24.f;

	UPROPERTY(EditDefaultsOnly, Category = "Pose", meta = (Units = "cm", ClampMin = "0"))
	float SwingFootClearance = 16.f;
};

UCLASS()
class UIrisAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Foot Contact")
	FIrisLedgeFootSettings LedgeFootSettings;

	UPROPERTY(EditDefaultsOnly, Category = "Foot Contact")
	FIrisShortFallSettings ShortFallSettings;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
