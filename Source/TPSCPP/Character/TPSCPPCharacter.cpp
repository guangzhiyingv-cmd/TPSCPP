// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCPPCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TPSCPP.h"
#include "GameMode/TPSCPPGameMode.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ATPSCPPCharacter::ATPSCPPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = NormalArmLength;
	CameraBoom->SocketOffset = NormalSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(NormalFOV);

	// Create a first-person camera used while aiming down sights
	FPS_Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPS_Camera"));
	FPS_Camera->SetupAttachment(RootComponent);
	FPS_Camera->bUsePawnControlRotation = true;
	FPS_Camera->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	FPS_Camera->SetFieldOfView(70.f);
	FPS_Camera->SetAutoActivate(false);

	CameraTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("CameraTimeline"));

	ADSTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("ADSTimeline"));

	CustomMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CustomMesh"));
	CustomMesh->SetupAttachment(GetMesh());
	CustomMesh->SetCollisionObjectType(ECC_SkeletalMesh);

	ViewModelWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ViewModelWeapon"));
	ViewModelWeapon->SetupAttachment(FPS_Camera);
	ViewModelWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ViewModelWeapon->SetCastShadow(false);
	ViewModelWeapon->SetVisibility(false);

	// Ignore camera channel on all character collision components to prevent camera clipping
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CustomMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
}

void ATPSCPPCharacter::BeginPlay()
{
	Super::BeginPlay();

	GameModeRef = GetWorld()->GetAuthGameMode<ATPSCPPGameMode>();
	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ATPSCPPCharacter::ReceiveDamage); 
	}
}

void ATPSCPPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (CameraCurveFloat && CameraTimeline)
	{
		FOnTimelineFloat ProgressUpdate;
		ProgressUpdate.BindUFunction(this, FName("CameraTimelineUpdate"));
		CameraTimeline->AddInterpFloat(CameraCurveFloat, ProgressUpdate);
		CameraTimeline->SetLooping(false);

		FOnTimelineEvent Finished;
		Finished.BindUFunction(this, FName("CameraTimelineFinished"));
		CameraTimeline->SetTimelineFinishedFunc(Finished);
	}

	if (ADSWeaponCurveFloat && ADSTimeline)
	{
		FOnTimelineFloat ADSProgress;
		ADSProgress.BindUFunction(this, FName("ADSWeaponTimelineUpdate"));
		ADSTimeline->AddInterpFloat(ADSWeaponCurveFloat, ADSProgress);
		ADSTimeline->SetLooping(false);
	}
}

void ATPSCPPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSCPPCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATPSCPPCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSCPPCharacter::Look);

		// Equipping
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ATPSCPPCharacter::DoEquip);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ATPSCPPCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ATPSCPPCharacter::DoSprintEnd);

		// Shoulder aiming
		EnhancedInputComponent->BindAction(ShoulderAimAction, ETriggerEvent::Started, this, &ATPSCPPCharacter::DoShoulderAimStart);
		EnhancedInputComponent->BindAction(ShoulderAimAction, ETriggerEvent::Completed, this, &ATPSCPPCharacter::DoShoulderAimEnd);

		// ADS toggle
		EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Started, this, &ATPSCPPCharacter::DoADSToggle);

		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ATPSCPPCharacter::DoFirePressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ATPSCPPCharacter::DoFireReleased);
	}
	else
	{
		UE_LOG(LogTPSCPP, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATPSCPPCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ATPSCPPCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ATPSCPPCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ATPSCPPCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		float Sensitivity = 1.f;
		if (AimState == EAimState::Shoulder)
		{
			Sensitivity = ShoulderSensitivity;
		}
		else if (AimState == EAimState::ADS)
		{
			if (AWeapon* Weapon = Combat->GetEquippedWeapon())
			{
				Sensitivity = Weapon->ADSSensitivity;
			}
			else
			{
				Sensitivity = ADSSensitivity;
			}
		}

		AddControllerYawInput(Yaw * Sensitivity);
		AddControllerPitchInput(Pitch * Sensitivity);
	}
}

void ATPSCPPCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ATPSCPPCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ATPSCPPCharacter::DoEquip()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(Combat->GetOverlappingWeapon());
		}
		else
		{
			Server_EquipWeapon();
		}
	}
}

void ATPSCPPCharacter::Server_EquipWeapon_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(Combat->GetOverlappingWeapon());
	}
}

bool ATPSCPPCharacter::Server_EquipWeapon_Validate()
{
	return true;
}

bool ATPSCPPCharacter::HasEquippedWeapon() const
{
	return Combat && Combat->GetEquippedWeapon() != nullptr;
}

bool ATPSCPPCharacter::IsAiming() const
{
	return AimState != EAimState::Hipfire;
}

void ATPSCPPCharacter::GetLeftHandSocketData(
	FTransform& OutRelativeTransform,
	FVector& OutXAxis,
	FVector& OutZAxis) const
{
	OutRelativeTransform = FTransform::Identity;
	OutXAxis = FVector::ZeroVector;
	OutZAxis = FVector::ZeroVector;

	if (!Combat || !CustomMesh)
	{
		return;
	}

	AWeapon* Weapon = Combat->GetEquippedWeapon();
	if (!Weapon || !Weapon->WeaponMesh)
	{
		return;
	}

	const FTransform SocketWorld = Weapon->WeaponMesh->GetSocketTransform(TEXT("LeftHandSocket"), RTS_World);
	const FTransform CustomMeshWorld = CustomMesh->GetComponentTransform();

	OutRelativeTransform = SocketWorld.GetRelativeTransform(CustomMeshWorld);

	OutXAxis = CustomMeshWorld.InverseTransformVector(SocketWorld.GetRotation().GetAxisX()).GetSafeNormal();
	OutZAxis = CustomMeshWorld.InverseTransformVector(SocketWorld.GetRotation().GetAxisZ()).GetSafeNormal();
}

float ATPSCPPCharacter::GetAimPitch() const
{
	//if (!GetController()) return 0.f;
	return FRotator::NormalizeAxis(GetBaseAimRotation().Pitch - GetActorRotation().Pitch);
}

void ATPSCPPCharacter::DoSprintStart()
{
	if (AimState != EAimState::Hipfire) return;

	// Cancel firing if the fire button is held while sprinting
	if (Combat && Combat->bFireButtonPressed)
	{
		Combat->FireButtonPressed(false);
	}

	bIsSprinting = true;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	if (!HasAuthority()) Server_SprintStart();
}

void ATPSCPPCharacter::Server_SprintStart_Implementation()
{
	if (AimState != EAimState::Hipfire) return;

	bIsSprinting = true;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

bool ATPSCPPCharacter::Server_SprintStart_Validate()
{
	return true;
}

void ATPSCPPCharacter::DoSprintEnd()
{
	if (AimState != EAimState::Hipfire) return;

	bIsSprinting = false;
	if (HasEquippedWeapon())
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (!HasAuthority())
	{
		Server_SprintEnd();
	}
}

void ATPSCPPCharacter::Server_SprintEnd_Implementation()
{
	if (AimState != EAimState::Hipfire) return;

	bIsSprinting = false;
	if (HasEquippedWeapon())
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

bool ATPSCPPCharacter::Server_SprintEnd_Validate()
{
	return true;
}
 
	void ATPSCPPCharacter::DoShoulderAimStart()
{
	if (AimState != EAimState::Hipfire || !HasEquippedWeapon())
	{
		return;
	}

	AimState = EAimState::Shoulder;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	DoSprintEnd();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.5f;
	CameraTimeline->SetPlayRate(1.f);
	CameraTimeline->Play();
	if (!HasAuthority())
	{
		Server_SetAimState(EAimState::Shoulder);
	}
}

void ATPSCPPCharacter::DoShoulderAimEnd()
{
	if (AimState != EAimState::Shoulder)
	{
		return;
	}

	AimState = EAimState::Hipfire;
	if (HasEquippedWeapon())
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		CameraTimeline->SetPlayRate(1.f);
		CameraTimeline->Reverse();
		if (!HasAuthority())
		{
			Server_SetAimState(EAimState::Hipfire);
		}
	}
	else
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		if (!HasAuthority())
		{
			Server_SetAimState(EAimState::Hipfire);
		}
	}
}

