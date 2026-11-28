#pragma once

#include "Components/ActorComponent.h"
#include "LedgePresentationComponent.generated.h"

class ATPSCPPCharacter;
class FLedgeViewExtension;

UCLASS(ClassGroup = Camera)
class ULedgePresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	float GetPresentationHeight() const { return PresentationHeight; }
	void SetStanceOffset(float Offset);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Shoulder Step Down", meta = (ClampMin = "1", Units = "cm"))
	float MinimumDrop = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoulder Step Down", meta = (ClampMin = "0", Units = "s"))
	float RecoveryTime = 0.16f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Shoulder Step Down", meta = (Units = "cm"))
	float PresentationHeight = 0.f;

private:
	UFUNCTION()
	void OnMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);
	void SetHeight(float Height);
	void UpdateDrawHeight();

	UPROPERTY(Transient)
	TObjectPtr<ATPSCPPCharacter> Character;
	TSharedPtr<FLedgeViewExtension, ESPMode::ThreadSafe> ViewExtension;
	FVector LastLocation = FVector::ZeroVector;
	double LastFloorZ = 0.;
	float InitialHeight = 0.f;
	float Elapsed = 0.f;
	float AppliedCameraHeight = 0.f;
	float StanceOffset = 0.f;
	bool bWasGrounded = false;
};
