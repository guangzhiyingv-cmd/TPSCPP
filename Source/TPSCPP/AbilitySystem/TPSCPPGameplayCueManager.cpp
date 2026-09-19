// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TPSCPPGameplayCueManager.h"
#include "AbilitySystem/TPSCPPNativeGameplayCues.h"
#include "GameplayCueSet.h"

TSharedPtr<FStreamableHandle> UTPSCPPGameplayCueManager::InitObjectLibrary(FGameplayCueObjectLibrary& Library)
{
	// The base class empties and refills the cue set, so the native cues have to be appended after it.
	const TSharedPtr<FStreamableHandle> Handle = Super::InitObjectLibrary(Library);

	if (Library.CueSet)
	{
		TPSCPPNativeGameplayCues::AddToCueSet(*Library.CueSet);
	}

	return Handle;
}
