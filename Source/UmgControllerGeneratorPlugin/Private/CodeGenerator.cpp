#include "CodeGenerator.h"
#include "HeaderLookupTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(CodeGeneratorSub, Log, All);

const FString HeaderFileTemplate = TEXT("\
#pragma once\n\
\n\
#include \"CoreMinimal.h\"\n\
#include \"Blueprint/UserWidget.h\"\n\
#include \"[WIDGET_NAME][WIDGET_SUFFIX].generated.h\"\n\
\n\
UCLASS()\n\
class U[WIDGET_NAME][WIDGET_SUFFIX] : public UUserWidget {\n\
    GENERATED_BODY()\n\
\n\
public: // Methods\n\
    U[WIDGET_NAME][WIDGET_SUFFIX](const FObjectInitializer& objectInitializer);\n\
    virtual ~U[WIDGET_NAME][WIDGET_SUFFIX]() { }\n\
\n\
public: // Create Method\n\
    static U[WIDGET_NAME][WIDGET_SUFFIX]* CreateInstance(APlayerController* playerController);\n\
\n\
public: // Properties\n\
// ---------- Generated Properties Section ---------- //\n\
//             (Don't modify manually)             //\n\
// ---------- End Generated Properties Section ---------- //\n\
};\n\
\n\
// ---------- Generated Loader Section ---------- //\n\
//             (Don't modify manually)            //\n\
UCLASS()\n\
class U[WIDGET_NAME]Loader : public UObject {\n\
    GENERATED_BODY()\n\
public:\n\
    U[WIDGET_NAME]Loader();\n\
    virtual ~U[WIDGET_NAME]Loader() { }\n\
\n\
public:\n\
    UPROPERTY()\n\
    UClass* WidgetTemplate = nullptr;\n\
    static const inline FString WidgetPath = TEXT(\"[WIDGET_PATH]\");\n\
};\n\
// ---------- End Generated Loader Section ---------- //\n\
");

const FString CppFileTemplate = TEXT("\
#include \"[WIDGET_NAME][WIDGET_SUFFIX].h\"\n\
\n\
// ---------- Generated Includes Section ---------- //\n\
//             (Don't modify manually)              //\n\
\n\
// ---------- End Generated Includes Section ---------- //\n\
\n\
U[WIDGET_NAME][WIDGET_SUFFIX]::U[WIDGET_NAME][WIDGET_SUFFIX](const FObjectInitializer& objectInitializer) : UUserWidget(objectInitializer) {\n\
\n\
}\n\
\n\
// ---------- Generated Methods Section ---------- //\n\
//             (Don't modify manually)             //\n\
U[WIDGET_NAME][WIDGET_SUFFIX]* U[WIDGET_NAME][WIDGET_SUFFIX]::CreateInstance(APlayerController* playerController) {\n\
    U[WIDGET_NAME]Loader* loader = NewObject<U[WIDGET_NAME]Loader>(playerController);\n\
    return Cast<U[WIDGET_NAME][WIDGET_SUFFIX]>(CreateWidget(playerController, loader->WidgetTemplate));\n\
}\n\
\n\
U[WIDGET_NAME]Loader::U[WIDGET_NAME]Loader() {\n\
    static ConstructorHelpers::FClassFinder<UUserWidget> widgetTemplateFinder(*WidgetPath);\n\
    WidgetTemplate = widgetTemplateFinder.Class;\n\
}\n\
// ---------- End Generated Methods Section ---------- //\n\
");

const FString PropertiesSectionStartMarker = TEXT("// ---------- Generated Properties Section ---------- //");
const FString PropertiesSectionEndMarker = TEXT("// ---------- End Generated Properties Section ---------- //");
const FString IncludeSectionStartMarker = TEXT("// ---------- Generated Includes Section ---------- //");
const FString IncludeSectionEndMarker = TEXT("// ---------- End Generated Includes Section ---------- //");
const FString MethodSectionStartMarker = TEXT("// ---------- Generated Methods Section ---------- //");
const FString MethodSectionEndMarker = TEXT("// ---------- End Generated Methods Section ---------- //");
const FString LoaderSectionStartMarker = TEXT("// ---------- Generated Loader Section ---------- //");
const FString LoaderSectionEndMarker = TEXT("// ---------- End Generated Loader Section ---------- //");
const FString DoNotModifyComment = TEXT("//             (Don't modify manually)              //");
const FString BindWidgetLabel = TEXT("UPROPERTY(BlueprintReadOnly, meta = (BindWidget))");
const FString WidgetNameMarker = TEXT("[WIDGET_NAME]");
const FString WidgetSuffixMarker = TEXT("[WIDGET_SUFFIX]");
const FString WidgetPathMarker = TEXT("[WIDGET_PATH]");

