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
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	/** Child skeletal mesh for weapon attachment. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* CustomMesh;

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

	/** Aim Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AimAction;

public:

	/** Constructor */
	ATPSCPPCharacter();	

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	/** Initialize input action bindings */
protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

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

	/** Server RPC: start aiming (sets bIsAiming and MaxWalkSpeed on authority). */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AimStart();

	/** Server RPC: stop aiming (clears bIsAiming and restores MaxWalkSpeed on authority). */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AimEnd();

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

	/** Handles aim pressed input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAimStart();

	/** Handles aim released input. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAimEnd();

protected:
	UPROPERTY(EditAnywhere, Category="Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category="Movement")
	float SprintSpeed = 1000.f;

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

	/** How fast the camera transitions between normal and aiming. Higher = faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = 0.1))
	float CameraInterpSpeed = 8.f;

	/** Timeline component for smooth camera transitions. */
	UPROPERTY()
	UTimelineComponent* CameraTimeline;

	/** Curve asset controlling the camera transition. Create CT_CameraTransition in Content Browser and assign here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCurveFloat* CameraCurveFloat;

	/** Timeline progress callback: interpolates camera parameters. */
	UFUNCTION()
	void CameraTimelineUpdate(float Value);

	/** Whether the player is currently aiming. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	bool bIsAiming = false;

public:

	/** Whether the character has a weapon equipped. Replicated to all clients. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Combat")
	bool bIsEquipped = false;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

