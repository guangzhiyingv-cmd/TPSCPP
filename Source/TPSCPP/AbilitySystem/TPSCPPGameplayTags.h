// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace TPSCPPGameplayTags
{
	/** SetByCaller magnitude tag that carries the incoming damage amount. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);

	/** SetByCaller magnitude tag that carries the duration of a gameplay effect, e.g. the weapon fire cooldown. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cooldown);

	/** Loose tag set while the character is reloading. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Reloading);

	/** Identity tag of the reload ability, used for cancellation. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Reload);

	/** Identity tag of the fire weapon ability. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Fire);

	/** Granted while the weapon fire ability is on cooldown. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Fire);

	/** Loose tag set while the character is sprinting. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprint);

	/** Loose tag set while the character is aiming down sights. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ADS);

	/** Loose tag set while the character is shoulder aiming. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ShoulderAim);

	/** Loose tag set for a short time after the character fired. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Firing);

	/** Identity tag of the sprint ability, used for cancellation. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);

	/** Gameplay cue: projectile impact against surfaces. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Weapon_Impact);

	/** Gameplay cue: blood played on a character that got hit. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Hit_Blood);
}
