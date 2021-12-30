// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSAIGuard.generated.h"

class UPawnSensingComponent;

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle,
	Suspicious,
	Alerted
};

UCLASS()
class FPSGAME_API AFPSAIGuard : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFPSAIGuard();

protected:

	UPROPERTY(EditInstanceOnly, Category = "AI")
	bool bIsPatrol;

	UPROPERTY(EditInstanceOnly, Category = "AI", meta = (EditCondition="bIsPatrol"))
	AActor* FirstPatrolPoint;

	UPROPERTY(EditInstanceOnly, Category = "AI", meta = (EditCondition = "bIsPatrol"))
	AActor* SecondPatrolPoint;

	UPROPERTY(EditInstanceOnly, Category = "AI", meta = (EditCondition = "bIsPatrol"))
	TArray<AActor*> PatrolPoints;

	int32 CurrentPatrolPoint;

	void MoveToNextPatrolPoint();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Set up Sensing Component that AI Guard will use to sense Player
	UPROPERTY(VisibleAnywhere, Category = "Components")
		UPawnSensingComponent* PawnSensingComp;

	UFUNCTION()
		void OnPawnSeen(APawn* SeenPawn);

	UFUNCTION()
	void DecrementAlertedState();
	APawn* SpottedPawn;

	UPROPERTY(BlueprintReadOnly)
	int32 AlertedState;


	FRotator OriginalRotation;
	FVector SavedLocation;
	FVector LastPlayerSeenLocation;
	//Boolean to track when AI is following a player to their last seen location
	bool bIsFollowing;
	//Boolean to track when the AI is searching a player's last seen location
	bool bIsSearching;
	//Float to track how long the AI has been searching for
	float SearchTime;

	UFUNCTION()
		void SearchSpottedLocation();

	UFUNCTION()
		void PlayerSpotted();

	UFUNCTION()
		void OnNoiseHeard(APawn* NoiseInstigator, const FVector& Location, float Volume);

	

	UFUNCTION()
	void ResetOrientation();

	UFUNCTION()
	void ResetLocation();
	FTimerHandle TimerHandle_Reset_Orientation;
	FTimerHandle TimerHandle_SpotTimer;

	UPROPERTY(ReplicatedUsing=OnRep_GuardState)
	EAIState GuardState;

	UFUNCTION()
	void OnRep_GuardState();


	UFUNCTION()
	void SetGuardState(EAIState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="AI")
	void OnStateChanged(EAIState NewState);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


};
