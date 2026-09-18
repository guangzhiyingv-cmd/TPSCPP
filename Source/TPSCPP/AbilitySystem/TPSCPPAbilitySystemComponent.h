// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "TPSCPPAbilitySystemComponent.generated.h"

/**
 * Project AbilitySystemComponent owned by the PlayerState.
 */
UCLASS()
class TPSCPP_API UTPSCPPAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Applies a one-shot damage GameplayEffect to this component's owner. */
	void ApplyDamage(float Damage, AActor* DamageCauser, AController* InstigatorController);
};
