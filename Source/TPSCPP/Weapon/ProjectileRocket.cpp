#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/TPSCPPPlayerState.h"
#include "GameFramework/Character.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(GetRootComponent());
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::ApplyWeaponData(const FWeaponData& InWeaponData)
{
	Super::ApplyWeaponData(InWeaponData);
	ExplosionRadius = InWeaponData.ExplosionRadius;
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			UGameplayStatics::ApplyRadialDamage(this, Damage, Hit.Location, ExplosionRadius, UDamageType::StaticClass(), TArray<AActor*>(), this, OwnerController);

		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
