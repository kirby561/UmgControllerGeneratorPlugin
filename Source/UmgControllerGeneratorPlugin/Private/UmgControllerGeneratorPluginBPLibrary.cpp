#include "UmgControllerGeneratorPluginBPLibrary.h"
#include "UmgControllerGeneratorPlugin.h"
#include "Components/Widget.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "CodeGenerator.h"

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

	CodeGenerator::CreateFiles(
		blueprint,
		contentPath,
		name,
		TEXT("Controller"),
		widgets,
		headerPath,
		cppPath
	);
}
