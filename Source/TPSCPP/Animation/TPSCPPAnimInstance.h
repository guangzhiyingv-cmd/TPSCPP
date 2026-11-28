// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "TurnaroundAnimation.h"
#include "TPSCPPAnimInstance.generated.h"

class UTurnaroundAnimLayer;
class UAbilitySystemComponent;
class ATPSCPPCharacter;

/**
 * FGameplayTagBlueprintPropertyMap with a way to register mappings from C++ (its mapping array is
 * protected).
 */
USTRUCT()
struct FTPSCPPGameplayTagPropertyMap : public FGameplayTagBlueprintPropertyMap
{
	GENERATED_BODY()

	/** Maps a gameplay tag onto a property of the owning animation instance, resolved by name. */
	void AddMapping(const FGameplayTag& Tag, FName PropertyName)
	{
		FGameplayTagBlueprintPropertyMapping& Mapping = PropertyMappings.AddDefaulted_GetRef();
		Mapping.TagToMap = Tag;
		Mapping.PropertyName = PropertyName;
	}

	/** Drops every mapping; used to rebuild them before binding (serialized names can come back empty). */
	void ClearMappings()
	{
		PropertyMappings.Reset();
	}
};

/**
 * Drives the anim graph's GameplayTag_* booleans straight from gameplay tags, so the character no
 * longer pushes its state into the anim instance every frame.
 */
UCLASS()
class TPSCPP_API UTPSCPPAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UTPSCPPAnimInstance();

	/** Binds the tag mappings to the ability system; safe to call again when the instance is recreated. */
	void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

	bool GetTurnaroundCycleTime(float PlayRate, float& Time) const;

protected:
	virtual void NativeInitializeAnimation() override;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Turnaround", meta=(ClampMin="90", ClampMax="180", Units="deg"))
	float ReversalAngle = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Turnaround", meta=(ClampMin="0", Units="cm/s"))
	float MinimumApproachSpeed = 150.f;

	/** Maximum extra distance from the closest stride phase, in half strides. */
	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Turnaround|Entry", meta=(ClampMin="0", ClampMax="1"))
	float EntryPhaseTolerance = 0.25f;

	/** Converts contact velocity error into distance for comparison with the position error. */
	UPROPERTY(EditDefaultsOnly, Category="Locomotion|Turnaround|Entry", meta=(ClampMin="0", Units="s"))
	float EntryVelocityWeightSeconds = 0.045f;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Locomotion|Turnaround")
	bool bTurnaroundActive = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Locomotion|Turnaround")
	bool bCameraFacingLocomotion = false;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Shared third-person pose for shoulder aim and ADS. */
	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	bool bUseThirdPersonAimPose = false;

	/** Adds the project's tag to property mappings. */
	void AddDefaultMappings();

	/** Tag to property mappings; the mapped properties live on the concrete anim instance class. */
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FTPSCPPGameplayTagPropertyMap GameplayTagPropertyMap;

private:
	void UpdateTurnaround(const ATPSCPPCharacter* Character, float DeltaSeconds);

	FVector PreviousVelocity = FVector::ZeroVector;
	FVector TurnaroundDirection = FVector::ZeroVector;
	float PreviousMeshYaw = 0.f;
	float PreviousActorYaw = 0.f;
	float EntryActorYaw = 0.f;
	bool bAwaitingTurnSide = false;
	bool bEnteredOnLeftFoot = false;
	float EntryMeshYaw = 0.f;
	FTurnaroundFootSample PreviousFeet[2];
	FTurnaroundFootSample EntryFoot;
	FVector EntryContactVelocity = FVector::ZeroVector;
	float EntryToeWeight = 0.f;
	FMarkerSyncAnimPosition EntryPhase;
	FVector PendingDirection = FVector::ZeroVector;
	float ApproachYaw = 0.f;
	TWeakObjectPtr<UTurnaroundAnimLayer> PlayingLayer;
	double PendingElapsed = 0.0;
	float CycleEntryTime = 0.f;
	float HandoffElapsed = -1.f;
	bool bHadGroundedSample = false;
};
