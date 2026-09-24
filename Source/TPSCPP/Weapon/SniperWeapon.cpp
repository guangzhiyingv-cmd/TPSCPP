// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SniperWeapon.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ASniperWeapon::ASniperWeapon()
{
	// The lens starts opaque, so the scope reads as glass until the player aims.
	ScopePostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("ScopePostProcess"));
	ScopePostProcess->SetupAttachment(RootComponent);
	ScopePostProcess->bUnbound = true;
	ScopePostProcess->bEnabled = true;
	ScopePostProcess->Priority = 100.f;
	ScopePostProcess->BlendWeight = 0.f;
}

void ASniperWeapon::ApplyWeaponData(bool bInitializeRuntimeState)
{
	Super::ApplyWeaponData(bInitializeRuntimeState);

	// The data table may have swapped the mesh, which drops the previous lens instance.
	ScopeLensMaterial = nullptr;
	SetAimBlend(0.f);

	if (ScopePostProcess && !ScopePostProcessMaterial.IsNull())
	{
		ScopePostProcessMaterialInstance = UMaterialInstanceDynamic::Create(
			ScopePostProcessMaterial.LoadSynchronous(),
			this);

		ScopePostProcess->Settings.WeightedBlendables.Array.Empty();
		ScopePostProcess->Settings.AddBlendable(ScopePostProcessMaterialInstance, 1.f);
	}
}

void ASniperWeapon::SetAimBlend(float Blend)
{
	EnsureScopeLensMaterial();

	if (ScopeLensMaterial)
	{
		ScopeLensMaterial->SetScalarParameterValue(
			ScopeLensAlphaParameter,
			FMath::Lerp(
				ScopeLensOpaqueAlpha,
				ScopeLensTransparentAlpha,
				FMath::Clamp(Blend, 0.f, 1.f)));
	}

	// SetAimBlend is only driven by the local ADS presentation (the ADS timeline plays solely on the
	// machine that owns the view), so the blend already belongs to the player looking through the
	// scope. Gating on GetOwner() was meant to skip remote machines, but a client does not always
	// resolve the replicated weapon owner in time, which silently killed the effect there.
	const float ScopeBlend = FMath::Clamp(Blend, 0.f, 1.f);

	if (ScopePostProcess)
	{
		ScopePostProcess->BlendWeight = ScopeBlend;
	}

	if (ScopePostProcessMaterialInstance)
	{
		ScopePostProcessMaterialInstance->SetScalarParameterValue(
			ScopeZoomParameter,
			FMath::Lerp(1.f, ScopeZoomScale, ScopeBlend));
		ScopePostProcessMaterialInstance->SetScalarParameterValue(
			ScopeMaskRadiusParameter,
			ScopeMaskRadius);
		ScopePostProcessMaterialInstance->SetScalarParameterValue(
			ScopeMaskBitParameter,
			static_cast<float>(ScopeMaskStencilBit));
	}
}

void ASniperWeapon::EnsureScopeLensMaterial()
{
	if (ScopeLensMaterial || !WeaponMesh)
	{
		return;
	}

	const int32 LensIndex = WeaponMesh->GetMaterialIndex(ScopeLensMaterialSlot);
	if (LensIndex == INDEX_NONE)
	{
		if (!bWarnedMissingScopeLensSlot)
		{
			bWarnedMissingScopeLensSlot = true;
			UE_LOG(LogTemp, Warning,
				TEXT("'%s' has no material slot '%s' for the scope lens."),
				*GetNameSafe(this),
				*ScopeLensMaterialSlot.ToString());
		}

		return;
	}

	ScopeLensMaterial = WeaponMesh->CreateDynamicMaterialInstance(
		LensIndex,
		nullptr,
		TEXT("ScopeLens"));
}