void ATPSCPPCharacter::DoADSStart()
{
	if (AimState != EAimState::Hipfire || !HasEquippedWeapon())
	{
		return;
	}

	AimState = EAimState::ADS;
	bPendingADS = true;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	DoSprintEnd();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.5f;

	CameraTimeline->SetPlayRate(2.f);
	CameraTimeline->PlayFromStart();

	if (!HasAuthority())
	{
		Server_SetAimState(EAimState::ADS);
	}
}

void ATPSCPPCharacter::DoADSToggle()
{
	if (AimState == EAimState::ADS)
	{
		DoADSEnd();
	}
	else
	{
		DoADSStart();
	}
}

void ATPSCPPCharacter::DoFirePressed()
{
	if (Combat && HasEquippedWeapon())
	{
		// Cancel sprinting before firing
		if (bIsSprinting)
		{
			DoSprintEnd();
		}
		Combat->FireButtonPressed(true);
	}
}

void ATPSCPPCharacter::DoFireReleased()
{
	if (Combat && HasEquippedWeapon())
	{
		Combat->FireButtonPressed(false);
	}
}

void ATPSCPPCharacter::PlayFireMontage(bool bPlay)
{
	if (!GetMesh() || !GetMesh()->GetAnimInstance()) return;

	if (bPlay)
	{
		if (FireMontage)
		{
			GetMesh()->GetAnimInstance()->Montage_Play(FireMontage);
		}
	}
	else
	{
		if (FireMontage)
		{
			GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, FireMontage);
		}
	}
}


void ATPSCPPCharacter::MulticastPlayHitReaction_Implementation(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation)
{
	if (BloodParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BloodParticles, ImpactPoint, ImpactRotation);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
	}
}

void ATPSCPPCharacter::DoADSEnd()
{
	if (AimState != EAimState::ADS)
	{
		return;
	}

	AimState = EAimState::Hipfire;
	bPendingADS = false;
	FPS_Camera->SetActive(false);
	FollowCamera->SetActive(true);

	if (IsLocallyControlled())
	{
		CustomMesh->SetVisibility(true);

		if (AWeapon* Weapon = Combat->GetEquippedWeapon())
		{
			Weapon->WeaponMesh->SetVisibility(true);
		}

		if (ViewModelWeapon)
		{
			ViewModelWeapon->SetVisibility(false);
		}

		ADSTimeline->Stop();

		CameraTimeline->SetPlayRate(2.f);
		CameraTimeline->Reverse();
	}

	if (HasEquippedWeapon())
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	else
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	if (!HasAuthority())
	{
		Server_SetAimState(EAimState::Hipfire);
	}
}

void ATPSCPPCharacter::CameraTimelineUpdate(float Value)
{
	CameraBoom->TargetArmLength = FMath::Lerp(NormalArmLength, AimingArmLength, Value);
	CameraBoom->SocketOffset = FMath::Lerp(NormalSocketOffset, AimingSocketOffset, Value);

	// ADS uses the equipped weapon's FOV, otherwise fall back to the character aim FOV
	float TargetFOV = AimingFOV;
	if (AimState == EAimState::ADS)
	{
		if (AWeapon* Weapon = Combat->GetEquippedWeapon())
		{
			TargetFOV = Weapon->ADSFOV;
		}
	}
	FollowCamera->SetFieldOfView(FMath::Lerp(NormalFOV, TargetFOV, Value));
}

