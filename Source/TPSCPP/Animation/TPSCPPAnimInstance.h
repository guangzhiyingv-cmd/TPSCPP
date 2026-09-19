// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "TPSCPPAnimInstance.generated.h"

class UAbilitySystemComponent;

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

protected:
	/** Tag to property mappings; the mapped properties live on the concrete anim instance class. */
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FTPSCPPGameplayTagPropertyMap GameplayTagPropertyMap;
};
