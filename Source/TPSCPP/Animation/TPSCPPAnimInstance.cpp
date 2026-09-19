// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/TPSCPPAnimInstance.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystemComponent.h"

UTPSCPPAnimInstance::UTPSCPPAnimInstance()
{
	AddDefaultMappings();
}

void UTPSCPPAnimInstance::AddDefaultMappings()
{
	// The mapped properties are the GameplayTag_* booleans owned by the concrete anim blueprint.
	GameplayTagPropertyMap.AddMapping(TPSCPPGameplayTags::State_Firing, TEXT("GameplayTag_IsFiring"));
	GameplayTagPropertyMap.AddMapping(TPSCPPGameplayTags::State_ADS, TEXT("GameplayTag_IsADS"));
	GameplayTagPropertyMap.AddMapping(TPSCPPGameplayTags::State_Reloading, TEXT("GameplayTag_IsReloading"));
	GameplayTagPropertyMap.AddMapping(TPSCPPGameplayTags::State_Sprint, TEXT("GameplayTag_IsDashing"));
}

void UTPSCPPAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	// Rebuild the mappings here instead of trusting the serialized ones: blueprint class default
	// objects do not keep the property names that were set from C++, so the map would find no
	// properties and drop every entry (leaving the anim graph stuck on stale values).
	GameplayTagPropertyMap.ClearMappings();
	AddDefaultMappings();

	GameplayTagPropertyMap.Initialize(this, ASC);
}
