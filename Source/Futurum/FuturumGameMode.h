// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EventDispatcher.h"
#include "FuturumGameMode.generated.h"

UCLASS(minimalapi)
class AFuturumGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFuturumGameMode();

	UPROPERTY()
	UEventDispatcher* EventDispatcher;

	virtual void StartPlay() override;

private:
	UFUNCTION()
	void SpawnEnemy();

	UFUNCTION()
	void SpawnEnemyWithLights();

	UFUNCTION()
	void SetLightsState(bool State);

	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<class ABallEnemy> BallEnemyClass;
};



