#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "CodeGenerator.generated.h"

/**
 * Generates the initial cpp/h files for a UMG widget controller
 * and updates ones that are already created. 
 */
UCLASS()
class UCodeGenerator : public UObject {
    GENERATED_BODY()

public:
    void CreateFiles(class UWidgetBlueprint* blueprint, FString widgetPath, FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath);
    void UpdateFiles(FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath);

private:
    FString UpdateHeaderFile(const TArray<UWidget*>& namedWidgets, FString headerContents);
    FString UpdateCppFile(const TArray<UWidget*>& namedWidgets, FString cppContents);
    TArray<UWidget*> GetNamedWidgets(const TArray<UWidget*> widgets);
    UClass* GetFirstNonGeneratedParent(UClass* inputClass);
    class UHeaderLookupTable* GetHeaderLookupTable();
    class UBlueprint* GetBlueprintForWidget(UWidget* widget);

    // Holds a lookup table from class name to header file
    UPROPERTY()
    class UHeaderLookupTable* _headerLookupTable = nullptr;

	// Keeps track of the currently running creation process.
	// This will be set to nullptr when completed.
    UPROPERTY()
	class UFileCreationProcess* _currentProcess = nullptr;

    // This is set to a function that should be invoked when live coding
    // completes. It will be nullptr unless we're currently waiting for
    // live coding to complete before we reparent a generated class.
    std::function<void()> _onPatchingComplete = nullptr;

    // True to enable auto blueprint reparenting.
    bool _autoReparentBlueprintsEnabled = true;
};
