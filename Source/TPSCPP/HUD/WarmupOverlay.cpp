#include "HUD/WarmupOverlay.h"
#include "Components/TextBlock.h"

void UWarmupOverlay::SetCountdownText(float Seconds)
{
	if (!CountdownText) return;

	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 RemainingSeconds = TotalSeconds % 60;
	CountdownText->SetText(FText::FromString(
		FString::Printf(TEXT("%02d:%02d"), Minutes, RemainingSeconds)));
}
