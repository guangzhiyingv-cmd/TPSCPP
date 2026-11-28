#include "Components/CombatComponent.h"
#include "Character/TPSCPPCharacter.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "HUD/PlayerHUD.h"
#include "TPSCPP.h"
#include "Engine/Engine.h"
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

	if (Character && Character->IsLocallyControlled() && EquippedWeapon)
	{
		UpdateSpread(DeltaTime);
	}
	SetHUDCrosshairs();
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

	USkeletalMeshComponent* CustomMesh = Character->GetCustomMesh();
	if (!CustomMesh)
	{
		Character->ResolveCustomMesh();
		CustomMesh = Character->GetCustomMesh();
	}

	if (!CustomMesh)
	{
		UE_LOG(LogTPSCPP, Error, TEXT("'%s' cannot equip a weapon without a valid CustomMesh."), *GetNameSafe(Character));
		return;
	}

	// Drop the currently equipped weapon if there is one
	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	// Attach the weapon mesh to the character's right hand socket
	EquippedWeapon->WeaponMesh->AttachToComponent(
		CustomMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("hand_rSocket"));

	SetOverlappingWeapon(nullptr);

	// Keep movement-facing when picking up a weapon during a sprint.
	Character->SetMovementFacing(Character->IsSprinting());
	Character->bIsEquipped = true;
	Character->LinkAnimLayer(EquippedWeapon->GetAnimLayer());
	UpdateAmmoHUD();
}

void UCombatComponent::DropEquippedWeapon()
{
	if (!EquippedWeapon) return;

	// Dropping cancels an in-progress reload.
	if (Character)
	{
		Character->CancelReloadAbility();
	}

	EquippedWeapon->Dropped();

	if (Character)
	{
		Character->LinkAnimLayer(Character->GetDefaultAnimLayer());
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (!Character) return;

	if (EquippedWeapon)
	{
		USkeletalMeshComponent* CustomMesh = Character->GetCustomMesh();
		if (!CustomMesh)
		{
			Character->ResolveCustomMesh();
			CustomMesh = Character->GetCustomMesh();
		}

		if (!CustomMesh)
		{
			UE_LOG(LogTPSCPP, Error, TEXT("'%s' cannot attach the replicated weapon without a valid CustomMesh."), *GetNameSafe(Character));
			return;
		}

		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		EquippedWeapon->WeaponMesh->AttachToComponent(
			CustomMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("hand_rSocket"));
		Character->SetMovementFacing(Character->IsSprinting());
		Character->LinkAnimLayer(EquippedWeapon->GetAnimLayer());
		UpdateAmmoHUD();
	}
}



//*****Fire*****//



void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bPressed)
	{
		if (Character)
		{
			Character->StartWeaponRecoilBurst();
		}

		bUseContinuousRecoil = false;
		ContinuousFireTime = 0.f;

		if (bCanFire && EquippedWeapon)
		{
			Fire();
		}
	}
	else
	{
		// Stop the predicted fire animation right away on the owning client; the server tells the others.
		if (!HasAuthority())
		{
			PlayFireCosmetics(false, HitTarget);
		}

		// Stop the fire animation on the other machines when the button is released
		ServerFire(false, HitTarget);
	}
}

