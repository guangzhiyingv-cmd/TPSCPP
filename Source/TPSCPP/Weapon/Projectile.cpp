#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TPSCPPCharacter.h"
#include "TPSCPP.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 15000.f;
	ProjectileMovementComponent->MaxSpeed = 15000.f;

	TracerParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TracerParticle"));
	TracerParticle->SetupAttachment(RootComponent);
	TracerParticle->SetTemplate(TracerParticleSystem);
	TracerParticle->bAutoActivate = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

		if (GetInstigator())
		{
			CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
		}
	}
}

void AProjectile::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority()) return;

	if (ATPSCPPCharacter* HitCharacter = Cast<ATPSCPPCharacter>(OtherActor))
	{
		HitCharacter->MulticastPlayHitReaction(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, GetActorLocation(), GetActorRotation());
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

