// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "GameFramework/Character.h"
#include "TPSCPPCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TimerManager.h"
#include "Weapon/Casing.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());
	PickupWidget->SetRelativeLocation(FVector(0, 0, 90));
	PickupWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PickupWidget->SetDrawSize(FVector2D(150.f, 40.f));
	PickupWidget->SetVisibility(false);

	Ammo = MagCapacity;
}

void AWeapon::SetAreaSphereCollisionEnabled(bool bEnabled)
{
	AreaSphere->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

	// Deferred to the next tick so prewarming does not slow down level loading.
	if (bPrewarmFireAssets)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AWeapon::PrewarmFireAssets);
	}
}

void AWeapon::PrewarmFireAssets()
{
	UWorld* World = GetWorld();
	if (!World || !CasingClass)
	{
		return;
	}

	const FVector PrewarmLocation = GetPrewarmLocation();
	for (int32 Index = 0; Index < PrewarmSpawnCount; ++Index)
	{
		// Spawning and destroying forces class load, component registration and physics creation
		// to happen now instead of on the first real shot.
		if (ACasing* PrewarmCasing = World->SpawnActor<ACasing>(CasingClass, PrewarmLocation, FRotator::ZeroRotator))
		{
			PrewarmCasing->Destroy();
		}
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, Ammo, COND_OwnerOnly);
}

void AWeapon::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ATPSCPPCharacter* OverlappingCharacter = Cast<ATPSCPPCharacter>(OtherActor))
	{
		if (UCombatComponent* Combat = OverlappingCharacter->GetCombat())
		{
			Combat->SetOverlappingWeapon(this);
		}
	}
}

void AWeapon::OnSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	if (ATPSCPPCharacter* OverlappingCharacter = Cast<ATPSCPPCharacter>(OtherActor))
	{
		if (UCombatComponent* Combat = OverlappingCharacter->GetCombat())
		{
			Combat->SetOverlappingWeapon(nullptr);
		}
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(bool bPlay, const FVector& HitTarget)
{
	USkeletalMeshComponent* TargetMesh = WeaponMesh;

	// ADS: play on the first-person view model instead of the third-person mesh
	if (ATPSCPPCharacter* OwnerCharacter = Cast<ATPSCPPCharacter>(GetOwner()))
	{
		if (OwnerCharacter->IsLocallyControlled() && OwnerCharacter->GetAimState() == EAimState::ADS)
		{
			if (USkeletalMeshComponent* ViewModel = OwnerCharacter->GetViewModelWeapon())
			{
				TargetMesh = ViewModel;
			}
		}
	}

	if (!TargetMesh) return;

	if (bPlay)
	{
		if (FireAnim)
		{
			TargetMesh->PlayAnimation(FireAnim, false);
		}
	}
	else
	{
		TargetMesh->Stop();
	}

	// Spawn and eject a casing at the AmmoEject socket when firing
	if (bPlay && CasingClass && WeaponMesh)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			ACasing* SpawnedCasing = GetWorld()->SpawnActor<ACasing>(
				CasingClass,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator());
			if (SpawnedCasing)
			{
				SpawnedCasing->GetCasingMesh()->AddImpulse(
					SocketTransform.GetRotation().GetAxisX() * EjectImpulseStrength);
			}
		}
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	default:
		break;
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TimeLastEquipped = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	default:
		break;
	}
}

void AWeapon::UpdateFiringTime()
{
	if (const UWorld* World = GetWorld())
	{
		TimeLastFired = World->GetTimeSeconds();
	}
}

float AWeapon::GetTimeSinceLastInteractedWith() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const double WorldTime = World->GetTimeSeconds();
	double Result = WorldTime - TimeLastEquipped;

	if (TimeLastFired > 0.0)
	{
		Result = FMath::Min(Result, WorldTime - TimeLastFired);
	}

	return static_cast<float>(Result);
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

void AWeapon::SetAmmo(int32 NewAmmo)
{
	Ammo = FMath::Clamp(NewAmmo, 0, MagCapacity);
}

void AWeapon::OnRep_Ammo()
{
	if (ATPSCPPCharacter* OwnerCharacter = Cast<ATPSCPPCharacter>(GetOwner()))
	{
		if (UCombatComponent* Combat = OwnerCharacter->GetCombat())
		{
			Combat->OnAmmoReplicated();
		}
	}
}

