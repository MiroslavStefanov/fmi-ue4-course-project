// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Utils/ECTFTeam.h"
#include "DataTables/TopDownARPGControllerStruct.h"
#include "TopDownARPGPlayerController.generated.h"

UCLASS()
class ATopDownARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATopDownARPGPlayerController();

	/** Property replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	ECTFTeam GetTeam() const;

	UFUNCTION()
	void SetTeam(ECTFTeam Team);

	UFUNCTION(Client, Reliable)
	void ClientOnEndMatch(bool IsVictory);

	UFUNCTION(Client, Reliable)
	void ClientToggleWinZone(AActor* WinTrigger, bool bShow);
protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();

protected:
	//Ability activation
	UFUNCTION()
	void RequestActivateAbility(int AbilityIndex);

	UFUNCTION()
	bool CanActivateAbility(int AbilityIndex) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerActivateAbility(int AbilityIndex, FVector AimLocation);

	//Flag capturing
	UFUNCTION()
	void RequestStartCaptureFlag();
	
	UFUNCTION()
	bool CanStartCaptureFlag(class AFlagActor* Flag) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartCaptureFlag(class AFlagActor* Flag);

protected:
	UPROPERTY(EditAnywhere)
	FDataTableRowHandle ControllerConfig;

	UUserWidget* VictoryScreenInstance = nullptr;
	UUserWidget* DefeatScreenInstance = nullptr;

private:
	void BindAbilityActions();
	bool QueryForFlagClick(FHitResult& OutHitResult) const;

private:
	UPROPERTY(VisibleAnywhere, Replicated)
	ECTFTeam Team = ECTFTeam::COUNT;
};


