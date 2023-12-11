#pragma once

#include "CoreMinimal.h"
#include "CodeGeneratorConfig.generated.h"

UCLASS(Config=Editor, defaultconfig)
class UCodeGeneratorConfig : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = Settings)
    FString ClassSuffix = FString(TEXT("Controller"));

    UPROPERTY(Config, EditAnywhere, Category = Settings)
    bool EnableAutoReparenting = true;
};
