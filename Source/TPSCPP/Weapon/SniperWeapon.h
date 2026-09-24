// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/ProjectileWeapon.h"
#include "SniperWeapon.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;

/**
 * Projectile sniper rifle. Adds the scope presentation: the lens fades from opaque to transparent
 * while aiming down sights, and an unbound post process magnifies the scope area for the local player.
 */
UCLASS()
class TPSCPP_API ASniperWeapon : public AProjectileWeapon
{
	GENERATED_BODY()

public:
	ASniperWeapon();

	/** Drives the scope lens from the ADS blend progress: 0 = hipfire, 1 = fully aimed. */
	virtual void SetAimBlend(float Blend) override;

protected:
	virtual void ApplyWeaponData(bool bInitializeRuntimeState = true) override;

	/** Unbound post process that magnifies the scope area on the local view while aiming. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scope")
	UPostProcessComponent* ScopePostProcess;

	/** Post process material that magnifies the scope area (Material Domain = Post Process). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	TSoftObjectPtr<UMaterialInterface> ScopePostProcessMaterial;

	/** Magnification reached while fully aimed. 0.5 = 2x, 0.33 = 3x. 1 = no magnification. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope", meta = (ClampMin = 0.01, ClampMax = 1.0))
	float ScopeZoomScale = 0.5f;

	/** Soft radius of the scope mask, in screen space units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope", meta = (ClampMin = 0.f, ClampMax = 1.f))
	float ScopeMaskRadius = 0.15f;

	/** Name of the post process material parameter that drives the magnification. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FName ScopeZoomParameter = TEXT("ZoomScale");

	/** Name of the post process material parameter that drives the mask soft radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FName ScopeMaskRadiusParameter = TEXT("Sight Exclude Radius");

	/** Name of the post process material parameter that selects the mask stencil bit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FName ScopeMaskBitParameter = TEXT("Stencil Mask Bit");

	/** Material slot of the weapon mesh that holds the scope lens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FName ScopeLensMaterialSlot = TEXT("glass");

	/** Scalar parameter of the scope lens material that controls the fade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FName ScopeLensAlphaParameter = TEXT("Alpha");

	/** Parameter value while hip firing, where the lens is opaque. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	float ScopeLensOpaqueAlpha = 1.f;

	/** Parameter value while fully aimed, where the lens is transparent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	float ScopeLensTransparentAlpha = 0.f;

private:
	/** Dynamic instance that drives the scope lens material. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ScopeLensMaterial;

	/** Dynamic instance of the scope post process material. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ScopePostProcessMaterialInstance;

	/** Set once when the missing scope lens slot warning was logged. */
	bool bWarnedMissingScopeLensSlot = false;

	/** Creates the lens material instance when it does not exist yet. */
	void EnsureScopeLensMaterial();
};
