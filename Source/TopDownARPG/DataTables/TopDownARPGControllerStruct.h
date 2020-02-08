#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "TopDownARPGControllerStruct.generated.h"

USTRUCT(BlueprintType)
struct FTopDownARPGControllerStruct : public FTableRowBase
{
	GENERATED_BODY()
		
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> VictoryScreenTemplate;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> DefeatScreenTemplate;
};