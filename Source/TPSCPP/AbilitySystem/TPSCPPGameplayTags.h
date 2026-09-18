// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace TPSCPPGameplayTags
{
	/** SetByCaller magnitude tag that carries the incoming damage amount. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);

	/** Loose tag set while the character is reloading. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Reloading);

	/** Identity tag of the reload ability, used for cancellation. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Reload);
}
