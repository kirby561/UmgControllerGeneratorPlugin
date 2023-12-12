#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "CodeGeneratorConfig.h"
#include "CodeGenerator.generated.h"

enum class ENotificationReason {
    Success,
    Warning,
    Error
};

/**
 * Generates the initial cpp/h files for a UMG widget controller
 * and updates ones that are already created. 
 */
UCLASS()
class UCodeGenerator : public UObject {
    GENERATED_BODY()

public:
    UCodeGenerator(const FObjectInitializer& initializer);

    void CreateFiles(class UWidgetBlueprint* blueprint, FString widgetPath, FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath);
    void UpdateFiles(FString widgetName, FString widgetSuffix, FString widgetPath, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath);
    void ShowNotification(FString message, ENotificationReason severity);

    FString GetClassSuffix() { return _config->ClassSuffix; }
    FString GetBlueprintSourceDirectory() { return _config->BlueprintSourceMapDirectory; }
    FString GetBlueprintSourceFilePath();
    bool IsAutoReparentingEnabled() { return _config->EnableAutoReparenting; }

private:
    FString UpdateHeaderFile(const TArray<UWidget*>& namedWidgets, FString headerContents, FString blueprintReferencePath);
    FString UpdateCppFile(const TArray<UWidget*>& namedWidgets, FString cppContents);
    TArray<UWidget*> GetNamedWidgets(const TArray<UWidget*> widgets);
    UClass* GetFirstNonGeneratedParent(UClass* inputClass);
    class UHeaderLookupTable* GetHeaderLookupTable();
    class UBlueprint* GetBlueprintForWidget(UWidget* widget);
    void ShowSuccessMessage(FString message) { ShowNotification(message, ENotificationReason::Success); }
    void ReportWarning(FString message) { ShowNotification(message, ENotificationReason::Warning); }
    void ReportError(FString message) { ShowNotification(message, ENotificationReason::Error); }

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

    UPROPERTY()
    UCodeGeneratorConfig* _config = nullptr;
};
