// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TPSCPPGameplayAbility.generated.h"

class ATPSCPPCharacter;
class AWeapon;
class UCombatComponent;
class UTPSCPPAbilityCost;

/**
 * Project gameplay ability. Server authoritative for now (prediction is a later phase) with
 * support for extra costs such as weapon ammo.
 */
UCLASS(Abstract)
class TPSCPP_API UTPSCPPGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTPSCPPGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Extra costs evaluated on top of the cost gameplay effect, e.g. weapon ammo. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Cost")
	TArray<TObjectPtr<UTPSCPPAbilityCost>> AdditionalCosts;

	/** Returns the character owning this ability. */
	UFUNCTION(BlueprintPure, Category = "Ability")
	ATPSCPPCharacter* GetTPSCPPCharacter() const;

	/** Returns the currently equipped weapon, if any. */
	UFUNCTION(BlueprintPure, Category = "Ability")
	AWeapon* GetEquippedWeapon() const;

	/** Returns the combat component of the owning character. */
	UFUNCTION(BlueprintPure, Category = "Ability")
	UCombatComponent* GetCombatComponent() const;
};
