#include "Character/TPSCPPCharacter.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraShakeBase.h"

void ATPSCPPCharacter::Landed(const FHitResult& Hit)
{
	const double DownwardSpeed = -GetVelocity().Z;
	Super::Landed(Hit);

	if (!IsLocallyControlled() || bEliminated || AimState == EAimState::ADS || !LandingCameraShake ||
		DownwardSpeed <= LandingSpeedRange.X)
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		FGameplayCueParameters Parameters;
		Parameters.NormalizedMagnitude = FMath::GetMappedRangeValueClamped(
			LandingSpeedRange, FVector2D(0.0, 1.0), DownwardSpeed);
		// Landed already runs on the owning client; a multicast would replay the local impact.
		ASC->InvokeGameplayCueEvent(TPSCPPGameplayTags::Cue_Character_Landing, EGameplayCueEvent::Executed, Parameters);
	}
}
