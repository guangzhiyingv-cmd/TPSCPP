#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PostMatchOverlay.generated.h"

UCLASS()
class TPSCPP_API UPostMatchOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CountdownText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TopPlayerText;

	void SetCountdownText(float Seconds);
	void SetTopPlayerInfo(const FString& PlayerName, int32 Kills);
};
