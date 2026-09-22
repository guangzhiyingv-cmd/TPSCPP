#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "TPSCPPGameplayCueTypes.generated.h"

class UParticleSystem;
class USoundBase;

/**
 * One authoritative impact location and orientation carried by a gameplay cue multicast.
 *
 * Multi-shot weapons (a shotgun fires one trace per pellet) send every impact of a single shot in
 * one RPC. The ability system cue multicast is unreliable and the net driver only sends
 * net.MaxRPCPerNetUpdate (2 by default) calls of the same RPC per net update, so one RPC per pellet
 * would silently drop most of the impacts.
 */
USTRUCT(BlueprintType)
struct FTPSCPPCueImpact
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cue")
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Cue")
	FRotator Rotation = FRotator::ZeroRotator;
};

/**
 * Presentation assets resolved from the weapon data table and sent with an impact cue multicast.
 *
 * Remote projectile replicas do not receive the firing weapon's data table. Soft references let the
 * authoritative side send the actual per-weapon FX without relying on projectile class defaults.
 */
USTRUCT(BlueprintType)
struct FTPSCPPCueImpactFX
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cue")
	TSoftObjectPtr<UParticleSystem> Particles;

	UPROPERTY(BlueprintReadOnly, Category = "Cue")
	TSoftObjectPtr<USoundBase> Sound;
};

/** Runtime FX carrier for a nonreplicated impact cue. */
UCLASS(Transient)
class TPSCPP_API UTPSCPPCueImpactFXSource : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UParticleSystem> Particles;

	UPROPERTY()
	TObjectPtr<USoundBase> Sound;
};
