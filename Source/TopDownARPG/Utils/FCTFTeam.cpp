// Fill out your copyright notice in the Description page of Project Settings.


#include "FCTFTeam.h"
#include "UObject/Object.h"
#include "GameFramework/PlayerController.h"
#include "Controllers/TopDownARPGPlayerController.h"
#include "Utils/FlagActor.h"
#include "Triggers/WinTrigger.h"
#include "TopDownARPG.h"

FCTFTeam::FCTFTeam(ECTFTeam Team) : Team(Team)
{
}

FCTFTeam::~FCTFTeam()
{
}

AFlagActor* FCTFTeam::GetOwnFlag() const
{
	if (!OwnFlag)
	{
		return nullptr;
	}

	return Cast<AFlagActor>(OwnFlag->ResolveObject());
}

AFlagActor* FCTFTeam::GetEnemyFlag() const
{
	if (!EnemyFlag)
	{
		return nullptr;
	}

	return Cast<AFlagActor>(EnemyFlag->ResolveObject());
}

AActor* FCTFTeam::GetSpawnPoint() const
{
	if (!SpawnPoint)
	{
		return nullptr;
	}
	auto Point = SpawnPoint->ResolveObject();
	return Cast<AActor>(Point);
}

AWinTrigger* FCTFTeam::GetWinTrigger() const
{
	if (!WinTrigger)
	{
		return nullptr;
	}
	return Cast<AWinTrigger>(WinTrigger->ResolveObject());
}

void FCTFTeam::AddMember(APlayerController* Member)
{
	auto Controller = Cast<ATopDownARPGPlayerController>(Member);
	if (IsValid(Controller))
	{
		Members.Add(Controller);
		Controller->SetTeam(Team);
	}
	else
	{
		UE_LOG(LogTopDownARPG, Warning, TEXT("Trying to add invalid player to a team!"));
	}
}

void FCTFTeam::SetOwnFlag(FSoftObjectPath* FlagPath)
{
	OwnFlag = FlagPath;
}

void FCTFTeam::SetEnemyFlag(FSoftObjectPath* FlagPath)
{
	EnemyFlag = FlagPath;
}

void FCTFTeam::SetSpawnPoint(FSoftObjectPath* SpawnPoint)
{
	this->SpawnPoint = SpawnPoint;
}

void FCTFTeam::SetWinTrigger(FSoftObjectPath* WinTrigger)
{
	this->WinTrigger = WinTrigger;
}

void FCTFTeam::ScorePoint()
{
	Points++;
}

void FCTFTeam::EndMatch(bool IsVictory)
{
	for (auto Member : Members)
	{
		auto Controller = Cast<ATopDownARPGPlayerController>(Member);
		if (IsValid(Controller))
		{
			Controller->ClientOnEndMatch(IsVictory);
		}
	}
}
