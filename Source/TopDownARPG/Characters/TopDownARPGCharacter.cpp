// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TopDownARPGCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "TopDownARPG.h"
#include "UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameModes/CTFGameMode.h"
#include "Abilities/Ability.h"
#include "Utils/FCTFTeam.h"
#include "TimerManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ATopDownARPGCharacter::ATopDownARPGCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->RelativeRotation = FRotator(-60.f, 0.f, 0.f);
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Create particle system
	OnCaptureParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
	OnCaptureParticle->bAutoActivate = false;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UDataTable> PlayerStatsObject(TEXT("DataTable'/Game/TopDownCPP/Blueprints/DataTables/CharacterStats.CharacterStats'"));
	if (PlayerStatsObject.Succeeded())
	{
		CharacterConfig.DataTable = PlayerStatsObject.Object;
	}

	OnTakeAnyDamage.AddDynamic(this, &ATopDownARPGCharacter::TakeAnyDamage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::BeginPlay()
{
	Super::BeginPlay();

	FTopDownARPGCharacterStruct* CharacterStruct = CharacterConfig.GetRow<FTopDownARPGCharacterStruct>(TEXT(""));

	if (CharacterStruct == nullptr)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("ATopDownARPGCharacter::BeginPlay CharacterStruct != nullptr"));
		return;
	}

	Health = CharacterStruct->MaximumHealth;
	GetCharacterMovement()->MaxWalkSpeed = CharacterStruct->MaximumWalkingSpeed;

	for (const TSubclassOf<UAbility>Template : CharacterStruct->AbilityTemplates)
	{
		AbilityInstances.Add(NewObject<UAbility>(this, Template));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::BeginDestroy()
{
	if (IsCurrentlyCapturing && TimerManager)
	{
		TimerManager->ClearTimer(CaptureTimerHandle);

		IsCurrentlyCapturing = false;
	}

	Super::BeginDestroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UAbility* ATopDownARPGCharacter::GetAbility(int Index)
{
	if (Index < 0 || Index >= AbilityInstances.Num())
	{
		UE_LOG(LogTopDownARPG, Warning, TEXT("Trying to access character ability at invalid index %d"), Index);
		return nullptr;
	}

	return AbilityInstances[Index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::StartCapturingFlag()
{
	if (Role != ROLE_Authority)
	{
		UE_LOG(LogTopDownARPG, Warning, TEXT("Calling ATopDownARPGCharacter::StartCapturingFlag not on the server!"));
		return;
	}

	UWorld* World = GetWorld();
	check(World);

	auto GameMode = Cast<ACTFGameMode>(World->GetAuthGameMode());
	check(GameMode);

	TimerManager = &World->GetTimerManager();

	FTimerDelegate StopCapturingDelegate;
	StopCapturingDelegate.BindUObject(this, &ATopDownARPGCharacter::EndCapturingFlag, true);
	TimerManager->SetTimer(CaptureTimerHandle, StopCapturingDelegate, GameMode->CaptureDuraiton, false);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bIsActive = false;

	IsCurrentlyCapturing = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::EndCapturingFlag(bool IsSuccessful)
{
	if (Role != ROLE_Authority)
	{
		UE_LOG(LogTopDownARPG, Warning, TEXT("Calling ATopDownARPGCharacter::EndCapturingFlag not on the server!"));
		return;
	}

	IsCurrentlyCapturing = false;

	if (IsSuccessful)
	{
		auto World = GetWorld();
		check(World);

		auto GameMode = Cast<ACTFGameMode>(World->GetAuthGameMode());
		check(GameMode);

		GameMode->OnFlagCaptured(this);
	}
	else if(TimerManager)
	{
		TimerManager->ClearTimer(CaptureTimerHandle);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATopDownARPGCharacter, Health);
	DOREPLIFETIME(ATopDownARPGCharacter, IsCurrentlyCapturing);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (CursorToWorld != nullptr)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::TakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigateBy, AActor* DamageCauser)
{
	UE_LOG(LogTopDownARPG, Display, TEXT("ATopDownARPGCharacter::TakeAnyDamage current health = %f"), (Health - Damage));
	Health -= Damage;
	if (Health <= 0.0f)
	{
		Death();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::OnRep_Capturing()
{
	if (Health > 0 && !IsCurrentlyCapturing)
	{
		OnCaptureParticle->ActivateSystem();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::Death()
{
	EnableMovement(false);

	if (IsCurrentlyCapturing)
	{
		EndCapturingFlag(false);
	}

	auto World = GetWorld();
	check(World);

	auto GameMode = Cast<ACTFGameMode>(World->GetAuthGameMode());
	check(GameMode);

	GameMode->OnCharactedDied(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGCharacter::EnableMovement(bool IsEnabled)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	check(Movement);

	FTopDownARPGCharacterStruct* CharacterStruct = CharacterConfig.GetRow<FTopDownARPGCharacterStruct>(TEXT(""));
	if (!CharacterStruct)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("ATopDownARPGCharacter::EnableMovement CharacterStruct is nullptr"));
		return;
	}

	if (IsEnabled)
	{
		Movement->MaxWalkSpeed = CharacterStruct->MaximumWalkingSpeed;
		Movement->bOrientRotationToMovement = true;
	}
	else
	{
		Movement->MaxWalkSpeed = 0.0f;
		Movement->bOrientRotationToMovement = false;
	}
}
