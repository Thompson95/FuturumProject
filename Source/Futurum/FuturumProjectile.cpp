// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FuturumProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Interactable.h"
#include "ConstructorHelpers.h"
#include "Engine/Classes/Particles/ParticleSystemComponent.h"
#include "Classes/Sound/SoundCue.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"

AFuturumProjectile::AFuturumProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(1.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AFuturumProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	Mesh->SetWorldScale3D(FVector(0.3f, 0.3f, 0.3f));
	Mesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> MeshMaterial(TEXT("Material'/Game/StarterContent/Materials/M_Metal_Steel.M_Metal_Steel'"));
	if (MeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(MeshAsset.Object);
	}
	if (MeshMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, MeshMaterial.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SoundAsset(TEXT("SoundWave'/Game/StarterContent/Audio/Explosion01.Explosion01'"));
	if (SoundAsset.Succeeded())
	{
		ExplosionSound = SoundAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> ExplosionAsset(TEXT("ParticleSystem'/Game/StarterContent/Particles/P_Explosion.P_Explosion'"));
	Explosion = ExplosionAsset.Object;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 9000.f;
	ProjectileMovement->MaxSpeed = 9000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;

	SetReplicates(true);
}

void AFuturumProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics

	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL))
	{
		TArray<AActor*> IgnoreActors;
		if (Role == ROLE_Authority)
		{
			UGameplayStatics::ApplyRadialDamage(this, 10.f, GetActorLocation(), ExplosionRadius, DamageType, IgnoreActors, this, nullptr, false, ECollisionChannel::ECC_Visibility);

			TArray<FOverlapResult> Overlaps;
			FCollisionObjectQueryParams QueryParams;
			QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
			QueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel2);

			FCollisionShape ExplosionShape;
			ExplosionShape.SetSphere(ExplosionRadius);
			GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, ExplosionShape);

			for (FOverlapResult Hit : Overlaps)
			{
				UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>((Hit.GetActor())->GetRootComponent());

				if (MeshComponent)
				{
					MeshComponent->AddRadialImpulse(GetActorLocation(), ExplosionRadius, 90000.f, ERadialImpulseFalloff::RIF_Linear);
				}
			}
		}
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Explosion, GetActorLocation());
		UGameplayStatics::SpawnSoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation(), FRotator::ZeroRotator, 2.0f);

		Destroy();
	}



}