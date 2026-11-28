#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Landing.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/TPSCPPCharacter.h"
#include "GameFramework/PlayerController.h"

void UTPSCPPCueNotify_Landing::PostInitProperties()
{
	Super::PostInitProperties();
	GameplayCueTag = TPSCPPGameplayTags::Cue_Character_Landing;
	GameplayCueName = GameplayCueTag.GetTagName();
}

bool UTPSCPPCueNotify_Landing::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	const ATPSCPPCharacter* Character = Cast<ATPSCPPCharacter>(Target);
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC || !PC->IsLocalPlayerController() || PC->GetViewTarget() != Character || !PC->PlayerCameraManager)
	{
		return false;
	}
	return PC->PlayerCameraManager->StartCameraShake(Character->GetLandingCameraShake(),
		Parameters.NormalizedMagnitude, ECameraShakePlaySpace::World) != nullptr;
}
