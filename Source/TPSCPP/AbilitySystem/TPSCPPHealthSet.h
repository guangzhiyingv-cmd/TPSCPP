// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TPSCPPHealthSet.generated.h"

#define TPSCPP_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Health attributes. Damage is a meta attribute: gameplay effects write the incoming damage into
 * it and PostGameplayEffectExecute applies it to Health once, so mitigation / shields / clamping
 * can all live in one place.
 */
UCLASS()
class TPSCPP_API UTPSCPPHealthSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UTPSCPPHealthSet();

	/** Current health. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
	FGameplayAttributeData Health;
	TPSCPP_ATTRIBUTE_ACCESSORS(UTPSCPPHealthSet, Health)

	/** Maximum health. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
	FGameplayAttributeData MaxHealth;
	TPSCPP_ATTRIBUTE_ACCESSORS(UTPSCPPHealthSet, MaxHealth)

	/** Meta attribute carrying incoming damage for a single execution. Never replicated. */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData Damage;
	TPSCPP_ATTRIBUTE_ACCESSORS(UTPSCPPHealthSet, Damage)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};
