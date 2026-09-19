// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCPPCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
#include "UObject/UnrealType.h"
#include "Damage/DamageZoneMultiplier.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PlayerState/TPSCPPPlayerState.h"
#include "AbilitySystem/TPSCPPAbilitySystemComponent.h"
#include "AbilitySystem/TPSCPPHealthSet.h"
#include "AbilitySystem/TPSCPPGameplayTags.h"
#include "AbilitySystem/Abilities/GA_FireWeapon.h"
#include "AbilitySystem/Abilities/GA_Reload.h"
#include "AbilitySystem/Abilities/GA_Sprint.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"

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

	ADSRecoilTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("ADSRecoilTimeline"));

	ViewModelWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ViewModelWeapon"));
	ViewModelWeapon->SetupAttachment(FPS_Camera);
	ViewModelWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ViewModelWeapon->SetCastShadow(false);
	ViewModelWeapon->SetVisibility(false);

	// Ignore camera channel on all character collision components to prevent camera clipping
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	ReloadAbilityClass = UGA_Reload::StaticClass();
	FireAbilityClass = UGA_FireWeapon::StaticClass();
	SprintAbilityClass = UGA_Sprint::StaticClass();

	ReserveAmmo = StartingReserveAmmo;
}

void ATPSCPPCharacter::BeginPlay()
{
	Super::BeginPlay();

	GameModeRef = GetWorld()->GetAuthGameMode<ATPSCPPGameMode>();
	InitAbilitySystem();
	if (HasAuthority())
	{
		// Point damage is broadcast first and records the hit zone multiplier used below.
		OnTakePointDamage.AddDynamic(this, &ATPSCPPCharacter::CacheHitZoneDamageMultiplier);
		OnTakeAnyDamage.AddDynamic(this, &ATPSCPPCharacter::ReceiveDamage); 
	}

	LinkAnimLayer(DefaultAnimLayer);
}


void ATPSCPPCharacter::Restart()
{
	Super::Restart();
	UpdateHUDHealth();
}

UAbilitySystemComponent* ATPSCPPCharacter::GetAbilitySystemComponent() const
{
	const ATPSCPPPlayerState* PS = Cast<ATPSCPPPlayerState>(GetPlayerState());
	return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

void ATPSCPPCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Rebind this pawn as the avatar of the PlayerState-owned ability system.
	InitAbilitySystem();
}

void ATPSCPPCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// The PlayerState (and its ability system) has arrived on this machine.
	InitAbilitySystem();
}

void ATPSCPPCharacter::InitAbilitySystem()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (ATPSCPPPlayerState* PS = Cast<ATPSCPPPlayerState>(GetPlayerState()))
	{
		PS->InitAbilityActorInfoForPawn(this);
	}

	if (!bAbilitySystemInitialized)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UTPSCPPHealthSet::GetHealthAttribute())
			.AddUObject(this, &ATPSCPPCharacter::OnHealthAttributeChanged);

		// The replicated reload state replaces the local prediction as soon as it arrives.
		ASC->RegisterGameplayTagEvent(TPSCPPGameplayTags::State_Reloading, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ATPSCPPCharacter::OnReloadTagChanged);

		bAbilitySystemInitialized = true;
	}

	// The server owns the initial values; clients receive them through attribute replication.
	if (HasAuthority() && ASC->GetSet<UTPSCPPHealthSet>())
	{
		ASC->SetNumericAttributeBase(UTPSCPPHealthSet::GetMaxHealthAttribute(), MaxHealth);
		ASC->SetNumericAttributeBase(UTPSCPPHealthSet::GetHealthAttribute(), MaxHealth);
	}

	// Grant the default abilities once on the server.
	if (HasAuthority() && !bAbilitiesGranted)
	{
		if (ReloadAbilityClass)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(ReloadAbilityClass, 1, INDEX_NONE, this));
		}

		if (FireAbilityClass)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(FireAbilityClass, 1, INDEX_NONE, this));
		}

		if (SprintAbilityClass)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(SprintAbilityClass, 1, INDEX_NONE, this));
		}

		bAbilitiesGranted = true;
	}

	UpdateHUDHealth();
}

void ATPSCPPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PollInit();
	UpdateRecoilReturn(DeltaTime);
	PushAnimStateToAnimInstance();
}

void ATPSCPPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	ResolveCustomMesh();

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

	if (ADSRecoilCurve && ADSRecoilTimeline)
	{
		FOnTimelineFloat ADSRecoilProgress;
		ADSRecoilProgress.BindUFunction(this, FName("ADSRecoilTimelineUpdate"));
		ADSRecoilTimeline->AddInterpFloat(ADSRecoilCurve, ADSRecoilProgress);
		ADSRecoilTimeline->SetLooping(false);
	}
}

void ATPSCPPCharacter::SetCustomMesh(USkeletalMeshComponent* InCustomMesh)
{
	CustomMesh = IsValid(InCustomMesh) ? InCustomMesh : nullptr;

	if (CustomMesh)
	{
		CustomMesh->SetCollisionObjectType(ECC_SkeletalMesh);
		CustomMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}

bool ATPSCPPCharacter::ResolveCustomMesh()
{
	if (CustomMeshComponentTag.IsNone())
	{
		SetCustomMesh(nullptr);
		UE_LOG(LogTPSCPP, Error, TEXT("'%s' CustomMeshComponentTag is not configured."), *GetNameSafe(this));
		return false;
	}

	USkeletalMeshComponent* FoundMesh = FindComponentByTag<USkeletalMeshComponent>(CustomMeshComponentTag);
	if (!FoundMesh || FoundMesh == GetMesh())
	{
		SetCustomMesh(nullptr);
		UE_LOG(
			LogTPSCPP,
			Error,
			TEXT("'%s' could not find a valid SkeletalMeshComponent tagged '%s'."),
			*GetNameSafe(this),
			*CustomMeshComponentTag.ToString());
		return false;
	}

	SetCustomMesh(FoundMesh);
	return true;
}

void ATPSCPPCharacter::LinkAnimLayer(TSubclassOf<UAnimInstance> AnimLayerClass)
{
	if (CurrentAnimLayer == AnimLayerClass)
	{
		return;
	}

	USkeletalMeshComponent* AnimMesh = GetMesh();
	UAnimInstance* AnimInstance = AnimMesh ? AnimMesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (CurrentAnimLayer)
	{
		AnimInstance->UnlinkAnimClassLayers(CurrentAnimLayer);
	}

	CurrentAnimLayer = AnimLayerClass;

	if (CurrentAnimLayer)
	{
		AnimInstance->LinkAnimClassLayers(CurrentAnimLayer);
	}
}

void ATPSCPPCharacter::UnlinkAnimLayer()
{
	LinkAnimLayer(DefaultAnimLayer);
}

void ATPSCPPCharacter::NotifyWeaponFired()
{
	if (const UWorld* World = GetWorld())
	{
		LastFireTime = World->GetTimeSeconds();
	}
}

bool ATPSCPPCharacter::IsFiring() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && (World->GetTimeSeconds() - LastFireTime) <= FiringStateDuration;
}

void ATPSCPPCharacter::PushAnimStateToAnimInstance()
{
	USkeletalMeshComponent* AnimMesh = GetMesh();
	UAnimInstance* AnimInstance = AnimMesh ? AnimMesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	// The anim blueprint owns these as gameplay-tag bound variables, so resolve them by name once per anim class.
	if (CachedAnimStateClass != AnimInstance->GetClass())
	{
		CachedAnimStateClass = AnimInstance->GetClass();
		CachedAnimStateProperties.Reset();

		static const FName AnimStatePropertyNames[] =
		{
			TEXT("GameplayTag_IsFiring"),
			TEXT("GameplayTag_IsADS"),
			TEXT("GameplayTag_IsReloading"),
			TEXT("GameplayTag_IsDashing"),
			TEXT("GameplayTag_IsMelee")
		};

		for (const FName& PropertyName : AnimStatePropertyNames)
		{
			if (FBoolProperty* Property = FindFProperty<FBoolProperty>(CachedAnimStateClass, PropertyName))
			{
				CachedAnimStateProperties.Add(PropertyName, Property);
			}
		}
	}

	auto ApplyState = [this, AnimInstance](const FName& PropertyName, bool bValue)
	{
		if (FBoolProperty* const* Found = CachedAnimStateProperties.Find(PropertyName))
		{
			(*Found)->SetPropertyValue_InContainer(AnimInstance, bValue);
		}
	};

	ApplyState(TEXT("GameplayTag_IsFiring"), IsFiring());
	ApplyState(TEXT("GameplayTag_IsADS"), AimState == EAimState::ADS);
	ApplyState(TEXT("GameplayTag_IsReloading"), IsReloading());
	ApplyState(TEXT("GameplayTag_IsDashing"), IsSprinting());
	ApplyState(TEXT("GameplayTag_IsMelee"), false);
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

		// Reloading
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ATPSCPPCharacter::DoReload);
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

