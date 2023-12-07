#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"

/**
 * Generates the initial cpp/h files for a UMG widget controller
 * and updates ones that are already created. 
 */
class CodeGenerator {
public:
    static void CreateFiles(FString widgetPath, FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath);
    static void UpdateFiles(FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath);

private:
    static FString UpdateHeaderFile(const TArray<UWidget*>& namedWidgets, FString headerContents);
    static FString UpdateCppFile(const TArray<UWidget*>& namedWidgets, FString cppContents);
    static TArray<UWidget*> GetNamedWidgets(const TArray<UWidget*> widgets);
    static UClass* GetFirstNonGeneratedParent(UClass* inputClass);
    static class UHeaderLookupTable* GetHeaderLookupTable();

    // Holds a lookup table from class name to header file
    static inline class UHeaderLookupTable* _headerLookupTable = nullptr;
};
