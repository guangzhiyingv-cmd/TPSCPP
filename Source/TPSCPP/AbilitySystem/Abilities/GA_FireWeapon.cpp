// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_FireWeapon.h"
#include "AbilitySystem/Abilities/TPSCPPAbilityCost_WeaponAmmo.h"
#include "AbilitySystem/TPSCPPFireCooldownEffect.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "Weapon/Weapon.h"

UGA_FireWeapon::UGA_FireWeapon()
{
	FGameplayTagContainer FireTags;
	FireTags.AddTag(TPSCPPGameplayTags::Ability_Weapon_Fire);
	SetAssetTags(FireTags);

	CooldownGameplayEffectClass = UTPSCPPFireCooldownEffect::StaticClass();

	UTPSCPPAbilityCost_WeaponAmmo* AmmoCost = CreateDefaultSubobject<UTPSCPPAbilityCost_WeaponAmmo>(TEXT("AmmoCost"));
	AdditionalCosts.Add(AmmoCost);
}

void UGA_FireWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// CommitAbility evaluates the cost gameplay effect and the additional costs (weapon ammo) and
	// starts the fire cooldown. A failed commit means the shot is refused.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_FireWeapon::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	const AWeapon* Weapon = GetEquippedWeapon();
	if (!CooldownEffect || !Weapon)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffect->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (SpecHandle.IsValid())
	{
		const float CooldownDuration = FMath::Max(Weapon->FireDelay - CooldownTolerance, 0.01f);
		SpecHandle.Data->SetSetByCallerMagnitude(TPSCPPGameplayTags::Data_Cooldown, CooldownDuration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}
