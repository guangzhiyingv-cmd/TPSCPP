// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TPSCPPHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UTPSCPPHealthSet::UTPSCPPHealthSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
}

void UTPSCPPHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UTPSCPPHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTPSCPPHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UTPSCPPHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UTPSCPPHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = GetDamage();
		SetDamage(0.f);

		if (LocalDamage > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - LocalDamage, 0.f, GetMaxHealth()));
		}
	}
}

void UTPSCPPHealthSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTPSCPPHealthSet, Health, OldHealth);
}

void UTPSCPPHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTPSCPPHealthSet, MaxHealth, OldMaxHealth);
}
