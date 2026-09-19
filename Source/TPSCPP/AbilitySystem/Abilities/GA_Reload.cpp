// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_Reload.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Character/TPSCPPCharacter.h"
#include "Components/CombatComponent.h"
#include "Weapon/Weapon.h"

UGA_Reload::UGA_Reload()
{
	FGameplayTagContainer ReloadTags;
	ReloadTags.AddTag(TPSCPPGameplayTags::Ability_Weapon_Reload);
	SetAssetTags(ReloadTags);

	// Owned while the ability runs, so every machine sees the reload state (replicated to all).
	ActivationOwnedTags.AddTag(TPSCPPGameplayTags::State_Reloading);

	// Reloading is exclusive with itself: a second request while one is running is refused.
	ActivationBlockedTags.AddTag(TPSCPPGameplayTags::State_Reloading);
}

void UGA_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ATPSCPPCharacter* Character = GetTPSCPPCharacter();
	AWeapon* Weapon = GetEquippedWeapon();

	// The magazine checks live here rather than in CanActivateAbility so that a refusal always runs
	// this code path and can tell the owning client to drop its predicted reload.
	const bool bCanReload = Character && Weapon
		&& Weapon->Ammo < Weapon->MagCapacity
		&& (Weapon->bInfiniteAmmo || Character->ReserveAmmo > 0);

	if (!bCanReload || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if (Character && ActorInfo && ActorInfo->IsNetAuthority())
		{
			Character->Client_StopReloadPresentation();
		}

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cosmetic montage on all machines; PlayRate keeps it in sync with ReloadTime. The owning client
	// plays its own predicted start, which is why the multicast skips it.
	Character->MulticastPlayReloadMontage(true, Weapon->ReloadTime);

	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Weapon->ReloadTime, 0.01f));
	if (Task)
	{
		Task->OnFinish.AddDynamic(this, &UGA_Reload::OnReloadDelayFinished);
		Task->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Reload::OnReloadDelayFinished()
{
	FinishReload();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Reload::FinishReload()
{
	ATPSCPPCharacter* Character = GetTPSCPPCharacter();
	AWeapon* Weapon = GetEquippedWeapon();
	if (!Character || !Weapon)
	{
		return;
	}

	if (Weapon->bInfiniteAmmo)
	{
		Weapon->SetAmmo(Weapon->MagCapacity);
	}
	else
	{
		const int32 AmmoNeeded = Weapon->MagCapacity - Weapon->Ammo;
		const int32 AmmoToLoad = FMath::Min(AmmoNeeded, Character->ReserveAmmo);
		Weapon->SetAmmo(Weapon->Ammo + AmmoToLoad);
		Character->ReserveAmmo -= AmmoToLoad;
	}

	if (UCombatComponent* Combat = GetCombatComponent())
	{
		Combat->UpdateAmmoHUD();
	}
}

void UGA_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ATPSCPPCharacter* Character = GetTPSCPPCharacter())
	{
		Character->MulticastPlayReloadMontage(false, 0.f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
