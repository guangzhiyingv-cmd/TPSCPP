// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DamageZoneMultiplier.generated.h"

/**
 * Row of a hit zone damage table.
 *
 * The row name is the bone that was hit (for example "head" or "thigh_l"), so the
 * multipliers live entirely in data and none of them are hard coded in gameplay code.
 */
USTRUCT(BlueprintType)
struct FDamageZoneMultiplier : public FTableRowBase
{
	GENERATED_BODY()

	/** Multiplier applied to damage taken on this bone. 1.0 is normal damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = 0.f))
	float DamageMultiplier = 1.f;
};
