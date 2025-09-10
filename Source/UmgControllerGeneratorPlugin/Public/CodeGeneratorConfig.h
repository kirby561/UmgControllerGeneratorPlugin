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
    FString GeneratedMethodsPrefix = TEXT("#pragma region Generated Methods Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedMethodsSuffix = TEXT("#pragma endregion Generated Methods Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedIncludesPrefix = TEXT("#pragma region Generated Includes Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedIncludesSuffix = TEXT("#pragma endregion Generated Includes Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedLoaderPrefix = TEXT("#pragma region Generated Loader Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedLoaderSuffix = TEXT("#pragma endregion Generated Loader Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedPropertiesPrefix = TEXT("#pragma region Generated Properties Section");

    UPROPERTY(Config, EditAnywhere, Category = "Settings|Sections")
    FString GeneratedPropertiesSuffix = TEXT("#pragma endregion Generated Properties Section");
};