void CodeGenerator::CreateFiles(FString widgetPath, FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath) {
    // Get the widgets that are not the default name
    TArray<UWidget*> namedWidgets = GetNamedWidgets(widgets);

    // Make the header from the template
    FString headerFileStr = HeaderFileTemplate;
    headerFileStr = headerFileStr.Replace(*WidgetNameMarker, *widgetName);
    headerFileStr = headerFileStr.Replace(*WidgetSuffixMarker, *widgetSuffix);
    headerFileStr = headerFileStr.Replace(*WidgetPathMarker, *widgetPath);

    // Fill in the dynamic content
    FString updatedHeaderFileContents = UpdateHeaderFile(namedWidgets, headerFileStr);
    if (updatedHeaderFileContents.IsEmpty()) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to update the header file at %s"), *headerPath);
        return;
    }

    // Make the CPP from the template
    FString cppFileStr = CppFileTemplate;
    cppFileStr = cppFileStr.Replace(*WidgetNameMarker, *widgetName);
    cppFileStr = cppFileStr.Replace(*WidgetSuffixMarker, *widgetSuffix);

    // Fill in the dynamic content
    FString updatedCppFileContents = UpdateCppFile(namedWidgets, cppFileStr);
    if (updatedCppFileContents.IsEmpty()) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to update the cpp file at %s"), *cppPath);
        return;
    }

    // Save both to a file
    if (!FFileHelper::SaveStringToFile(updatedHeaderFileContents, *headerPath)) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to save the header file to %s"), *headerPath);
        return;
    }
    if (!FFileHelper::SaveStringToFile(updatedCppFileContents, *cppPath)) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to save the cpp file to %s"), *cppPath);
        return;
    }
}

void CodeGenerator::UpdateFiles(FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath) {
    // Get the widgets that are not the default name
    TArray<UWidget*> namedWidgets = GetNamedWidgets(widgets);

    // Load each file from disk, replace the areas between the markers with the new data
    FString headerFileContents;
    if (!FFileHelper::LoadFileToString(headerFileContents, *headerPath)) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to load the header file at %s"), *headerPath);
        return;
    }

    FString updatedHeaderFileContents = UpdateHeaderFile(widgets, headerFileContents);
    if (updatedHeaderFileContents.IsEmpty()) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to update the header file at %s"), *headerPath);
        return;
    }

    FString cppFileContents;
    if (!FFileHelper::LoadFileToString(cppFileContents, *cppPath)) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to load the cpp file at %s"), *cppPath);
        return;
    }

    FString updatedCppFileContents = UpdateCppFile(widgets, cppFileContents);
    if (updatedCppFileContents.IsEmpty()) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to update the cpp file at %s"), *cppPath);
        return;
    }

    // Write both to a file
    if (!FFileHelper::SaveStringToFile(updatedHeaderFileContents, *headerPath)) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to save the header file to %s"), *headerPath);
        return;
    }
    if (!FFileHelper::SaveStringToFile(updatedCppFileContents, *cppPath)) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("Failed to save the header file to %s"), *cppPath);
        return;
    }
}

FString CodeGenerator::UpdateHeaderFile(const TArray<UWidget*>& namedWidgets, FString headerContents) {
    FString result;

    // Find the different sections of the file
    int propertiesSectionStartIndex = headerContents.Find(PropertiesSectionStartMarker);
    if (propertiesSectionStartIndex < 0) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("No properties section found in header"));
        return FString();
    }

    int propertiesSectionEndIndex = headerContents.Find(PropertiesSectionEndMarker);
    if (propertiesSectionEndIndex < 0) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("No properties section end found in header"));
        return FString();
    }

    result.Append(headerContents.Left(propertiesSectionStartIndex + PropertiesSectionStartMarker.Len()));
    result.Append(TEXT("\n"));
    result.Append(DoNotModifyComment + TEXT("\n"));

    // For each named widget, add it to the properties section
    bool isFirst = true;
    for (UWidget* widget : namedWidgets) {
        FString widgetName = widget->GetName();

        // Get the first non-generated class
        UClass* widgetClass = GetFirstNonGeneratedParent(widget->GetClass());
        FString widgetClassName = widgetClass->GetName();

        if (!isFirst)
            result.Append(TEXT("\n"));
        result.Append(TEXT("    ") + BindWidgetLabel + TEXT("\n"));
        result.Append(TEXT("    class U") + widgetClassName + TEXT("* ") + widgetName + TEXT(" = nullptr;\n"));

        isFirst = false;
    }

    // Finish off the file
    result.Append(PropertiesSectionEndMarker + TEXT("\n"));
    result.Append(headerContents.RightChop(propertiesSectionEndIndex + PropertiesSectionEndMarker.Len() + 1));

    return result;
}

