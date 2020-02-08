// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "DataTables/TopDownARPGCharacterStruct.h"
#include "Utils/ECTFTeam.h"
#include "TopDownARPGCharacter.generated.h" 

UCLASS(Blueprintable)
class ATopDownARPGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATopDownARPGCharacter();

	/** Property replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }
	/** Returns UParticleSystemComponent */
	FORCEINLINE class UParticleSystemComponent* GetOnCaptureParticle() { return OnCaptureParticle; }

	FORCEINLINE float GetHealth() const { return Health; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsCurrentlyCapturing() const{ return IsCurrentlyCapturing; }

	UFUNCTION()
	UAbility* GetAbility(int Index);

	UFUNCTION()
	void StartCapturingFlag();

	UFUNCTION()
	void EndCapturingFlag(bool IsSuccessful);

private:
	////////////////////////////////////////
	///////////////COMPONENTS///////////////
	////////////////////////////////////////
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

	UPROPERTY(VisibleAnywhere, Category = Visual)
	class UParticleSystemComponent* OnCaptureParticle;

	////////////////////////////////////////
	//////////////CONFIGURATION/////////////
	////////////////////////////////////////
	UPROPERTY(EditAnywhere)
	FDataTableRowHandle CharacterConfig;

	////////////////////////////////////////
	/////////////////RUNTIME////////////////
	////////////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY()
	TArray<UAbility*> AbilityInstances;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Capturing, meta = (AllowPrivateAccess = "true"))
	bool IsCurrentlyCapturing;

	FTimerManager* TimerManager = nullptr;
	FTimerHandle CaptureTimerHandle;

private:
	UFUNCTION()
	void TakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigateBy, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_Capturing();

	void Death();
	void EnableMovement(bool IsEnabled);
};