bool ATPSCPPCharacter::IsReloading() const
{
	if (bPredictedReloading)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC != nullptr && ASC->HasMatchingGameplayTag(TPSCPPGameplayTags::State_Reloading);
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

	USkeletalMeshComponent* CustomMeshComponent = GetCustomMesh();
	if (!Combat || !CustomMeshComponent)
	{
		return;
	}

	AWeapon* Weapon = Combat->GetEquippedWeapon();
	if (!Weapon || !Weapon->WeaponMesh)
	{
		return;
	}

	const FTransform SocketWorld = Weapon->WeaponMesh->GetSocketTransform(TEXT("LeftHandSocket"), RTS_World);
	const FTransform CustomMeshWorld = CustomMeshComponent->GetComponentTransform();

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

	// The ability owns the replicated State.Sprint tag and the authoritative movement, and rejects
	// the request while aiming or reloading.
	if (!TrySprintAbility())
	{
		return;
	}

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
}

void ATPSCPPCharacter::DoSprintEnd()
{
	if (AimState != EAimState::Hipfire) return;

	bIsSprinting = false;
	StopSprintAbility();

	if (HasEquippedWeapon())
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

bool ATPSCPPCharacter::TrySprintAbility()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && SprintAbilityClass ? ASC->TryActivateAbilityByClass(SprintAbilityClass) : false;
}

void ATPSCPPCharacter::StopSprintAbility()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// CancelAbilities skips specs that are not active locally, and a server only ability is never
	// instanced on the owning client, so ask the server to cancel the ability it is running.
	if (!HasAuthority())
	{
		Server_StopSprint();
		return;
	}

	FGameplayTagContainer SprintTags;
	SprintTags.AddTag(TPSCPPGameplayTags::Ability_Sprint);
	ASC->CancelAbilities(&SprintTags);
}

void ATPSCPPCharacter::Server_StopSprint_Implementation()
{
	StopSprintAbility();
}

bool ATPSCPPCharacter::Server_StopSprint_Validate()
{
	return true;
}

bool ATPSCPPCharacter::IsSprinting() const
{
	if (bIsSprinting)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC != nullptr && ASC->HasMatchingGameplayTag(TPSCPPGameplayTags::State_Sprint);
}

