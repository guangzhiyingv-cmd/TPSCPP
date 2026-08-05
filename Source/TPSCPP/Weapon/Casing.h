// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class TPSCPP_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACasing();

	/** Casing mesh, simulated with physics on spawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* CasingMesh;

	FORCEINLINE class UStaticMeshComponent* GetCasingMesh() const { return CasingMesh; }

	/** Sound played once when the casing first hits another actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	class USoundBase* HitSound;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Called when the casing mesh collides with another actor. */
	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

private:
	bool bHasPlayedHitSound = false;

};
