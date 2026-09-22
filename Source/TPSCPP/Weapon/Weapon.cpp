// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "GameFramework/Character.h"
#include "TPSCPPCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimationAsset.h"
#include "Curves/CurveFloat.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Weapon/Casing.h"

namespace
{
	template <typename TObjectType>
	TObjectType* LoadSoftObject(const TSoftObjectPtr<TObjectType>& SoftObject)
	{
		return SoftObject.IsNull() ? nullptr : SoftObject.LoadSynchronous();
	}
}

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

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyWeaponData(true);
}

void AWeapon::ApplyWeaponData(bool bInitializeRuntimeState)
{
	const UDataTable* ResolvedDataTable = WeaponDataRow.DataTable;
	if (!ResolvedDataTable && !DefaultWeaponDataTable.IsNull())
	{
		ResolvedDataTable = DefaultWeaponDataTable.LoadSynchronous();
	}

	if (!ResolvedDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' could not resolve a weapon data table."), *GetNameSafe(this));
		return;
	}

	FName ResolvedRowName = WeaponDataRow.RowName;
	if (ResolvedRowName.IsNone())
	{
		FString ClassName = GetClass()->GetName();
		ClassName.RemoveFromEnd(TEXT("_C"));
		ResolvedRowName = FName(*ClassName);
	}

	const FWeaponData* Row = ResolvedDataTable->FindRow<FWeaponData>(
		ResolvedRowName,
		TEXT("AWeapon::ApplyWeaponData"),
		true);

	if (!Row)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("'%s' could not find weapon data row '%s'."),
			*GetNameSafe(this),
			*ResolvedRowName.ToString());
		return;
	}

	WeaponData = *Row;

	const bool bHasMeshOverride = !WeaponData.WeaponMesh.IsNull();
	if (bHasMeshOverride && WeaponMesh)
	{
		WeaponMesh->SetSkeletalMeshAsset(LoadSoftObject(WeaponData.WeaponMesh));
	}

	FireAnim = WeaponData.FireAnim.IsNull()
		? FireAnim
		: LoadSoftObject(WeaponData.FireAnim);

	AnimLayer = WeaponData.AnimLayer
		? WeaponData.AnimLayer
		: AnimLayer;
	CasingClass = WeaponData.CasingClass
		? WeaponData.CasingClass
		: CasingClass;
	EjectImpulseStrength = WeaponData.EjectImpulseStrength;
	bPrewarmFireAssets = WeaponData.bPrewarmFireAssets;
	PrewarmSpawnCount = WeaponData.PrewarmSpawnCount;
	PrewarmSpawnDepth = WeaponData.PrewarmSpawnDepth;
	FireDelay = WeaponData.FireDelay;
	bAutomatic = WeaponData.bAutomatic;
	Damage = WeaponData.Damage;
	PelletCount = FMath::Max(WeaponData.PelletCount, 1);
	PelletSpreadMaxAngleDegrees = FMath::Max(WeaponData.PelletSpreadMaxAngleDegrees, 0.f);
	CurrentPelletSpreadAngleDegrees = PelletSpreadMaxAngleDegrees;
	MagCapacity = FMath::Max(WeaponData.MagCapacity, 1);
	if (bInitializeRuntimeState || !bWeaponDataInitialized)
	{
		Ammo = MagCapacity;
	}
	bWeaponDataInitialized = true;
	ReloadTime = WeaponData.ReloadTime;
	bInfiniteAmmo = WeaponData.bInfiniteAmmo;
	ADSTimelinePlayRate = WeaponData.ADSTimelinePlayRate;
	FPSWeaponRelativeLocation = WeaponData.FPSWeaponRelativeLocation;
	ADSFOV = WeaponData.ADSFOV;
	ADSSensitivity = WeaponData.ADSSensitivity;
	SingleShotHorizontalRecoil = WeaponData.SingleShotHorizontalRecoil;
	SingleShotVerticalRecoil = WeaponData.SingleShotVerticalRecoil;
	AutoRecoilHorizontalCurve = WeaponData.AutoRecoilHorizontalCurve.IsNull()
		? AutoRecoilHorizontalCurve
		: LoadSoftObject(WeaponData.AutoRecoilHorizontalCurve);
	AutoRecoilVerticalCurve = WeaponData.AutoRecoilVerticalCurve.IsNull()
		? AutoRecoilVerticalCurve
		: LoadSoftObject(WeaponData.AutoRecoilVerticalCurve);
	HorizontalRecoilPerturbation = WeaponData.HorizontalRecoilPerturbation;
	VerticalRecoilPerturbation = WeaponData.VerticalRecoilPerturbation;
	RecoilRecoverySpeed = WeaponData.RecoilRecoverySpeed;
	RecoilRecoveryDelay = WeaponData.RecoilRecoveryDelay;
	VerticalRecoilRecoveryMultiplier = WeaponData.VerticalRecoilRecoveryMultiplier;
	CrosshairsCenter = WeaponData.CrosshairsCenter.IsNull()
		? CrosshairsCenter
		: LoadSoftObject(WeaponData.CrosshairsCenter);
	CrosshairsLeft = WeaponData.CrosshairsLeft.IsNull()
		? CrosshairsLeft
		: LoadSoftObject(WeaponData.CrosshairsLeft);
	CrosshairsRight = WeaponData.CrosshairsRight.IsNull()
		? CrosshairsRight
		: LoadSoftObject(WeaponData.CrosshairsRight);
	CrosshairsTop = WeaponData.CrosshairsTop.IsNull()
		? CrosshairsTop
		: LoadSoftObject(WeaponData.CrosshairsTop);
	CrosshairsBottom = WeaponData.CrosshairsBottom.IsNull()
		? CrosshairsBottom
		: LoadSoftObject(WeaponData.CrosshairsBottom);
}

void AWeapon::SetAreaSphereCollisionEnabled(bool bEnabled)
{
	AreaSphere->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Refresh cached presentation data before the weapon becomes active. This makes PIE use the
	// current DataTable asset even when an already-loaded editor world actor still holds stale
	// transient values from an earlier data-table edit.
	ApplyWeaponData(false);

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
	DOREPLIFETIME(AWeapon, PelletCount);
	DOREPLIFETIME(AWeapon, PelletSpreadMaxAngleDegrees);
	DOREPLIFETIME(AWeapon, CurrentPelletSpreadAngleDegrees);
}

void AWeapon::SetPelletSpreadAngleDegrees(float NewAngle)
{
	CurrentPelletSpreadAngleDegrees = FMath::Clamp(
		NewAngle,
		0.f,
		PelletSpreadMaxAngleDegrees);
}

void AWeapon::SetPelletSpreadMaxAngleDegrees(float NewMaxAngle)
{
	PelletSpreadMaxAngleDegrees = FMath::Max(NewMaxAngle, 0.f);
	CurrentPelletSpreadAngleDegrees = FMath::Min(
		CurrentPelletSpreadAngleDegrees,
		PelletSpreadMaxAngleDegrees);
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
	ATPSCPPCharacter* OwnerCharacter = Cast<ATPSCPPCharacter>(GetOwner());

	// ADS: play on the first-person view model instead of the third-person mesh
	if (OwnerCharacter)
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
		//TargetMesh->Stop();
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

