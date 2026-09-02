// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PlayerController/TPSCPPPlayerController.h"
#include "TPSCPPPlayerState.generated.h"


UCLASS()
class TPSCPP_API ATPSCPPPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Score() override;
	void AddToScore(float ScoreAmount);

	/** Number of times this player has been eliminated. Replicated to all clients. */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Defeats)
	int32 Defeats = 0;

	/** Adds to the player's defeat count on the server and notifies clients. */
	void AddToDefeats(int32 DefeatsAmount);

	/** Sets warmup timing data on the server and replicates it to clients. */
	void SetWarmupData(float InWarmupStartTime, float InWarmupTime);

	/** Server time when the warmup countdown started. */
	UPROPERTY(VisibleAnywhere, Replicated, Category = "Match")
	float WarmupStartTime = 0.f;

	/** Total warmup countdown duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Match", meta = (ClampMin = 1))
	float WarmupTime = 10.f;

	/** Server time when the match started. */
	UPROPERTY(VisibleAnywhere, Replicated, Category = "Match")
	float MatchStartTime = 0.f;

	/** Total duration of the match after warmup ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Match", meta = (ClampMin = 1))
	float MatchTime = 120.f;

	/** Sets match timing data on the server and replicates it to clients. */
	void SetMatchData(float InMatchStartTime, float InMatchTime);

	/** Server time when the post-match countdown started. */
	UPROPERTY(VisibleAnywhere, Replicated, Category = "Match")
	float PostMatchStartTime = 0.f;

	/** Total post-match countdown duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Match", meta = (ClampMin = 1))
	float PostMatchTime = 10.f;

	/** Sets post-match timing data on the server and replicates it to clients. */
	void SetPostMatchData(float InPostMatchStartTime, float InPostMatchTime);

private:
	UFUNCTION()
	void OnRep_Defeats();

	class ATPSCPPCharacter* Character;
	class ATPSCPPPlayerController* PlayerController;
};