void ATPSCPPCharacter::CameraTimelineFinished()
{
	if (AimState == EAimState::ADS && bPendingADS && IsLocallyControlled())
	{
		bPendingADS = false;
		FollowCamera->SetActive(false);
		FPS_Camera->SetActive(true);
		CustomMesh->SetVisibility(false);

		if (AWeapon* Weapon = Combat->GetEquippedWeapon())
		{
			Weapon->WeaponMesh->SetVisibility(false);

			if (ViewModelWeapon)
			{
				ViewModelWeapon->SetSkeletalMeshAsset(Weapon->WeaponMesh->GetSkeletalMeshAsset());
				ViewModelWeapon->SetRelativeLocation(FPSWeaponStartLocation);
				ViewModelWeapon->SetRelativeRotation(FPSWeaponRelativeRotation);
				ViewModelWeapon->SetVisibility(true);
			}

			ADSTimeline->SetPlayRate(Weapon->ADSTimelinePlayRate);
			ADSTimeline->PlayFromStart();
		}
	}
}

void ATPSCPPCharacter::ADSWeaponTimelineUpdate(float Value)
{
	if (ViewModelWeapon)
	{
		ViewModelWeapon->SetRelativeLocation(
			FMath::Lerp(FPSWeaponStartLocation, FPSWeaponRelativeLocation, Value));
	}
}

void ATPSCPPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATPSCPPCharacter, AimState, COND_None);
	DOREPLIFETIME_CONDITION(ATPSCPPCharacter, bIsEquipped, COND_None);
	DOREPLIFETIME_CONDITION(ATPSCPPCharacter, Health, COND_None);
}

void ATPSCPPCharacter::Server_SetAimState_Implementation(EAimState NewState)
{
	if (NewState == EAimState::Shoulder || NewState == EAimState::ADS)
	{
		if (AimState != EAimState::Hipfire)
		{
			return;
		}
	}

	AimState = NewState;

	if (NewState == EAimState::Shoulder || NewState == EAimState::ADS)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.5f;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

bool ATPSCPPCharacter::Server_SetAimState_Validate(EAimState NewState)
{
	return true;
}





//*****Health*****//





void ATPSCPPCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	if (HasAuthority())
	{
		UpdateHUDHealth();
	}

	if (Health == 0.f && GameModeRef)
	{
		PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetController()) : PlayerController;
		ATPSCPPPlayerController* AttackerController = Cast<ATPSCPPPlayerController>(InstigatorController);
		GameModeRef->PlayerEliminated(this, PlayerController, AttackerController);
	}
}

void ATPSCPPCharacter::OnRep_Health()
{
	UpdateHUDHealth();
}


void ATPSCPPCharacter::UpdateHUDHealth()
{
	PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetController()) : PlayerController;
	if (PlayerController)
	{
		PlayerController->SetHealthHUD(Health, MaxHealth);
	}
}


void ATPSCPPCharacter::Elim_Implementation()
{
	if (bEliminated) return;
	bEliminated = true;

	// Stop movement and input so the eliminated character cannot keep acting
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (GetController())
	{
		GetController()->SetIgnoreMoveInput(true);
		GetController()->SetIgnoreLookInput(true);
	}

	// Stop firing if the fire button was held
	if (Combat)
	{
		if (Combat->bFireButtonPressed)
		{
			Combat->FireButtonPressed(false);
		}
	}

	// Disable capsule and child mesh collision queries so the corpse cannot block or be picked up
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CustomMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Drop the equipped weapon so it falls as a physical pickup instead of floating on the ragdoll
	if (Combat)
	{
		Combat->DropEquippedWeapon();
	}

	// Turn the mesh into a ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);

	// Attach the camera boom to the mesh so the camera follows the ragdoll corpse
	CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform);

	// Destroy the actor after 5 seconds
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ATPSCPPCharacter::ElimTimerFinished, 5.f);
}

void ATPSCPPCharacter::ElimTimerFinished()
{
	Destroy();
}
