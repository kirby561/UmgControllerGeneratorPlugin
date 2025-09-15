#pragma once

#include "CoreMinimal.h"
#include "CodeGeneratorConfig.generated.h"

UCLASS(Config=Editor, defaultconfig)
class UCodeGeneratorConfig : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = Settings)
    FString ClassSuffix = TEXT("Controller");

    UPROPERTY(Config, EditAnywhere, Category = Settings)
    FString BlueprintSourceMapDirectory = TEXT("");

    UPROPERTY(Config, EditAnywhere, Category = Settings)
    bool EnableAutoReparenting = true;

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedMethodsPrefix = TEXT("// ---------- Generated Methods Section ---------- //\n//             (Don't modify manually)             //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedMethodsSuffix = TEXT("// ---------- End Generated Methods Section ---------- //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedIncludesPrefix = TEXT("// ---------- Generated Includes Section ---------- //\n//             (Don't modify manually)              //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedIncludesSuffix = TEXT("// ---------- End Generated Includes Section ---------- //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedLoaderPrefix = TEXT("// ---------- Generated Loader Section ---------- //\n//             (Don't modify manually)            //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedLoaderSuffix = TEXT("// ---------- End Generated Loader Section ---------- //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedPropertiesPrefix = TEXT("// ---------- Generated Properties Section ---------- //\n//              (Don't modify manually)               //");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedPropertiesSuffix = TEXT("// ---------- End Generated Properties Section ---------- //");
};
