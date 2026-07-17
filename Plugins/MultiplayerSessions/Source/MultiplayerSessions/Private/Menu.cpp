// Fill out your copyright notice in the Description page of Project Settings.

#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Engine/GameInstance.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch)
{
    // Store session parameters for later use when the host button is clicked
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;

    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    bIsFocusable = true;

    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }

    // Cache the subsystem
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    // Bind button click events
    if (HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
    }
    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
    }
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddUObject(this, &ThisClass::OncreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddUObject(this, &ThisClass::OnDestroySession);
    }
}

void UMenu::MenuTearDown()
{
    RemoveFromParent();

    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
            PlayerController->SetShowMouseCursor(false);
        }
    }
}

void UMenu::NativeDestruct()
{
    MenuTearDown();
    Super::NativeDestruct();
}

void UMenu::HostButtonClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Hosting session with %d players, match type: %s"), NumPublicConnections, *MatchType));
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}

	
}

void UMenu::JoinButtonClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, TEXT("Searching for sessions..."));
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
    }
}

void UMenu::OncreateSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, TEXT("Session created successfully!"));
        }

        UWorld* World = GetWorld();
        if (World)
        {
            World->ServerTravel("/Game/Maps/Lobby?listen");
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to create session!"));
        }
    }
}

void UMenu::OnFindSessions(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SessionResults)
{
    int32 Count = SessionResults.Num();
    if (bWasSuccessful)
    {
        if (Count > 0)
        {
            // Cache results and try joining the first session
            CachedSessionResults = SessionResults;
            LastSessionSearchIndex = 0;

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
                    FString::Printf(TEXT("Found %d session(s). Trying session %d/%d..."),
                        Count, LastSessionSearchIndex + 1, Count));
            }
            if (MultiplayerSessionsSubsystem)
            {
                MultiplayerSessionsSubsystem->JoinSession(CachedSessionResults[LastSessionSearchIndex]);
            }
        }
        else
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow,
                    TEXT("Search succeeded but no sessions found. Host a game first."));
            }
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
                TEXT("Failed to find sessions! Check network or Steam connection."));
        }
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, TEXT("Joined session!"));
        }
        // Clear cached results since we joined successfully
        CachedSessionResults.Empty();
        LastSessionSearchIndex = -1;
    }
    else
    {
        // Failed to join the current session; try the next one in the cache
        LastSessionSearchIndex++;
        int32 Total = CachedSessionResults.Num();

        if (LastSessionSearchIndex < Total)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
                    FString::Printf(TEXT("Failed to join session %d/%d. Trying next..."),
                        LastSessionSearchIndex, Total));
            }
            if (MultiplayerSessionsSubsystem)
            {
                MultiplayerSessionsSubsystem->JoinSession(CachedSessionResults[LastSessionSearchIndex]);
            }
        }
        else
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
                    TEXT("All join attempts failed. No available sessions to join."));
            }
            // Clean up cached results
            CachedSessionResults.Empty();
            LastSessionSearchIndex = -1;
        }
    }
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, TEXT("Session destroyed."));
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to destroy session!"));
        }
    }
}

