// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/TPSCPPAnimInstance.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystemComponent.h"

UTPSCPPAnimInstance::UTPSCPPAnimInstance()
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

	GameplayTagPropertyMap.Initialize(this, ASC);
}
