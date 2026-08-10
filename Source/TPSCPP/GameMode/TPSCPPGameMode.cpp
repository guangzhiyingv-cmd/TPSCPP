#include "GameMode/TPSCPPGameMode.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "Character/TPSCPPCharacter.h"

ATPSCPPGameMode::ATPSCPPGameMode()
{

}

void ATPSCPPGameMode::PlayerEliminated(ATPSCPPCharacter* ElimmedCharacter, ATPSCPPPlayerController* VictimController, ATPSCPPPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}
