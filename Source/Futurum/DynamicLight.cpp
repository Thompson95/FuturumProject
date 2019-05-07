// Fill out your copyright notice in the Description page of Project Settings.

#include "DynamicLight.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Classes/Components/StaticMeshComponent.h"
#include "ConstructorHelpers.h"
#include "EngineGlobals.h"
#include <Runtime/Engine/Classes/Engine/Engine.h>
#include "Classes/Particles/ParticleSystemComponent.h"
#include "FuturumGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#define Interactable ECC_GameTraceChannel2

// Sets default values
ADynamicLight::ADynamicLight()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Stationary);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->AttachTo(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LampAsset(TEXT("StaticMesh'/Game/SM_Lamp_Ceiling.SM_Lamp_Ceiling'"));
	if (LampAsset.Succeeded())
	{
		Mesh->SetStaticMesh(LampAsset.Object);
		Mesh->SetMobility(EComponentMobility::Stationary);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionProfileName(TEXT("Interactable"));
	}

	Sparks = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Sparks"));
	Sparks->AttachTo(Root);;
	static ConstructorHelpers::FObjectFinder<UParticleSystem> SparksAsset(TEXT("ParticleSystem'/Game/StarterContent/Particles/P_Sparks.P_Sparks'"));

	if (SparksAsset.Succeeded())
	{
		Sparks->SetTemplate(SparksAsset.Object);
		Sparks->SetRelativeLocation(FVector(0.f, 0.f, -130.f));
	}
	Sparks->SetVisibility(false);

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule collision"));
	CapsuleCollision->AttachTo(Root);
	CapsuleCollision->SetCollisionProfileName(TEXT("Interactable"));
	CapsuleCollision->SetRelativeLocation(FVector(0.f, 0.f, -70.f));
	CapsuleCollision->SetRelativeScale3D(FVector(1.25f, 1.25f, 2.5f));

	LightColor = FLinearColor::Red;
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->AttachTo(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, -135.f));
	Light->SetIntensity(40000.f);
	Light->SetLightColor(LightColor);
	Light->SetMobility(EComponentMobility::Stationary);
	Light->SetIndirectLightingIntensity(0.01f);
	Light->SetAttenuationRadius(1200.f);
	

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ADynamicLight::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		AFuturumGameMode* GameMode = (AFuturumGameMode*)GetWorld()->GetAuthGameMode();
		GameMode->EventDispatcher->OnEnemyDestroyed.AddDynamic(this, &ADynamicLight::TurnOff);
		GameMode->EventDispatcher->SetLightsState.AddDynamic(this, &ADynamicLight::MulticastSetState);
	}
}

// Called every frame
void ADynamicLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Role == ROLE_Authority)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABallEnemy::StaticClass(), FoundActors);
		FVector PlayerLocation = FoundActors.Num() != 0 ? FoundActors.Last(0)->GetActorLocation() : FVector::ZeroVector;
		FVector LampLocation = GetActorLocation();
		FVector RelativePosition = LampLocation - PlayerLocation;
		RelativePosition = RelativePosition.GetSafeNormal();
		float Angle = FMath::Atan2(RelativePosition.X, RelativePosition.Y) + PI;
		LightColor = FLinearColor(FMath::Cos(Angle) / 2 + 0.5f, FMath::Cos(Angle + 2 * PI / 3) / 2 + 0.5f, FMath::Cos(Angle - 2 * PI / 3) / 2 + 0.5f);
	}
	Light->SetLightColor(LightColor);
}

void ADynamicLight::Use()
{
	MulticastUse();
}

void ADynamicLight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADynamicLight, LightColor);
}


float ADynamicLight::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{	
	MulticastUse();
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ADynamicLight::MulticastUse_Implementation()
{
	ToggleVisibility();
}

bool ADynamicLight::MulticastUse_Validate()
{
	return true;
}

void ADynamicLight::MulticastSetState_Implementation(bool State)
{
	SetState(State);
}

bool ADynamicLight::MulticastSetState_Validate(bool State)
{
	return true;
}

void ADynamicLight::SetState(bool State)
{
	Light->SetVisibility(State);
	Sparks->SetVisibility(!State);
}

void ADynamicLight::TurnOff()
{
	MulticastSetState(false);
}

void ADynamicLight::ToggleVisibility()
{
	Light->ToggleVisibility();
	Sparks->ToggleVisibility();
}