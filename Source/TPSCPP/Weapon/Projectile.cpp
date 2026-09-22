#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TPSCPPCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Impact.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayCueManager.h"
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

	// A projectile destroyed in the same frame it hits never exists long enough for its clients to
	// inspect, so presentation data is sent before the character branch. Body hits receive the blood
	// cue only; the surface impact cue remains reserved for environment hits.
	if (ATPSCPPCharacter* HitCharacter = Cast<ATPSCPPCharacter>(OtherActor))
	{
		TArray<FTPSCPPCueImpact> Impacts;
		FTPSCPPCueImpact Impact;
		Impact.Location = Hit.ImpactPoint;
		Impact.Rotation = Hit.ImpactNormal.Rotation();
		Impacts.Add(Impact);

		HitCharacter->MulticastExecuteBloodCues(Impacts);
	}
	else
	{
		FTPSCPPCueImpactFX ImpactFX;
		ImpactFX.Particles = TSoftObjectPtr<UParticleSystem>(HitParticles);
		ImpactFX.Sound = TSoftObjectPtr<USoundBase>(HitSound);

		MulticastExecuteImpactCue(Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), ImpactFX);
	}

	Destroy();
}

void AProjectile::MulticastExecuteImpactCue_Implementation(
	const FVector_NetQuantize& ImpactPoint,
	const FRotator& ImpactRotation,
	const FTPSCPPCueImpactFX& ImpactFX)
{
	AActor* TargetActor = GetInstigator();
	if (!TargetActor)
	{
		TargetActor = GetOwner();
	}

	if (!TargetActor)
	{
		return;
	}

	UTPSCPPCueImpactFXSource* FXSource = NewObject<UTPSCPPCueImpactFXSource>(GetTransientPackage(), NAME_None, RF_Transient);
	FXSource->Particles = ImpactFX.Particles.LoadSynchronous();
	FXSource->Sound = ImpactFX.Sound.LoadSynchronous();

	if (!FXSource->Particles && !FXSource->Sound)
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Location = ImpactPoint;
	Params.Normal = ImpactRotation.Vector();
	Params.SourceObject = FXSource;

	// The FX travels with the RPC because a remote projectile replica never receives the firing
	// weapon's data table application. Execute the static cue against the instigator directly: the
	// local ability system can be absent on a remote shooter, and the multicast already reached
	// every machine, so routing through ASC would broadcast the cue again.
	UGameplayCueManager::ExecuteGameplayCue_NonReplicated(
		TargetActor,
		TPSCPPGameplayTags::Cue_Weapon_Impact,
		Params);
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

void AProjectile::ApplyWeaponData(const FWeaponData& InWeaponData)
{
	Damage = InWeaponData.Damage;

	TracerParticleSystem = InWeaponData.TracerParticleSystem.IsNull()
		? TracerParticleSystem
		: InWeaponData.TracerParticleSystem.LoadSynchronous();
	HitParticles = InWeaponData.HitParticles.IsNull()
		? HitParticles
		: InWeaponData.HitParticles.LoadSynchronous();
	HitSound = InWeaponData.HitSound.IsNull()
		? HitSound
		: InWeaponData.HitSound.LoadSynchronous();

	if (TracerParticle)
	{
		TracerParticle->SetTemplate(TracerParticleSystem);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = InWeaponData.ProjectileInitialSpeed;
		ProjectileMovementComponent->MaxSpeed = InWeaponData.ProjectileMaxSpeed;
		ProjectileMovementComponent->Velocity =
			GetActorForwardVector() * InWeaponData.ProjectileInitialSpeed;
	}
}
