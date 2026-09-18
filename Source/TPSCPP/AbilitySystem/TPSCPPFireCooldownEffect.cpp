// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TPSCPPFireCooldownEffect.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UTPSCPPFireCooldownEffect::UTPSCPPFireCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCallerDuration;
	SetByCallerDuration.DataTag = TPSCPPGameplayTags::Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCallerDuration);

	// 5.3+ grants tags through a GameplayEffectComponent; this also refreshes the GE's cached tags.
	UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(TPSCPPGameplayTags::Cooldown_Weapon_Fire);
	TagsComponent->SetAndApplyTargetTagChanges(TagChanges);
	GEComponents.Add(TagsComponent);
}
