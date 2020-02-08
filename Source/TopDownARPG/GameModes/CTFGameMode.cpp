// Fill out your copyright notice in the Description page of Project Settings.


#include "CTFGameMode.h"
#include "TopDownARPG.h"
#include "Controllers/TopDownARPGPlayerController.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/FlagActor.h"
#include "Triggers/WinTrigger.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ACTFGameMode::ACTFGameMode()
{
	for (int teamInt = 0; teamInt < (int)ECTFTeam::COUNT; ++teamInt)
	{
		const ECTFTeam team = (ECTFTeam)teamInt;
		Teams.Add(team, FCTFTeam(team));
	}

	// use our custom PlayerController class
	PlayerControllerClass = ATopDownARPGPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	Teams[ECTFTeam::Team1].SetOwnFlag(&Team1Flag);
	Teams[ECTFTeam::Team1].SetEnemyFlag(&Team2Flag);
	Teams[ECTFTeam::Team1].SetSpawnPoint(&Team1PlayerStart);
	Teams[ECTFTeam::Team1].SetWinTrigger(&Team1WinTrigger);

	Teams[ECTFTeam::Team2].SetOwnFlag(&Team2Flag);
	Teams[ECTFTeam::Team2].SetEnemyFlag(&Team1Flag);
	Teams[ECTFTeam::Team2].SetSpawnPoint(&Team2PlayerStart);
	Teams[ECTFTeam::Team2].SetWinTrigger(&Team2WinTrigger);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::PostLogin(APlayerController* NewPlayer)
{
	ChooseRandomTeamForPlayer(NewPlayer);

	auto PlayerController = Cast<ATopDownARPGPlayerController>(NewPlayer);
	auto PlayerTeam = IsValid(PlayerController) ? PlayerController->GetTeam() : ECTFTeam::COUNT;

	if (PlayerTeam == ECTFTeam::COUNT)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Could not assign team to player!"));
		return;
	}

	RestartPlayerAtPlayerStart(NewPlayer, Teams[PlayerTeam].GetSpawnPoint());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//AActor* ACTFGameMode::ChoosePlayerStart_Implementation(AController* Player)
//{
//	auto PlayerController = Cast<ATopDownARPGPlayerController>(Player);
//	check(PlayerController);
//
//	auto PlayerTeam = PlayerController->GetTeam();
//	if (PlayerTeam != ECTFTeam::COUNT)
//	{
//		return Teams[PlayerTeam].GetSpawnPoint();
//	}
//
//	UE_LOG(LogTopDownARPG, Error, TEXT("Choosing spawn point for player with no team!"));
//	return Super::ChoosePlayerStart_Implementation(Player);
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ACTFGameMode::ReadyToStartMatch_Implementation()
{
	Super::ReadyToStartMatch_Implementation();

	for (auto& Team : Teams)
	{
		if (Team.Value.GetMembersNum() < TeamSize)
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ACTFGameMode::ReadyToEndMatch_Implementation()
{
	for (auto& Team : Teams)
	{
		if (Team.Value.GetPoints() >= WinPoints)
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::HandleMatchHasEnded()
{
	for (auto& Team : Teams)
	{
		Team.Value.EndMatch(Team.Value.GetPoints() == WinPoints);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::OnFlagCaptured(ACharacter* Character)
{
	auto PlayerController = Cast<ATopDownARPGPlayerController>(Character->GetController());
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Flag captured by invalid controller!"));
		return;
	}

	auto PlayerTeam = PlayerController->GetTeam();
	if (PlayerTeam == ECTFTeam::COUNT)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Flag captured by controller with invalid team %d!"), (int)PlayerTeam);
		return;
	}

	auto CapturedFlag = Teams[PlayerTeam].GetEnemyFlag();
	if (!IsValid(CapturedFlag))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Invalid flag captured!"));
		return;
	}

	if (!CapturedFlag->IsCaptured())
	{
		CapturedFlag->SetHolder(Character);
		auto WinTrigger = Teams[PlayerTeam].GetWinTrigger();
		if (IsValid(WinTrigger))
		{
			PlayerController->ClientToggleWinZone(WinTrigger, true);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::OnCharactedDied(ACharacter* Character)
{
	auto PlayerController = Cast<ATopDownARPGPlayerController>(Character->GetController());
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Character that died has invalid controller!"));
		return;
	}

	auto PlayerTeam = PlayerController->GetTeam();
	if (PlayerTeam == ECTFTeam::COUNT)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Character that died has invalid team!"));
		return;
	}

	auto CapturedFlag = Teams[PlayerTeam].GetEnemyFlag();
	if (!IsValid(CapturedFlag))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Invalid flag!"));
		return;
	}

	if (CapturedFlag->GetHolder() == Character)
	{
		CapturedFlag->SetHolder(nullptr); // Relese flag
		auto WinTrigger = Teams[PlayerTeam].GetWinTrigger();
		if (IsValid(WinTrigger))
		{
			PlayerController->ClientToggleWinZone(WinTrigger, false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::OnCharacterEnteredReturnZone(ACharacter* Character)
{
	auto PlayerController = Cast<ATopDownARPGPlayerController>(Character->GetController());
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Character that died has invalid controller!"));
		return;
	}

	auto PlayerTeam = PlayerController->GetTeam();
	if (PlayerTeam == ECTFTeam::COUNT)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Character that died has invalid team!"));
		return;
	}

	auto CapturedFlag = Teams[PlayerTeam].GetEnemyFlag();
	if (!IsValid(CapturedFlag))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Invalid flag!"));
		return;
	}

	if (CapturedFlag->GetHolder() == Character)
	{
		auto& Team = Teams[PlayerTeam];
		auto TeamFlag = Team.GetOwnFlag();
		if (IsValid(TeamFlag) && !TeamFlag->IsCaptured())
		{
			ScorePoint(PlayerTeam);
		}
		auto WinTrigger = Teams[PlayerTeam].GetWinTrigger();
		if (IsValid(WinTrigger))
		{
			PlayerController->ClientToggleWinZone(WinTrigger, false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ACTFGameMode::IsFlagHolder(const APlayerController* Controller)
{
	auto PlayerController = Cast<ATopDownARPGPlayerController>(Controller);
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Character that died has invalid controller!"));
		return false;
	}

	auto PlayerTeam = PlayerController->GetTeam();
	if (PlayerTeam == ECTFTeam::COUNT)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Character that died has invalid team!"));
		return false;
	}

	auto CapturedFlag = Teams[PlayerTeam].GetEnemyFlag();
	if (!IsValid(CapturedFlag))
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Invalid flag!"));
		return false;
	}

	return CapturedFlag->GetHolder() == Controller->GetPawn();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::ChooseRandomTeamForPlayer(APlayerController* NewPlayer)
{
	auto& Team1 = Teams[ECTFTeam::Team1];
	auto& Team2 = Teams[ECTFTeam::Team2];

	auto Team1Size = Team1.GetMembersNum();
	auto Team2Size = Team2.GetMembersNum();

	if (Team1Size >= TeamSize && Team2Size > TeamSize)
	{
		UE_LOG(LogTopDownARPG, Error, TEXT("Teams are already full!"));
		return;
	}

	if (Team1Size == TeamSize)
	{
		Team2.AddMember(NewPlayer);
	}
	else if (Team2Size == TeamSize)
	{
		Team1.AddMember(NewPlayer);
	}
	else
	{
		if (rand() % 2 == 0)
		{
			Team1.AddMember(NewPlayer);
		}
		else
		{
			Team2.AddMember(NewPlayer);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ACTFGameMode::ScorePoint(ECTFTeam PlayerTeam)
{
	auto& ScoreTeam = Teams[PlayerTeam];
	ScoreTeam.ScorePoint();
	StartNextRound();
}

void ACTFGameMode::StartNextRound()
{
	for (auto& Team : Teams)
	{
		for (auto Player : Team.Value.GetMembers())
		{
			RestartPlayerAtPlayerStart(Player, Team.Value.GetSpawnPoint());
		}

		auto Flag = Team.Value.GetOwnFlag();
		check(Flag);
		Flag->SetHolder(nullptr);
	}
}
