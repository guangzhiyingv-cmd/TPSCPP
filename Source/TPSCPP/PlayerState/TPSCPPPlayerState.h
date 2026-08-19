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

private:
	UFUNCTION()
	void OnRep_Defeats();

	class ATPSCPPCharacter* Character;
	class ATPSCPPPlayerController* PlayerController;
};
