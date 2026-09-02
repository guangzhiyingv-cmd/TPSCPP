#include "GameState/TPSCPPGameState.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void ATPSCPPGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCPPGameState, TopKillerName);
	DOREPLIFETIME(ATPSCPPGameState, TopKills);
}

void ATPSCPPGameState::UpdateTopKiller()
{
	if (!HasAuthority())
	{
		return;
	}

	TopKillerName.Reset();
	TopKills = 0;

	for (APlayerState* PS : PlayerArray)
	{
		if (PS && PS->GetScore() > TopKills)
		{
			TopKills = FMath::FloorToInt(PS->GetScore());
			TopKillerName = PS->GetPlayerName();
		}
	}
}

void ATPSCPPGameState::OnRep_TopKiller()
{
}
