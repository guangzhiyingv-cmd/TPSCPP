#include "PlayerState/TPSCPPPlayerState.h"
#include "Character/TPSCPPCharacter.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "Net/UnrealNetwork.h"

void ATPSCPPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCPPPlayerState, Defeats);
}

void ATPSCPPPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<ATPSCPPCharacter>(GetPawn()) : Character;
	if (Character)
	{
		PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetPlayerController()) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetScoreHUD(GetScore());
		}
	}
}

void ATPSCPPPlayerState::AddToScore(float ScoreAmount)
{
	if (HasAuthority())
	{
		SetScore(GetScore() + ScoreAmount);

		Character = Character == nullptr ? Cast<ATPSCPPCharacter>(GetPawn()) : Character;
		if (Character)
		{
			PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetPlayerController()) : PlayerController;
			if (PlayerController)
			{
				PlayerController->SetScoreHUD(GetScore());

			}
		}
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("ATPSCPPPlayerState::AddToScore is not called on server!"))
	}
}

void ATPSCPPPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	if (HasAuthority())
	{
		Defeats += DefeatsAmount;

		Character = Character == nullptr ? Cast<ATPSCPPCharacter>(GetPawn()) : Character;
		if (Character)
		{
			PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetPlayerController()) : PlayerController;
			if (PlayerController)
			{
				PlayerController->SetDefeatsHUD(Defeats);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ATPSCPPPlayerState::AddToDefeats is not called on server!"));
	}
}

void ATPSCPPPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<ATPSCPPCharacter>(GetPawn()) : Character;
	if (Character)
	{
		PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetPlayerController()) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetDefeatsHUD(Defeats);
		}
	}
}
