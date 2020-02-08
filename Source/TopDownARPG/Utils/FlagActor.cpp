// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagActor.h"
#include "GameFramework/Character.h"
#include "TopDownARPG.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealNetwork.h"

// Sets default values
AFlagActor::AFlagActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	bAlwaysRelevant = 1;
	CaptureRange = 100.f;
	Team = ECTFTeam::COUNT;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
	if (MeshComponent)
	{
		RootComponent = MeshComponent;
	}
}

void AFlagActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFlagActor, Holder);
}

bool AFlagActor::IsCaptured() const
{
	return IsValid(Holder);
}

ACharacter* AFlagActor::GetHolder() const
{
	return Holder;
}

void AFlagActor::SetHolder(ACharacter* Holder)
{
	this->Holder = Holder;

	if (IsValid(Holder))
	{
		auto SkeletalComp = Cast<USceneComponent>(Holder->FindComponentByClass(USkeletalMeshComponent::StaticClass()));
		if (!IsValid(SkeletalComp))
		{
			UE_LOG(LogTopDownARPG, Error, TEXT("Could not attach the flag to the actor!"));
			return;
		}

		SetActorEnableCollision(false);
		AttachToComponent(SkeletalComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(TEXT("flagSocket")));
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorEnableCollision(true);
		MulticastResetPosition();
	}
}

void AFlagActor::BeginPlay()
{
	InitialPosition = GetActorLocation();
	InitialOrientation = GetActorRotation();
}

// Called every frame
void AFlagActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFlagActor::MulticastResetPosition_Implementation()
{
	SetActorLocationAndRotation(InitialPosition, InitialOrientation);
}

