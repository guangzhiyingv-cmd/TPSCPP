// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/TPSCPPAbilityCost.h"
#include "TPSCPPAbilityCost_WeaponAmmo.generated.h"

/**
 * Consumes rounds from the equipped weapon's magazine. Evaluated through CommitAbility, so firing
 * is refused when the magazine is empty.
 */
UCLASS()
class TPSCPP_API UTPSCPPAbilityCost_WeaponAmmo : public UTPSCPPAbilityCost
{
	GENERATED_BODY()

public:
	/** Rounds removed from the magazine each time the ability commits. */
	UPROPERTY(EditDefaultsOnly, Category = "Cost", meta = (ClampMin = 1))
	int32 NumRounds = 1;

	virtual bool CheckCost(const UTPSCPPGameplayAbility* Ability, const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const UTPSCPPGameplayAbility* Ability, const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const override;
};
