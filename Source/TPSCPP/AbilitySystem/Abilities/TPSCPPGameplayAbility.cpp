// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/TPSCPPGameplayAbility.h"
#include "AbilitySystem/Abilities/TPSCPPAbilityCost.h"
#include "Character/TPSCPPCharacter.h"
#include "Components/CombatComponent.h"
#include "Weapon/Weapon.h"

UTPSCPPGameplayAbility::UTPSCPPGameplayAbility()
{
	// Server authoritative for now; prediction is a later phase.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UTPSCPPGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	for (const UTPSCPPAbilityCost* Cost : AdditionalCosts)
	{
		if (Cost && !Cost->CheckCost(this, Handle, ActorInfo, OptionalRelevantTags))
		{
			return false;
		}
	}

	return true;
}

void UTPSCPPGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	for (const UTPSCPPAbilityCost* Cost : AdditionalCosts)
	{
		if (Cost)
		{
			Cost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);
		}
	}
}

ATPSCPPCharacter* UTPSCPPGameplayAbility::GetTPSCPPCharacter() const
{
	// Use CurrentActorInfo (null on the CDO) instead of GetAvatarActorFromActorInfo, which asserts
	// when the pre-activation checks run against the class default object.
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	return ActorInfo ? Cast<ATPSCPPCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
}

AWeapon* UTPSCPPGameplayAbility::GetEquippedWeapon() const
{
	const UCombatComponent* Combat = GetCombatComponent();
	return Combat ? Combat->GetEquippedWeapon() : nullptr;
}

UCombatComponent* UTPSCPPGameplayAbility::GetCombatComponent() const
{
	const ATPSCPPCharacter* Character = GetTPSCPPCharacter();
	return Character ? Character->GetCombat() : nullptr;
}
