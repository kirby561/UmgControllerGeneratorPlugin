#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "CodeGeneratorConfig.h"
#include "CodeGenerator.generated.h"

using TSection = TPair<FString, FString>;
using TSections = TArray<TSection>;
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
    FString GetGeneratedMethodsPrefix() { return UnescapeNewlines(_config->GeneratedMethodsPrefix); }
    FString GetGeneratedMethodsSuffix() { return UnescapeNewlines(_config->GeneratedMethodsSuffix); }
    FString GetGeneratedIncludesPrefix() { return UnescapeNewlines(_config->GeneratedIncludesPrefix); }
    FString GetGeneratedIncludesSuffix() { return UnescapeNewlines(_config->GeneratedIncludesSuffix); }
    FString GetGeneratedLoaderPrefix() { return UnescapeNewlines(_config->GeneratedLoaderPrefix); }
    FString GetGeneratedLoaderSuffix() { return UnescapeNewlines(_config->GeneratedLoaderSuffix); }
    FString GetGeneratedPropertiesPrefix() { return UnescapeNewlines(_config->GeneratedPropertiesPrefix); }
    FString GetGeneratedPropertiesSuffix() { return UnescapeNewlines(_config->GeneratedPropertiesSuffix); }

private: // Fill Templates Sections
    FString FillHeaderTemplateSections(const FString headerTemplate);
    FString FillCppTemplateSections(const FString cppTemplate);
    FString ReplaceSections(const FString& source, const TSections& sections);

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
    FString UnescapeNewlines(const FString& input);

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

    // The templates for created header and cpp files
    FString _headerFileTemplate;
    FString _cppFileTemplate;
};
