#include "HUD/PlayerHUD.h"
#include "Engine/Texture2D.h"

void APlayerHUD::DrawHUD()
{
	Super::DrawHUD();

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
