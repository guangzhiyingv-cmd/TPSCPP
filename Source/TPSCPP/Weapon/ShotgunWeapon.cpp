#include "Weapon/ShotgunWeapon.h"

void AShotgunWeapon::Fire(bool bPlay, const FVector& HitTarget)
{
	// AWeapon::Fire plays the single fire animation and ejects one casing. Calling the
	// HitScan base would perform a single additional center trace, so it is skipped here.
	AWeapon::Fire(bPlay, HitTarget);

	if (!HasAuthority() || !bPlay)
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn)
	{
		return;
	}

	AController* InstigatorController = InstigatorPawn->GetController();
	if (!InstigatorController)
	{
		return;
	}

	const FVector TraceStart = GetMuzzleLocation();
	const FVector ToTarget = HitTarget - TraceStart;
	const FVector CenterDirection = ToTarget.GetSafeNormal();
	if (CenterDirection.IsNearlyZero())
	{
		return;
	}

	const float TraceDistance = FMath::Max(ToTarget.Size() * 1.5f, 100.f);
	const float SpreadAngleRadians = FMath::DegreesToRadians(
		GetPelletSpreadAngleDegrees());
	const int32 PelletsToFire = FMath::Max(GetPelletCount(), 1);

	// Collect every pellet and send the cues once, so one shot costs one impact RPC and one blood
	// RPC per victim instead of one per pellet.
	TArray<FHitScanHit> Hits;
	Hits.Reserve(PelletsToFire);

	for (int32 PelletIndex = 0; PelletIndex < PelletsToFire; ++PelletIndex)
	{
		const FVector PelletDirection = SpreadAngleRadians > SMALL_NUMBER
			? FMath::VRandCone(CenterDirection, SpreadAngleRadians)
			: CenterDirection;

		FHitScanHit Hit;
		if (PerformHitScan(
			TraceStart,
			PelletDirection,
			TraceDistance,
			InstigatorController,
			Hit))
		{
			Hits.Add(Hit);
		}
	}

	SendHitCues(Hits);
}
