#include "UmgControllerGeneratorPluginBPLibrary.h"
#include "UmgControllerGeneratorPlugin.h"
#include "Components/Widget.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "CodeGenerator.h"
#include "BlueprintSourceMap.h"

DEFINE_LOG_CATEGORY_STATIC(UmgControllerGeneratorPluginSub, Log, All);

UUmgControllerGeneratorPluginBPLibrary::UUmgControllerGeneratorPluginBPLibrary(const FObjectInitializer& objectInitializer) : Super(objectInitializer) {
	// Nothing to do
}

void UUmgControllerGeneratorPluginBPLibrary::CreateUmgController(UObject* inputBlueprint, FString headerPath, FString cppPath) {
	// The input class should be a UWidgetBlueprint
	UWidgetBlueprint* blueprint = Cast<UWidgetBlueprint>(inputBlueprint);
	if (blueprint == nullptr) {
		UE_LOG(UmgControllerGeneratorPluginSub, Error, TEXT("CreateUmgController called without a widget blueprint."));
		return;
	}

	UWidgetTree* widgetTree = blueprint->WidgetTree;

	TArray<UWidget*> widgets;
	widgetTree->ForEachWidget([&widgets] (UWidget* widget) {
		UE_LOG(UmgControllerGeneratorPluginSub, Display, TEXT("Widget: %s of type %s"), *widget->GetName(), *widget->GetClass()->GetName());
		widgets.Add(widget);
	});

	FString name = blueprint->GetName();
	FString wbpPrefix = TEXT("WBP_");
	if (name.StartsWith(wbpPrefix)) {
		name = name.RightChop(wbpPrefix.Len());
	}

	// Get the widget path name.
	FString contentPath = blueprint->GetPathName();

	// We don't need the extension
	int dotIndex = -1;
	if (contentPath.FindLastChar(TEXT('.'), dotIndex)) {
		contentPath = contentPath.Left(dotIndex);
	}

	GetCodeGenerator()->CreateFiles(
		blueprint,
		contentPath,
		name,
		GetCodeGenerator()->GetClassSuffix(),
		widgets,
		headerPath,
		cppPath
	);
}

bool UUmgControllerGeneratorPluginBPLibrary::UpdateUmgController(UObject* inputBlueprint) {
	// The input class should be a UWidgetBlueprint
	UWidgetBlueprint* blueprint = Cast<UWidgetBlueprint>(inputBlueprint);
	if (blueprint == nullptr) {
		UE_LOG(UmgControllerGeneratorPluginSub, Error, TEXT("UpdateUmgController called without a widget blueprint."));
		return false;
	}

    UBlueprintSourceMap* sourceMap = NewObject<UBlueprintSourceMap>();
    sourceMap->LoadMapping(FPaths::GameSourceDir(), GetCodeGenerator()->GetBlueprintSourceFilePath());
	FBlueprintSourceModel entry = sourceMap->GetSourcePathsFor(blueprint);
	if (!entry.IsValid()) {
		UE_LOG(UmgControllerGeneratorPluginSub, Error, TEXT("No source map entry for %s. Fix the mapping or try Update Mappings."), *blueprint->GetPathName());
		return false;
	}

	UWidgetTree* widgetTree = blueprint->WidgetTree;
	TArray<UWidget*> widgets;
	widgetTree->ForEachWidget([&widgets] (UWidget* widget) {
		UE_LOG(UmgControllerGeneratorPluginSub, Display, TEXT("Widget: %s of type %s"), *widget->GetName(), *widget->GetClass()->GetName());
		widgets.Add(widget);
	});

	FString name = blueprint->GetName();
	FString wbpPrefix = TEXT("WBP_");
	if (name.StartsWith(wbpPrefix)) {
		name = name.RightChop(wbpPrefix.Len());
	}

	// Get the widget path name.
	FString contentPath = blueprint->GetPathName();

	// We don't need the extension
	int dotIndex = -1;
	if (contentPath.FindLastChar(TEXT('.'), dotIndex)) {
		contentPath = contentPath.Left(dotIndex);
	}

	GetCodeGenerator()->UpdateFiles(name, GetCodeGenerator()->GetClassSuffix(), contentPath, widgets, entry.HeaderPath, entry.CppPath);

	return true;
}

bool UUmgControllerGeneratorPluginBPLibrary::UpdateMappings(TArray<UObject*> inputBlueprints) {
	TArray<UBlueprint*> blueprints;
	int index = 0;
	for (UObject* obj : inputBlueprints) {
		UBlueprint* blueprint = Cast<UBlueprint>(obj);
		if (blueprint == nullptr) {
			UE_LOG(UmgControllerGeneratorPluginSub, Error, TEXT("UpdateMappings called without a blueprint in index %d."), index);
		} else {
			blueprints.Add(blueprint);
		}
		index++;
	}

    UBlueprintSourceMap* sourceMap = NewObject<UBlueprintSourceMap>();
    sourceMap->LoadMapping(FPaths::GameSourceDir(), GetCodeGenerator()->GetBlueprintSourceFilePath());
	if (sourceMap->UpdateMappings(blueprints, GetCodeGenerator()->GetClassSuffix())) {
		GetCodeGenerator()->ShowNotification(TEXT("Mappings updated."), ENotificationReason::Success);
		return true;
	} else {
		GetCodeGenerator()->ShowNotification(TEXT("There was a problem updating the file."), ENotificationReason::Error);
		return false;
	}
}

UCodeGenerator* UUmgControllerGeneratorPluginBPLibrary::GetCodeGenerator() {
	if (_codeGeneratorInstance == nullptr) {
		_codeGeneratorInstance = NewObject<UCodeGenerator>();
		_codeGeneratorInstance->AddToRoot();
	}
	return _codeGeneratorInstance;
}