#pragma once

#include "CoreMinimal.h"
#include "UhtModuleModel.h"
#include "UhtManifestModel.generated.h"

/**
 * This is a model to match the JSON structure of the UHT manifest file. 
 */
USTRUCT()
struct FUhtManifestModel {
    GENERATED_BODY()

    UPROPERTY()
    bool IsGameTarget = false;

    UPROPERTY()
    FString RootLocalPath = FString();

    UPROPERTY()
    FString TargetName = FString();

    UPROPERTY()
    TArray<FUhtModuleModel> Modules;
};
