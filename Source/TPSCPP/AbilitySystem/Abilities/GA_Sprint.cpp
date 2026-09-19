// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_Sprint.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Character/TPSCPPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Sprint::UGA_Sprint()
{
	FGameplayTagContainer SprintTags;
	SprintTags.AddTag(TPSCPPGameplayTags::Ability_Sprint);
	SetAssetTags(SprintTags);

	// Aiming and reloading outrank sprinting, so sprinting cannot start while either is active.
	ActivationBlockedTags.AddTag(TPSCPPGameplayTags::State_ADS);
	ActivationBlockedTags.AddTag(TPSCPPGameplayTags::State_ShoulderAim);
	ActivationBlockedTags.AddTag(TPSCPPGameplayTags::State_Reloading);
}

void UGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ATPSCPPCharacter* Character = GetTPSCPPCharacter();
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Loose tag so every machine knows the character is sprinting. It has to be published explicitly:
	// a loose tag with the default replication state stays on the machine that set it, so the other
	// clients would never see this state.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(
			TPSCPPGameplayTags::State_Sprint, 1, EGameplayTagReplicationState::TagOnly);
	}

	ApplySprintMovement(true);
}

void UGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(
			TPSCPPGameplayTags::State_Sprint, 0, EGameplayTagReplicationState::TagOnly);
	}

	ApplySprintMovement(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Sprint::ApplySprintMovement(bool bSprinting)
{
	ATPSCPPCharacter* Character = GetTPSCPPCharacter();
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	if (bSprinting)
	{
		Character->bUseControllerRotationYaw = false;
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
		Movement->MaxWalkSpeed = Character->GetSprintSpeed();
		return;
	}

	// Back to the armed stance when a weapon is equipped, otherwise to movement orientation.
	if (Character->HasEquippedWeapon())
	{
		Character->bUseControllerRotationYaw = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->bOrientRotationToMovement = false;
	}
	else
	{
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}

	Movement->MaxWalkSpeed = Character->GetWalkSpeed();
}
