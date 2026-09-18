// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TPSCPPAbilitySystemComponent.h"
#include "AbilitySystem/TPSCPPDamageEffect.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"

void UTPSCPPAbilitySystemComponent::ApplyDamage(float Damage, AActor* DamageCauser, AController* InstigatorController)
{
	if (Damage <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddInstigator(InstigatorController, DamageCauser);

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(UTPSCPPDamageEffect::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(TPSCPPGameplayTags::Data_Damage, Damage);
	ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
