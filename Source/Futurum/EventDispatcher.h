// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EventDispatcher.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSetLightsState, bool, State);

UCLASS()
class FUTURUM_API UEventDispatcher : public UObject
{
	GENERATED_BODY()
	
public:
	UEventDispatcher();

	UPROPERTY()
	FOnEnemyDestroyed OnEnemyDestroyed;

	UPROPERTY()
	FSetLightsState SetLightsState;
	
};
