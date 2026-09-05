// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/Weapon.h"
#include "TimerManager.h"
#include "Engine/NetSerialization.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSCPP_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	/** Equip the given weapon: set its state to Equipped, attach it to the character's hand socket, and store the reference. */
	void EquipWeapon(AWeapon* WeaponToEquip);

	/** Drop the currently equipped weapon so it falls as a physical pickup. */
	void DropEquippedWeapon();

	/** Starts a reload on the equipped weapon. Authority resolves ammo after ReloadTime. */
	void StartReload();

	/** Set the weapon the owner is currently overlapping (server authority). */
	void SetOverlappingWeapon(AWeapon* Weapon);

	/** Get the weapon the owner is currently overlapping. */
	AWeapon* GetOverlappingWeapon() const { return OverlappingWeapon; }

	/** Get the weapon currently equipped by the owner. */
	AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	/** Updates the ammo text in the player HUD. */
	void UpdateAmmoHUD();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend class ATPSCPPCharacter;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void SetHUDCrosshairs(float DeltaTime);

private:
	class ATPSCPPCharacter* Character;
	class ATPSCPPPlayerController* Controller;
	class APlayerHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);
	bool bFireButtonPressed;
	/** True after automatic fire continues from the first shot. */
	bool bUseContinuousRecoil = false;
	/** Time accumulated while holding automatic fire. Used as the recoil curve X value. */
	float ContinuousFireTime = 0.f;
	bool bCanFire = true;
	FTimerHandle FireTimer;

	void Fire();
	void StartFireTimer();
	void FireTimerFinished();

	void ReloadTimerFinished();

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReloadFinished();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReload(bool bPlay,float ReloadTime);

	UPROPERTY(Replicated)
	bool bReloading = false;
	FTimerHandle ReloadTimer;

	UFUNCTION(Server,Reliable)
	void ServerFire(bool bPressed, const FVector_NetQuantize& InHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(bool bPressed, const FVector_NetQuantize& InHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	/** Multiplies character speed to determine crosshair spread. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0))
	float VelocitySpreadMultiplier = 0.1f;

	/** Amount subtracted from crosshair spread while shoulder aiming. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0))
	float ShoulderAimSpreadReduction = 8.f;

	/** Amount subtracted from crosshair spread while aiming down sights. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0))
	float ADSAimSpreadReduction = 16.f;

	/** Minimum crosshair spread while in hipfire state. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0))
	float HipfireMinSpread = 10.f;

	/** Minimum crosshair spread while shoulder aiming. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0))
	float ShoulderMinSpread = 5.f;

	/** Maximum crosshair spread bonus applied while airborne. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0))
	float AirborneSpreadBonus = 20.f;

	/** Interpolation speed for the airborne spread bonus. Higher = faster. */
	UPROPERTY(EditAnywhere, Category = "Crosshairs", meta = (ClampMin = 0.1))
	float SpreadInterpSpeed = 8.f;

	/** Current airborne spread bonus, interpolated each frame. */
	float AirborneSpread = 0.f;

	/** Current aim spread reduction, interpolated toward the aim state's target each frame. */
	float AimSpreadReduction = 0.f;
	
	//Shooting Spread
	float ShootingSpread = 0.f;

	/** Replicated only to the owning client when it changes. */
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	FVector_NetQuantize HitTarget;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
};
