// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "TPSCPPGameplayCueManager.generated.h"

/**
 * Registers the project's native (C++) gameplay cue notifies. The stock manager only discovers cue
 * notifies that exist as blueprint assets, so native classes have to be appended after every build
 * of a cue set.
 */
UCLASS()
class TPSCPP_API UTPSCPPGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()

protected:
	//~UGameplayCueManager interface
	virtual TSharedPtr<FStreamableHandle> InitObjectLibrary(FGameplayCueObjectLibrary& Library) override;
	//~End of UGameplayCueManager interface
};
