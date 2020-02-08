// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ECTFTeam.h"
#include "FlagActor.generated.h"

UCLASS()
class TOPDOWNARPG_API AFlagActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFlagActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	bool IsCaptured() const;
	UFUNCTION()
	class ACharacter* GetHolder() const;
	UFUNCTION()
	void SetHolder(class ACharacter* Holder);

	UFUNCTION()
	FORCEINLINE ECTFTeam GetTeam() { return Team; }

	UFUNCTION()
	FORCEINLINE float GetCaptureRange() { return CaptureRange; }

	FORCEINLINE class UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetPosition();

private:
	UPROPERTY(Replicated)
	class ACharacter* Holder = nullptr;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere)
	ECTFTeam Team;

	UPROPERTY(EditDefaultsOnly)
	float CaptureRange;

	FVector InitialPosition = FVector::ZeroVector;
	FRotator InitialOrientation = FRotator::ZeroRotator;
};
