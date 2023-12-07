#pragma once

#include "CoreMinimal.h"
#include "HeaderLookupTable.generated.h"

/**
 * This class is used to store a lookup table from class name to
 * header file include path for classes within a project and its
 * dependent modules.
 */
UCLASS()
class UHeaderLookupTable : public UObject {
    GENERATED_BODY()

public:
    UHeaderLookupTable(const FObjectInitializer& objectInitializer) : UObject(objectInitializer) { }
    virtual ~UHeaderLookupTable() { }

    void InitTable();
    FString GetIncludeFilePathFor(FString className);

private:
    // Class Name to Relative Header File Path mapping
    //    Note: The class name does not include the "U" prefix.
    //          Ex: "Button" instead of "UButton"
    TMap<FString, FString> _lookupTable;
};
