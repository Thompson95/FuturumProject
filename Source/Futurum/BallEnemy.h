// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BallEnemy.generated.h"


UCLASS()
class FUTURUM_API ABallEnemy : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABallEnemy();

	UPROPERTY(EditAnywhere)
	class USphereComponent* SphereCollision = nullptr;

	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = AI)
	class UBehaviorTree* EnemyBehavior = nullptr;

	UPROPERTY(EditAnywhere)
	class UParticleSystemComponent* FireComponent = nullptr;

	UPROPERTY(EditAnywhere)
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Replicated)
	float CurrentHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category=FX)
	UParticleSystem* Explosion = nullptr;

	UPROPERTY(EditAnywhere)
	UParticleSystemComponent* SparksComponent = nullptr;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastDestroyObject();

	UFUNCTION()
	void DestroyObject();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MulticastPlayDamageEffects();
	
	UFUNCTION()
	void PlayDamageEffects();
};
