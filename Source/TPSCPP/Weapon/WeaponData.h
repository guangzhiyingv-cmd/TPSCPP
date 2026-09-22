#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponData.generated.h"

class AProjectile;
class ACasing;
class UAnimInstance;
class UAnimationAsset;
class UCurveFloat;
class UParticleSystem;
class USkeletalMesh;
class USoundBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct TPSCPP_API FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UAnimationAsset> FireAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSubclassOf<UAnimInstance> AnimLayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<ACasing> CasingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (ClampMin = 0))
	float EjectImpulseStrength = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TSoftObjectPtr<UParticleSystem> HitParticles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TSoftObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prewarm")
	bool bPrewarmFireAssets = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prewarm", meta = (ClampMin = 0))
	int32 PrewarmSpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prewarm")
	float PrewarmSpawnDepth = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.01))
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	float Damage = 20.f;

	/** Number of independent traces fired by one shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 1))
	int32 PelletCount = 1;

	/** Maximum pellet deviation from the aim direction, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	float PelletSpreadMaxAngleDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = 1))
	int32 MagCapacity = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = 0.01))
	float ReloadTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	bool bInfiniteAmmo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS")
	float ADSTimelinePlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS")
	FVector FPSWeaponRelativeLocation = FVector(30.f, 0.f, -20.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = 1, ClampMax = 160))
	float ADSFOV = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ADSSensitivity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0))
	float SingleShotHorizontalRecoil = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0))
	float SingleShotVerticalRecoil = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	TSoftObjectPtr<UCurveFloat> AutoRecoilHorizontalCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil")
	TSoftObjectPtr<UCurveFloat> AutoRecoilVerticalCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0))
	float HorizontalRecoilPerturbation = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0))
	float VerticalRecoilPerturbation = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0))
	float RecoilRecoverySpeed = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0))
	float RecoilRecoveryDelay = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = 0, ClampMax = 1))
	float VerticalRecoilRecoveryMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs")
	TSoftObjectPtr<UTexture2D> CrosshairsCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs")
	TSoftObjectPtr<UTexture2D> CrosshairsLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs")
	TSoftObjectPtr<UTexture2D> CrosshairsRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs")
	TSoftObjectPtr<UTexture2D> CrosshairsTop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crosshairs")
	TSoftObjectPtr<UTexture2D> CrosshairsBottom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSoftObjectPtr<UParticleSystem> TracerParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0))
	float ProjectileInitialSpeed = 15000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0))
	float ProjectileMaxSpeed = 15000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0))
	float ExplosionRadius = 500.f;
};
