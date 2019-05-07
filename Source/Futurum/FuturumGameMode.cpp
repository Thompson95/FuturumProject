// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FuturumGameMode.h"
#include "FuturumHUD.h"
#include "FuturumCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "BallEnemy.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/Public/TimerManager.h"

AFuturumGameMode::AFuturumGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("Class'/Script/Futurum.FuturumCharacter'"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFuturumHUD::StaticClass();
	BallEnemyClass = ABallEnemy::StaticClass();
	
	EventDispatcher = CreateDefaultSubobject<UEventDispatcher>(TEXT("Event Dispatcher"));
	EventDispatcher->OnEnemyDestroyed.AddDynamic(this, &AFuturumGameMode::SpawnEnemyWithLights);
}

void AFuturumGameMode::StartPlay()
{
	Super::StartPlay();
	SpawnEnemy();
}

void AFuturumGameMode::SpawnEnemyWithLights()
{
	if (Role == ROLE_Authority)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			FVector StartLocation(FMath::RandRange(-1250.f, 1250.f), FMath::RandRange(-1250.f, 1250.f), 500.f);
			ABallEnemy* Enemy = World->SpawnActor<ABallEnemy>(BallEnemyClass, StartLocation, FRotator::ZeroRotator, ActorSpawnParams);

			FVector StartVelocity(FMath::RandRange(-1250.f, 1250.f), FMath::RandRange(-1250.f, 1250.f), FMath::RandRange(-1250.f, 1250.f));
			Enemy->StaticMesh->SetPhysicsLinearVelocity(StartVelocity);

			FTimerHandle LightsTimerHandle;
			FTimerDelegate LightsTimerDelegate;
			bool State = true;
			LightsTimerDelegate.BindUFunction(this, FName("SetLightsState"), State);
			World->GetTimerManager().SetTimer(LightsTimerHandle, LightsTimerDelegate, 3.f, false);
		}
	}
}

void AFuturumGameMode::SpawnEnemy()
{
	if (Role == ROLE_Authority)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			FVector StartLocation(FMath::RandRange(-1250.f, 1250.f), FMath::RandRange(-1250.f, 1250.f), 500.f);
			ABallEnemy* Enemy = World->SpawnActor<ABallEnemy>(BallEnemyClass, StartLocation, FRotator::ZeroRotator, ActorSpawnParams);

			FVector StartVelocity(FMath::RandRange(-1250.f, 1250.f), FMath::RandRange(-1250.f, 1250.f), FMath::RandRange(-1250.f, 1250.f));
			Enemy->StaticMesh->SetPhysicsLinearVelocity(StartVelocity);

			FTimerHandle LightsTimerHandle;
			FTimerDelegate LightsTimerDelegate;
			bool State = true;
			LightsTimerDelegate.BindUFunction(this, FName("SetLightsState"), State);
		}
	}
}

void AFuturumGameMode::SetLightsState(bool State)
{
	EventDispatcher->SetLightsState.Broadcast(State);
}
