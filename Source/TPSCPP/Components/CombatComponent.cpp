#include "Components/CombatComponent.h"
#include "Character/TPSCPPCharacter.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "HUD/PlayerHUD.h"
#include "TPSCPP.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

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
	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
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
	UpdateAmmoHUD();
}

void UCombatComponent::DropEquippedWeapon()
{
	if (!EquippedWeapon) return;

	if (bReloading)
	{
		bReloading = false;
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	}

	EquippedWeapon->Dropped();
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (!Character) return;

	if (EquippedWeapon)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		EquippedWeapon->WeaponMesh->AttachToComponent(
			Character->GetCustomMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("hand_rSocket"));
		Character->bUseControllerRotationYaw = false;
		Character->GetCharacterMovement()->bUseControllerDesiredRotation = true;
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		UpdateAmmoHUD();
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
	if (!EquippedWeapon || EquippedWeapon->Ammo <= 0) return;

	// Firing cancels an in-progress reload
	bReloading = false;

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

	if (!EquippedWeapon) return;

	// Automatically start a reload when the magazine is empty
	if (EquippedWeapon->Ammo <= 0)
	{
		StartReload();
		return;
	}

	// Continue firing while the button is held and the weapon supports full auto
	if (EquippedWeapon->bAutomatic && bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::StartReload()
{
	if (!EquippedWeapon || !Character || bReloading) return;
	if (EquippedWeapon->Ammo >= EquippedWeapon->MagCapacity) return;
	if (!EquippedWeapon->bInfiniteAmmo && Character->ReserveAmmo <= 0) return;

	if (Character->GetAimState() == EAimState::ADS)
	{
		Character->DoADSEnd();
	}

	bReloading = true;

	if (Character->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UCombatComponent::ReloadTimerFinished, EquippedWeapon->ReloadTime);
		MulticastReload(true, EquippedWeapon->ReloadTime);
	}
	else
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!EquippedWeapon || !Character || bReloading) return;
	if (EquippedWeapon->Ammo >= EquippedWeapon->MagCapacity) return;
	if (!EquippedWeapon->bInfiniteAmmo && Character->ReserveAmmo <= 0) return;

	bReloading = true;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UCombatComponent::ReloadTimerFinished, EquippedWeapon->ReloadTime);
	MulticastReload(true, EquippedWeapon->ReloadTime);
}

void UCombatComponent::MulticastReload_Implementation(bool bPlay, float ReloadTime)
{
	if (Character)
	{
		Character->PlayReloadMontage(bPlay, ReloadTime);
	}
}

void UCombatComponent::ReloadTimerFinished()
{
	if (!EquippedWeapon)
	{
		bReloading = false;
		return;
	}

	if (EquippedWeapon->bInfiniteAmmo)
	{
		EquippedWeapon->SetAmmo(EquippedWeapon->MagCapacity);
	}
	else
	{
		const int32 AmmoNeeded = EquippedWeapon->MagCapacity - EquippedWeapon->Ammo;
		const int32 AmmoToLoad = FMath::Min(AmmoNeeded, Character->ReserveAmmo);
		EquippedWeapon->SetAmmo(EquippedWeapon->Ammo + AmmoToLoad);
		Character->ReserveAmmo -= AmmoToLoad;
	}

	UpdateAmmoHUD();
	MulticastReloadFinished();
}

void UCombatComponent::MulticastReloadFinished_Implementation()
{
	bReloading = false;
	if (Character)
	{
		Character->PlayReloadMontage(false);
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
	if (bPressed)
	{
		// Firing cancels an in-progress reload
		if (bReloading)
		{
			bReloading = false;
			GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
			MulticastReload(false, 0.f);
		}

		if (EquippedWeapon && EquippedWeapon->Ammo > 0)
		{
			EquippedWeapon->SetAmmo(EquippedWeapon->Ammo - 1);
			UpdateAmmoHUD();
		}
	}
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
  		}
  		else
  		{
  			TraceHitResult.ImpactPoint = End;
  			HitTarget = End;
  		}

  		// Move the fire target slightly forward along the screen-center ray
  		HitTarget += CrosshairWorldDirection * 15.f;
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

void UCombatComponent::UpdateAmmoHUD()
{
	if (!Character) return;

	ATPSCPPPlayerController* PlayerController = Character->PlayerController;
	if (!PlayerController)
	{
		PlayerController = Cast<ATPSCPPPlayerController>(Character->GetController());
	}
	Controller = PlayerController;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<APlayerHUD>(Controller->GetHUD()) : HUD;
		if (HUD && HUD->CharacterOverlay)
		{
			const int32 Ammo = EquippedWeapon ? EquippedWeapon->Ammo : 0;
			const int32 Reserve = Character->ReserveAmmo;
			Controller->SetAmmoHUD(Ammo, Reserve);
		}
	}
}