void UCombatComponent::Fire()
{
	if (!EquippedWeapon || GetPredictedAmmo() <= 0) return;

	FHitResult TraceHitResult;
	TraceUnderCrosshairs(
		TraceHitResult,
		EquippedWeapon ? EquippedWeapon->ShouldApplyAimSpread() : true);
	if (Character)
	{
		Character->ApplyWeaponRecoil(
			EquippedWeapon->bAutomatic && bFireButtonPressed && bUseContinuousRecoil,
			ContinuousFireTime);
	}

	// Predict the shot locally so the owner sees it immediately: the ammo text and the weapon
	// animation. The server stays authoritative and the next replicated ammo update resyncs us.
	if (!HasAuthority())
	{
		++PredictedAmmoCost;
		UpdateAmmoHUD();
		PlayFireCosmetics(true, HitTarget);
	}

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
	if (GetPredictedAmmo() <= 0)
	{
		if (Character)
		{
			Character->TryReload();
		}
		return;
	}

	// Continue firing while the button is held and the weapon supports full auto
	if (EquippedWeapon->bAutomatic && bFireButtonPressed)
	{
		bUseContinuousRecoil = true;
		ContinuousFireTime += EquippedWeapon->FireDelay;
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(bool bPressed, const FVector_NetQuantize& InHitTarget)
{
	if (!bPressed)
	{
		// Stop the fire animation on all machines when the button is released.
		MulticastFire(false, InHitTarget);
		return;
	}

	// Firing interrupts a reload.
	if (Character)
	{
		Character->CancelReloadAbility();
	}

	// The fire ability owns ammo consumption and the fire rate. Broadcast the shot (animation,
	// casing, recoil, projectile) only when the server accepted it.
	if (Character && Character->TryFireWeapon())
	{
		UE_LOG(LogTPSCPP, VeryVerbose, TEXT("Shot accepted: aim=%s ammo=%d"),
			*UEnum::GetValueAsString(Character->GetAimState()), EquippedWeapon->Ammo);
		MulticastFire(true, InHitTarget);
	}
}

void UCombatComponent::MulticastFire_Implementation(bool bPressed, const FVector_NetQuantize& InHitTarget)
{
	// The owning client has already played its predicted shot locally.
	if (!HasAuthority() && Character && Character->IsLocallyControlled())
	{
		return;
	}

	PlayFireCosmetics(bPressed, InHitTarget);
}

void UCombatComponent::PlayFireCosmetics(bool bPressed, const FVector_NetQuantize& InHitTarget)
{
	if (!EquippedWeapon || !Character)
	{
		return;
	}

	EquippedWeapon->Fire(bPressed, InHitTarget);

	if (bPressed)
	{
		EquippedWeapon->UpdateFiringTime();
		Character->NotifyWeaponFired();
		Character->PlayADSRecoil(1.f / FMath::Max(EquippedWeapon->FireDelay, 0.01f));
	}
}


void UCombatComponent::TraceUnderCrosshairs(
	FHitResult& TraceHitResult,
	bool bApplyAimSpread)
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	
	if (bApplyAimSpread)
	{
		const float Spread = CalculateShootingSpread();
		CrosshairLocation += FMath::RandPointInCircle(Spread);
		UE_LOG(LogTPSCPP, VeryVerbose, TEXT("Shot trace: aim=%s spread=%.3f px"),
			*UEnum::GetValueAsString(Character->GetAimState()), Spread);
	}
	
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



void UCombatComponent::UpdateSpread(float DeltaTime)
{
	// Airborne spread: smoothly ramp to the bonus while in the air and recover to 0 after landing
	const float AirborneTarget = Character->GetCharacterMovement()->IsMovingOnGround()
		? 0.f
		: EquippedWeapon->AirborneSpreadBonus;
	AirborneSpread = FMath::FInterpTo(
		AirborneSpread,
		AirborneTarget,
		DeltaTime,
		EquippedWeapon->SpreadInterpSpeed);

	// Aim reduction: interpolate toward the target reduction for the current aim state
	float TargetAimReduction = 0.f;
	switch (Character->GetAimState())
	{
	case EAimState::Shoulder:
		TargetAimReduction = EquippedWeapon->ShoulderAimSpreadReduction;
		break;
	case EAimState::ADS:
		TargetAimReduction = EquippedWeapon->ADSAimSpreadReduction;
		break;
	default:
		break;
	}
	AimSpreadReduction = FMath::FInterpTo(
		AimSpreadReduction,
		TargetAimReduction,
		DeltaTime,
		EquippedWeapon->SpreadInterpSpeed);
}

float UCombatComponent::CalculateShootingSpread() const
{
	const float Spread = Character->GetVelocity().Size2D() * EquippedWeapon->VelocitySpreadMultiplier
		+ AirborneSpread - AimSpreadReduction;
	switch (Character->GetAimState())
	{
	case EAimState::Hipfire:
		return FMath::Max3(Spread, EquippedWeapon->HipfireMinSpread, 0.f);
	case EAimState::Shoulder:
		return FMath::Max3(Spread, EquippedWeapon->ShoulderMinSpread, 0.f);
	default:
		return FMath::Max(Spread, 0.f);
	}
}

void UCombatComponent::SetHUDCrosshairs()
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

			HUDPackage.CrosshairSpread = EquippedWeapon ? CalculateShootingSpread() : 0.f;

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
			const int32 Ammo = GetPredictedAmmo();
			const int32 Reserve = Character->ReserveAmmo;
			Controller->SetAmmoHUD(Ammo, Reserve);
		}
	}
}

int32 UCombatComponent::GetPredictedAmmo() const
{
	return EquippedWeapon ? FMath::Max(EquippedWeapon->Ammo - PredictedAmmoCost, 0) : 0;
}

void UCombatComponent::OnAmmoReplicated()
{
	// The authoritative value arrived: drop the optimistic prediction and show the real value.
	PredictedAmmoCost = 0;
	UpdateAmmoHUD();
}



