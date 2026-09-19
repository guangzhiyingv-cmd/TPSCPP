// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/TPSCPPGameplayAbility.h"
#include "GA_Sprint.generated.h"

/**
 * Sprinting the character. Activated while the sprint input is held and cancelled when it is
 * released or when a higher priority action (aiming, firing) takes over. It sets the replicated
 * State.Sprint tag and applies the authoritative movement settings.
 */
UCLASS()
class TPSCPP_API UGA_Sprint : public UTPSCPPGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Sprint();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Applies or restores the movement settings backing the sprint state. */
	void ApplySprintMovement(bool bSprinting);
};
