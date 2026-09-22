#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"


UCLASS()
class TPSCPP_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
	
public:
	AProjectileRocket();

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit) override;

	virtual void ApplyWeaponData(const FWeaponData& InWeaponData) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ExplosionRadius = 500;
private:

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* RocketMesh;
};
