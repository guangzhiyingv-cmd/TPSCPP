#include "Animation/TurnaroundAnimLayer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNode_StateMachine.h"

namespace
{
// Represent a full L-R-L stride on [0, 2) so samples beside a marker stay adjacent.
bool StridePhase(const FMarkerSyncAnimPosition& Marker, float& Out)
{
	static const FName Left(TEXT("L")), Right(TEXT("R"));
	if (Marker.PreviousMarkerName == Left && Marker.NextMarkerName == Right)
	{
		Out = Marker.PositionBetweenMarkers;
		return true;
	}
	if (Marker.PreviousMarkerName == Right && Marker.NextMarkerName == Left)
	{
		Out = 1.f + Marker.PositionBetweenMarkers;
		return true;
	}
	return false;
}

FVector2D TravelAtTime(const UTurnaroundEntryData& Data, float Time)
{
	const float Frame = FMath::Clamp(Time / Data.TravelSampleInterval, 0.f, float(Data.TravelVelocity.Num() - 1));
	const int32 Index = FMath::FloorToInt(Frame);
	return FMath::Lerp(Data.TravelVelocity[Index], Data.TravelVelocity[FMath::Min(Index + 1, Data.TravelVelocity.Num() - 1)], Frame - Index);
}

// Consume the entry phase offset while the running pose still carries most of the blend.
float EntryTime(float Elapsed, float Nominal, float Offset, float BrakeInterval)
{
	if (BrakeInterval <= 0.f || Elapsed >= BrakeInterval)
	{
		return Nominal + Elapsed;
	}
	const float Remaining = 1.f - Elapsed / BrakeInterval;
	// A late entry can be close to the brake point. Keep its clock moving forward.
	const float Curvature = Offset > 0.f ? FMath::Clamp(BrakeInterval / Offset - 1.f, 0.f, 1.f) : 1.f;
	return Nominal + Elapsed + Offset * Remaining * FMath::Lerp(1.f, Remaining, Curvature);
}

float ChooseEntry(const UAnimSequence& Sequence, const UTurnaroundEntryData& Data,
	const FMarkerSyncAnimPosition& Phase, const FVector& Point, const FVector& Velocity, float ToeWeight,
	float PhaseTolerance, float VelocityWeightSeconds)
{
	static const TArray<FName> Feet = {TEXT("L"), TEXT("R")};
	const int32 Count = Data.Samples.Num() - 1;
	if (!ensure(Count > 0 && Data.SampleInterval > 0.f && Data.BrakeTime > Data.NominalEntryTime
		&& Data.FirstSampleTime >= 0.f && Data.FirstSampleTime + (Count - 1) * Data.SampleInterval < Data.BrakeTime))
	{
		return Data.NominalEntryTime;
	}
	float SourcePhase = 0.f;
	const bool bHasPhase = StridePhase(Phase, SourcePhase);
	TArray<float, TInlineAllocator<32>> Distances;
	float Nearest = 1.f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Time = Data.FirstSampleTime + Index * Data.SampleInterval;
		FMarkerPair Prev, Next;
		Sequence.GetMarkerIndicesForTime(Time, false, Feet, Prev, Next);
		float DestPhase = 0.f;
		float Distance = 0.f;
		if (bHasPhase && StridePhase(Sequence.GetMarkerSyncPositionFromMarkerIndicies(Prev.MarkerIndex, Next.MarkerIndex, Time, nullptr), DestPhase))
		{
			const float D = FMath::Abs(DestPhase - SourcePhase);
			Distance = FMath::Min(D, 2.f - D);
		}
		else if (bHasPhase)
		{
			Distance = 1.f;
		}
		Nearest = FMath::Min(Nearest, Distance);
		Distances.Add(Distance);
	}
	float BestTime = Data.NominalEntryTime;
	float BestCost = MAX_flt;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Distances[Index] > Nearest + PhaseTolerance)
		{
			continue;
		}
		const float Time = Data.FirstSampleTime + Index * Data.SampleInterval;
		const FVector P = Data.Samples[Index].ContactPoint(ToeWeight);
		const FVector V = (Data.Samples[Index + 1].ContactPoint(ToeWeight) - P) / Data.SampleInterval;
		const float Cost = (P - Point).SizeSquared2D() + FMath::Square(VelocityWeightSeconds) * (V - Velocity).SizeSquared2D();
		if (Cost < BestCost)
		{
			BestTime = Time;
			BestCost = Cost;
		}
	}
	return BestTime;
}
}

