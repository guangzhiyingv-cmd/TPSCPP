// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponHeaderWidget.generated.h"

/**
 * 
 */
UCLASS()
class TPSCPP_API UWeaponHeaderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Show the pickup prompt: set text to "E-Equip". */
	void Show();

	/** Hide the pickup prompt: clear the text. */
	void Hide();

protected:
	/** Text block bound in the Widget Blueprint (named "Text"). */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text;
	
};
