// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UGameplayCueSet;

/**
 * Registration of the project's native (C++) gameplay cue notifies. The gameplay cue manager only
 * discovers cue notifies that exist as blueprint assets, so native classes have to be added to a cue
 * set explicitly.
 */
namespace TPSCPPNativeGameplayCues
{
	/** Adds the project's native cue notifies to the given cue set. */
	void AddToCueSet(UGameplayCueSet& CueSet);
}
