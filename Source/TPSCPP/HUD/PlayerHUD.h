#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;
	/** Distance between the center and the edge crosshair textures in pixels. */
	float CrosshairSpread = 16.f;
};

UCLASS()
class TPSCPP_API APlayerHUD : public AHUD
{
	GENERATED_BODY()
	
private:
	FHUDPackage HUDPackage;
	bool bShowCrosshair = true;

	APlayerController* PlayerController;

public:
	virtual void DrawHUD() override;

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	TSubclassOf<class UUserWidget> WarmupOverlayClass;

	class UWarmupOverlay* WarmupOverlay;

	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	TSubclassOf<class UUserWidget> PostMatchOverlayClass;

	class UPostMatchOverlay* PostMatchOverlay;

	void ShowWarmupOverlay(bool bShow);
	void ShowCharacterOverlay(bool bShow);
	void SetWarmupCountdownText(float Seconds);
	void ShowPostMatchOverlay(bool bShow);
	void SetPostMatchCountdownText(float Seconds);
	void SetPostMatchTopPlayerText(const FString& PlayerName, int32 Kills);
	void SetShowCrosshair(bool bShow);

protected:
	virtual void BeginPlay() override;
	void AddCharacterOverlay();
	void AddWarmupOverlay();
	void AddPostMatchOverlay();

private:
	void DrawCrosshairTexture(UTexture2D* Texture, const FVector2D& Center, const FVector2D& Offset);
};
