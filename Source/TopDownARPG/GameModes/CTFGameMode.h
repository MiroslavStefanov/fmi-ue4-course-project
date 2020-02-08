// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Utils/FCTFTeam.h"
#include "UObject/SoftObjectPath.h"
#include "CTFGameMode.generated.h"

/**
 * 
 */
UCLASS()
class TOPDOWNARPG_API ACTFGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ACTFGameMode();

	UPROPERTY(EditDefaultsOnly)
	int CaptureDuraiton;
public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION()
	void OnFlagCaptured(ACharacter* Character);

	UFUNCTION()
	void OnCharactedDied(ACharacter* Character);

	UFUNCTION()
	void OnCharacterEnteredReturnZone(ACharacter* Character);

	UFUNCTION()
	bool IsFlagHolder(const APlayerController* PlayerController);

protected:
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual bool ReadyToEndMatch_Implementation() override;
	virtual void HandleMatchHasEnded() override;

protected:
	TMap<ECTFTeam, FCTFTeam> Teams;

private:
	void ChooseRandomTeamForPlayer(APlayerController* NewPlayer);
	void ScorePoint(ECTFTeam Team);
	void StartNextRound();

private:
	UPROPERTY(EditDefaultsOnly)
	int TeamSize;

	UPROPERTY(EditDefaultsOnly)
	int WinPoints;

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "PlayerStart"))
	FSoftObjectPath Team1PlayerStart;

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "PlayerStart"))
	FSoftObjectPath Team2PlayerStart;

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "FlagActor"))
	FSoftObjectPath Team1Flag;

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "FlagActor"))
	FSoftObjectPath Team2Flag;

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "WinTrigger"))
	FSoftObjectPath Team1WinTrigger;

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "WinTrigger"))
	FSoftObjectPath Team2WinTrigger;
};
