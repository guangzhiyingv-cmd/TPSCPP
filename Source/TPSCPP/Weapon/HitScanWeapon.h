#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class TPSCPP_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
	

public:
	AHitScanWeapon();
	
	
	virtual  void Fire(bool bPlay, const FVector& HitTarget) override;

	virtual void PrewarmFireAssets() override;
	
	/** Particle system spawned at the impact point when the projectile hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UParticleSystem* HitParticles;

	/** Sound played at the impact point when the projectile hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* HitSound;

private:
	/**
	 * Executes the impact gameplay cue on all machines after an authoritative hit. Gameplay cues are
	 * not replicated on their own, so the cue is executed locally on every machine and the hit data
	 * travels in the RPC parameters.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExecuteImpactCue(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation);
};
