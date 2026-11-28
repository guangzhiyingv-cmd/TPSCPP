#pragma once

#include "Animation/IrisAnimInstance.h"
#include "BonePose.h"

class ATPSCPPCharacter;
class UPrimitiveComponent;

struct FIrisLandingPose
{
	void Prepare(const ATPSCPPCharacter* Character, const FIrisShortFallSettings& InSettings, float DeltaSeconds);
	void Apply(FCompactPose& Output, FCSPose<FCompactPose>& Pose, const FTransform& Mesh,
		const FVector& HeelInAnkle, const FVector& ForefootInToe);
	bool IsActive() const { return Alpha > 0.; }

private:
	FIrisShortFallSettings Settings;
	FVector LastCenter = FVector::ZeroVector;
	FVector FloorPoint = FVector::ZeroVector;
	FVector FloorNormal = FVector::UpVector;
	TWeakObjectPtr<const UPrimitiveComponent> Base;
	FTransform BaseTransform = FTransform::Identity;
	FVector PlantPoint = FVector::ZeroVector;
	double Alpha = 0.;
	double Gap = 0.;
	double GroundTime = 0.;
	int32 Support = INDEX_NONE;
	bool bWasWalking = false;
	bool bTracking = false;
	bool bGrounded = false;
	bool bHasBase = false;
	bool bPlanted = false;
	bool bPlantHeel = false;
};
