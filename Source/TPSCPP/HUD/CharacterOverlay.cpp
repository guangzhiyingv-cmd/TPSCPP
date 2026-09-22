// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Curves/CurveFloat.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
// Debug-only timing of the health bar interpolation: this is the visible (cosmetic) part of the
// health latency, separate from the replication cost measured in TPSCPPCharacter.cpp. One overlay
// per PIE client, so plain file scope state is enough.
namespace
{
	double GHealthInterpStart = 0.0;
	bool bGHealthInterpTiming = false;

	bool HealthLatencyDebugEnabled()
	{
		static IConsoleVariable* CVar =
			IConsoleManager::Get().FindConsoleVariable(TEXT("tpscpp.DebugHealthLatency"));
		return CVar && CVar->GetInt() > 0;
	}

	void LogHealthInterp(const float InterpSpeed, const UCurveFloat* Curve, double StartSeconds)
	{
		UE_LOG(LogTemp, Warning, TEXT("HealthLatency|VISUAL interp=%.0fms speed=%.2f curve=%s"),
			(FPlatformTime::Seconds() - StartSeconds) * 1000.0,
			InterpSpeed,
			*GetNameSafe(Curve));
	}
}
#endif

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

#if !UE_BUILD_SHIPPING
		if (HealthLatencyDebugEnabled())
		{
			// No curve: the bar snaps, so the cosmetic term of the latency is zero.
			LogHealthInterp(HealthBarInterpSpeed, HealthBarCurve, FPlatformTime::Seconds());
		}
#endif
		return;
	}

	StartHealthPercent = CurrentHealthPercent;
	InterpProgress = 0.f;
	bInterpolating = true;

#if !UE_BUILD_SHIPPING
	if (HealthLatencyDebugEnabled())
	{
		GHealthInterpStart = FPlatformTime::Seconds();
		bGHealthInterpTiming = true;
	}
#endif
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

#if !UE_BUILD_SHIPPING
		if (bGHealthInterpTiming)
		{
			bGHealthInterpTiming = false;
			LogHealthInterp(HealthBarInterpSpeed, HealthBarCurve, GHealthInterpStart);
		}
#endif
	}
}