void ATPSCPPCharacter::SyncAimStateTags()
{
	// Mirrors the aim state into loose tags so abilities (and other machines) can gate on it.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	ASC->SetLooseGameplayTagCount(TPSCPPGameplayTags::State_ADS, AimState == EAimState::ADS ? 1 : 0);
	ASC->SetLooseGameplayTagCount(TPSCPPGameplayTags::State_ShoulderAim, AimState == EAimState::Shoulder ? 1 : 0);
}
 
	void ATPSCPPCharacter::DoShoulderAimStart()
{
	if (AimState != EAimState::Hipfire || !HasEquippedWeapon())
	{
		return;
	}

	// Sprinting has the lower priority: cancel it before switching the aim state.
	DoSprintEnd();

	AimState = EAimState::Shoulder;
	SyncAimStateTags();
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
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
	SyncAimStateTags();
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
	if (AimState != EAimState::Hipfire || !HasEquippedWeapon() || IsReloading())
	{
		return;
	}

	// Sprinting has the lower priority: cancel it before switching the aim state.
	DoSprintEnd();

	AimState = EAimState::ADS;
	SyncAimStateTags();
	bPendingADS = true;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
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
	if (Combat && HasEquippedWeapon() && !bEliminated)
	{
		// Cancel sprinting before firing
		if (IsSprinting())
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

void ATPSCPPCharacter::DoReload()
{
	if (!bEliminated)
	{
		TryReload();
	}
}

void ATPSCPPCharacter::TryReload()
{
	AWeapon* Weapon = Combat ? Combat->GetEquippedWeapon() : nullptr;
	if (!Weapon) return;
	if (Weapon->Ammo >= Weapon->MagCapacity) return;
	if (!Weapon->bInfiniteAmmo && ReserveAmmo <= 0) return;

	// Already reloading: either predicted locally or confirmed by the server's replicated state.
	// A second request must be ignored instead of replaying the montage.
	if (IsReloading())
	{
		return;
	}

	// Leaving ADS is a local camera action, so it must happen on the machine requesting the reload.
	if (AimState == EAimState::ADS)
	{
		DoADSEnd();
	}

	// Predict the reload so the owner sees it immediately. The server's replicated reload state (or
	// the refusal notification) ends the prediction.
	if (!HasAuthority())
	{
		bPredictedReloading = true;
		PlayReloadMontage(true, Weapon->ReloadTime);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->TryActivateAbilityByClass(ReloadAbilityClass);
	}
}

bool ATPSCPPCharacter::TryFireWeapon()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && FireAbilityClass ? ASC->TryActivateAbilityByClass(FireAbilityClass) : false;
}

void ATPSCPPCharacter::CancelReloadAbility()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		FGameplayTagContainer ReloadTags;
		ReloadTags.AddTag(TPSCPPGameplayTags::Ability_Weapon_Reload);
		ASC->CancelAbilities(&ReloadTags);
	}
}

void ATPSCPPCharacter::PlayReloadMontage(bool bPlay, float ReloadTime)
{
	if (!GetMesh() || !GetMesh()->GetAnimInstance() || !ReloadMontage)
	{
		return;
	}

	if (bPlay)
	{
		const float PlayRate = ReloadMontage->GetSectionLength(0) / FMath::Max(ReloadTime, 0.01f);
		GetMesh()->GetAnimInstance()->Montage_Play(ReloadMontage, PlayRate);
	}
	else
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, ReloadMontage);
	}
}

void ATPSCPPCharacter::MulticastPlayReloadMontage_Implementation(bool bPlay, float ReloadTime)
{
	// The owning client plays its predicted start itself, but every machine (including it) has to
	// receive the stop so a reload cancelled by the server ends everywhere at the same time.
	if (bPlay && !HasAuthority() && IsLocallyControlled())
	{
		return;
	}

	PlayReloadMontage(bPlay, ReloadTime);
}

void ATPSCPPCharacter::Client_StopReloadPresentation_Implementation()
{
	bPredictedReloading = false;
	PlayReloadMontage(false, 0.f);
}

void ATPSCPPCharacter::OnReloadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// The server's authoritative reload state (started, finished or cancelled) is mirrored here, so
	// the local prediction is no longer needed.
	bPredictedReloading = false;
}


void ATPSCPPCharacter::MulticastExecuteBloodCue_Implementation(const FVector_NetQuantize& ImpactPoint, const FRotator& ImpactRotation)
{
	// Executed locally on every machine: manual cue execution does not replicate, and this keeps the
	// hit point authoritative (the same value the server traced).
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Location = ImpactPoint;
	Params.Normal = ImpactRotation.Vector();

	ASC->ExecuteGameplayCue(TPSCPPGameplayTags::Cue_Hit_Blood, Params);
}

bool ATPSCPPCharacter::ShouldPlayHitSound()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastHitSoundTime < HitSoundCooldown)
	{
		return false;
	}

	LastHitSoundTime = CurrentTime;
	return true;
}

