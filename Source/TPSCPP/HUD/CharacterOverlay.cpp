// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
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

