// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Weapon/WeaponData.h"
#include "Weapon.generated.h"

class UAnimInstance;
class USkeletalMesh;

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
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Data")
	FDataTableRowHandle WeaponDataRow;

	/** Shared weapon table used when WeaponDataRow does not specify a table. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Data")
	TSoftObjectPtr<UDataTable> DefaultWeaponDataTable =
		TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Weapons.DT_Weapons")));

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon Data")
	FWeaponData WeaponData;

	virtual void ApplyWeaponData(bool bInitializeRuntimeState = true);

	/** Tracks whether this actor has already received its initial data-table runtime state. */
	bool bWeaponDataInitialized = false;

	/** Set once when the missing reload montage warning was logged for this weapon. */
	mutable bool bWarnedMissingReloadMontage = false;

	/** When enabled, fire assets and actors are prewarmed shortly after BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm")
	bool bPrewarmFireAssets = true;

	/** Number of dummy actors spawned during prewarm to initialize classes, components and physics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm", meta = (ClampMin = 0))
	int32 PrewarmSpawnCount = 1;

	/** Distance below the weapon used for prewarm spawns so they are never visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm")
	float PrewarmSpawnDepth = 10000.f;

	/** Location used for prewarm spawns. */
	FVector GetPrewarmLocation() const { return GetActorLocation() - FVector(0.f, 0.f, PrewarmSpawnDepth); }

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

	/** Prewarms the assets and actors used when this weapon fires so the first shot does not hitch. */
	UFUNCTION(BlueprintCallable, Category = "Prewarm")
	virtual void PrewarmFireAssets();

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

	/** Animation layer linked on the character's main mesh while this weapon is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSubclassOf<UAnimInstance> AnimLayer;

	/** Montage played while this weapon is reloading. Falls back to the character's when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* ReloadMontage;

	/** Returns the animation layer class linked while this weapon is equipped. */
	UFUNCTION(BlueprintPure, Category = "Animation")
	TSubclassOf<UAnimInstance> GetAnimLayer() const { return AnimLayer; }

	/**
	 * Returns the reload montage of this weapon. Logs a one-time warning when the weapon data row
	 * has none, so a weapon that silently falls back to the character's montage is noticed.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation")
	class UAnimMontage* GetReloadMontage() const;

	UFUNCTION(BlueprintPure, Category = "Weapon Data")
	const FWeaponData& GetWeaponData() const { return WeaponData; }

	/** Shotguns disable screen-space aim spread so all pellets share the exact screen center. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool ShouldApplyAimSpread() const { return true; }

	/**
	 * Called with the aim down sights blend progress while the aim presentation plays: 0 = hipfire,
	 * 1 = fully aimed. Weapons with aim driven visuals (for example a scope lens) override this.
	 */
	virtual void SetAimBlend(float Blend) {}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetPelletCount() const { return PelletCount; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetPelletSpreadAngleDegrees() const { return CurrentPelletSpreadAngleDegrees; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetPelletSpreadMaxAngleDegrees() const { return PelletSpreadMaxAngleDegrees; }

	/** Sets the current pellet angle. The value is clamped to the configured maximum. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetPelletSpreadAngleDegrees(float NewAngle);

	/** Sets the runtime maximum pellet angle and clamps the current angle. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetPelletSpreadMaxAngleDegrees(float NewMaxAngle);

	/** Records the current world time as the last time this weapon fired. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void UpdateFiringTime();

	/** Returns how long it has been since this weapon was equipped or last fired. */
	UFUNCTION(BlueprintPure, Category = "Animation")
	float GetTimeSinceLastInteractedWith() const;

	/** World time this weapon was last equipped. */
	double TimeLastEquipped = 0.0;

	/** World time this weapon last fired. */
	double TimeLastFired = 0.0;

	/** Casing actor class ejected when the weapon fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<class ACasing> CasingClass;

	/** Impulse applied to the ejected casing along the AmmoEject socket +X axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = 0))
	float EjectImpulseStrength = 500.f;

	/** Seconds between consecutive shots. Controls the weapon fire rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = 0.01))
	float FireDelay = 0.15f;

	/** Whether the weapon keeps firing while the fire button is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon")
	int32 PelletCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon")
	float PelletSpreadMaxAngleDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon")
	float CurrentPelletSpreadAngleDegrees = 0.f;

	/** Spread added per unit of ground speed. Drives the crosshair size and the shot randomization. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0))
	float VelocitySpreadMultiplier = 0.1f;

	/** Spread subtracted while shoulder aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0))
	float ShoulderAimSpreadReduction = 8.f;

	/** Spread subtracted while aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0))
	float ADSAimSpreadReduction = 16.f;

	/** Minimum spread while hip firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0))
	float HipfireMinSpread = 10.f;

	/** Minimum spread while shoulder aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0))
	float ShoulderMinSpread = 5.f;

	/** Extra spread ramped in while airborne. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0))
	float AirborneSpreadBonus = 20.f;

	/** Interpolation speed of the airborne and aim spread terms. Higher = faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = 0.1))
	float SpreadInterpSpeed = 8.f;


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

	/** Horizontal camera kick magnitude for a single shot. Direction is randomly left or right. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0))
	float SingleShotHorizontalRecoil = 0.05f;

	/** Vertical camera kick magnitude for a single shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0))
	float SingleShotVerticalRecoil = 0.3f;

	/** Continuous fire horizontal recoil by time held. X is seconds since sustained fire began. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	class UCurveFloat* AutoRecoilHorizontalCurve;

	/** Continuous fire vertical recoil by time held. X is seconds since sustained fire began. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	class UCurveFloat* AutoRecoilVerticalCurve;

	/** Random perturbation added to the horizontal recoil magnitude each shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0))
	float HorizontalRecoilPerturbation = 0.01f;

	/** Random perturbation added to the vertical recoil each shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0))
	float VerticalRecoilPerturbation = 0.05f;

	/** Interpolation speed for the delayed reverse recoil. Higher returns faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0))
	float RecoilRecoverySpeed = 4.f;

	/** Delay after releasing fire before recovery starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0))
	float RecoilRecoveryDelay = 0.1f;

	/** 1 returns the last shot's full vertical recoil, 0.6 returns 60%. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = 0, ClampMax = 1))
	float VerticalRecoilRecoveryMultiplier = 0.7f;

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }


	/**
	* Textures for the weapon crosshairs
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshairs")
	class UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshairs")
	class UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshairs")
	class UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshairs")
	class UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshairs")
	class UTexture2D* CrosshairsBottom;

	/** Skeletal mesh drawn only to CustomDepth while aiming: it marks the scope area the post process magnifies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	TSoftObjectPtr<USkeletalMesh> ScopeMaskMesh;

	/** Socket on the weapon mesh the scope mask attaches to. Empty uses the relative transform alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FName ScopeMaskSocket;

	/** Offset of the scope mask from that socket, or from the view model root when no socket is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	FTransform ScopeMaskRelativeTransform;

	/** CustomDepth stencil value used by the scope mask mesh and the scope post process. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope", meta = (ClampMin = 0, ClampMax = 255))
	int32 ScopeMaskStencilBit = 1;

	/** Reticle shown through the scope once the player is fully aimed down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
	class UTexture2D* ScopeReticleTexture;
};
