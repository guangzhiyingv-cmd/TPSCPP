#include "GameMode/TPSCPPGameMode.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "Character/TPSCPPCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/TPSCPPPlayerState.h"


ATPSCPPGameMode::ATPSCPPGameMode()
{

}

void ATPSCPPGameMode::PlayerEliminated(ATPSCPPCharacter* ElimmedCharacter, ATPSCPPPlayerController* VictimController, ATPSCPPPlayerController* AttackerController)
{
	ATPSCPPPlayerState* AttackerPlayerState = AttackerController ? Cast<ATPSCPPPlayerState>(AttackerController->PlayerState) : nullptr;
	ATPSCPPPlayerState* VictimPlayerState = VictimController ? Cast<ATPSCPPPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f);
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
