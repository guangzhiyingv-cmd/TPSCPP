#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPSCPPPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

UCLASS(abstract)
class ATPSCPPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATPSCPPPlayerController();

	void SetHealthHUD(float Health, float MaxHealth);
	void SetScoreHUD(float Score);
	void SetDefeatsHUD(int32 Defeats);
	void SetAmmoHUD(int32 Ammo, int32 ReserveAmmo);

	/** Shows the scope reticle on the character overlay, or hides it when null. */
	void SetScopeReticleHUD(class UTexture2D* ReticleTexture);

	float GetServerTime() const;
	void SetTimeHUD(float Time);
	void ShowWarmupHUD(bool bShow);
	void ShowCharacterOverlayHUD(bool bShow);
	void SetCountdownHUD(float Seconds);
	float GetWarmupRemainingTime() const;
	float GetMatchRemainingTime() const;
	void ShowPostMatchHUD(bool bShow);
	void SetPostMatchCountdownHUD(float Seconds);
	void SetPostMatchTopPlayerHUD(const FString& PlayerName, int32 Kills);
	float GetPostMatchRemainingTime() const;

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	void RequestServerTime();

private:
	class APlayerHUD* PlayerHUD;
	float ClientServerDelta = 0.f;
	FTimerHandle TimeSyncTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Time", meta = (ClampMin = 0.1f))
	float TimeSyncFrequency = 5.f;
};
