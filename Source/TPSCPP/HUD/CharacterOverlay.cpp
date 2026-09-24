// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Curves/CurveFloat.h"

void UCharacterOverlay::SetHealthPercent(float Health, float MaxHealth)
{
	if (MaxHealth <= 0.f) return;

	const float TargetPercent = FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
	TargetHealthPercent = TargetPercent;

	// Without a curve assigned, snap directly to the target value
	if (!HealthBarCurve)
	{
		CurrentHealthPercent = TargetPercent;
		if (HealthBar)
		{
			HealthBar->SetPercent(CurrentHealthPercent);
		}
		return;
	}

	StartHealthPercent = CurrentHealthPercent;
	InterpProgress = 0.f;
	bInterpolating = true;
}

void UCharacterOverlay::SetTimeText(float Seconds)
{
	if (!TimeText)
	{
		return;
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Seconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 RemainingSeconds = TotalSeconds % 60;
	TimeText->SetText(FText::FromString(
		FString::Printf(TEXT("%02d:%02d"), Minutes, RemainingSeconds)));
}

void UCharacterOverlay::SetScopeReticle(UTexture2D* ReticleTexture)
{
	if (!ScopeReticleImage)
	{
		return;
	}

	if (ReticleTexture)
	{
		ScopeReticleImage->SetBrushFromTexture(ReticleTexture);
		ScopeReticleImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		// Collapsed instead of Hidden so the widget takes no space when there is no scope reticle.
		ScopeReticleImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	// The scope reticle visibility is owned by the ADS state, so start hidden no matter what the
	// designer left on the image.
	SetScopeReticle(nullptr);
}

void UCharacterOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bInterpolating || !HealthBar || !HealthBarCurve) return;

	InterpProgress = FMath::Clamp(InterpProgress + InDeltaTime * HealthBarInterpSpeed, 0.f, 1.f);
	const float CurveValue = HealthBarCurve->GetFloatValue(InterpProgress);
	CurrentHealthPercent = FMath::Lerp(StartHealthPercent, TargetHealthPercent, CurveValue);
	HealthBar->SetPercent(CurrentHealthPercent);

	if (InterpProgress >= 1.f)
	{
		CurrentHealthPercent = TargetHealthPercent;
		HealthBar->SetPercent(CurrentHealthPercent);
		bInterpolating = false;
	}
}

