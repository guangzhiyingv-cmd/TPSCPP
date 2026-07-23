// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/WeaponHeaderWidget.h"
#include "Components/TextBlock.h"

void UWeaponHeaderWidget::Show()
{
	if (Text)
	{
		Text->SetText(FText::FromString("E-Equip"));
	}
}

void UWeaponHeaderWidget::Hide()
{
	if (Text)
	{
		Text->SetText(FText::GetEmpty());
	}
}
