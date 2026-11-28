#include "Character/TPSCharacterMovementComponent.h"

UTPSCharacterMovementComponent::UTPSCharacterMovementComponent()
{
	FRichCurve* Curve = GroundTurnRate.GetRichCurve();
	Curve->AddKey(45.f, 300.f);
	Curve->AddKey(90.f, 500.f);
}

FRotator UTPSCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation,
	float DeltaTime, FRotator& DeltaRotation) const
{
	const FRotator Target = Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
	if (IsMovingOnGround() && !Acceleration.IsNearlyZero())
	{
		const float RemainingYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, Target.Yaw));
		const float Rate = GroundTurnRate.GetRichCurveConst()->Eval(RemainingYaw, RotationRate.Yaw);
		DeltaRotation.Yaw = FMath::Clamp(Rate * DeltaTime, 0.f, 360.f);
	}
	return Target;
}
