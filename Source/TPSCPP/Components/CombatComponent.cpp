#include "Components/CombatComponent.h"
#include "TPSCPPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

	FHitResult TraceHitResult;
	TraceUnderCrosshairs(TraceHitResult);
	ServerFire(bPressed, HitTarget);
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
		FVector Start = CrosshairWorldPosition;
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(TraceHitResult,Start,End,ECollisionChannel::ECC_Visibility);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}
		else
		{
			HitTarget = TraceHitResult.ImpactPoint;
			DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint,12.f,12,FColor::Red);
		}
	}
	
}
