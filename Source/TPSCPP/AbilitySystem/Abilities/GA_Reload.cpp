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
}

bool UGA_Reload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Resolve from the passed ActorInfo so this check is safe on the class default object (the
	// client never instantiates a ServerOnly ability, so its pre-activation checks run on the CDO).
	const ATPSCPPCharacter* Character = ActorInfo ? Cast<ATPSCPPCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UCombatComponent* Combat = Character ? Character->GetCombat() : nullptr;
	const AWeapon* Weapon = Combat ? Combat->GetEquippedWeapon() : nullptr;
	if (!Character || !Weapon)
	{
		return false;
	}

	if (Weapon->Ammo >= Weapon->MagCapacity)
	{
		return false;
	}

	if (!Weapon->bInfiniteAmmo && Character->ReserveAmmo <= 0)
	{
		return false;
	}

	return true;
}

void UGA_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ATPSCPPCharacter* Character = GetTPSCPPCharacter();
	AWeapon* Weapon = GetEquippedWeapon();
	if (!Character || !Weapon || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Loose tag so every machine knows the character is reloading (it replicates).
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(TPSCPPGameplayTags::State_Reloading, 1);
	}

	// Reloading cancels aiming down sights. This is done on the requesting machine (see
	// ATPSCPPCharacter::TryReload) because the ADS camera swap is a local, per-player action.

	// Cosmetic montage on all machines; PlayRate keeps it in sync with ReloadTime.
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
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(TPSCPPGameplayTags::State_Reloading, 0);
	}

	if (ATPSCPPCharacter* Character = GetTPSCPPCharacter())
	{
		Character->MulticastPlayReloadMontage(false, 0.f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
