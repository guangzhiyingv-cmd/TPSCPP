// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/** 
 * 
 */
UCLASS()
class TPSCPP_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	/** Minimum number of players required to trigger seamless travel to the game level. Counts all players including the host. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
	int32 MinPlayersToStart = 2;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

};
