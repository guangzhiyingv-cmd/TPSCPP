// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCPP.h"
#include "Modules/ModuleManager.h"
#include "AbilitySystemGlobals.h"

class FTPSCPPGameModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		// Required before any AbilitySystemComponent is used.
		UAbilitySystemGlobals::Get().InitGlobalData();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FTPSCPPGameModule, TPSCPP, "TPSCPP" );

DEFINE_LOG_CATEGORY(LogTPSCPP)
