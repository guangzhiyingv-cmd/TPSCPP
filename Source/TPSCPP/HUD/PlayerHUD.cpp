#include "HUD/PlayerHUD.h"
#include "Engine/Texture2D.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/WarmupOverlay.h"
#include "HUD/PostMatchOverlay.h"

void APlayerHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowCrosshair)
	{
		return;
	}

	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2D Center(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f);

	DrawCrosshairTexture(HUDPackage.CrosshairCenter, Center, FVector2D::ZeroVector);
	DrawCrosshairTexture(HUDPackage.CrosshairLeft, Center, FVector2D(-HUDPackage.CrosshairSpread, 0.f));
	DrawCrosshairTexture(HUDPackage.CrosshairRight, Center, FVector2D(HUDPackage.CrosshairSpread, 0.f));
	DrawCrosshairTexture(HUDPackage.CrosshairTop, Center, FVector2D(0.f, -HUDPackage.CrosshairSpread));
	DrawCrosshairTexture(HUDPackage.CrosshairBottom, Center, FVector2D(0.f, HUDPackage.CrosshairSpread));
}

void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = GetOwningPlayerController();
	AddCharacterOverlay();
	AddWarmupOverlay();
	AddPostMatchOverlay();
}

void APlayerHUD::AddCharacterOverlay()
{
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
		CharacterOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerHUD::AddWarmupOverlay()
{
	if (PlayerController && WarmupOverlayClass)
	{
		WarmupOverlay = CreateWidget<UWarmupOverlay>(PlayerController, WarmupOverlayClass);
		WarmupOverlay->AddToViewport();
		WarmupOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerHUD::AddPostMatchOverlay()
{
	if (PlayerController && PostMatchOverlayClass)
	{
		PostMatchOverlay = CreateWidget<UPostMatchOverlay>(PlayerController, PostMatchOverlayClass);
		PostMatchOverlay->AddToViewport();
		PostMatchOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerHUD::ShowWarmupOverlay(bool bShow)
{
	if (WarmupOverlay)
	{
		WarmupOverlay->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void APlayerHUD::ShowCharacterOverlay(bool bShow)
{
	if (CharacterOverlay)
	{
		CharacterOverlay->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void APlayerHUD::SetWarmupCountdownText(float Seconds)
{
	if (WarmupOverlay)
	{
		WarmupOverlay->SetCountdownText(Seconds);
	}
}

void APlayerHUD::ShowPostMatchOverlay(bool bShow)
{
	if (PostMatchOverlay)
	{
		PostMatchOverlay->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void APlayerHUD::SetPostMatchCountdownText(float Seconds)
{
	if (PostMatchOverlay)
	{
		PostMatchOverlay->SetCountdownText(Seconds);
	}
}

void APlayerHUD::SetPostMatchTopPlayerText(const FString& PlayerName, int32 Kills)
{
	if (PostMatchOverlay)
	{
		PostMatchOverlay->SetTopPlayerInfo(PlayerName, Kills);
	}
}

void APlayerHUD::SetShowCrosshair(bool bShow)
{
	bShowCrosshair = bShow;
}

void APlayerHUD::DrawCrosshairTexture(UTexture2D* Texture, const FVector2D& Center, const FVector2D& Offset)
{
	if (!Texture) return;

	const FVector2D TextureSize(Texture->GetSizeX(), Texture->GetSizeY());
	const FVector2D DrawPosition = Center + Offset - TextureSize * 0.5f;

	DrawTexture(
		Texture,
		DrawPosition.X,
		DrawPosition.Y,
		TextureSize.X,
		TextureSize.Y,
		0.f,
		0.f,
		1.f,
		1.f);
}
