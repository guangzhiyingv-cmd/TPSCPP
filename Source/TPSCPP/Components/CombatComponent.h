// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/Weapon.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSCPP_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	/** Equip the given weapon: set its state to Equipped, attach it to the character's hand socket, and store the reference. */
	void EquipWeapon(AWeapon* WeaponToEquip);

	/** Set the weapon the owner is currently overlapping (server authority). */
	void SetOverlappingWeapon(AWeapon* Weapon);

	/** Get the weapon the owner is currently overlapping. */
	AWeapon* GetOverlappingWeapon() const { return OverlappingWeapon; }

	/** Get the weapon currently equipped by the owner. */
	AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend class ATPSCPPCharacter;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	class ATPSCPPCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	/** Replicated only to the owning client when it changes. */
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
};