void ATPSCPPCharacter::DoADSEnd()
{
	if (AimState != EAimState::ADS)
	{
		return;
	}

	AimState = EAimState::Hipfire;
	SyncAimStateTags();
	bPendingADS = false;
	FPS_Camera->SetActive(false);
	FollowCamera->SetActive(true);

	if (IsLocallyControlled())
	{
		if (USkeletalMeshComponent* CustomMeshComponent = GetCustomMesh())
		{
			CustomMeshComponent->SetVisibility(true);
		}

		if (AWeapon* Weapon = Combat->GetEquippedWeapon())
		{
			Weapon->WeaponMesh->SetVisibility(true);
		}

		if (ViewModelWeapon)
		{
			ViewModelWeapon->SetVisibility(false);
		}

		ADSTimeline->Stop();

		if (ADSRecoilTimeline)
		{
			ADSRecoilTimeline->Stop();
		}

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
		if (USkeletalMeshComponent* CustomMeshComponent = GetCustomMesh())
		{
			CustomMeshComponent->SetVisibility(false);
		}

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
	if (!ViewModelWeapon || !Combat) return;

	if (AWeapon* Weapon = Combat->GetEquippedWeapon())
	{
		ViewModelWeapon->SetRelativeLocation(
			FMath::Lerp(FPSWeaponStartLocation, Weapon->FPSWeaponRelativeLocation, Value));
	}
}

void ATPSCPPCharacter::ApplyWeaponRecoil(bool bUseContinuousRecoil, float ContinuousFireTime)
{
	if (!IsLocallyControlled() || !Combat) return;

	AWeapon* Weapon = Combat->GetEquippedWeapon();
	if (!Weapon) return;

	float HorizontalMagnitude = Weapon->SingleShotHorizontalRecoil;
	float VerticalMagnitude = Weapon->SingleShotVerticalRecoil;

	if (bUseContinuousRecoil)
	{
		if (Weapon->AutoRecoilHorizontalCurve)
		{
			HorizontalMagnitude += Weapon->AutoRecoilHorizontalCurve->GetFloatValue(ContinuousFireTime);
		}

		if (Weapon->AutoRecoilVerticalCurve)
		{
			VerticalMagnitude += Weapon->AutoRecoilVerticalCurve->GetFloatValue(ContinuousFireTime);
		}
	}

	HorizontalMagnitude += FMath::RandRange(
		-Weapon->HorizontalRecoilPerturbation,
		Weapon->HorizontalRecoilPerturbation);
	HorizontalMagnitude = FMath::Max(0.f, HorizontalMagnitude);

	const float HorizontalDirection = FMath::RandBool() ? 1.f : -1.f;
	const float HorizontalRecoil = HorizontalMagnitude * HorizontalDirection;

	VerticalMagnitude += FMath::RandRange(
		-Weapon->VerticalRecoilPerturbation,
		Weapon->VerticalRecoilPerturbation);

	const float AppliedPitch = -VerticalMagnitude;

	AddControllerYawInput(HorizontalRecoil);
	AddControllerPitchInput(AppliedPitch);

	LastShotAppliedPitch = AppliedPitch;
	TimeSinceFireEnded = 0.f;
}

void ATPSCPPCharacter::StartWeaponRecoilBurst()
{
	LastShotAppliedPitch = 0.f;
	RecoilReturnProgress = 0.f;
	AppliedRecoilReturn = 0.f;
	TimeSinceFireEnded = 0.f;
}

void ATPSCPPCharacter::UpdateRecoilReturn(float DeltaTime)
{
	if (!IsLocallyControlled() || !Combat) return;

	AWeapon* Weapon = Combat->GetEquippedWeapon();
	if (!Weapon) return;

	if (Combat->bFireButtonPressed)
	{
		TimeSinceFireEnded = 0.f;
		RecoilReturnProgress = 0.f;
		AppliedRecoilReturn = 0.f;
		return;
	}

	TimeSinceFireEnded += DeltaTime;
	if (TimeSinceFireEnded < Weapon->RecoilRecoveryDelay) return;
	if (Weapon->RecoilRecoverySpeed <= 0.f) return;
	if (FMath::IsNearlyZero(LastShotAppliedPitch)) return;
	if (Weapon->VerticalRecoilRecoveryMultiplier <= 0.f) return;

	const float TotalVerticalReturn = -LastShotAppliedPitch * Weapon->VerticalRecoilRecoveryMultiplier;
	RecoilReturnProgress = FMath::FInterpTo(
		RecoilReturnProgress,
		1.f,
		DeltaTime,
		Weapon->RecoilRecoverySpeed);

	const float NewAppliedReturn = TotalVerticalReturn * RecoilReturnProgress;
	const float AppliedReturn = NewAppliedReturn - AppliedRecoilReturn;
	AppliedRecoilReturn = NewAppliedReturn;

	AddControllerPitchInput(AppliedReturn);
}

void ATPSCPPCharacter::PlayADSRecoil(float PlayRate)
{
	if (!IsLocallyControlled() || AimState != EAimState::ADS || !ADSRecoilTimeline) return;

	ADSRecoilTimeline->SetPlayRate(PlayRate);
	ADSRecoilTimeline->PlayFromStart();
}

void ATPSCPPCharacter::ADSRecoilTimelineUpdate(float Value)
{
	if (!Combat || !ViewModelWeapon) return;

	if (AWeapon* Weapon = Combat->GetEquippedWeapon())
	{
		FVector StartLocation = Weapon->FPSWeaponRelativeLocation;
		FVector EndLocation = StartLocation + FVector(-3.0f, 0.0f, 0.0f);
		FVector TargetLocation =  FMath::Lerp(StartLocation, EndLocation, Value);
		ViewModelWeapon->SetRelativeLocation(TargetLocation);
	}
}

void ATPSCPPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATPSCPPCharacter, AimState, COND_None);
	DOREPLIFETIME_CONDITION(ATPSCPPCharacter, bIsEquipped, COND_None);
	DOREPLIFETIME_CONDITION(ATPSCPPCharacter, ReserveAmmo, COND_OwnerOnly);
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
	SyncAimStateTags();

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





