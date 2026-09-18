// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/TPSCPPAbilityCost_WeaponAmmo.h"
#include "Character/TPSCPPCharacter.h"
#include "Components/CombatComponent.h"
#include "Weapon/Weapon.h"

namespace
{
	/**
	 * Resolved from the passed ActorInfo: the client evaluates the cost on the class default object
	 * (a ServerOnly ability is never instanced there), where instance state is unavailable.
	 */
	AWeapon* ResolveEquippedWeapon(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const ATPSCPPCharacter* Character = ActorInfo ? Cast<ATPSCPPCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
		UCombatComponent* Combat = Character ? Character->GetCombat() : nullptr;
		return Combat ? Combat->GetEquippedWeapon() : nullptr;
	}
}

bool UTPSCPPAbilityCost_WeaponAmmo::CheckCost(const UTPSCPPGameplayAbility* Ability, const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AWeapon* Weapon = ResolveEquippedWeapon(ActorInfo);
	if (!Weapon)
	{
		return false;
	}

	return Weapon->bInfiniteAmmo || Weapon->Ammo >= NumRounds;
}

void UTPSCPPAbilityCost_WeaponAmmo::ApplyCost(const UTPSCPPGameplayAbility* Ability, const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const
{
	AWeapon* Weapon = ResolveEquippedWeapon(ActorInfo);
	if (!Weapon || Weapon->bInfiniteAmmo)
	{
		return;
	}

	Weapon->SetAmmo(Weapon->Ammo - NumRounds);

	if (const ATPSCPPCharacter* Character = ActorInfo ? Cast<ATPSCPPCharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		if (UCombatComponent* Combat = Character->GetCombat())
		{
			Combat->UpdateAmmoHUD();
		}
	}
}