FString CodeGenerator::UpdateCppFile(const TArray<UWidget*>& namedWidgets, FString cppContents) {
    FString result;

    // Find the different sections of the file
    int includesSectionStartIndex = cppContents.Find(IncludeSectionStartMarker);
    if (includesSectionStartIndex < 0) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("No includes section found in cpp"));
        return FString();
    }

    int includesSectionEndIndex = cppContents.Find(IncludeSectionEndMarker);
    if (includesSectionEndIndex < 0) {
        UE_LOG(CodeGeneratorSub, Error, TEXT("No properties section end found in cpp"));
        return FString();
    }

    result.Append(cppContents.Left(includesSectionStartIndex + IncludeSectionStartMarker.Len()));
    result.Append(TEXT("\n"));
    result.Append(DoNotModifyComment + TEXT("\n"));

    // Get the include for each widget and keep them in a set
    UHeaderLookupTable* lookupTable = GetHeaderLookupTable();
    TSet<FString> includes;
    for (UWidget* widget : namedWidgets) {
        UClass* widgetClass = GetFirstNonGeneratedParent(widget->GetClass());
        FString name = widgetClass->GetName();
        FString headerFilePath = lookupTable->GetIncludeFilePathFor(name);
        if (!includes.Contains(headerFilePath)) {
            includes.Add(headerFilePath);
        }
    }

    for (FString includePath : includes) {
        result.Append(TEXT("#include \"") + includePath + "\"\n");
    }

    // Finish off the file
    result.Append(IncludeSectionEndMarker + TEXT("\n"));
    result.Append(cppContents.RightChop(includesSectionEndIndex + IncludeSectionEndMarker.Len() + 1));

    return result;
}

/**
 * For each widget, this method detects if it is an automated name or a user-given
 * name and returns a list of just the widgets with user-given names. 
 */
TArray<UWidget*> CodeGenerator::GetNamedWidgets(const TArray<UWidget*> widgets) {
    TArray<UWidget*> namedWidgets;

    for (UWidget* widget : widgets) {
        FString widgetName = widget->GetName();
        UClass* widgetClass = widget->GetClass();
        FString widgetClassName = widgetClass->GetName();

        // Generated classes have a _C at the end of their name
        if (widgetClass->ClassGeneratedBy != nullptr) {
            FString classSuffix = TEXT("_C");
            if (widgetClassName.EndsWith(TEXT("_C"))) {
                widgetClassName = widgetClassName.LeftChop(classSuffix.Len());
            }
        }

        // Automated names will be of the form "widgetClassName" or "widgetClassName_#"
        // so see if there's an _# at the end first.
        TCHAR delimitter = '_';
        int underscoreIndex = -1;
        if (widgetName.FindLastChar(TEXT('_'), underscoreIndex)) {
            // Check if the right side is a number
            FString rightSide = widgetName.Mid(underscoreIndex + 1);
            if (rightSide.IsNumeric()) {
                widgetName = widgetName.Left(underscoreIndex);
            }
        }

        // Now just check if the adjusted widget name is the same as the class. If it
        // is, it is not user-given and so we don't add it to the list
        if (!widgetName.Equals(widgetClassName)) {
            namedWidgets.Add(widget);
        }
    }

    return namedWidgets;
}

UClass* CodeGenerator::GetFirstNonGeneratedParent(UClass* inputClass) {
    UClass* result = inputClass;
    while (result != nullptr && result->ClassGeneratedBy != nullptr) {
        result = result->GetSuperClass();
    }

    // This should never happen but just in case it does, do something reasonable
    if (result == nullptr) {
        // This is odd; there are no non-generated classes?
        // Just use the original class and let the user figure it out.
        // We hate our users here.
        UE_LOG(CodeGeneratorSub, Warning, TEXT("%s has no non-generated super classes! Using the base one."));
        result = inputClass;
    }

    return result;
}

UHeaderLookupTable* CodeGenerator::GetHeaderLookupTable() {
    if (_headerLookupTable == nullptr) {
        _headerLookupTable = NewObject<UHeaderLookupTable>();
        _headerLookupTable->AddToRoot();
        _headerLookupTable->InitTable();        
    }
    return _headerLookupTable;
}