float ATPSCPPCharacter::ResolveZoneDamageMultiplier(FName BoneName) const
{
	// Fall back to the default multiplier whenever the table or the bone lookup is unavailable.
	if (!DamageZoneTable || BoneName.IsNone())
	{
		return FMath::Max(DefaultDamageMultiplier, 0.f);
	}

	static const FString ContextString(TEXT("ATPSCPPCharacter::ResolveZoneDamageMultiplier"));
	const FDamageZoneMultiplier* Row = DamageZoneTable->FindRow<FDamageZoneMultiplier>(BoneName, ContextString, /*bWarnIfRowMissing=*/false);
	if (!Row)
	{
		return FMath::Max(DefaultDamageMultiplier, 0.f);
	}

	return FMath::Max(Row->DamageMultiplier, 0.f);
}

void ATPSCPPCharacter::CacheHitZoneDamageMultiplier(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	PendingZoneDamageMultiplier = ResolveZoneDamageMultiplier(BoneName);
	bHasPendingZoneDamageMultiplier = true;
}

void ATPSCPPCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	// Point damage is broadcast before generic damage, so the hit zone multiplier is already resolved.
	const float ZoneMultiplier = bHasPendingZoneDamageMultiplier ? PendingZoneDamageMultiplier : 1.f;
	bHasPendingZoneDamageMultiplier = false;
	PendingZoneDamageMultiplier = 1.f;

	UTPSCPPAbilitySystemComponent* ASC = Cast<UTPSCPPAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ASC)
	{
		return;
	}

	ASC->ApplyDamage(Damage * ZoneMultiplier, DamageCauser, InstigatorController);

	// Instant effects resolve synchronously, so the health attribute already reflects the hit.
	const UTPSCPPHealthSet* HealthSet = ASC->GetSet<UTPSCPPHealthSet>();
	if (!bEliminated && HealthSet && HealthSet->GetHealth() <= 0.f && GameModeRef)
	{
		PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetController()) : PlayerController;
		GameModeRef->PlayerEliminated(this, PlayerController, Cast<ATPSCPPPlayerController>(InstigatorController));
	}
}

void ATPSCPPCharacter::OnRep_ReserveAmmo()
{
	if (Combat)
	{
		Combat->UpdateAmmoHUD();
	}
}


