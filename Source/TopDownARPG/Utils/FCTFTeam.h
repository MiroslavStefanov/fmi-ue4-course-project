// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Utils/ECTFTeam.h"

/**
 * 
 */
class TOPDOWNARPG_API FCTFTeam
{
public:
	explicit FCTFTeam(ECTFTeam Team);
	~FCTFTeam();
private:
	ECTFTeam Team;

	TArray<class APlayerController*> Members;

	FSoftObjectPath* OwnFlag = nullptr;
	FSoftObjectPath* EnemyFlag = nullptr;
	FSoftObjectPath* SpawnPoint = nullptr;
	FSoftObjectPath* WinTrigger = nullptr;

	int Points = 0;

public:
	FORCEINLINE int GetMembersNum() const { return Members.Num(); }
	FORCEINLINE ECTFTeam GetTeam() const { return Team; }
	FORCEINLINE int GetPoints() const { return Points; }
	FORCEINLINE const TArray<class APlayerController*> GetMembers() const { return Members; }

	class AFlagActor* GetOwnFlag() const;
	class AFlagActor* GetEnemyFlag() const;
	class AActor* GetSpawnPoint() const;
	class AWinTrigger* GetWinTrigger() const;

	void AddMember(APlayerController* Member);
	void SetOwnFlag(FSoftObjectPath* FlagPath);
	void SetEnemyFlag(FSoftObjectPath* FlagPath);
	void SetSpawnPoint(FSoftObjectPath* SpawnPoint);
	void SetWinTrigger(FSoftObjectPath* WinTrigger);
	void ScorePoint();

	void EndMatch(bool IsVictory);
};
