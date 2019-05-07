// Fill out your copyright notice in the Description page of Project Settings.

#include "BallEnemy.h"
#include "ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Classes/Materials/MaterialInstanceDynamic.h"
#include "Classes/Particles/ParticleSystemComponent.h"
#include "Futurum/FuturumProjectile.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "FuturumGameMode.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ABallEnemy::ABallEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	//Root->SetWorldScale3D(FVector(0.75f, 0.75f, 0.75f));
	//SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere collision"));
	//SphereCollision->SetupAttachment(Root);
	//SphereCollision->SetSphereRadius(50.f);
	//SphereCollision->SetSimulatePhysics(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> MeshMaterial(TEXT("Material'/Game/StarterContent/Materials/M_Rock_Basalt.M_Rock_Basalt'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FireAsset(TEXT("ParticleSystem'/Game/StarterContent/Particles/P_Fire.P_Fire'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> ExplosionAsset(TEXT("ParticleSystem'/Game/StarterContent/Particles/P_ExplosionElectric.P_ExplosionElectric'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> SparksAsset(TEXT("ParticleSystem'/Game/StarterContent/Particles/P_SparksEnemy.P_SparksEnemy'"));

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static mesh"));
	RootComponent = StaticMesh;
	if (MeshAsset.Succeeded())
	{
		StaticMesh->SetStaticMesh(MeshAsset.Object);
	}
	StaticMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	StaticMesh->SetSimulatePhysics(true);
	StaticMesh->SetMassOverrideInKg(NAME_None, 60.f, true);
	StaticMesh->SetEnableGravity(false);

	if (MeshMaterial.Succeeded())
	{
		StaticMesh->SetMaterial(0, MeshMaterial.Object);
	}
	
	if (ExplosionAsset.Succeeded())
	{
		Explosion = ExplosionAsset.Object;
	}

	FireComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Fire"));
	FireComponent->SetupAttachment(RootComponent);
	FireComponent->BodyInstance.bLockRotation = true;
	FireComponent->BodyInstance.bLockXRotation = true;
	FireComponent->BodyInstance.bLockYRotation = true;
	FireComponent->BodyInstance.bLockZRotation = true;
	if (FireAsset.Succeeded())
	{
		FireComponent->SetTemplate(FireAsset.Object);
	}

	SparksComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Sparks"));
	SparksComponent->AttachTo(RootComponent);

	if (SparksAsset.Succeeded())
	{
		SparksComponent->SetTemplate(SparksAsset.Object);
	}
	SparksComponent->SetVisibility(false);

	CurrentHealth = MaxHealth;

	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicateMovement = true;
	bReplicates = true;
	SetReplicates(true);
}

// Called when the game starts or wh`en spawned
void ABallEnemy::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABallEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABallEnemy::DestroyObject()
{
	if (Role == ROLE_Authority)
	{
		AFuturumGameMode* GameMode = (AFuturumGameMode*)GetWorld()->GetAuthGameMode();
		GameMode->EventDispatcher->OnEnemyDestroyed.Broadcast();
	}

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Explosion, GetActorLocation());

	FireComponent->DeactivateSystem();
	SparksComponent->DeactivateSystem();
	FireComponent->SetVisibility(false);
	SparksComponent->SetVisibility(false);
	StaticMesh->SetEnableGravity(true);

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionShape ExplosionShape;
	ExplosionShape.SetSphere(5000.f);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, ExplosionShape);

	for (FOverlapResult Hit : Overlaps)
	{
		UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>((Hit.GetActor())->GetRootComponent());

		if (MeshComponent)
		{
			MeshComponent->AddRadialImpulse(GetActorLocation(), 5000.f, 90000.f, ERadialImpulseFalloff::RIF_Linear);
		}
	}
	Destroy();
}

void ABallEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABallEnemy, CurrentHealth);
}

float ABallEnemy::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		if (CurrentHealth <= 0.f)
			return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
		CurrentHealth -= Damage;
		if (CurrentHealth <= 0.f)
		{
			MulticastDestroyObject();
		}
		else if (CurrentHealth <= 40.f)
		{
			MulticastPlayDamageEffects();
		}
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ABallEnemy::PlayDamageEffects()
{
	if (!SparksComponent->IsVisible())
		SparksComponent->SetVisibility(true);
}

void ABallEnemy::MulticastDestroyObject_Implementation()
{
	DestroyObject();
}

bool ABallEnemy::MulticastDestroyObject_Validate()
{
	return true;
}

void ABallEnemy::MulticastPlayDamageEffects_Implementation()
{
	PlayDamageEffects();
}

bool ABallEnemy::MulticastPlayDamageEffects_Validate()
{
	return true;
}