void UTurnaroundAnimLayer::CaptureRelinkPose()
{
	// The outgoing item layer disappears immediately, but its main locomotion state can still be fading.
	static const FName RelinkPose(TEXT("TurnaroundRelink"));
	SavePoseSnapshot(RelinkPose);
	bUseRelinkPose = true;
}

bool UTurnaroundAnimLayer::CanReuseOtherPose() const
{
	// A third reversal can arrive while the first pose is still fading out.
	const FAnimNode_StateMachine* Machine = GetStateMachineInstanceFromName(TEXT("TurnaroundSM"));
	if (!Machine)
	{
		return true;
	}
	const int32 Other = Machine->GetStateIndex(bUseTurnB ? TEXT("TurnaroundA") : TEXT("TurnaroundB"));
	return Other == INDEX_NONE || !FAnimWeight::IsRelevant(Machine->GetStateWeight(Other));
}

bool UTurnaroundAnimLayer::StartTurn(bool bSprint, bool bLeft, bool bLeftFootLow, bool bRestart, double Elapsed,
	float MeshYaw, float ApproachYaw, const FMarkerSyncAnimPosition& Phase, const FTurnaroundFootSample& Foot,
	const FVector& ContactVelocity, float ToeWeight, float PhaseTolerance, float VelocityWeightSeconds)
{
	const FTurnaroundSide& Left = bSprint ? SprintLeftTurn : LeftTurn;
	const FTurnaroundSide& Right = bSprint ? SprintRightTurn : RightTurn;
	const FTurnaroundSide& Side = bLeft ? Left : Right;
	const FTurnaroundClip& Clip = bLeftFootLow ? Side.LeftFootLow : Side.RightFootLow;
	if (!Clip.Sequence || Clip.ExitTime <= Elapsed)
	{
		return false;
	}
	if (!bRestart)
	{
		TurnB = FTurnaroundPose();
	}
	bUseRelinkPose = false;
	SprintCarryWeight = bSprint && SprintCarryPose ? 1.f : 0.f;
	bUseTurnB = bRestart && !bUseTurnB;
	FTurnaroundPose& Pose = bUseTurnB ? TurnB : TurnA;
	Pose = FTurnaroundPose();
	Pose.Sequence = Clip.Sequence;
	Pose.EntryYaw = MeshYaw;
	Pose.ApproachYaw = ApproachYaw;
	Pose.Elapsed = Elapsed;
	Pose.ExitTime = Clip.ExitTime;
	Pose.CycleEntry = Clip.CycleEntryTime;
	Pose.MaxRunOutRateAdjustment = Clip.MaxRunOutRateAdjustment;
	Pose.ExtraRunOutStrideAdjustment = Clip.ExtraRunOutStrideAdjustment;
	Pose.ExitStrideScale = Clip.ExitStrideScale;
	Pose.ExitStrideLeadTime = Clip.ExitStrideLeadTime;
	if (const auto* Data = Clip.Sequence->FindMetaDataByClass<UTurnaroundEntryData>())
	{
		Pose.NominalTime = Data->NominalEntryTime;
		Pose.BrakeInterval = Data->BrakeTime - Pose.NominalTime;
		Pose.EntryOffset = ChooseEntry(*Clip.Sequence, *Data, Phase, Foot.ContactPoint(ToeWeight), ContactVelocity,
			ToeWeight, PhaseTolerance, VelocityWeightSeconds) - Pose.NominalTime;
	}
	return true;
}

