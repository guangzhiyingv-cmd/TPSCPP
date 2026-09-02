// Copyright Epic Games, Inc. All Rights Reserved.


#include "PlayerController/TPSCPPPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "HUD/PlayerHUD.h"
#include "HUD/CharacterOverlay.h"
#include "TPSCPP.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "TimerManager.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameState.h"
#include "PlayerState/TPSCPPPlayerState.h"
#include "GameState/TPSCPPGameState.h"

ATPSCPPPlayerController::ATPSCPPPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATPSCPPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	PlayerHUD = Cast<APlayerHUD>(GetHUD());
	if (IsLocalPlayerController())
	{
		RequestServerTime();
		GetWorldTimerManager().SetTimer(
			TimeSyncTimerHandle,
			this,
			&ATPSCPPPlayerController::RequestServerTime,
			TimeSyncFrequency,
			true);
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogTPSCPP, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ATPSCPPPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocalPlayerController())
	{
		return;
	}

	SetTimeHUD(GetServerTime());

	const AGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGameState>() : nullptr;
	const FName State = GS ? GS->GetMatchState() : MatchState::WaitingToStart;

	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->SetShowCrosshair(State == MatchState::InProgress);
	}

	if (State == MatchState::InProgress)
	{
		ShowWarmupHUD(false);
		ShowPostMatchHUD(false);
		ShowCharacterOverlayHUD(true);
		SetTimeHUD(GetMatchRemainingTime());
	}
	else if (State == MatchState::WaitingPostMatch)
	{
		ShowWarmupHUD(false);
		ShowCharacterOverlayHUD(false);
		ShowPostMatchHUD(true);
		SetPostMatchCountdownHUD(GetPostMatchRemainingTime());

		if (const ATPSCPPGameState* TPSGS = Cast<ATPSCPPGameState>(GS))
		{
			SetPostMatchTopPlayerHUD(TPSGS->TopKillerName, TPSGS->TopKills);
		}
	}
	else
	{
		ShowWarmupHUD(true);
		ShowCharacterOverlayHUD(false);
		ShowPostMatchHUD(false);
		SetCountdownHUD(GetWarmupRemainingTime());
	}
}

void ATPSCPPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ATPSCPPPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

float ATPSCPPPlayerController::GetServerTime() const
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ATPSCPPPlayerController::RequestServerTime()
{
	ServerRequestServerTime(GetWorld()->GetTimeSeconds());
}

void ATPSCPPPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ATPSCPPPlayerController::ClientReportServerTime_Implementation(
	float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ATPSCPPPlayerController::SetTimeHUD(float Time)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay && PlayerHUD->CharacterOverlay->TimeText)
	{
		PlayerHUD->CharacterOverlay->SetTimeText(Time);
	}
}

void ATPSCPPPlayerController::ShowWarmupHUD(bool bShow)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->ShowWarmupOverlay(bShow);
	}
}

void ATPSCPPPlayerController::ShowCharacterOverlayHUD(bool bShow)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->ShowCharacterOverlay(bShow);
	}
}

void ATPSCPPPlayerController::SetCountdownHUD(float Seconds)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->SetWarmupCountdownText(Seconds);
	}
}

float ATPSCPPPlayerController::GetWarmupRemainingTime() const
{
	const ATPSCPPPlayerState* PS = GetPlayerState<ATPSCPPPlayerState>();
	if (!PS)
	{
		return 0.f;
	}

	if (PS->WarmupStartTime > 0.f)
	{
		return FMath::Max(0.f, PS->WarmupTime - (GetServerTime() - PS->WarmupStartTime));
	}
	return PS->WarmupTime;
}

float ATPSCPPPlayerController::GetMatchRemainingTime() const
{
	const ATPSCPPPlayerState* PS = GetPlayerState<ATPSCPPPlayerState>();
	if (!PS || PS->MatchStartTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, PS->MatchTime - (GetServerTime() - PS->MatchStartTime));
}

void ATPSCPPPlayerController::ShowPostMatchHUD(bool bShow)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->ShowPostMatchOverlay(bShow);
	}
}

void ATPSCPPPlayerController::SetPostMatchCountdownHUD(float Seconds)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->SetPostMatchCountdownText(Seconds);
	}
}

void ATPSCPPPlayerController::SetPostMatchTopPlayerHUD(const FString& PlayerName, int32 Kills)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		PlayerHUD->SetPostMatchTopPlayerText(PlayerName, Kills);
	}
}

float ATPSCPPPlayerController::GetPostMatchRemainingTime() const
{
	const ATPSCPPPlayerState* PS = GetPlayerState<ATPSCPPPlayerState>();
	if (!PS || PS->PostMatchStartTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, PS->PostMatchTime - (GetServerTime() - PS->PostMatchStartTime));
}

void ATPSCPPPlayerController::SetHealthHUD(float Health, float MaxHealth)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay)
	{
		PlayerHUD->CharacterOverlay->SetHealthPercent(Health, MaxHealth);
	}
}

void ATPSCPPPlayerController::SetScoreHUD(float Score)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay && PlayerHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PlayerHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ATPSCPPPlayerController::SetDefeatsHUD(int32 Defeats)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay && PlayerHUD->CharacterOverlay->DefeatsAmount)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		PlayerHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ATPSCPPPlayerController::SetAmmoHUD(int32 Ammo, int32 ReserveAmmo)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay && PlayerHUD->CharacterOverlay->AmmoAmount && PlayerHUD->CharacterOverlay->ReloadAmount)
	{
		PlayerHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(FString::FromInt(Ammo)));
		PlayerHUD->CharacterOverlay->ReloadAmount->SetText(FText::FromString(FString::FromInt( ReserveAmmo)));
	}
}
