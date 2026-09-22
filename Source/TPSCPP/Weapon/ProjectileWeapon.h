// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "Weapon/Projectile.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class TPSCPP_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(bool bPlay, const FVector& HitTarget) override;

	virtual void PrewarmFireAssets() override;

	virtual void ApplyWeaponData(bool bInitializeRuntimeState = true) override;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AProjectile> ProjectileClass;
};
