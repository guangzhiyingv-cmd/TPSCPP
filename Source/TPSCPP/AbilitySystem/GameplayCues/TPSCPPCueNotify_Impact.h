// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "AbilitySystem/TPSCPPGameplayCueTypes.h"
#include "TPSCPPCueNotify_Impact.generated.h"

/**
 * Surface impact feedback for projectiles. The assets stay configured on the projectile blueprint
 * and are carried in the cue parameters, because a point-blank projectile never exists on clients.
 */
UCLASS()
class TPSCPP_API UTPSCPPCueNotify_Impact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UTPSCPPCueNotify_Impact();

	virtual void PostInitProperties() override;
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;
};
