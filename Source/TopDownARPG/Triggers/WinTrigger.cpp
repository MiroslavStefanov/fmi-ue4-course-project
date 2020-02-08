// Fill out your copyright notice in the Description page of Project Settings.


#include "WinTrigger.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameModes/CTFGameMode.h"
#include "Characters/TopDownARPGCharacter.h"
#include "Controllers/TopDownARPGPlayerController.h"
#include "GameModes/CTFGameMode.h"
#include "UnrealNetwork.h"

AWinTrigger::AWinTrigger()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RootComponent = SphereComponent;
	SphereComponent->SetVisibility(false);

	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
	ParticleSystemComponent->bAutoActivate = false;

	SphereComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AWinTrigger::OnOverlap);

	Radius = 200.f;
	Team = ECTFTeam::COUNT;
}


void AWinTrigger::BeginPlay()
{
	SphereComponent->SetSphereRadius(Radius, false);

	FParticleSysParam RadiusParam;
	RadiusParam.Name = TEXT("Radius");
	RadiusParam.Vector = FVector(Radius, Radius, Radius);
	RadiusParam.ParamType = PSPT_Vector;
	ParticleSystemComponent->InstanceParameters.Add(std::move(RadiusParam));
}

void AWinTrigger::ToggleParticle(bool bActive)
{
	if (ParticleSystemComponent)
	{
		if (bActive)
		{
			ParticleSystemComponent->Activate();
		}
		else
		{
			ParticleSystemComponent->Deactivate();
		}
	}
}

void AWinTrigger::OnOverlap(UPrimitiveComponent * OverlappedComp, AActor * Other, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (Role == ROLE_Authority)
	{
		auto PlayerCharacter = Cast<ACharacter>(Other);
		auto PlayerController = IsValid(PlayerCharacter) ? Cast<ATopDownARPGPlayerController>(PlayerCharacter->GetController()) : nullptr;
		if (IsValid(PlayerController) && PlayerController->GetTeam() == Team)
		{
			auto GameMode = Cast<ACTFGameMode>(GetWorld()->GetAuthGameMode());
			if (IsValid(GameMode))
			{
				GameMode->OnCharacterEnteredReturnZone(PlayerCharacter);
			}
		}
	}
}
