// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UmgControllerGeneratorPluginBPLibrary.generated.h"

/**
 * Contains the entry methods for this plugin.	
 */
UCLASS()
class UUmgControllerGeneratorPluginBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create UMG Controller", Keywords = "UmgControllerGeneratorPlugin create umg controller"), Category = "UmgControllerGeneratorPlugin")
	static void CreateUmgController(UObject* inputBlueprint, FString headerPath, FString cppPath);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Update UMG Controller", Keywords = "UmgControllerGeneratorPlugin update umg controller"), Category = "UmgControllerGeneratorPlugin")
	static bool UpdateUmgController(UObject* inputBlueprint);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Update Mappings ", Keywords = "UmgControllerGeneratorPlugin update mappings"), Category = "UmgControllerGeneratorPlugin")
	static bool UpdateMappings(TArray<UObject*> inputBlueprints);
};