void UTurnaroundAnimLayer::Advance(float DeltaSeconds, const FTransform& MeshTransform, const FVector& WorldVelocity)
{
	const float MeshYaw = MeshTransform.Rotator().Yaw;
	const float Speed = WorldVelocity.Size2D();
	for (FTurnaroundPose* Pose : {&TurnA, &TurnB})
	{
		if (!Pose->Sequence)
		{
			continue;
		}
		const auto* Data = Pose->Sequence->FindMetaDataByClass<UTurnaroundEntryData>();
		const bool bHasTravel = Data && !Data->TravelVelocity.IsEmpty() && Data->TravelSampleInterval > 0.f;
		float Rate = 1.f;
		float RunOutWeight = 0.f;
		const bool bCorrectRunOut = Pose->MaxRunOutRateAdjustment > 0.f || Pose->ExtraRunOutStrideAdjustment > 0.f
			|| Pose->ExitStrideScale > 1.f;
		if (bHasTravel && bCorrectRunOut && Pose->Elapsed > Pose->BrakeInterval && DeltaSeconds > 0.f)
		{
			const FVector2D Travel = TravelAtTime(*Data, Pose->Time);
			const float ReferenceSpeed = Travel.Size();
			if (ReferenceSpeed > UE_SMALL_NUMBER)
			{
				const FVector Desired = WorldVelocity.GetSafeNormal2D().RotateAngleAxis(-Pose->ApproachYaw, FVector::UpVector);
				// Fade in after both trajectories have reversed. Keep the braking plant on its authored clock.
				const float Away = FMath::Min(float(-Travel.X / ReferenceSpeed), float(-Desired.X));
				RunOutWeight = FMath::SmoothStep(0.f, 1.f, Away)
					* FMath::SmoothStep(0.f, WarpSpeedThreshold, FMath::Min(Speed, ReferenceSpeed));
				const float TargetRate = FMath::Clamp(Speed / ReferenceSpeed, 1.f - Pose->MaxRunOutRateAdjustment, 1.f + Pose->MaxRunOutRateAdjustment);
				Rate = FMath::Lerp(1.f, TargetRate, RunOutWeight);
			}
		}
		const bool bBeforeExit = Pose->Elapsed < Pose->ExitTime;
		Pose->Elapsed += DeltaSeconds * Rate;
		Pose->Time = FMath::Min(EntryTime(Pose->Elapsed, Pose->NominalTime, Pose->EntryOffset, Pose->BrakeInterval),
			Pose->Sequence->GetPlayLength());
		Pose->RootYaw = FMath::FindDeltaAngleDegrees(MeshYaw, Pose->EntryYaw);
		Pose->WarpAlpha = 0.f;
		if (!bHasTravel)
		{
			continue;
		}
		const FVector2D Travel = TravelAtTime(*Data, Pose->Time);
		const float ReferenceSpeed = Travel.Size();
		const float SupportSpeed = FMath::Min(Speed, ReferenceSpeed);
		if (SupportSpeed <= UE_SMALL_NUMBER)
		{
			continue;
		}
		const float ReferenceYaw = Pose->ApproachYaw + FMath::RadiansToDegrees(FMath::Atan2(Travel.Y, Travel.X));
		const float Correction = FMath::FindDeltaAngleDegrees(ReferenceYaw, WorldVelocity.Rotation().Yaw);
		Pose->OrientationAngle = FMath::Clamp(Correction, -MaxOrientationAngle, MaxOrientationAngle);
		Pose->StrideDirection = MeshTransform.InverseTransformVectorNoScale(WorldVelocity.GetSafeNormal2D());
		// Apply only the remaining speed error to stride length once playback rate has changed.
		const float StrideLimit = FMath::Min(0.9f, MaxStrideAdjustment + RunOutWeight * Pose->ExtraRunOutStrideAdjustment);
		// The shared nodes previously capped extension at neutral before interpolation.
		const float BaseStride = FMath::Clamp(Speed / (ReferenceSpeed * Rate), 1.f - StrideLimit, 1.f);
		const float ExitWeight = bBeforeExit && Pose->Elapsed >= Pose->ExitTime - Pose->ExitStrideLeadTime ? RunOutWeight : 0.f;
		Pose->StrideScale = BaseStride * FMath::Lerp(1.f, Pose->ExitStrideScale, ExitWeight);
		// At the brake point the two trajectories can cross zero on different frames.
		Pose->WarpAlpha = FMath::SmoothStep(0.f, WarpSpeedThreshold, SupportSpeed)
			* (1.f - FMath::SmoothStep(60.f, 90.f, FMath::Abs(Correction)));
	}
}
