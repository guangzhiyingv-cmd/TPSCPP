// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSCPPGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ATPSCPPGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ATPSCPPGameMode();

	virtual void PlayerEliminated(class ATPSCPPCharacter* ElimmedCharacter, class ATPSCPPPlayerController* VictimController, class ATPSCPPPlayerController* AttackerController);

	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, class AController* ElimmedController);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Seconds to wait in warmup once enough players have joined. */
	UPROPERTY(EditAnywhere, Category = "Match", meta = (ClampMin = 1))
	float WarmupTime = 10.f;

	/** Minimum players required to start the warmup countdown. */
	UPROPERTY(EditAnywhere, Category = "Match", meta = (ClampMin = 1))
	int32 MinPlayersToStart = 2;

	/** Total duration of the match after warmup ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (ClampMin = 1))
	float MatchTime = 120.f;

	/** Seconds shown after the match ends before restarting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (ClampMin = 1))
	float PostMatchTime = 10.f;

	/** Server-side remaining warmup time. */
	float GetWarmupRemainingTime() const;

	/** Sends warmup timing data to all player states. */
	void BroadcastWarmupStart();

	/** Server-side remaining match time. */
	float GetMatchRemainingTime() const;

	/** Sends match timing data to all player states. */
	void BroadcastMatchStart();

	/** Server-side remaining post-match time. */
	float GetPostMatchRemainingTime() const;

	/** Sends post-match timing data to all player states. */
	void BroadcastPostMatchStart();

	/** Destroys every player-controlled pawn in the world. */
	void DestroyAllPlayerPawns();

private:
	bool bWarmupCountdownStarted = false;
	float WarmupStartTime = 0.f;
	float MatchStartTime = 0.f;
	bool bPostMatchStarted = false;
	float PostMatchStartTime = 0.f;
};



