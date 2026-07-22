// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"
#include "TPSCPPCharacter.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCombatComponent, OverlappingWeapon, COND_OwnerOnly);
}

void UCombatComponent::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon != Weapon)
	{
		OverlappingWeapon = Weapon;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	// Drop the currently equipped weapon if there is one
	if (EquippedWeapon)
	{
		EquippedWeapon->WeaponState = EWeaponState::EWS_Dropped;
		EquippedWeapon->WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->WeaponState = EWeaponState::EWS_Equipped;

	// Attach the weapon mesh to the character's right hand socket
	EquippedWeapon->WeaponMesh->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("hand_rSocket"));
}

