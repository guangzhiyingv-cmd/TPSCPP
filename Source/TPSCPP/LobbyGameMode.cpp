// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (NewPlayer)
    {
        int32 PlayerCount = GetNumPlayers();
        APlayerState* NewPS = NewPlayer->GetPlayerState<APlayerState>();
        FString PlayerName = NewPS ? NewPS->GetPlayerName() : TEXT("Unknown");

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 10.f, FColor::Cyan,
                FString::Printf(TEXT("Player joined: %s (Total: %d)"), *PlayerName, PlayerCount)
            );
        }

        if (PlayerCount >= MinPlayersToStart)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1, 10.f, FColor::Green,
                    FString::Printf(TEXT("Starting game with %d players!"), PlayerCount)
                );
            }

            GetWorld()->SeamlessTravel(TEXT("/Game/Maps/GameLevel"));
        }
    }
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    if (Exiting)
    {
        FString PlayerName = Exiting->GetName();
        int32 PlayerCount = GetNumPlayers();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 10.f, FColor::Orange,
                FString::Printf(TEXT("Player left: %s (Total: %d)"), *PlayerName, PlayerCount)
            );
        }
    }
}
