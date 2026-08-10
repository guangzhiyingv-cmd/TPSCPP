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

	APlayerController* PlayerController;

public:
	virtual void DrawHUD() override;

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	class UCharacterOverlay* CharacterOverlay;

protected:
	virtual void BeginPlay() override;
	void AddCharacterOverlay();

private:
	void DrawCrosshairTexture(UTexture2D* Texture, const FVector2D& Center, const FVector2D& Offset);
};
