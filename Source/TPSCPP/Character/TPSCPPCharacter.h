// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "AbilitySystemInterface.h"
#include "TPSCPPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
class UAnimInstance;
class UMaterialInstanceDynamic;
class FBoolProperty;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);


UENUM(BlueprintType)
enum class EAimState : uint8
{
	Hipfire,
	Shoulder,
	ADS
};

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ATPSCPPCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** First-person camera used while aiming down sights. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FPS_Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	/** Local-only weapon view model shown on the first-person camera during ADS. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ViewModelWeapon;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Equip Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* EquipAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SprintAction;

	/** Shoulder Aim Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ShoulderAimAction;

	/** ADS Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ADSAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FireAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ReloadAction;

public:

	/** Constructor */
	ATPSCPPCharacter();	

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

protected:
	virtual void BeginPlay() override;

	virtual void Restart() override;

	virtual void Tick(float DeltaTime) override;

	void UpdateHUDHealth();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initialize component references after all subobjects are created */
	virtual void PostInitializeComponents() override;

	/** Bind this pawn as the avatar of the PlayerState-owned ability system. */
	virtual void PossessedBy(AController* NewController) override;

	/** Component tag used to find the Blueprint-owned custom mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Custom Mesh")
	FName CustomMeshComponentTag = TEXT("CustomMesh");

	/** Runtime reference to the Blueprint-owned custom mesh. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Custom Mesh")
	USkeletalMeshComponent* CustomMesh = nullptr;

	/** Animation layer linked on the main mesh when no weapon is equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayer;

	/** Animation layer currently linked on the main mesh. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> CurrentAnimLayer;

	/** Seconds after a shot during which the character is treated as firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = 0.f))
	float FiringStateDuration = 0.25f;

	/** World time of the most recent shot. */
	float LastFireTime = -1000.f;

	/** Anim instance class the cached state properties were resolved for. */
	UPROPERTY(Transient)
	UClass* CachedAnimStateClass = nullptr;

	/** Cached gameplay-tag style bool properties on the anim instance, resolved by name. */
	TMap<FName, FBoolProperty*> CachedAnimStateProperties;

	/** Mirrors character state into the anim instance's gameplay-tag style bool properties. */
	void PushAnimStateToAnimInstance();

protected:

	/** Server RPC: equip the overlapping weapon. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipWeapon();

	/** Server RPC: start sprinting (sets MaxWalkSpeed on authority). */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SprintStart();

	/** Server RPC: stop sprinting (restores MaxWalkSpeed on authority). */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SprintEnd();

	/** Server RPC: sync the current aim state to the server. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAimState(EAimState NewState);

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles equip input from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoEquip();

	/** Returns the Combat component. */
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }

	/** Assigns the Blueprint-owned custom mesh used for weapon attachment. */
	UFUNCTION(BlueprintCallable, Category = "Custom Mesh")
	void SetCustomMesh(USkeletalMeshComponent* InCustomMesh);

	/** Resolves the Blueprint-owned custom mesh by component tag. */
	UFUNCTION(BlueprintCallable, Category = "Custom Mesh")
	bool ResolveCustomMesh();

	/** Returns the Blueprint-owned custom mesh used for weapon attachment. */
	UFUNCTION(BlueprintPure, Category = "Custom Mesh")
	USkeletalMeshComponent* GetCustomMesh() const { return CustomMesh; }

	/** Links the given animation layer on the main mesh, unlinking the previous one. Pass nullptr to only unlink. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void LinkAnimLayer(TSubclassOf<UAnimInstance> AnimLayerClass);

	/** Unlinks the current animation layer and restores the default (unarmed) layer. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void UnlinkAnimLayer();

	/** Marks the character as firing so the anim layer can raise the weapon. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void NotifyWeaponFired();

	/** Returns true while the character is considered to be firing. */
	UFUNCTION(BlueprintPure, Category = "Animation")
	bool IsFiring() const;

	/** Returns the animation layer linked when no weapon is equipped. */
	UFUNCTION(BlueprintPure, Category = "Animation")
	TSubclassOf<UAnimInstance> GetDefaultAnimLayer() const { return DefaultAnimLayer; }

	/** Returns the animation layer currently linked on the main mesh. */
	UFUNCTION(BlueprintPure, Category = "Animation")
	TSubclassOf<UAnimInstance> GetCurrentAnimLayer() const { return CurrentAnimLayer; }

	/** Returns true if the character currently has a weapon equipped. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool HasEquippedWeapon() const;

	/** Returns whether the character is currently reloading its equipped weapon. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsReloading() const;

	/** Returns true if the character is in either aiming state. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsAiming() const;

	/** Returns the current aim state. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	EAimState GetAimState() const { return AimState; }

	/** Returns the first-person view model weapon. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	USkeletalMeshComponent* GetViewModelWeapon() const { return ViewModelWeapon; }

	/** Outputs the equipped weapon's LeftHandSocket data in CustomMesh component space. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void GetLeftHandSocketData(FTransform& OutRelativeTransform, FVector& OutXAxis, FVector& OutZAxis) const;

	/** Returns the pitch angle for aim offset blending in the animation blueprint. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	float GetAimPitch() const;

	/** Handles sprint pressed input from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Handles sprint released input from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** Handles shoulder aim pressed input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoShoulderAimStart();

	/** Handles shoulder aim released input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoShoulderAimEnd();

	/** Handles ADS pressed input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoADSStart();

	/** Handles ADS released input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoADSEnd();

	/** Handles ADS toggle input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoADSToggle();

	/** Handles fire pressed input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFirePressed();

	/** Handles fire released input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFireReleased();

	/** Handles reload pressed input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoReload();

	/** Plays the local ADS weapon recoil animation when firing while aiming. */
	void PlayADSRecoil(float PlayRate);

	/** Captures the current camera offset as the starting point for a new fire burst. */
	void StartWeaponRecoilBurst();

	/** Applies the equipped weapon's camera recoil for the current shot. */
	void ApplyWeaponRecoil(bool bUseContinuousRecoil, float ContinuousFireTime);

	/** Plays or stops the reload montage on the character mesh. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void PlayReloadMontage(bool bPlay,float ReloadTime=2.0f);

	/** Plays the hit feedback (blood particles and sound) on all machines. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation);
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	/** Captures the hit zone damage multiplier. Point damage is broadcast before generic damage. */
	UFUNCTION()
	void CacheHitZoneDamageMultiplier(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation,
		class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
		const UDamageType* DamageType, AActor* DamageCauser);
