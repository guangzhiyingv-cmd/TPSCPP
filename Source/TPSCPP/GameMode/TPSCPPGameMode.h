// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TPSCPPGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ATPSCPPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ATPSCPPGameMode();

	virtual void PlayerEliminated(class ATPSCPPCharacter* ElimmedCharacter, class ATPSCPPPlayerController* VictimController, class ATPSCPPPlayerController* AttackerController);
};



