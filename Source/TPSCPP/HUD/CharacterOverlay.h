// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class TPSCPP_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DefeatsAmount;

	/** Starts an interpolated update of the health bar toward the new health value. */
	void SetHealthPercent(float Health, float MaxHealth);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Percent currently displayed on the health bar. */
	UPROPERTY(Transient)
	float CurrentHealthPercent = 1.f;

	/** Percent the health bar is interpolating toward. */
	float TargetHealthPercent = 1.f;

	/** Percent the current interpolation started from. */
	float StartHealthPercent = 1.f;

	/** Progress of the current interpolation (0..1). */
	float InterpProgress = 1.f;

	/** Whether the health bar is currently interpolating. */
	bool bInterpolating = false;

	/** Curve used to ease the health bar interpolation over time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar", meta = (AllowPrivateAccess = "true"))
	class UCurveFloat* HealthBarCurve;

	/** Multiplier for the interpolation speed. Higher = faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar", meta = (ClampMin = 0.1, AllowPrivateAccess = "true"))
	float HealthBarInterpSpeed = 1.f;
};
