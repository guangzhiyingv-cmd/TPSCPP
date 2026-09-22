// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Impact.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayCueNotifyTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Weapon/HitScanWeapon.h"
#include "Weapon/Projectile.h"

UTPSCPPCueNotify_Impact::UTPSCPPCueNotify_Impact()
{
	GameplayCueTag = TPSCPPGameplayTags::Cue_Weapon_Impact;
	GameplayCueName = GameplayCueTag.GetTagName();
}

void UTPSCPPCueNotify_Impact::PostInitProperties()
{
	Super::PostInitProperties();

	// Re-assert: the base class may derive the tag from the asset name.
	GameplayCueTag = TPSCPPGameplayTags::Cue_Weapon_Impact;
	GameplayCueName = GameplayCueTag.GetTagName();
}

bool UTPSCPPCueNotify_Impact::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}

void UTPSCPPCueNotify_Impact::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (EventType != EGameplayCueEvent::Executed || !MyTarget)
	{
		return;
	}

	// The source can be a transient FX carrier sent by a cue multicast, or the actor that reported
	// the hit when the cue is executed through another path.
	const UObject* Source = Parameters.SourceObject.Get();
	UParticleSystem* Particles = nullptr;
	USoundBase* Sound = nullptr;

	if (const UTPSCPPCueImpactFXSource* ImpactFX = Cast<UTPSCPPCueImpactFXSource>(Source))
	{
		Particles = ImpactFX->Particles;
		Sound = ImpactFX->Sound;
	}
	else if (const AProjectile* Projectile = Cast<AProjectile>(Source))
	{
		Particles = Projectile->HitParticles;
		Sound = Projectile->HitSound;
	}
	else if (const AHitScanWeapon* Weapon = Cast<AHitScanWeapon>(Source))
	{
		Particles = Weapon->HitParticles;
		Sound = Weapon->HitSound;
	}

	if (!Particles && !Sound)
	{
		return;
	}

	const FVector ImpactLocation = Parameters.Location;
	const FRotator ImpactRotation = Parameters.Normal.Rotation();

	if (Particles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(MyTarget->GetWorld(), Particles, ImpactLocation, ImpactRotation);
	}

	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Sound, ImpactLocation);
	}
}
