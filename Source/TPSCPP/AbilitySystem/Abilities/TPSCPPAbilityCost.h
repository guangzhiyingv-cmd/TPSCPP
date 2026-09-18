// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTagContainer.h"
#include "TPSCPPAbilityCost.generated.h"

class UTPSCPPGameplayAbility;

/**
 * Base class for custom ability costs (for example weapon ammo). Evaluated by
 * UTPSCPPGameplayAbility on top of the regular cost gameplay effect.
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class TPSCPP_API UTPSCPPAbilityCost : public UObject
{
	GENERATED_BODY()

public:
	/** Returns true when the ability can afford this cost. */
	virtual bool CheckCost(const UTPSCPPGameplayAbility* Ability, const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
	{
		return true;
	}

	/** Applies the cost after the ability has committed. */
	virtual void ApplyCost(const UTPSCPPGameplayAbility* Ability, const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const
	{
	}
};
