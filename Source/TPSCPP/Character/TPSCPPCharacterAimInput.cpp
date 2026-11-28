#include "Character/TPSCPPCharacter.h"

void ATPSCPPCharacter::HandleShoulderHold()
{
	if (bShoulderHoldHandled)
	{
		return;
	}
	bShoulderHoldHandled = true;
	if (AimState != EAimState::ADS || !HasEquippedWeapon() || bEliminated)
	{
		return;
	}
	DoADSEnd();
	DoShoulderAimStart();
}

void ATPSCPPCharacter::EndShoulderHold()
{
	bShoulderHoldHandled = false;
	DoShoulderAimEnd();
}
