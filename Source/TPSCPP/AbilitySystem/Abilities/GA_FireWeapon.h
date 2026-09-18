// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/TPSCPPGameplayAbility.h"
#include "GA_FireWeapon.generated.h"

/**
 * Fires the equipped weapon once: committing consumes ammo (weapon ammo cost) and starts the
 * weapon fire delay as a cooldown, so the server refuses shots that are out of ammo or faster than
 * the weapon's fire rate.
 */
UCLASS()
class TPSCPP_API UGA_FireWeapon : public UTPSCPPGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_FireWeapon();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Uses the equipped weapon's fire delay as the cooldown duration. */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/**
	 * Subtracted from the fire delay so the server cooldown is never tighter than the owning
	 * client's own fire pacing, which would otherwise drop shots at the fire rate boundary.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (ClampMin = 0))
	float CooldownTolerance = 0.05f;
};
