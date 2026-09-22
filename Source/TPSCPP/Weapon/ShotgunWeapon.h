#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "ShotgunWeapon.generated.h"

UCLASS()
class TPSCPP_API AShotgunWeapon : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(bool bPlay, const FVector& HitTarget) override;

	/** Keeps the shared aim center at the exact screen center. */
	virtual bool ShouldApplyAimSpread() const override { return false; }
};
