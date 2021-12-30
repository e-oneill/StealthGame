// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSAIGuard.h"
#include "Perception/PawnSensingComponent.h"
#include "DrawDebugHelpers.h"
#include "FPSGameMode.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AFPSAIGuard::AFPSAIGuard()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComp"));

	PawnSensingComp->OnSeePawn.AddDynamic(this, &AFPSAIGuard::OnPawnSeen);
	PawnSensingComp->OnHearNoise.AddDynamic(this, &AFPSAIGuard::OnNoiseHeard);

	GuardState = EAIState::Idle;
	AlertedState = 0;
	bReplicates = true;
	SearchTime = 0.f;
	//SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void AFPSAIGuard::BeginPlay()
{
	Super::BeginPlay();

	OriginalRotation = GetActorRotation();
	SavedLocation = GetActorLocation();
	if (bIsPatrol)
	{
		CurrentPatrolPoint = -1;
		MoveToNextPatrolPoint();
	}
	
}

void AFPSAIGuard::OnNoiseHeard(APawn* NoiseInstigator, const FVector& Location, float Volume)
{
	if (HasAuthority())
	{
		if (GuardState == EAIState::Alerted)
		{
			return;
		}
		DrawDebugSphere(GetWorld(), Location, 32.0f, 12, FColor::Green, false, 10.0f);
		FVector Direction = Location - GetActorLocation();
		//OriginalRotation = GetActorRotation();
		Direction.Normalize();
		FRotator NewLookAt = FRotationMatrix::MakeFromX(Direction).Rotator();
		NewLookAt.Pitch = 0.0f;
		NewLookAt.Roll = 0.0f;

		SetActorRotation(NewLookAt);

		GetWorldTimerManager().ClearTimer(TimerHandle_Reset_Orientation);
		GetWorldTimerManager().SetTimer(TimerHandle_Reset_Orientation, this, &AFPSAIGuard::ResetLocation, 3.0f);

		SetGuardState(EAIState::Suspicious);

		AController* PawnController = GetController();
		if (PawnController)
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(PawnController, Location);
			//PawnController->StopMovement();
		}
	}

}


//This function is called when a Player Spawn is seen. This will increment the AIGuard's Alert Variable
void AFPSAIGuard::OnPawnSeen(APawn* SeenPawn)
{
	if (SeenPawn == nullptr || GuardState == EAIState::Alerted)
	{

		return;
	}

	LastPlayerSeenLocation = SeenPawn->GetActorLocation();
	//SavedLocation = GetActorLocation();

	DrawDebugSphere(GetWorld(), LastPlayerSeenLocation, 32.0f, 12, FColor::Red, false, 10.0f);
	SpottedPawn = SeenPawn;
	
	
	AController* PawnController = GetController();
	if (PawnController)
	{
		bIsFollowing = true;
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(PawnController, LastPlayerSeenLocation);
		//PawnController->StopMovement();
	}

	SetGuardState(EAIState::Suspicious);

	AlertedState = AlertedState + 10;
	UE_LOG(LogTemp, Warning, TEXT("Player Spotted, Alerted State is now %d"), AlertedState);

	//GetWorldTimerManager().ClearTimer(TimerHandle_Reset_Orientation);
	//GetWorldTimerManager().SetTimer(TimerHandle_Reset_Orientation, this, &AFPSAIGuard::ResetLocation, 3.0f);

}

void AFPSAIGuard::DecrementAlertedState()
{
	AlertedState = AlertedState - 10;
	UE_LOG(LogTemp, Warning, TEXT("Timer Callback executed, decrementing Alerted State to: %d"), AlertedState);
}

//This isn't yet functional - want to get AI to search random points in the area after a player is spotted
void AFPSAIGuard::SearchSpottedLocation()
{
	UE_LOG(LogTemp, Log, TEXT("Starting Search of random locations in Search Area"));

	UNavigationSystemV1* NavArea = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	FNavLocation targetPos = FNavLocation(LastPlayerSeenLocation);
	if (NavArea)
	{
		if (NavArea->GetRandomReachablePointInRadius(LastPlayerSeenLocation, 30.0f, targetPos))
		{
			UE_LOG(LogTemp, Log, TEXT("Random location found"));
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), targetPos.Location);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Random Location Failed!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Couldn't get Nav Area"));
	}
}

