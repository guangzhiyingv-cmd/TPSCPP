#include "Weapon/HitScanWeapon.h"

#include "TPSCPP.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TPSCPPCharacter.h"
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
				UGameplayStatics::ApplyDamage(ClosestHit->GetActor(), Damage, InstigatorController, this, UDamageType::StaticClass());

				if (ATPSCPPCharacter* HitCharacter = Cast<ATPSCPPCharacter>(ClosestHit->GetActor()))
				{
					HitCharacter->MulticastPlayHitReaction(ClosestHit->ImpactPoint, ClosestHit->ImpactNormal.Rotation());
				}

				MulticastSpawnImpact(ClosestHit->ImpactPoint, ClosestHit->ImpactNormal.Rotation());
			}
		}
	}
}

void AHitScanWeapon::MulticastSpawnImpact_Implementation(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation)
{
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, ImpactPoint, ImpactRotation);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
	}
}
