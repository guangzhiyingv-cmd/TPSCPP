// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TPSCPPNativeGameplayCues.h"
#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Blood.h"
#include "AbilitySystem/GameplayCues/TPSCPPCueNotify_Impact.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "GameplayCueSet.h"

namespace
{
	struct FNativeGameplayCue
	{
		FGameplayTag Tag;
		UClass* CueClass = nullptr;
	};
}

void TPSCPPNativeGameplayCues::AddToCueSet(UGameplayCueSet& CueSet)
{
	const FNativeGameplayCue NativeCues[] =
	{
		{ TPSCPPGameplayTags::Cue_Hit_Blood, UTPSCPPCueNotify_Blood::StaticClass() },
		{ TPSCPPGameplayTags::Cue_Weapon_Impact, UTPSCPPCueNotify_Impact::StaticClass() },
	};

	TArray<FGameplayCueReferencePair> Pairs;
	Pairs.Reserve(UE_ARRAY_COUNT(NativeCues));
	for (const FNativeGameplayCue& Cue : NativeCues)
	{
		if (Cue.Tag.IsValid() && Cue.CueClass)
		{
			Pairs.Add(FGameplayCueReferencePair(Cue.Tag, FSoftObjectPath(Cue.CueClass->GetPathName())));
		}
	}

	CueSet.AddCues(Pairs);

	// Native classes are already in memory, so bind them straight away instead of relying on a load.
	for (int32 Index = 0; Index < Pairs.Num(); ++Index)
	{
		if (const int32* DataIndex = CueSet.GameplayCueDataMap.Find(Pairs[Index].GameplayCueTag))
		{
			if (CueSet.GameplayCueData.IsValidIndex(*DataIndex))
			{
				CueSet.GameplayCueData[*DataIndex].LoadedGameplayCueClass = NativeCues[Index].CueClass;
			}
		}
	}
}
