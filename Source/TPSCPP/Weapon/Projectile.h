#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "Projectile.generated.h"

UCLASS()
class TPSCPP_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay() override;

	/** Called when the projectile collides with an obstacle. */
	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	/** Final cleanup when the projectile is destroyed. */
	virtual void Destroyed() override;

	/**
	 * Broadcasts the impact cue (particles + sound) to every machine at the authoritative hit
	 * location. Kept separate from Destroyed because a projectile fired point-blank is destroyed
	 * in the same frame it is spawned and therefore never replicates to clients. Reliable so the
	 * cue is flushed before the actor is destroyed and its net channel closes.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnImpact(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation);

	UPROPERTY(EditAnywhere)
	float Damage = 0.f;
public:	
	virtual void Tick(float DeltaTime) override;

	void SetDamage(float NewDamage);

	/** Marks this projectile as a prewarm dummy so it skips damage and impact effects. */
	void SetPrewarmDummy(bool bInPrewarmDummy) { bPrewarmDummy = bInPrewarmDummy; }

private:
	/** True when this actor only exists to prewarm classes, components and render resources. */
	bool bPrewarmDummy = false;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

public:
	/** Movement component that drives the projectile forward and rotates it toward its velocity. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	class UProjectileMovementComponent* ProjectileMovementComponent;

	/** Particle component that renders the tracer trail attached to the projectile. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UParticleSystemComponent* TracerParticle;

	/** Cascade particle system used as the tracer trail. Assign in the projectile blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UParticleSystem* TracerParticleSystem;

	/** Particle system spawned at the impact point when the projectile hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UParticleSystem* HitParticles;

	/** Sound played at the impact point when the projectile hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* HitSound;
};
