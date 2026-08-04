// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"
#include "TPSCPPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
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
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCombatComponent, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCombatComponent, EquippedWeapon, COND_None);
}

void UCombatComponent::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	if (OverlappingWeapon != Weapon)
	{
		OverlappingWeapon = Weapon;
	}
	if (Character->IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void UCombatComponent::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	// Drop the currently equipped weapon if there is one
	if (EquippedWeapon)
	{
		EquippedWeapon->WeaponState = EWeaponState::EWS_Dropped;
		EquippedWeapon->SetAreaSphereCollisionEnabled(true);
		EquippedWeapon->WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->WeaponState = EWeaponState::EWS_Equipped;
	EquippedWeapon->SetAreaSphereCollisionEnabled(false);
	// Attach the weapon mesh to the character's right hand socket
	EquippedWeapon->WeaponMesh->AttachToComponent(
		Character->GetCustomMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("hand_rSocket"));

	SetOverlappingWeapon(nullptr);

	// Use camera-relative rotation when weapon is equipped
	Character->bUseControllerRotationYaw = false;
	Character->GetCharacterMovement()->bUseControllerDesiredRotation = true;
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bIsEquipped = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (!Character) return;

	if (EquippedWeapon)
	{
		Character->bUseControllerRotationYaw = false;
		Character->GetCharacterMovement()->bUseControllerDesiredRotation = true;
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (Character)
	{
		Character->PlayFireMontage(bPressed);
	}
}
