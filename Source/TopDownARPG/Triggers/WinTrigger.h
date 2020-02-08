// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utils/ECTFTeam.h"
#include "WinTrigger.generated.h"

UCLASS()
class TOPDOWNARPG_API AWinTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	AWinTrigger();

	virtual void BeginPlay() override;

	FORCEINLINE class USphereComponent* GetSphereComponent() const { return SphereComponent; }
	FORCEINLINE class UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	UFUNCTION()
	void ToggleParticle(bool bActive);

protected:
	UPROPERTY(VisibleAnywhere, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	class UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY(EditAnywhere)
	ECTFTeam Team;

	UPROPERTY(EditDefaultsOnly)
	float Radius;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
