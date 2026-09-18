// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TPSCPPDamageEffect.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystem/TPSCPPHealthSet.h"

UTPSCPPDamageEffect::UTPSCPPDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCallerMagnitude;
	SetByCallerMagnitude.DataTag = TPSCPPGameplayTags::Data_Damage;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UTPSCPPHealthSet::GetDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCallerMagnitude);

	Modifiers.Add(Modifier);
}
