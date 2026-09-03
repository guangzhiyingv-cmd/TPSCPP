// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class TPSCPP_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

	/** Skeletal mesh for the weapon visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMesh;

	/** Sphere component for detecting player overlap. Collision is enabled only on the server in BeginPlay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* AreaSphere;

	/** Widget component displaying the pickup prompt above the weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* PickupWidget;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	/** Called when a pawn overlaps the area sphere on the server. */
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/** Called when a pawn leaves the area sphere on the server. */
	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

public:
	void ShowPickupWidget(bool bShowWidget);

	/** Enable or disable the area sphere collision. */
	void SetAreaSphereCollisionEnabled(bool bEnabled);

	/** Plays or stops the fire montage on the weapon mesh. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	virtual void Fire(bool bPlay, const FVector& HitTarget);

public:
	UFUNCTION()
	void OnRep_WeaponState();

	UFUNCTION()
	void SetWeaponState(EWeaponState State);

	UFUNCTION()
	void Dropped();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState)
	EWeaponState WeaponState;

	/** Montage played when the weapon fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimationAsset* FireAnim;

	/** Casing actor class ejected when the weapon fires. */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<class ACasing> CasingClass;

	/** Impulse applied to the ejected casing along the AmmoEject socket +X axis. */
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = 0))
	float EjectImpulseStrength = 500.f;

	/** Seconds between consecutive shots. Controls the weapon fire rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = 0.01))
	float FireDelay = 0.15f;

	/** Whether the weapon keeps firing while the fire button is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 20;

	/** Magazine capacity of this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (ClampMin = 1))
	int32 MagCapacity = 30;

	/** Ammo currently loaded in the magazine. Replicated to the owning client. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Ammo, Category = "Ammo")
	int32 Ammo = 30;

	/** Time in seconds required to reload the magazine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (ClampMin = 0.01))
	float ReloadTime = 2.f;

	/** When enabled, firing never consumes reserve ammo and reload always refills. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	bool bInfiniteAmmo = false;

	/** Called when Ammo or ReserveAmmo is replicated to this machine. */
	UFUNCTION()
	void OnRep_Ammo();

	/** Sets the current magazine ammo on the server. */
	void SetAmmo(int32 NewAmmo);

	/** Playback speed of the ADS weapon raise timeline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	float ADSTimelinePlayRate = 1.f;

	/** Final local view model position when this weapon is aimed down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	FVector FPSWeaponRelativeLocation = FVector(30.f, 0.f, -20.f);

	/** Field of view used while aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS", meta = (ClampMin = 1, ClampMax = 160))
	float ADSFOV = 70.f;

	/** Look sensitivity multiplier while aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ADSSensitivity = 0.5f;

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }


	/**
	* Textures for the weapon crosshairs
	*/
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsBottom;
};