protected:
	UPROPERTY(EditAnywhere, Category="Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category="Movement")
	float SprintSpeed = 1000.f;

	/** Whether the character is currently sprinting. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	bool bIsSprinting = false;

	/** Normal (hip-fire) camera: arm length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 50))
	float NormalArmLength = 400.f;

	/** Normal (hip-fire) camera: socket offset from spring arm target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector NormalSocketOffset = FVector::ZeroVector;

	/** Normal (hip-fire) camera: field of view. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 10, ClampMax = 160))
	float NormalFOV = 90.f;

	/** Aiming camera: arm length (shoulder view). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 50))
	float AimingArmLength = 200.f;

	/** Aiming camera: socket offset (shoulder shift). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector AimingSocketOffset = FVector(60.f, 0.f, 10.f);

	/** Aiming camera: field of view. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 10, ClampMax = 160))
	float AimingFOV = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator FPSWeaponRelativeRotation = FRotator::ZeroRotator;

	/** Weapon start offset (bottom-right) before the ADS animation moves it to the aim position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector FPSWeaponStartLocation = FVector(100.f, 70.f, -70.f);

	/** How fast the camera transitions between normal and aiming. Higher = faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 0.1))
	float CameraInterpSpeed = 8.f;

	/** Timeline component for smooth camera transitions. */
	UPROPERTY()
	UTimelineComponent* CameraTimeline;

	/** Timeline for the ADS weapon raise animation. */
	UPROPERTY()
	UTimelineComponent* ADSTimeline;

	/** Timeline for the local ADS weapon recoil while firing. */
	UPROPERTY()
	UTimelineComponent* ADSRecoilTimeline;

	/** Curve asset controlling the camera transition. Create CT_CameraTransition in Content Browser and assign here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCurveFloat* CameraCurveFloat;

	/** Curve asset controlling the ADS weapon raise animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCurveFloat* ADSWeaponCurveFloat;

	/** Curve asset controlling the local ADS weapon recoil. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCurveFloat* ADSRecoilCurve;

	/** Timeline progress callback: interpolates camera parameters. */
	UFUNCTION()
	void CameraTimelineUpdate(float Value);

	/** Called when the camera timeline finishes. */
	UFUNCTION()
	void CameraTimelineFinished();

	/** Timeline progress callback for the ADS weapon animation. */
	UFUNCTION()
	void ADSWeaponTimelineUpdate(float Value);

	/** Timeline progress callback for the local ADS weapon recoil. */
	UFUNCTION()
	void ADSRecoilTimelineUpdate(float Value);

	/** Current aiming state. Hipfire is the base state; Shoulder and ADS cannot switch directly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	EAimState AimState = EAimState::Hipfire;

	/** Mouse/aim look sensitivity multiplier while shoulder aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ShoulderSensitivity = 1.f;

	/** Mouse/aim look sensitivity multiplier while aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ADSSensitivity = 0.5f;

	/** Montage played while reloading the equipped weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* ReloadMontage;

	/** Niagara system spawned at the hit point when this character is shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UNiagaraSystem* BloodNiagaraSystem;

	/** Sound played at the hit point when this character is shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* HitSound;

	/** Minimum seconds between hit sound plays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (ClampMin = 0.f))
	float HitSoundCooldown = 0.7f;

	/** Last time the hit sound was played on this machine. */
	float LastHitSoundTime = -1.f;

	/** Pending ADS camera sequence flag: shoulder animation first, then switch to FPS camera. */
	UPROPERTY()
	bool bPendingADS = false;

private:

	/** Resolves the damage multiplier for the hit bone from DamageZoneTable. */
	float ResolveZoneDamageMultiplier(FName BoneName) const;

	/** Maps hit bone names to damage multipliers. Bones without a row use DefaultDamageMultiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	class UDataTable* DamageZoneTable;

	/** Multiplier used when the hit bone has no row in DamageZoneTable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = 0.f, AllowPrivateAccess = "true"))
	float DefaultDamageMultiplier = 1.f;

	/** Damage multiplier captured by the point damage handler for the damage event in flight. */
	float PendingZoneDamageMultiplier = 1.f;

	/** True while PendingZoneDamageMultiplier belongs to the damage event being processed. */
	bool bHasPendingZoneDamageMultiplier = false;

	/** Maximum health this character can have. */
	UPROPERTY(EditAnywhere, Category = "PlayerStats", meta = (ClampMin = 1, AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;

	/** Current health, replicated to all clients when it changes. */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Health, Category = "PlayerStats")
	float Health = 100.f;

	/** Called when Health is replicated to this machine. */
	UFUNCTION()
	void OnRep_Health();

	/** Called when ReserveAmmo is replicated to this machine. */
	UFUNCTION()
	void OnRep_ReserveAmmo();

	/** Timer handle for delayed destruction after elimination. */
	FTimerHandle ElimTimer;

	/** Whether this character has already been eliminated. */
	bool bEliminated = false;

	/** Actual pitch applied by the most recent shot. */
	float LastShotAppliedPitch = 0.f;

	/** Interpolation progress of the delayed reverse recoil. */
	float RecoilReturnProgress = 0.f;

	/** Amount of the reverse recoil already applied to the camera. */
	float AppliedRecoilReturn = 0.f;

	/** Time since the fire button was released. */
	float TimeSinceFireEnded = 0.f;

	/** Updates the delayed reverse vertical recoil each tick. */
	void UpdateRecoilReturn(float DeltaTime);

	/** Destroys the actor after the elimination delay. */
	void ElimTimerFinished();

	/** Dissolve effect*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere, Category = "Effects")
	UCurveFloat* DissolveCurve;

	/** Scalar parameter name driven by the dissolve timeline. */
	UPROPERTY(EditAnywhere, Category = "Effects")
	FName DissolveParameterName = TEXT("Dissolve");

	/** Per-material-slot dynamic instances used to dissolve the whole character. */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> DissolveMIs;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ATPSCPPPlayerController* PlayerController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class ATPSCPPGameMode* GameModeRef;

	/** Reserve ammo carried by the player, shared across reloads. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ReserveAmmo, Category = "PlayerStats")
	int32 ReserveAmmo = 0;

	/** Reserve ammo the player starts with on spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerStats", meta = (ClampMin = 0))
	int32 StartingReserveAmmo = 120;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class ATPSCPPPlayerState* PlayerStateRef;

	/** Whether the character has a weapon equipped. Replicated to all clients. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	bool bIsEquipped = false;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UFUNCTION()
	void Elim();	//GameMode call this function so it only runs on the server


protected:
	//Poll for any relelvant classes and initialize our HUD
	void PollInit();
};

