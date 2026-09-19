#include "Weapon/HitScanWeapon.h"

#include "TPSCPP.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TPSCPPCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"

AHitScanWeapon::AHitScanWeapon()
{
	
}

void AHitScanWeapon::Fire(bool bPlay, const FVector& HitTarget)
{
	Super::Fire(bPlay, HitTarget);
	
	if (!HasAuthority() || !bPlay) return;
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn) return;
	AController* InstigatorController = InstigatorPawn->GetController();
	if (!InstigatorController) return;
	
	const USkeletalMeshSocket* MuzzleFlashScoket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashScoket)
	{
		FTransform SocketTransform = MuzzleFlashScoket->GetSocketTransform(GetWeaponMesh());
		//From muzzle flash socket to hit location from TraceUnderCrosshairs
		FVector TraceStart = SocketTransform.GetLocation();
		FVector TraceEnd = TraceStart+(HitTarget-TraceStart)*1.5f;

		FHitResult VisibilityHit;
		FHitResult SkeletalHit;
		
		FCollisionQueryParams CollisionParameters = FCollisionQueryParams();
		CollisionParameters.AddIgnoredActor(InstigatorPawn);
		
		UWorld* World = GetWorld();
		if (World)
		{
			bool bVisHit = GetWorld()->LineTraceSingleByChannel(VisibilityHit, TraceStart, TraceEnd, ECC_Visibility,CollisionParameters);
			bool bSkelHit = GetWorld()->LineTraceSingleByChannel(SkeletalHit, TraceStart, TraceEnd, ECC_SkeletalMesh,CollisionParameters);

			FHitResult* ClosestHit = nullptr;
			if (bVisHit && bSkelHit)
			{
				ClosestHit = (VisibilityHit.ImpactPoint - TraceStart).SizeSquared() < (SkeletalHit.ImpactPoint - TraceStart).SizeSquared()
					? &VisibilityHit : &SkeletalHit;
			}
			else if (bVisHit)
			{
				ClosestHit = &VisibilityHit;
			}
			else if (bSkelHit)
			{
				ClosestHit = &SkeletalHit;
			}
			if (ClosestHit)
			{
				UGameplayStatics::ApplyPointDamage(ClosestHit->GetActor(), Damage, ClosestHit->ImpactPoint, *ClosestHit, InstigatorController, this, UDamageType::StaticClass());

				if (ATPSCPPCharacter* HitCharacter = Cast<ATPSCPPCharacter>(ClosestHit->GetActor()))
				{
					HitCharacter->MulticastExecuteBloodCue(ClosestHit->ImpactPoint, ClosestHit->ImpactNormal.Rotation());
				}

				MulticastExecuteImpactCue(ClosestHit->ImpactPoint, ClosestHit->ImpactNormal.Rotation());
			}
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

void AHitScanWeapon::MulticastExecuteImpactCue_Implementation(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation)
{
	FGameplayCueParameters Params;
	Params.Location = ImpactPoint;
	Params.Normal = ImpactRotation.Vector();
	// See AProjectile::MulticastExecuteImpactCue: the cue resolves its FX from the class defaults so
	// that it does not depend on the reporting actor still being alive.
	Params.SourceObject = GetClass()->GetDefaultObject();

	// The shooter owns the cosmetic cue for its own shots.
	if (const ATPSCPPCharacter* OwnerCharacter = Cast<ATPSCPPCharacter>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
		{
			ASC->ExecuteGameplayCue(TPSCPPGameplayTags::Cue_Weapon_Impact, Params);
		}
	}
}
