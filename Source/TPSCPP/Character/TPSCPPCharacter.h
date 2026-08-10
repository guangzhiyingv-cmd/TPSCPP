// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "TPSCPPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
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
class ATPSCPPCharacter : public ACharacter
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

	/** Child skeletal mesh for weapon attachment. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* CustomMesh;

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

public:

	/** Constructor */
	ATPSCPPCharacter();	

protected:
	virtual void BeginPlay() override;

	void UpdateHUDHealth();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initialize component references after all subobjects are created */
	virtual void PostInitializeComponents() override;

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

	/** Returns the custom child mesh used for weapon attachment. */
	FORCEINLINE class USkeletalMeshComponent* GetCustomMesh() const { return CustomMesh; }

	/** Returns true if the character currently has a weapon equipped. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool HasEquippedWeapon() const;

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

	/** Plays or stops the fire montage on the character mesh. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void PlayFireMontage(bool bPlay);

	/** Plays the hit feedback (blood particles and sound) on all machines. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation);
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
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

	/** Weapon offset when attached to the first-person camera during ADS. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector FPSWeaponRelativeLocation = FVector(30.f, 0.f, -20.f);

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

	/** Curve asset controlling the camera transition. Create CT_CameraTransition in Content Browser and assign here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCurveFloat* CameraCurveFloat;

	/** Curve asset controlling the ADS weapon raise animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCurveFloat* ADSWeaponCurveFloat;

	/** Timeline progress callback: interpolates camera parameters. */
	UFUNCTION()
	void CameraTimelineUpdate(float Value);

	/** Called when the camera timeline finishes. */
	UFUNCTION()
	void CameraTimelineFinished();

	/** Timeline progress callback for the ADS weapon animation. */
	UFUNCTION()
	void ADSWeaponTimelineUpdate(float Value);

	/** Current aiming state. Hipfire is the base state; Shoulder and ADS cannot switch directly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	EAimState AimState = EAimState::Hipfire;

	/** Mouse/aim look sensitivity multiplier while shoulder aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ShoulderSensitivity = 1.f;

	/** Mouse/aim look sensitivity multiplier while aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float ADSSensitivity = 0.5f;

	/** Montage played when firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* FireMontage;

	/** Blood particle system spawned at the hit point when this character is shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UParticleSystem* BloodParticles;

	/** Sound played at the hit point when this character is shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* HitSound;

	/** Pending ADS camera sequence flag: shoulder animation first, then switch to FPS camera. */
	UPROPERTY()
	bool bPendingADS = false;

private:

	/** Maximum health this character can have. */
	UPROPERTY(EditAnywhere, Category = "PlayerStats", meta = (ClampMin = 1, AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;

	/** Current health, replicated to all clients when it changes. */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Health, Category = "PlayerStats")
	float Health = 100.f;

	/** Called when Health is replicated to this machine. */
	UFUNCTION()
	void OnRep_Health();

	/** Timer handle for delayed destruction after elimination. */
	FTimerHandle ElimTimer;

	/** Whether this character has already been eliminated. */
	bool bEliminated = false;

	/** Destroys the actor after the elimination delay. */
	void ElimTimerFinished();

	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ATPSCPPPlayerController* PlayerController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class ATPSCPPGameMode* GameModeRef;

	/** Whether the character has a weapon equipped. Replicated to all clients. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	bool bIsEquipped = false;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(NetMulticast, Reliable)
	void Elim();
};

