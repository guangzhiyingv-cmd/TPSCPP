// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/TPSCPPGameplayCueTypes.h"
#include "GameplayTagContainer.h"
#include "TPSCPPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
class UAnimInstance;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
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

	/** Mask drawn only to CustomDepth while aiming: it marks the scope area the post process magnifies. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ScopeMask;

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

	/** Character ability Input Action. Optional: assign it in the character blueprint. */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AbilityAction;

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

	/** Rebinds the ability system once the PlayerState has replicated to this machine. */
	virtual void OnRep_PlayerState() override;

	/** Binds the ability system (avatar, health delegate, initial attributes) when available. */
	void InitAbilitySystem();

	/** Pushes the health attribute to the HUD. */
	void OnHealthAttributeChanged(const struct FOnAttributeChangeData& Data);

	/** True once the health attribute delegate has been bound. */
	bool bAbilitySystemInitialized = false;

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

	/** Clears the State.Firing tag once the firing state expires. */
	void ClearFiringStateTag();

	/** Sets a state tag so every other machine sees it while the owning machine reacts immediately. */
	void SetStateTag(const FGameplayTag& Tag, bool bActive);

	/** Animation instance the tag driven state is currently bound to (the mesh can recreate it). */
	TWeakObjectPtr<class UAnimInstance> TagDrivenAnimInstance;

	/** Timer that clears the firing state tag. */
	FTimerHandle FiringStateTimer;

protected:

	/** Server RPC: equip the overlapping weapon. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipWeapon();

	/**
	 * Server RPC: cancel the sprint ability on the authority. A server only ability is never
	 * instanced on the owning client, so the client cannot cancel it itself.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StopSprint();

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

	/** Plays or stops the reload montage locally. */
	void PlayReloadMontage(bool bPlay, float ReloadTime);

	/** Client RPC: drop the locally predicted reload because the server refused it. */
	UFUNCTION(Client, Reliable)
	void Client_StopReloadPresentation();

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

	/** Activates the sprint ability. Returns false when sprinting is not allowed right now. */
	bool TrySprintAbility();

	/** Cancels the sprint ability, e.g. when the input is released, firing or entering an aim state. */
	void StopSprintAbility();

	/** Returns true while the character is sprinting: the local immediate flag or the replicated tag. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const;

	/** Base movement speed. */
	FORCEINLINE float GetWalkSpeed() const { return WalkSpeed; }

	/** Sprinting movement speed. */
	FORCEINLINE float GetSprintSpeed() const { return SprintSpeed; }

	/** Mirrors the current aim state into the replicated State.ADS / State.ShoulderAim tags. */
	void SyncAimStateTags();

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

	/**
	 * Handles the character ability input. Empty by default: subclasses grant and activate their own
	 * gameplay ability here (C++ overrides DoAbility_Implementation, blueprints implement the event).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Input")
	void DoAbility();
	virtual void DoAbility_Implementation();

	/** Plays the local ADS weapon recoil animation when firing while aiming. */
	void PlayADSRecoil(float PlayRate);

	/** Captures the current camera offset as the starting point for a new fire burst. */
	void StartWeaponRecoilBurst();

	/** Applies the equipped weapon's camera recoil for the current shot. */
	void ApplyWeaponRecoil(bool bUseContinuousRecoil, float ContinuousFireTime);

	/** Plays or stops the reload montage on every machine. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayReloadMontage(bool bPlay, float ReloadTime);

	/** Activates the reload ability on this character's ability system. */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TryReload();

	/** Activates the fire ability. Returns whether the shot was accepted (ammo and fire rate). */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool TryFireWeapon();

	/** Cancels the reload ability, e.g. when firing or dropping the weapon. */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void CancelReloadAbility();

	/**
	 * Returns the reload montage to play: the equipped weapon's, or the character fallback when the
	 * weapon data row has none. The weapon getter logs a one-time warning in the fallback case.
	 */
	UAnimMontage* GetReloadMontage() const;

	/**
	 * Executes the blood gameplay cue on every machine at the authoritative hit points. A shotgun
	 * blast sends all of its pellets in one call: the ability system cue multicast is unreliable and
	 * throttled to net.MaxRPCPerNetUpdate calls per net update, so one RPC per pellet would drop
	 * most of the impacts.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExecuteBloodCues(const TArray<FTPSCPPCueImpact>& Impacts);

	/** Niagara system played at the hit point when this character is shot. */
	UFUNCTION(BlueprintPure, Category = "Effects")
	class UNiagaraSystem* GetBloodNiagaraSystem() const { return BloodNiagaraSystem; }

	/** Sound played at the hit point when this character is shot. */
	UFUNCTION(BlueprintPure, Category = "Effects")
	class USoundBase* GetHitSound() const { return HitSound; }

	/** True when the hit sound may play again, throttled per character. */
	bool ShouldPlayHitSound();
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

	/** Sprinting movement speed. */
	UPROPERTY(EditAnywhere, Category="Movement")
	float SprintSpeed = 1000.f;

	/**
	 * Local immediate sprint flag. Sprinting is replicated through the State.Sprint tag that the
	 * sprint ability sets, so other machines must use IsSprinting() instead of this flag.
	 */
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

	/** Shows or hides the scope reticle on the owning client's HUD. */
	void UpdateScopeReticleVisibility(bool bShouldShow);

	/** Current aiming state. Hipfire is the base state; Shoulder and ADS cannot switch directly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	EAimState AimState = EAimState::Hipfire;

	/** Mouse/aim look sensitivity multiplier while shoulder aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ShoulderSensitivity = 1.f;

	/** Mouse/aim look sensitivity multiplier while aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ADSSensitivity = 0.5f;

	/** ADS blend required before the scope reticle shows at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.f, ClampMax = 1.f))
	float ScopeReticleShowBlend = 0.99f;

	/** Fallback reload montage used when the equipped weapon's data row has none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* ReloadMontage;

	/** Reload ability granted to this character. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<class UGameplayAbility> ReloadAbilityClass;

	/** Fire ability granted to this character. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<class UGameplayAbility> FireAbilityClass;

	/** Sprint ability granted to this character. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<class UGameplayAbility> SprintAbilityClass;

	/** True once the default abilities have been granted on the server. */
	bool bAbilitiesGranted = false;

	/** Local optimistic reload state; the replicated State.Reloading tag is the authority. */
	bool bPredictedReloading = false;

	/** Set once when the "no reload montage at all" warning was logged for this character. */
	mutable bool bWarnedNoReloadMontage = false;

	/** The reload montage this character actually started, so a stop targets it and nothing else. */
	TWeakObjectPtr<UAnimMontage> ActiveReloadMontage;

	/** Clears the local predicted reload once the server's authoritative state arrives or it is refused. */
	void OnReloadTagChanged(const FGameplayTag Tag, int32 NewCount);

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

	/** True while leaving ADS: the FOV stays at the normal value instead of blending with the camera timeline. */
	bool bInstantFOVRecovery = false;

	/** True once the scope reticle has been shown, so repeated shows do not touch the widget. */
	bool bScopeReticleVisible = false;

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

	/** Maximum health this character can have. Also initializes the health attribute. */
	UPROPERTY(EditAnywhere, Category = "PlayerStats", meta = (ClampMin = 1, AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;

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

