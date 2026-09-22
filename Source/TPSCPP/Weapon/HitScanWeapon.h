#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "AbilitySystem/TPSCPPGameplayCueTypes.h"
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

	virtual void ApplyWeaponData(bool bInitializeRuntimeState = true) override;
	
	/** Particle system spawned at the impact point when the projectile hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UParticleSystem* HitParticles;

	/** Sound played at the impact point when the projectile hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* HitSound;

protected:
	/** One authoritative trace result, kept so a multi-shot weapon can batch all of its pellets. */
	struct FHitScanHit
	{
		FVector_NetQuantize ImpactPoint = FVector::ZeroVector;
		FRotator ImpactRotation = FRotator::ZeroRotator;

		/** Victim when the trace hit a character, so the caller can send its blood cue. */
		TWeakObjectPtr<class ATPSCPPCharacter> HitCharacter;
	};

	/** Returns the world-space MuzzleFlash socket location, or the mesh location as a fallback. */
	FVector GetMuzzleLocation() const;

	/**
	 * Executes one authoritative hit-scan trace and applies its damage. Returns true when something
	 * was hit and fills OutHit with the impact data; cues are not sent here, see SendHitCues.
	 */
	bool PerformHitScan(
		const FVector& TraceStart,
		const FVector& TraceDirection,
		float TraceDistance,
		AController* InstigatorController,
		FHitScanHit& OutHit);

	/**
	 * Sends the cues of one shot: a single impact multicast carrying every impact point, plus one
	 * blood multicast per victim. Must be called once per shot, after all of its traces.
	 */
	void SendHitCues(const TArray<FHitScanHit>& Hits);

private:
	void ApplyHitScanHit(
		const FHitResult& Hit,
		AController* InstigatorController,
		FHitScanHit& OutHit);

	/**
	 * Executes the impact gameplay cue on all machines after an authoritative hit. Gameplay cues are
	 * not replicated on their own, so the cue is executed locally on every machine and the hit data
	 * travels in the RPC parameters.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExecuteImpactCues(const TArray<FTPSCPPCueImpact>& Impacts, const FTPSCPPCueImpactFX& ImpactFX);
};
