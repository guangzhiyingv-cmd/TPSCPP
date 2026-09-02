#include "GameMode/TPSCPPGameMode.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "Character/TPSCPPCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/TPSCPPPlayerState.h"
#include "GameState/TPSCPPGameState.h"


ATPSCPPGameMode::ATPSCPPGameMode()
{
	bDelayedStart = true;
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = ATPSCPPGameState::StaticClass();
}

void ATPSCPPGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetMatchState() == MatchState::WaitingToStart)
	{
		const int32 PlayerCount = GetNumPlayers();
		if (!bWarmupCountdownStarted && PlayerCount >= MinPlayersToStart)
		{
			bWarmupCountdownStarted = true;
			WarmupStartTime = GetWorld()->GetTimeSeconds();
			BroadcastWarmupStart();
		}

		if (bWarmupCountdownStarted && GetWarmupRemainingTime() <= 0.f)
		{
			StartMatch();
		}
	}
	else if (GetMatchState() == MatchState::InProgress)
	{
		if (GetMatchRemainingTime() <= 0.f)
		{
			EndMatch();
		}
	}
	else if (GetMatchState() == MatchState::WaitingPostMatch)
	{
		if (GetPostMatchRemainingTime() <= 0.f)
		{
			RestartGame();
		}
	}
}

void ATPSCPPGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	MatchStartTime = GetWorld()->GetTimeSeconds();
	BroadcastMatchStart();
}

void ATPSCPPGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	bPostMatchStarted = true;
	PostMatchStartTime = GetWorld()->GetTimeSeconds();
	DestroyAllPlayerPawns();
	BroadcastPostMatchStart();
}

void ATPSCPPGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ATPSCPPPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ATPSCPPPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}

	if (GetMatchState() == MatchState::WaitingToStart && bWarmupCountdownStarted)
	{
		PS->SetWarmupData(WarmupStartTime, WarmupTime);
	}
	else if (GetMatchState() == MatchState::InProgress)
	{
		PS->SetMatchData(MatchStartTime, MatchTime);
	}
	else if (GetMatchState() == MatchState::WaitingPostMatch)
	{
		PS->SetPostMatchData(PostMatchStartTime, PostMatchTime);
	}
}

float ATPSCPPGameMode::GetWarmupRemainingTime() const
{
	if (!bWarmupCountdownStarted)
	{
		return WarmupTime;
	}
	return FMath::Max(0.f, WarmupTime - (GetWorld()->GetTimeSeconds() - WarmupStartTime));
}

void ATPSCPPGameMode::BroadcastWarmupStart()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATPSCPPPlayerController* PC = Cast<ATPSCPPPlayerController>(It->Get()))
		{
			if (ATPSCPPPlayerState* PS = PC->GetPlayerState<ATPSCPPPlayerState>())
			{
				PS->SetWarmupData(WarmupStartTime, WarmupTime);
			}
		}
	}
}

float ATPSCPPGameMode::GetMatchRemainingTime() const
{
	if (MatchStartTime <= 0.f)
	{
		return MatchTime;
	}
	return FMath::Max(0.f, MatchTime - (GetWorld()->GetTimeSeconds() - MatchStartTime));
}

void ATPSCPPGameMode::BroadcastMatchStart()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATPSCPPPlayerController* PC = Cast<ATPSCPPPlayerController>(It->Get()))
		{
			if (ATPSCPPPlayerState* PS = PC->GetPlayerState<ATPSCPPPlayerState>())
			{
				PS->SetMatchData(MatchStartTime, MatchTime);
			}
		}
	}
}

float ATPSCPPGameMode::GetPostMatchRemainingTime() const
{
	if (!bPostMatchStarted || PostMatchStartTime <= 0.f)
	{
		return PostMatchTime;
	}
	return FMath::Max(0.f, PostMatchTime - (GetWorld()->GetTimeSeconds() - PostMatchStartTime));
}

void ATPSCPPGameMode::BroadcastPostMatchStart()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATPSCPPPlayerController* PC = Cast<ATPSCPPPlayerController>(It->Get()))
		{
			if (ATPSCPPPlayerState* PS = PC->GetPlayerState<ATPSCPPPlayerState>())
			{
				PS->SetPostMatchData(PostMatchStartTime, PostMatchTime);
			}
		}
	}
}

void ATPSCPPGameMode::DestroyAllPlayerPawns()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = Cast<APlayerController>(It->Get()))
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (ATPSCPPCharacter* Character = Cast<ATPSCPPCharacter>(Pawn))
				{
					if (UCombatComponent* Combat = Character->GetCombat())
					{
						if (AWeapon* Weapon = Combat->GetEquippedWeapon())
						{
							Weapon->Destroy();
						}
					}
				}
				Pawn->Destroy();
			}
		}
	}
}

void ATPSCPPGameMode::PlayerEliminated(ATPSCPPCharacter* ElimmedCharacter, ATPSCPPPlayerController* VictimController, ATPSCPPPlayerController* AttackerController)
{
	ATPSCPPPlayerState* AttackerPlayerState = AttackerController ? Cast<ATPSCPPPlayerState>(AttackerController->PlayerState) : nullptr;
	ATPSCPPPlayerState* VictimPlayerState = VictimController ? Cast<ATPSCPPPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f);
		if (ATPSCPPGameState* GS = GetGameState<ATPSCPPGameState>())
		{
			GS->UpdateTopKiller();
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ATPSCPPGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (GetMatchState() != MatchState::InProgress)
	{
		return;
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		if (PlayerStarts.Num() == 0)
		{
			return;
		}
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
