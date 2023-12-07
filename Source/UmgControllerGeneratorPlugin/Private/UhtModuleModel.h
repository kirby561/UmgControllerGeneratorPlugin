#pragma once

#include "CoreMinimal.h"
#include "UhtModuleModel.generated.h"

/**
 * This structure is meant to match each Model entry in the UHT Manifest file. 
 */
USTRUCT()
struct FUhtModuleModel {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;
    
    UPROPERTY()
    FString ModuleType;
    
    UPROPERTY()
    FString OverrideModuleType;
    
    UPROPERTY()
    FString BaseDirectory;
    
    UPROPERTY()
    FString IncludeBase;
    
    UPROPERTY()
    FString OutputDirectory;
    
    UPROPERTY()
    TArray<FString> ClassesHeaders;
    
    UPROPERTY()
    TArray<FString> PublicHeaders;
    
    UPROPERTY()
    TArray<FString> InternalHeaders;
    
    UPROPERTY()
    TArray<FString> PrivateHeaders;
    
    UPROPERTY()
    TArray<FString> PublicDefines;
    
    UPROPERTY()
    FString GeneratedCPPFilenameBase;
    
    UPROPERTY()
    bool SaveExportedHeaders = false;
    
    UPROPERTY()
    FString UHTGeneratedCodeVersion = TEXT("None");    
};
