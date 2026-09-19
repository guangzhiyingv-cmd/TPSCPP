// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCPP.h"
#include "Modules/ModuleManager.h"
#include "AbilitySystem/TPSCPPNativeGameplayCues.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"

namespace
{
	/** Adds the project's native cue notifies to the gameplay cue manager's runtime cue set. */
	void RegisterNativeGameplayCues()
	{
		UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
		UGameplayCueSet* CueSet = CueManager ? CueManager->GetRuntimeCueSet() : nullptr;
		if (!CueSet)
		{
			// The asset registry can still be gathering at startup, in which case the cue manager
			// builds its cue set later and this is retried on world initialization.
			return;
		}

		TPSCPPNativeGameplayCues::AddToCueSet(*CueSet);
	}

	void HandlePreWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
	{
		// The cue manager rebuilds its cue sets when a world is initialized, which drops the native
		// cues that were registered earlier.
		RegisterNativeGameplayCues();
	}
}

class FTPSCPPGameModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		// Required before any AbilitySystemComponent is used.
		UAbilitySystemGlobals::Get().InitGlobalData();

		// Gameplay cue notifies are normally discovered as blueprint assets, so the project's native
		// ones are registered explicitly (and again whenever the cue manager rebuilds its cue sets).
		RegisterNativeGameplayCues();
		FWorldDelegates::OnPreWorldInitialization.AddStatic(&HandlePreWorldInitialization);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FTPSCPPGameModule, TPSCPP, "TPSCPP" );

DEFINE_LOG_CATEGORY(LogTPSCPP)
