// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TopDownARPGPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Engine/World.h"
#include "Characters/TopDownARPGCharacter.h"
#include "TopDownARPG.h"
#include "Engine/CollisionProfile.h"
#include "Utils/FlagActor.h"
#include "UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Triggers/WinTrigger.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ATopDownARPGPlayerController::ATopDownARPGPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	static ConstructorHelpers::FObjectFinder<UDataTable> ControllerStatsObject(TEXT("DataTable'/Game/TopDownCPP/Blueprints/DataTables/ControllerStats.ControllerStats'"));
	if (ControllerStatsObject.Succeeded())
	{
		ControllerConfig.DataTable = ControllerStatsObject.Object;
		ControllerConfig.RowName = FName(TEXT("DefaultController"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATopDownARPGPlayerController, Team);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ECTFTeam ATopDownARPGPlayerController::GetTeam() const
{
	return Team;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::SetTeam(ECTFTeam Team)
{
	this->Team = Team;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::ClientOnEndMatch_Implementation(bool IsVictory)
{
	if (IsVictory)
	{
		if (!IsValid(VictoryScreenInstance))
		{
			auto ControllerRow = ControllerConfig.GetRow<FTopDownARPGControllerStruct>(TEXT(""));
			auto VictoryScreenTemplate = ControllerRow ? ControllerRow->VictoryScreenTemplate : nullptr;
			if (!IsValid(VictoryScreenTemplate))
			{
				UE_LOG(LogTopDownARPG, Error, TEXT("No victory screen widget template!"));
				return;
			}

			VictoryScreenInstance = CreateWidget<UUserWidget>(this, VictoryScreenTemplate);
			if (!IsValid(VictoryScreenInstance))
			{
				UE_LOG(LogTopDownARPG, Error, TEXT("Could not instantiate victory screen!"));
				return;
			}
		}

		if (!VictoryScreenInstance->IsInViewport())
		{
			VictoryScreenInstance->AddToViewport();
		}
	}
	else
	{
		if (!IsValid(DefeatScreenInstance))
		{
			auto ControllerRow = ControllerConfig.GetRow<FTopDownARPGControllerStruct>(TEXT(""));
			auto DefeatScreenTemplate = ControllerRow ? ControllerRow->DefeatScreenTemplate : nullptr;
			if (!IsValid(DefeatScreenTemplate))
			{
				UE_LOG(LogTopDownARPG, Error, TEXT("No defeat screen widget template!"));
				return;
			}

			DefeatScreenInstance = CreateWidget<UUserWidget>(this, DefeatScreenTemplate);
			if (!IsValid(DefeatScreenInstance))
			{
				UE_LOG(LogTopDownARPG, Error, TEXT("Could not instantiate defeat screen!"));
				return;
			}
		}

		if (!DefeatScreenInstance->IsInViewport())
		{
			DefeatScreenInstance->AddToViewport();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::ClientToggleWinZone_Implementation(AActor* Trigger, bool bShow)
{
	auto WinTrigger = Cast<AWinTrigger>(Trigger);
	if (IsValid(WinTrigger))
	{
		WinTrigger->ToggleParticle(bShow);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ATopDownARPGPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &ATopDownARPGPlayerController::OnSetDestinationReleased);
	InputComponent->BindAction("StartCaptureFlag", IE_Pressed, this, &ATopDownARPGPlayerController::RequestStartCaptureFlag);

	BindAbilityActions();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::MoveToMouseCursor()
{
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if ((Distance > 120.0f))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
			auto PlayerCharacter = Cast<ATopDownARPGCharacter>(MyPawn);
			if (IsValid(PlayerCharacter) && PlayerCharacter->GetIsCurrentlyCapturing())
			{
				PlayerCharacter->EndCapturingFlag(false);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::RequestActivateAbility(int AbilityIndex)
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (!Hit.bBlockingHit)
	{
		return;
	}

	if (!CanActivateAbility(AbilityIndex))
	{
		UE_LOG(LogTopDownARPG, Log, TEXT("Cannot activate ability at index %d right now"), AbilityIndex);
		return;
	}

	ServerActivateAbility(AbilityIndex, Hit.ImpactPoint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ATopDownARPGPlayerController::CanActivateAbility(int AbilityIndex) const
{
	ATopDownARPGCharacter* PlayerCharacter = Cast<ATopDownARPGCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Cannot get valid character in player controller!"));
		return false;
	}

	if (PlayerCharacter->GetIsCurrentlyCapturing())
	{
		return false;
	}

	UAbility* Ability = PlayerCharacter->GetAbility(AbilityIndex);
	if (!IsValid(Ability))
	{
		return false;
	}

	return Ability->IsOffCooldown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ATopDownARPGPlayerController::ServerActivateAbility_Validate(int AbilityIndex, FVector AimLocation)
{
	return CanActivateAbility(AbilityIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::ServerActivateAbility_Implementation(int AbilityIndex, FVector AimLocation)
{
	ATopDownARPGCharacter* PlayerCharacter = Cast<ATopDownARPGCharacter>(GetPawn());
	UAbility* Ability = PlayerCharacter->GetAbility(AbilityIndex);
	Ability->Activate(AimLocation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::RequestStartCaptureFlag()
{
	FHitResult HitResult;
	bool IsFlagClicked = QueryForFlagClick(HitResult);
	if (!IsFlagClicked)
	{
		return;
	}

	auto FlagActor = Cast<AFlagActor>(HitResult.GetActor());

	if (!CanStartCaptureFlag(FlagActor))
	{
		return;
	}

	ServerStartCaptureFlag(FlagActor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ATopDownARPGPlayerController::CanStartCaptureFlag(AFlagActor* Flag) const
{
	ATopDownARPGCharacter* PlayerCharacter = Cast<ATopDownARPGCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter) || PlayerCharacter->GetIsCurrentlyCapturing())
	{
		return false;
	}

	if (Team == ECTFTeam::COUNT)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Controller with invalid team %d!"), (int)Team);
		return false;
	}
	
	return IsValid(Flag)
		&& Flag->GetTeam() != Team;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ATopDownARPGPlayerController::ServerStartCaptureFlag_Validate(AFlagActor* Flag)
{
	return CanStartCaptureFlag(Flag);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::ServerStartCaptureFlag_Implementation(AFlagActor* Flag)
{
	ATopDownARPGCharacter* PlayerCharacter = Cast<ATopDownARPGCharacter>(GetPawn());
	PlayerCharacter->StartCapturingFlag();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATopDownARPGPlayerController::BindAbilityActions()
{
	for (int i = 0; i < 2; i++)
	{
		FString ActionName("Ability");
		ActionName.AppendChar((char)(i + 1 + '0'));
		FInputActionBinding AbilityActionBinding(FName(*ActionName), IE_Pressed);
		
		FInputActionHandlerSignature AbilityHandler;
		AbilityHandler.BindUFunction(this, FName("RequestActivateAbility"), i);

		AbilityActionBinding.ActionDelegate = AbilityHandler;
		InputComponent->AddActionBinding(AbilityActionBinding);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ATopDownARPGPlayerController::QueryForFlagClick(FHitResult& OutHitResult) const
{
	TArray<TEnumAsByte<EObjectTypeQuery> > flagTypes;

	ECollisionChannel CollisionChannel;
	FCollisionResponseParams ResponseParams;
	if (UCollisionProfile::GetChannelAndResponseParams(FName(TEXT("Flag")), CollisionChannel, ResponseParams))
	{
		flagTypes.Add(UEngineTypes::ConvertToObjectType(CollisionChannel));
	}

	return GetHitResultUnderCursorForObjects(flagTypes, true, OutHitResult);
}
