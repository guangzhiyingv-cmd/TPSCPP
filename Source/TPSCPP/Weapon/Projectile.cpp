#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TPSCPPCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayEffectTypes.h"
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
	if (bPrewarmDummy || !HasAuthority()) return;

	// Broadcast the impact cue first, so it reaches every machine even when the projectile is
	// spawned and destroyed within a single frame and therefore never replicates to clients.
	MulticastExecuteImpactCue(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

	if (ATPSCPPCharacter* HitCharacter = Cast<ATPSCPPCharacter>(OtherActor))
	{
		HitCharacter->MulticastExecuteBloodCue(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	Destroy();
}

void AProjectile::MulticastExecuteImpactCue_Implementation(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation)
{
	// Carry the impact data in the cue parameters instead of reading it back from this actor: the
	// actor may already be gone on a remote machine by the time a cue would resolve it.
	FGameplayCueParameters Params;
	Params.Location = ImpactPoint;
	Params.Normal = ImpactRotation.Vector();
	// The class default object, not the actor: the cue is executed at the end of the frame, by which
	// point a projectile that hit in the same frame it spawned is already gone on clients. The BP
	// configured FX live on the class defaults, so the values are identical.
	Params.SourceObject = GetClass()->GetDefaultObject();

	// The shooter owns the cosmetic cue for its own shots.
	if (const ATPSCPPCharacter* InstigatorCharacter = Cast<ATPSCPPCharacter>(GetInstigator()))
	{
		if (UAbilitySystemComponent* ASC = InstigatorCharacter->GetAbilitySystemComponent())
		{
			ASC->ExecuteGameplayCue(TPSCPPGameplayTags::Cue_Weapon_Impact, Params);
		}
	}
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::SetDamage(float NewDamage)
{
	Damage = NewDamage;
}

