// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FuturumCharacter.h"
#include "FuturumProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h"
#include "Interactable.h"
#include "Net/UnrealNetwork.h"
#include "ConstructorHelpers.h"
#include "Classes/Animation/AnimBlueprint.h"

// for FXRMotionControllerBase::RightHandSourceId

#define Interactable ECC_GameTraceChannel2

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AFuturumCharacter

AFuturumCharacter::AFuturumCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("SkeletalMesh'/Game/FirstPerson/Character/Mesh/SK_Mannequin_Arms.SK_Mannequin_Arms'"));
	static ConstructorHelpers::FObjectFinder<UAnimBlueprint> Animation(TEXT("AnimBlueprint'/Game/FirstPerson/Animations/FirstPerson_AnimBP.FirstPerson_AnimBP'"));
	if (MeshAsset.Succeeded())
	{
		Mesh1P->SetSkeletalMesh(MeshAsset.Object);
	}
	if (Animation.Succeeded())
	{
		Mesh1P->SetAnimInstanceClass(Animation.Object->GeneratedClass);
	}
	Mesh1P->SetOnlyOwnerSee(false);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GunMesh(TEXT("SkeletalMesh'/Game/FirstPerson/FPWeapon/Mesh/SK_FPGun.SK_FPGun'"));
	if (GunMesh.Succeeded())
	{
		FP_Gun->SetSkeletalMesh(GunMesh.Object);
	}
	FP_Gun->SetOnlyOwnerSee(false);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));
	SpotLight->AttachTo(FirstPersonCameraComponent);
	SpotLight->SetRelativeRotation(FirstPersonCameraComponent->GetForwardVector().Rotation());
	SpotLight->SetInnerConeAngle(1.5f);
	SpotLight->SetIntensity(80000.f);
	SpotLight->SetOuterConeAngle(10.0f);
	SpotLight->SetAttenuationRadius(6400.0f);
	SpotLight->SetIsReplicated(true);

	ProjectileClass = AFuturumProjectile::StaticClass();

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireAnimationMontage(TEXT("AnimMontage'/Game/FirstPerson/Animations/FirstPersonFire_Montage.FirstPersonFire_Montage'"));
	if (FireAnimationMontage.Succeeded())
	{
		FireAnimation = FireAnimationMontage.Object;
	}
	//SetReplicates(true);
	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void AFuturumCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFuturumCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFuturumCharacter::OnFire);

	PlayerInputComponent->BindAction("Use", IE_Pressed, this, &AFuturumCharacter::OnUse);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AFuturumCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFuturumCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFuturumCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFuturumCharacter::LookUpAtRate);
}

void AFuturumCharacter::OnUse()
{
	if (Role < ROLE_Authority)
	{
		ServerOnUse();
		return;
	}

	FVector Location;
	FRotator Rotation;
	GetActorEyesViewPoint(Location, Rotation);

	FHitResult LineTraceHit;
	FCollisionQueryParams TraceParameters(FName(TEXT("")), false, this);
	GetWorld()->LineTraceSingleByChannel(
		OUT LineTraceHit,
		Location,
		Location + Rotation.Vector() * Reach,
		Interactable
	);
	
	AActor* ActorHit = LineTraceHit.GetActor();
	
	if (ActorHit)
	{
		bool bIsImplemented = ActorHit->GetClass()->ImplementsInterface(UInteractable::StaticClass());
		IInteractable* ReactingObject = Cast<IInteractable>(ActorHit);
		ReactingObject->Use();
	}
}

void AFuturumCharacter::OnFire()
{
	if (Role < ROLE_Authority)
	{
		ServerOnFire();
		return;
	}
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<AFuturumProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AFuturumCharacter::ServerOnFire_Implementation()
{
	OnFire();
}

bool AFuturumCharacter::ServerOnFire_Validate()
{
	return true;
}

void AFuturumCharacter::ServerOnUse_Implementation()
{
	OnUse();
}

bool AFuturumCharacter::ServerOnUse_Validate()
{
	return true;
}


void AFuturumCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value*MovementScale);
	}
}

void AFuturumCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value*MovementScale);
	}
}

void AFuturumCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFuturumCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}