void AFPSAIGuard::PlayerSpotted()
{
	AFPSGameMode* GM = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode());
	if (GM && SpottedPawn)
	{
		GM->CompleteMission(SpottedPawn, false);
	}

	SetGuardState(EAIState::Alerted);
}


void AFPSAIGuard::ResetOrientation()
{
	if (GuardState == EAIState::Alerted)
	{
		return;
	}
	SetActorRotation(OriginalRotation);

	SetGuardState(EAIState::Idle);

	if (bIsPatrol)
	{
		MoveToNextPatrolPoint();
	}
}

void AFPSAIGuard::ResetLocation()
{
	if (HasAuthority())
	{
		if (GuardState == EAIState::Alerted)
		{
			return;
		}
		AController* PawnController = GetController();
		if (PawnController)
		{
			UE_LOG(LogTemp, Warning, TEXT("Returning AI to Saved Location"));
			if (bIsPatrol)
			{
				UE_LOG(LogTemp, Warning, TEXT("AI returning to patrol route"));
				MoveToNextPatrolPoint();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AI is not part of patrol, returning to Saved Location"));
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), SavedLocation);
			}

			//PawnController->StopMovement();
		}
		SetGuardState(EAIState::Idle);
	}
	
}



void AFPSAIGuard::SetGuardState(EAIState NewState)
{
	if (GuardState == NewState)
	{
		return;
	}
	
	GuardState = NewState;
	OnRep_GuardState();

	//UE_LOG(LogTemp, Warning, TEXT("State Changed"));
	OnStateChanged(GuardState);
}

//Replication function called on the Client
void AFPSAIGuard::OnRep_GuardState()
{
	OnStateChanged(GuardState);
}

// Called every frame
void AFPSAIGuard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//This block of code will tell an AI that is Idle to move to the next patrol point
	if (HasAuthority())
	{
		if (bIsPatrol && GuardState == EAIState::Idle)
		{
			FVector Delta = GetActorLocation() - PatrolPoints[CurrentPatrolPoint]->GetActorLocation();
			float DistanceToGoal = Delta.Size();
			//UE_LOG(LogTemp, Warning, TEXT("Character has Patrol Point, Distance To Goal is %f"), DistanceToGoal);
			if (DistanceToGoal < 100)
			{
				MoveToNextPatrolPoint();
			}
		}
		//This block will decrement the alert level of an AI that is Idle
		if (GuardState == EAIState::Idle && AlertedState > 0)
		{
			float AlertDecrement = 10;
			float NewAlertedState = AlertedState - (AlertDecrement * DeltaTime * 1.0f);
			(NewAlertedState > 0) ? AlertedState = NewAlertedState : AlertedState = 0;
			UE_LOG(LogTemp, Warning, TEXT("Timer Callback executed, decrementing Alerted State to: %d"), AlertedState);
		}
		if (bIsFollowing)
		{
			FVector Delta = GetActorLocation() - LastPlayerSeenLocation;
			float DistanceToGoal = Delta.Size();
			//UE_LOG(LogTemp, Warning, TEXT("Character has Patrol Point, Distance To Goal is %f"), DistanceToGoal);
			if (DistanceToGoal < 100)
			{
				bIsSearching = true;
				bIsFollowing = false;
				SearchSpottedLocation();
			}
		}
		if (bIsSearching)
		{
			SearchTime = SearchTime + (1.0f * DeltaTime);
			if (SearchTime > 10.f)
			{
				bIsSearching = false;
				SearchTime = 0.f;
				ResetLocation();
			}
			else {
				//SearchSpottedLocation();
			}
			
		}

		//If the AlertedState Reaches 100, the player has been spotted.
		if (AlertedState >= 100)
		{
			PlayerSpotted();
		}
	}


}


void AFPSAIGuard::MoveToNextPatrolPoint()
{
	if (HasAuthority())
	{
		if (CurrentPatrolPoint == PatrolPoints.Num() - 1)
		{
			CurrentPatrolPoint = 0;
		}
		else
		{
			CurrentPatrolPoint++;
		}

		AActor* Target = PatrolPoints[CurrentPatrolPoint];

		UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), Target);
	}
}

void AFPSAIGuard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSAIGuard, GuardState);
}


