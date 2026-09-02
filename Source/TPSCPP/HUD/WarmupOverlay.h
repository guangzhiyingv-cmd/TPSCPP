#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WarmupOverlay.generated.h"

UCLASS()
class TPSCPP_API UWarmupOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CountdownText;

	void SetCountdownText(float Seconds);
};
