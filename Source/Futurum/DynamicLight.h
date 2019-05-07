// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Classes/Components/PointLightComponent.h"
#include "Engine/Classes/Components/CapsuleComponent.h"
#include "Interactable.h"
#include "BallEnemy.h"
#include "DynamicLight.generated.h"

UCLASS()
class FUTURUM_API ADynamicLight : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADynamicLight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	USceneComponent* Root = nullptr;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Mesh = nullptr;
	
	UPROPERTY(EditAnywhere)
	UPointLightComponent* Light = nullptr;

	UPROPERTY(EditAnywhere)
	UCapsuleComponent* CapsuleCollision = nullptr;
	
	UPROPERTY(EditAnywhere)
	UParticleSystemComponent* Sparks = nullptr;

	UPROPERTY(VisibleAnywhere, Replicated)
	FLinearColor LightColor;

	virtual void Use() override;

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastUse();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastSetState(bool State);

	UFUNCTION()
	void TurnOff();

	UFUNCTION()
	void SetState(bool State);

	UFUNCTION()
	void ToggleVisibility();

};
