#pragma once

#include "CoreMinimal.h"
#include "CodeEventSignaler.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UCodeEventSignaler : public UObject {
    GENERATED_BODY()

public:
    UCodeEventSignaler(const FObjectInitializer& objInitializer) : UObject(objInitializer) { }
    virtual ~UCodeEventSignaler() { }

    UFUNCTION(BlueprintImplementableEvent)
    void OnCompileCompleted();
};
