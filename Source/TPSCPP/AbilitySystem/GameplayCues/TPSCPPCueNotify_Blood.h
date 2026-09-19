// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "TPSCPPCueNotify_Blood.generated.h"

/**
 * Blood hit feedback. The assets stay configured on the hit character; this cue only decides where
 * and how they play, so hit reactions can later be driven by gameplay effects or prediction.
 */
UCLASS()
class TPSCPP_API UTPSCPPCueNotify_Blood : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UTPSCPPCueNotify_Blood();

	virtual void PostInitProperties() override;
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;
};
