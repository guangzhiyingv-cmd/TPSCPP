// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Blood.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "Character/TPSCPPCharacter.h"
#include "GameplayCueNotifyTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UTPSCPPCueNotify_Blood::UTPSCPPCueNotify_Blood()
{
	GameplayCueTag = TPSCPPGameplayTags::Cue_Hit_Blood;
	GameplayCueName = GameplayCueTag.GetTagName();
}

void UTPSCPPCueNotify_Blood::PostInitProperties()
{
	Super::PostInitProperties();

	// Re-assert: the base class may derive the tag from the asset name.
	GameplayCueTag = TPSCPPGameplayTags::Cue_Hit_Blood;
	GameplayCueName = GameplayCueTag.GetTagName();
}

bool UTPSCPPCueNotify_Blood::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}

void UTPSCPPCueNotify_Blood::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	// The cue is executed on the hit character's ability system, so MyTarget is that character.
	ATPSCPPCharacter* HitCharacter = Cast<ATPSCPPCharacter>(MyTarget);
	if (!HitCharacter)
	{
		return;
	}

	const FVector ImpactLocation = Parameters.Location;
	const FRotator ImpactRotation = Parameters.Normal.Rotation();

	if (UNiagaraSystem* BloodSystem = HitCharacter->GetBloodNiagaraSystem())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(HitCharacter->GetWorld(), BloodSystem, ImpactLocation, ImpactRotation);
	}

	if (USoundBase* Sound = HitCharacter->GetHitSound())
	{
		if (HitCharacter->ShouldPlayHitSound())
		{
			UGameplayStatics::PlaySoundAtLocation(HitCharacter, Sound, ImpactLocation);
		}
	}
}
