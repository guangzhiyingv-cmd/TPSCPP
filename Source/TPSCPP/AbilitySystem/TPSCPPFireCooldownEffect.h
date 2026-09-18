// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "TPSCPPFireCooldownEffect.generated.h"

/**
 * Weapon fire cooldown. The duration comes from the Data.Cooldown SetByCaller magnitude (the
 * weapon's fire delay) and the granted tag blocks re-activation until the effect expires.
 */
UCLASS()
class TPSCPP_API UTPSCPPFireCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UTPSCPPFireCooldownEffect();
};