void ATPSCPPCharacter::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetController()) : PlayerController;
	if (!PlayerController)
	{
		return;
	}

	float CurrentMaxHealth = MaxHealth;
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (const UTPSCPPHealthSet* HealthSet = ASC->GetSet<UTPSCPPHealthSet>())
		{
			CurrentMaxHealth = HealthSet->GetMaxHealth();
		}
	}

	PlayerController->SetHealthHUD(Data.NewValue, CurrentMaxHealth);
}

void ATPSCPPCharacter::UpdateHUDHealth()
{
	PlayerController = PlayerController == nullptr ? Cast<ATPSCPPPlayerController>(GetController()) : PlayerController;
	if (!PlayerController)
	{
		return;
	}

	float CurrentHealth = 0.f;
	float CurrentMaxHealth = MaxHealth;
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (const UTPSCPPHealthSet* HealthSet = ASC->GetSet<UTPSCPPHealthSet>())
		{
			CurrentHealth = HealthSet->GetHealth();
			CurrentMaxHealth = HealthSet->GetMaxHealth();
		}
	}

	PlayerController->SetHealthHUD(CurrentHealth, CurrentMaxHealth);
}


void ATPSCPPCharacter::Elim()
{
	if (bEliminated) return;
	MulticastElim();

	// Destroy the actor after 5 seconds
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ATPSCPPCharacter::ElimTimerFinished, 5.f);
}



void ATPSCPPCharacter::MulticastElim_Implementation()
{
	if (bEliminated) return;
	bEliminated = true;

	// Stop movement and input so the eliminated character cannot keep acting
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		//PC->SetIgnoreMoveInput(true);
		//PC->SetIgnoreLookInput(true);
		DisableInput(PC);
	}

	// Stop firing if the fire button was held
	if (Combat)
	{
		if (Combat->bFireButtonPressed)
		{
			Combat->FireButtonPressed(false);
		}
	}

	// Stop sprinting so the corpse does not keep the replicated sprint state.
	bIsSprinting = false;
	StopSprintAbility();

	// Disable capsule and child mesh collision queries so the corpse cannot block or be picked up
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (USkeletalMeshComponent* CustomMeshComponent = GetCustomMesh())
	{
		CustomMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Drop the equipped weapon so it falls as a physical pickup instead of floating on the ragdoll
	if (Combat)
	{
		Combat->DropEquippedWeapon();
		Combat->bCanFire = false;
	}

	// Turn the mesh into a ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);

	// Attach the camera boom to the mesh so the camera follows the ragdoll corpse
	CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform);

	StartDissolve();
}

void ATPSCPPCharacter::ElimTimerFinished()
{
	if (GameModeRef)
	{
		GameModeRef->RequestRespawn(this, PlayerController);
	}
}

void ATPSCPPCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	for (UMaterialInstanceDynamic* MI : DissolveMIs)
	{
		if (MI)
		{
			MI->SetScalarParameterValue(DissolveParameterName, DissolveValue);
		}
	}
}

void ATPSCPPCharacter::StartDissolve()
{
	if (!DissolveCurve || !DissolveTimeline) return;

	DissolveMIs.Empty();
	for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
	{
		DissolveMIs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(i));
	}
	if (USkeletalMeshComponent* CustomMeshComponent = GetCustomMesh())
	{
		for (int32 i = 0; i < CustomMeshComponent->GetNumMaterials(); ++i)
		{
			DissolveMIs.Add(CustomMeshComponent->CreateAndSetMaterialInstanceDynamic(i));
		}
	}

	DissolveTrack.BindDynamic(this, &ATPSCPPCharacter::UpdateDissolveMaterial);
	DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
	DissolveTimeline->Play();
}

void ATPSCPPCharacter::PollInit()
{
	if (PlayerStateRef == nullptr)
	{
		PlayerStateRef = GetPlayerState<ATPSCPPPlayerState>();
		if (PlayerStateRef)
		{
			PlayerStateRef->AddToScore(0.0f);
			PlayerStateRef->AddToDefeats(0.0f);
		}
	}
}
