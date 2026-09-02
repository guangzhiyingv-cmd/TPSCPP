#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TPSCPPGameState.generated.h"

UCLASS()
class TPSCPP_API ATPSCPPGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Recalculates the player with the most kills from all player states. Server only. */
	void UpdateTopKiller();

	/** Name of the player with the most kills. */
	UPROPERTY(ReplicatedUsing = OnRep_TopKiller, BlueprintReadOnly, Category = "Match")
	FString TopKillerName;

	/** Kill count of the current top killer. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 TopKills = 0;

private:
	UFUNCTION()
	void OnRep_TopKiller();
};
