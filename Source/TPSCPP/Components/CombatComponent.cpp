#include "Components/CombatComponent.h"
#include "Character/TPSCPPCharacter.h"
#include "TPSCPPPlayerController.h"
#include "HUD/PlayerHUD.h"
#include "TPSCPP.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}



void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCombatComponent, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCombatComponent, EquippedWeapon, COND_None);
}


//*****Overlap&EquipWeapon*****//



void UCombatComponent::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	if (OverlappingWeapon != Weapon)
	{
		OverlappingWeapon = Weapon;
	}
	if (Character->IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void UCombatComponent::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	// Drop the currently equipped weapon if there is one
	if (EquippedWeapon)
	{
		EquippedWeapon->WeaponState = EWeaponState::EWS_Dropped;
		EquippedWeapon->SetAreaSphereCollisionEnabled(true);
		EquippedWeapon->WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->WeaponState = EWeaponState::EWS_Equipped;
	EquippedWeapon->SetAreaSphereCollisionEnabled(false);
	// Attach the weapon mesh to the character's right hand socket
	EquippedWeapon->WeaponMesh->AttachToComponent(
		Character->GetCustomMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("hand_rSocket"));

	SetOverlappingWeapon(nullptr);

	// Use camera-relative rotation when weapon is equipped
	Character->bUseControllerRotationYaw = false;
	Character->GetCharacterMovement()->bUseControllerDesiredRotation = true;
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bIsEquipped = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (!Character) return;

	if (EquippedWeapon)
	{
		Character->bUseControllerRotationYaw = false;
		Character->GetCharacterMovement()->bUseControllerDesiredRotation = true;
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}



//*****Fire*****//



void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bPressed)
	{
		if (bCanFire && EquippedWeapon)
		{
			Fire();
		}
	}
	else
	{
		// Stop the fire animation on all machines when the button is released
		ServerFire(false, HitTarget);
	}
}

void UCombatComponent::Fire()
{
	FHitResult TraceHitResult;
	TraceUnderCrosshairs(TraceHitResult);
	ServerFire(true, HitTarget);
	StartFireTimer();
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon) return;

	bCanFire = false;
	GetWorld()->GetTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;

	// Continue firing while the button is held and the weapon supports full auto
	if (EquippedWeapon && EquippedWeapon->bAutomatic && bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::MulticastFire_Implementation(bool bPressed, const FVector_NetQuantize& InHitTarget)
{
	if (!EquippedWeapon) return;
	if (Character)
	{
		EquippedWeapon->Fire(bPressed, InHitTarget);
		Character->PlayFireMontage(bPressed);
	}
}

void UCombatComponent::ServerFire_Implementation(bool bPressed, const FVector_NetQuantize& InHitTarget)
{
	MulticastFire(bPressed, InHitTarget);
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
  	);
  	if (bScreenToWorld)
  	{
  		// Start the trace at the character's position along the crosshair direction
  		// to avoid picking hit points between the camera and the pawn
  		float CameraToCharacterDistance = FVector::Dist(CrosshairWorldPosition, Character->GetActorLocation()) + 50.f;
  		FVector Start = CrosshairWorldPosition + CrosshairWorldDirection * CameraToCharacterDistance;
  		FVector End = CrosshairWorldPosition + CrosshairWorldDirection * TRACE_LENGTH;

  		// Trace multiple channels and keep the closest hit
  		FHitResult VisibilityHit;
  		FHitResult SkeletalHit;
  		bool bVisHit = GetWorld()->LineTraceSingleByChannel(VisibilityHit, Start, End, ECC_Visibility);
  		bool bSkelHit = GetWorld()->LineTraceSingleByChannel(SkeletalHit, Start, End, ECC_SkeletalMesh);

  		const FHitResult* ClosestHit = nullptr;
  		if (bVisHit && bSkelHit)
  		{
  			ClosestHit = (VisibilityHit.ImpactPoint - Start).SizeSquared() < (SkeletalHit.ImpactPoint - Start).SizeSquared()
  				? &VisibilityHit : &SkeletalHit;
  		}
  		else if (bVisHit)
  		{
  			ClosestHit = &VisibilityHit;
  		}
  		else if (bSkelHit)
  		{
  			ClosestHit = &SkeletalHit;
  		}

  		if (ClosestHit)
  		{
  			TraceHitResult = *ClosestHit;
  			HitTarget = ClosestHit->ImpactPoint;
  			DrawDebugSphere(GetWorld(), ClosestHit->ImpactPoint, 12.f, 12, FColor::Red);
  		}
  		else
  		{
  			TraceHitResult.ImpactPoint = End;
  			HitTarget = End;
  		}
  	}
	
}




//*****Player HUD*****//



void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<ATPSCPPPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<APlayerHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			FHUDPackage HUDPackage;
			if (EquippedWeapon && Character->GetAimState() != EAimState::ADS)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}

			// Crosshair spread grows with movement speed and narrows while aiming
			// Ground speed (ignore Z axis) so slopes and vertical movement do not widen the crosshair
			FVector CharacterVelocity = Character->GetVelocity();
			CharacterVelocity.Z = 0.f;
			float Spread = CharacterVelocity.Size() * VelocitySpreadMultiplier;

			// Airborne spread: smoothly ramp to the bonus while in the air and recover to 0 after landing
			if (!Character->GetCharacterMovement()->IsMovingOnGround())
			{
				AirborneSpread = FMath::FInterpTo(AirborneSpread, AirborneSpreadBonus, DeltaTime, SpreadInterpSpeed);
			}
			else
			{
				AirborneSpread = FMath::FInterpTo(AirborneSpread, 0.f, DeltaTime, SpreadInterpSpeed);
			}
			Spread += AirborneSpread;

			// Aim reduction: interpolate toward the target reduction for the current aim state
			float TargetAimReduction = 0.f;
			switch (Character->GetAimState())
			{
			case EAimState::Shoulder:
				TargetAimReduction = ShoulderAimSpreadReduction;
				break;
			case EAimState::ADS:
				TargetAimReduction = ADSAimSpreadReduction;
				break;
			default:
				break;
			}
			AimSpreadReduction = FMath::FInterpTo(AimSpreadReduction, TargetAimReduction, DeltaTime, SpreadInterpSpeed);
			Spread -= AimSpreadReduction;

			// Final per-state minimum clamp; ADS is intentionally unclamped
			switch (Character->GetAimState())
			{
			case EAimState::Hipfire:
				Spread = FMath::Max(Spread, HipfireMinSpread);
				break;
			case EAimState::Shoulder:
				Spread = FMath::Max(Spread, ShoulderMinSpread);
				break;
			case EAimState::ADS:
				break;
			default:
				break;
			}
			HUDPackage.CrosshairSpread = FMath::Max(Spread, 0.f);

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}
