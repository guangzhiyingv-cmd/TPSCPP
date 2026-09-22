#include "PlayerState/TPSCPPPlayerState.h"
#include "AbilitySystem/TPSCPPAbilitySystemComponent.h"
#include "AbilitySystem/TPSCPPHealthSet.h"
#include "Character/TPSCPPCharacter.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "Net/UnrealNetwork.h"

ATPSCPPPlayerState::ATPSCPPPlayerState()
{
	// APlayerState defaults its net update frequency to 1 Hz, but this is the actor that hosts the
	// ability system and therefore the replicated health attributes: at 1 Hz a health change waits
	// up to a full second before the owning client sees it. Match Lyra (ALyraPlayerState) here.
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UTPSCPPAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Player controlled ASC: gameplay effects go to the owner, attributes to everyone.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Named default subobject so both server and clients create the same set and replicate into it.
	HealthSet = CreateDefaultSubobject<UTPSCPPHealthSet>(TEXT("HealthSet"));
}

void ATPSCPPPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent)
	{
		// Avatar may be null this early; it is rebound in InitAbilityActorInfoForPawn on possess.
		AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
	}
}

void ATPSCPPPlayerState::InitAbilityActorInfoForPawn(APawn* Pawn)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, Pawn);
	}
}

void ATPSCPPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCPPPlayerState, Defeats);
	DOREPLIFETIME(ATPSCPPPlayerState, WarmupStartTime);
	DOREPLIFETIME(ATPSCPPPlayerState, WarmupTime);
	DOREPLIFETIME(ATPSCPPPlayerState, MatchStartTime);
	DOREPLIFETIME(ATPSCPPPlayerState, MatchTime);
	DOREPLIFETIME(ATPSCPPPlayerState, PostMatchStartTime);
	DOREPLIFETIME(ATPSCPPPlayerState, PostMatchTime);
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

void ATPSCPPPlayerState::SetWarmupData(float InWarmupStartTime, float InWarmupTime)
{
	if (!HasAuthority())
	{
		return;
	}

	WarmupStartTime = InWarmupStartTime;
	WarmupTime = InWarmupTime;
}

void ATPSCPPPlayerState::SetMatchData(float InMatchStartTime, float InMatchTime)
{
	if (!HasAuthority())
	{
		return;
	}

	MatchStartTime = InMatchStartTime;
	MatchTime = InMatchTime;
}

void ATPSCPPPlayerState::SetPostMatchData(float InPostMatchStartTime, float InPostMatchTime)
{
	if (!HasAuthority())
	{
		return;
	}

	PostMatchStartTime = InPostMatchStartTime;
	PostMatchTime = InPostMatchTime;
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
