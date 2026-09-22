#include "Weapon/HitScanWeapon.h"

#include "TPSCPP.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TPSCPPCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Impact.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayCueManager.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

AHitScanWeapon::AHitScanWeapon()
{
	
}

void AHitScanWeapon::ApplyWeaponData(bool bInitializeRuntimeState)
{
	Super::ApplyWeaponData(bInitializeRuntimeState);

	HitParticles = WeaponData.HitParticles.IsNull()
		? HitParticles
		: WeaponData.HitParticles.LoadSynchronous();
	HitSound = WeaponData.HitSound.IsNull()
		? HitSound
		: WeaponData.HitSound.LoadSynchronous();
}

void AHitScanWeapon::Fire(bool bPlay, const FVector& HitTarget)
{
	Super::Fire(bPlay, HitTarget);
	
	if (!HasAuthority() || !bPlay) return;
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn) return;
	AController* InstigatorController = InstigatorPawn->GetController();
	if (!InstigatorController) return;

	const FVector TraceStart = GetMuzzleLocation();
	const FVector ToTarget = HitTarget - TraceStart;
	const FVector TraceDirection = ToTarget.GetSafeNormal();
	const float TraceDistance = FMath::Max(ToTarget.Size() * 1.5f, 100.f);

	FHitScanHit Hit;
	if (PerformHitScan(TraceStart, TraceDirection, TraceDistance, InstigatorController, Hit))
	{
		TArray<FHitScanHit> Hits;
		Hits.Add(Hit);
		SendHitCues(Hits);
	}
}

FVector AHitScanWeapon::GetMuzzleLocation() const
{
	if (const USkeletalMeshComponent* Mesh = GetWeaponMesh())
	{
		if (const USkeletalMeshSocket* MuzzleSocket =
			Mesh->GetSocketByName(FName("MuzzleFlash")))
		{
			return MuzzleSocket->GetSocketTransform(Mesh).GetLocation();
		}

		return Mesh->GetComponentLocation();
	}

	return GetActorLocation();
}

bool AHitScanWeapon::PerformHitScan(
	const FVector& TraceStart,
	const FVector& TraceDirection,
	float TraceDistance,
	AController* InstigatorController,
	FHitScanHit& OutHit)
{
	UWorld* World = GetWorld();
	if (!World || TraceDistance <= 0.f)
	{
		return false;
	}

	FCollisionQueryParams CollisionParameters(SCENE_QUERY_STAT(HitScanWeapon), false);
	CollisionParameters.AddIgnoredActor(this);
	if (const APawn* InstigatorPawn = Cast<APawn>(GetOwner()))
	{
		CollisionParameters.AddIgnoredActor(InstigatorPawn);
	}

	const FVector TraceEnd = TraceStart + TraceDirection * TraceDistance;

	FHitResult VisibilityHit;
	FHitResult SkeletalHit;
	const bool bVisibilityHit = World->LineTraceSingleByChannel(
		VisibilityHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		CollisionParameters);
	const bool bSkeletalHit = World->LineTraceSingleByChannel(
		SkeletalHit,
		TraceStart,
		TraceEnd,
		ECC_SkeletalMesh,
		CollisionParameters);

	const FHitResult* ClosestHit = nullptr;
	if (bVisibilityHit && bSkeletalHit)
	{
		ClosestHit = VisibilityHit.Distance < SkeletalHit.Distance
			? &VisibilityHit
			: &SkeletalHit;
	}
	else if (bVisibilityHit)
	{
		ClosestHit = &VisibilityHit;
	}
	else if (bSkeletalHit)
	{
		ClosestHit = &SkeletalHit;
	}

	if (!ClosestHit)
	{
		return false;
	}

	ApplyHitScanHit(*ClosestHit, InstigatorController, OutHit);
	return true;
}

void AHitScanWeapon::ApplyHitScanHit(
	const FHitResult& Hit,
	AController* InstigatorController,
	FHitScanHit& OutHit)
{
	if (!Hit.GetActor() || !InstigatorController)
	{
		return;
	}

	UGameplayStatics::ApplyPointDamage(
		Hit.GetActor(),
		Damage,
		Hit.ImpactPoint,
		Hit,
		InstigatorController,
		this,
		UDamageType::StaticClass());

	OutHit.ImpactPoint = Hit.ImpactPoint;
	OutHit.ImpactRotation = Hit.ImpactNormal.Rotation();
	OutHit.HitCharacter = Cast<ATPSCPPCharacter>(Hit.GetActor());
}

void AHitScanWeapon::SendHitCues(const TArray<FHitScanHit>& Hits)
{
	if (!HasAuthority() || Hits.IsEmpty())
	{
		return;
	}

	TArray<FTPSCPPCueImpact> SurfaceImpacts;
	SurfaceImpacts.Reserve(Hits.Num());

	// One blood multicast per victim, so a shotgun hitting one character still sends one RPC.
	TMap<ATPSCPPCharacter*, TArray<FTPSCPPCueImpact>> BloodByVictim;

	for (const FHitScanHit& Hit : Hits)
	{
		FTPSCPPCueImpact Impact;
		Impact.Location = Hit.ImpactPoint;
		Impact.Rotation = Hit.ImpactRotation;

		if (ATPSCPPCharacter* Victim = Hit.HitCharacter.Get())
		{
			BloodByVictim.FindOrAdd(Victim).Add(Impact);
		}
		else
		{
			// Hits on characters receive the blood cue only. The surface impact cue stays reserved
			// for environment hits so a body hit does not play both sets of FX.
			SurfaceImpacts.Add(Impact);
		}
	}

	FTPSCPPCueImpactFX ImpactFX;
	ImpactFX.Particles = TSoftObjectPtr<UParticleSystem>(HitParticles);
	ImpactFX.Sound = TSoftObjectPtr<USoundBase>(HitSound);

	if (!SurfaceImpacts.IsEmpty())
	{
		MulticastExecuteImpactCues(SurfaceImpacts, ImpactFX);
	}

	for (const TPair<ATPSCPPCharacter*, TArray<FTPSCPPCueImpact>>& BloodEntry : BloodByVictim)
	{
		if (ATPSCPPCharacter* Victim = BloodEntry.Key)
		{
			Victim->MulticastExecuteBloodCues(BloodEntry.Value);
		}
	}
}

void AHitScanWeapon::PrewarmFireAssets()
{
	Super::PrewarmFireAssets();

	// PrimeSound initializes the audio source without actually playing anything.
	if (HitSound)
	{
		UGameplayStatics::PrimeSound(HitSound);
	}

	if (HitParticles)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, HitParticles, GetPrewarmLocation(), FRotator::ZeroRotator);
		}
	}
}

void AHitScanWeapon::MulticastExecuteImpactCues_Implementation(const TArray<FTPSCPPCueImpact>& Impacts, const FTPSCPPCueImpactFX& ImpactFX)
{
	AActor* TargetActor = GetOwner();
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

	for (const FTPSCPPCueImpact& Impact : Impacts)
	{
		FGameplayCueParameters Params;
		Params.Location = Impact.Location;
		Params.Normal = Impact.Rotation.Vector();
		Params.SourceObject = FXSource;

		// This multicast already reaches every machine, including the one that sent it. Execute the
		// static cue directly against the owner actor; the local ability system may not exist for a
		// remote shooter, and routing through ASC would broadcast the cue again.
		UGameplayCueManager::ExecuteGameplayCue_NonReplicated(
			TargetActor,
			TPSCPPGameplayTags::Cue_Weapon_Impact,
			Params);
	}
}
