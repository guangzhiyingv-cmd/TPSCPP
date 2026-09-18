// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "TPSCPPDamageEffect.generated.h"

/**
 * One-shot damage effect. The amount is supplied at runtime through the Data.Damage SetByCaller
 * magnitude, so no damage value is hard coded here.
 */
UCLASS()
class TPSCPP_API UTPSCPPDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UTPSCPPDamageEffect();
};
