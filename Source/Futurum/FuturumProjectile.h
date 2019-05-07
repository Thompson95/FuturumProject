// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FuturumProjectile.generated.h"

UCLASS(config=Game)
class AFuturumProjectile : public AActor
{
	GENERATED_BODY()

	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	class USphereComponent* CollisionComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly)
	class UStaticMeshComponent* Mesh = nullptr;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	class UProjectileMovementComponent* ProjectileMovement = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UParticleSystem* Explosion = nullptr;

	UPROPERTY(VisibleAnywhere)
	class USoundBase* ExplosionSound = nullptr;

	UPROPERTY(EditAnywhere)
	float ExplosionRadius = 400.f;

public:
	AFuturumProjectile();

	/** called when projectile hits something */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	FORCEINLINE class USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ProjectileMovement subobject **/
	FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UDamageType> DamageType;
};

