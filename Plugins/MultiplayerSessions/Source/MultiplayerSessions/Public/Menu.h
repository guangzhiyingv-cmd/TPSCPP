// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Menu.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable)
    void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString("FreeForAll"));

    void MenuTearDown();

protected:
    virtual void NativeDestruct() override;

    // Callbacks for button clicks
    UFUNCTION()
    void HostButtonClicked();

    UFUNCTION()
    void JoinButtonClicked();

    // BindWidget matches buttons created in the UMG Blueprint by name
    UPROPERTY(meta = (BindWidget))
    class UButton* HostButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* JoinButton;


private:
    void OncreateSession(bool bWasSuccessful);
    void OnFindSessions(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SessionResults);
    void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySession(bool bWasSuccessful);

    class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    int32 NumPublicConnections{4};
    FString MatchType{TEXT("FreeForAll")};
    TArray<FOnlineSessionSearchResult> CachedSessionResults;
    int32 LastSessionSearchIndex{-1};
};
