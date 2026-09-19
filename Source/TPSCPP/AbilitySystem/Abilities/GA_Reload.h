// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/TPSCPPGameplayAbility.h"
#include "GA_Reload.generated.h"

/**
 * Reloads the equipped weapon. Timing is authoritative through a wait task while the reload
 * montage is played cosmetically on every machine.
 */
UCLASS()
class TPSCPP_API UGA_Reload : public UTPSCPPGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Reload();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Moves reserve ammo into the magazine. */
	void FinishReload();

	UFUNCTION()
	void OnReloadDelayFinished();
};
