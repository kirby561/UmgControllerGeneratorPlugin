#include "CodeGenerator.h"
#include "HeaderLookupTable.h"
#include "CodeEventSignaler.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ILiveCodingModule.h"
#include "BlueprintEditorLibrary.h"
#include "FileHelpers.h"
#include "WidgetBlueprint.h"
#include "BlueprintSourceMap.h" 	
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

DEFINE_LOG_CATEGORY_STATIC(CodeGeneratorSub, Log, All);

const FString HeaderFileTemplate = TEXT("\
#pragma once\n\
\n\
#include \"CoreMinimal.h\"\n\
#include \"Blueprint/UserWidget.h\"\n\
#include \"[HEADER_FILE_NAME].generated.h\"\n\
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
#include \"[HEADER_FILE_NAME].h\"\n\
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
const FString HeaderFileNameMarker = TEXT("[HEADER_FILE_NAME]");
const FString WidgetLineMarker = TEXT("static const inline FString WidgetPath = ");

UCodeGenerator::UCodeGenerator(const FObjectInitializer& initializer) {
    _config = CreateDefaultSubobject<UCodeGeneratorConfig>(TEXT("Config"));
}

void UCodeGenerator::CreateFiles(UWidgetBlueprint* blueprint, FString widgetPath, FString widgetName, FString widgetSuffix, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath) {
    if (_currentProcess == nullptr) {
        _currentProcess = NewObject<UFileCreationProcess>();

        // We also want to listen for compile events
        ILiveCodingModule* liveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
        if (liveCoding != nullptr) {
            // Add a live coding delegate. Technically we should save the handle this returns so we can remove it
            // on shutdown but we're alive for the whole editor lifetime anyway so who cares.
            liveCoding->GetOnPatchCompleteDelegate().AddLambda([this] () {
                if (_onPatchingComplete) {
                    _onPatchingComplete();
                }
            });
        }
    }

    FString className = widgetName + widgetSuffix;
    _currentProcess->Start(
        className,
        [this, widgetPath, widgetName, widgetSuffix, widgets, className, blueprint] (FString headerFilePath, FString cppFilePath) {
            // Get the widgets that are not the default name
            TArray<UWidget*> namedWidgets = GetNamedWidgets(widgets);

            // Make the header from the template
            FString headerFileName = FPaths::GetBaseFilename(headerFilePath);
            FString headerFileStr = HeaderFileTemplate;
            headerFileStr = headerFileStr.Replace(*WidgetNameMarker, *widgetName);
            headerFileStr = headerFileStr.Replace(*WidgetSuffixMarker, *widgetSuffix);
            headerFileStr = headerFileStr.Replace(*WidgetPathMarker, *widgetPath);
            headerFileStr = headerFileStr.Replace(*HeaderFileNameMarker, *headerFileName);

            // Fill in the dynamic content
            FString updatedHeaderFileContents = UpdateHeaderFile(namedWidgets, headerFileStr, widgetPath);
            if (updatedHeaderFileContents.IsEmpty()) {
                ReportError(FString::Printf(TEXT("Failed to update the header file at %s"), *headerFilePath));
                return;
            }

            // Make the CPP from the template
            FString cppFileStr = CppFileTemplate;
            cppFileStr = cppFileStr.Replace(*WidgetNameMarker, *widgetName);
            cppFileStr = cppFileStr.Replace(*WidgetSuffixMarker, *widgetSuffix);
            cppFileStr = cppFileStr.Replace(*HeaderFileNameMarker, *headerFileName);

            // Fill in the dynamic content
            FString updatedCppFileContents = UpdateCppFile(namedWidgets, cppFileStr);
            if (updatedCppFileContents.IsEmpty()) {
                ReportError(FString::Printf(TEXT("Failed to update the cpp file at %s"), *cppFilePath));
                return;
            }

            // Save both to a file
            if (!FFileHelper::SaveStringToFile(updatedHeaderFileContents, *headerFilePath)) {
                ReportError(FString::Printf(TEXT("Failed to save the header file to %s"), *headerFilePath));
                return;
            }
            if (!FFileHelper::SaveStringToFile(updatedCppFileContents, *cppFilePath)) {
                ReportError(FString::Printf(TEXT("Failed to save the cpp file to %s"), *cppFilePath));
                return;
            }

            // Update the header map
            UBlueprintSourceMap* sourceMap = NewObject<UBlueprintSourceMap>();
            sourceMap->LoadMapping(FPaths::GameSourceDir(), GetBlueprintSourceFilePath());
            sourceMap->AddMapping(blueprint, headerFilePath, cppFilePath);
            sourceMap->SaveMapping();

            // Trigger a live compile for the changes
            ILiveCodingModule* liveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
            if (liveCoding != nullptr && liveCoding->IsEnabledForSession())	{
                if (liveCoding->AutomaticallyCompileNewClasses()) {
                    if (IsAutoReparentingEnabled()) {
                        // Setup the patch complete callback and trigger a compile. This will let us reparent
                        // the class when the compile has finished in the blueprint so the user doesn't have to.
                        FString capturedClassName = className;
                        _onPatchingComplete = [this, blueprint, capturedClassName] () {
                            // There doesn't appear to be a way to get the result of the last compile
                            // from the live coding module, so we'll assume it succeeded and check
                            // if reparenting fails later. The user will see the output in the console
                            // either way.
                            UE_LOG(CodeGeneratorSub, Display, TEXT("Compile finished."));

                            // Now that the compile is done, lookup the class by name:
                            UClass* resultClass = nullptr;
                            for (TObjectIterator<UClass> uclassIterator; uclassIterator; ++uclassIterator) {
                                FString nameToTest = uclassIterator->GetName();
                                if (nameToTest == capturedClassName) {
                                    resultClass = *uclassIterator;
                                    break;
                                }
                            }

                            if (resultClass != nullptr) {
                                UE_LOG(CodeGeneratorSub, Display, TEXT("Reparenting to the new class."));
                                UBlueprintEditorLibrary::ReparentBlueprint(blueprint, resultClass);

                                // ReparentBlueprint doesn't save the blueprint after compiling so it won't work
                                // after restarting the editor. So we need to save it manually
                                TArray<UPackage*> packagesToSave;
                                packagesToSave.Add(blueprint->GetOutermost());
                                FEditorFileUtils::EPromptReturnCode result = FEditorFileUtils::PromptForCheckoutAndSave( 
                                    packagesToSave,
                                    false, // bCheckDirty, 
                                    false // bPromptToSave
                                );

                                if (result != FEditorFileUtils::EPromptReturnCode::PR_Success) {
                                    ReportError(FString::Printf(TEXT("There was an issue saving the blueprint after reparenting. You will need to reparent manually.")));
                                } else {
                                    ShowSuccessMessage(TEXT("Class created and reparenting complete."));
                                }
                            } else {
                                ReportError(FString::Printf(TEXT("Could not find the generated class. You will need to reparent manually.")));
                            }

                            _onPatchingComplete = nullptr;
                        };
                    }

                    // Invoke a compile
                    UE_LOG(CodeGeneratorSub, Display, TEXT("Manually invoking compile"));
                    ELiveCodingCompileFlags flags = ELiveCodingCompileFlags::None;
                    ELiveCodingCompileResult result = ELiveCodingCompileResult::Failure;
                    int attempts = 0;
                    bool succeeded = false;
                    while (!succeeded) {
                        succeeded = liveCoding->Compile(flags, &result);

                        // Sometimes the live coding module is still running even though its OnPatchingCompleted delegate
                        // has fired. However, we know it will be reset shortly so just wait for it. This could be a timer
                        // so it doesn't block the thread but this is an editor tool and it's very quick so whatever.
                        if (!succeeded) {
                            UE_LOG(CodeGeneratorSub, Warning, TEXT("Could not invoke compile. result: %d, attempts: %d"), (int)result, attempts);

                            // Just hang the thread while we wait for the current compile to finish.
		                    FPlatformProcess::Sleep(0.1f);
                        }

                        // Give it about 5 seconds before giving up
                        attempts++;
                        if (attempts > 50) {
                            UE_LOG(CodeGeneratorSub, Warning, TEXT("Exceeded max number of attempts waiting for compilation."));
                            break;
                        }
                    }

                    if (!succeeded) {
                        ReportError(FString::Printf(TEXT("Failed to trigger a compile. You will need to reparent your class manually to the controller.")));
                        _onPatchingComplete = nullptr;
                    }
                }
            }
        },
        [this] (FString errorDescription) {
            if (!errorDescription.IsEmpty()) {
                ReportError(FString::Printf(TEXT("Something went wrong creating the controller files: %s"), *errorDescription));
            } // Else it's just a cancel
        }
    );
}

void UCodeGenerator::UpdateFiles(FString widgetName, FString widgetSuffix, FString blueprintPath, const TArray<UWidget*>& widgets, FString headerPath, FString cppPath) {
    // Get the widgets that are not the default name
    TArray<UWidget*> namedWidgets = GetNamedWidgets(widgets);

    // Load each file from disk, replace the areas between the markers with the new data
    FString headerFileContents;
    if (!FFileHelper::LoadFileToString(headerFileContents, *headerPath)) {
        ReportError(FString::Printf(TEXT("Failed to load the header file at %s"), *headerPath));
        return;
    }

    FString updatedHeaderFileContents = UpdateHeaderFile(namedWidgets, headerFileContents, blueprintPath);
    if (updatedHeaderFileContents.IsEmpty()) {
        ReportError(FString::Printf(TEXT("Failed to update the header file at %s"), *headerPath));
        return;
    }

    FString cppFileContents;
    if (!FFileHelper::LoadFileToString(cppFileContents, *cppPath)) {
        ReportError(FString::Printf(TEXT("Failed to load the cpp file at %s"), *cppPath));
        return;
    }

    FString updatedCppFileContents = UpdateCppFile(namedWidgets, cppFileContents);
    if (updatedCppFileContents.IsEmpty()) {
        ReportError(FString::Printf(TEXT("Failed to update the cpp file at %s"), *cppPath));
        return;
    }

    // Write both to a file
    if (!FFileHelper::SaveStringToFile(updatedHeaderFileContents, *headerPath)) {
        ReportError(FString::Printf(TEXT("Failed to save the header file to %s"), *headerPath));
        return;
    }
    if (!FFileHelper::SaveStringToFile(updatedCppFileContents, *cppPath)) {
        ReportError(FString::Printf(TEXT("Failed to save the header file to %s"), *cppPath));
        return;
    }

    ShowSuccessMessage(FString::Printf(TEXT("%s updated."), *widgetName));
}

FString UCodeGenerator::UpdateHeaderFile(const TArray<UWidget*>& namedWidgets, FString headerContents, FString blueprintReferencePath) {
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

    // Write the end section
    result.Append(PropertiesSectionEndMarker + TEXT("\n"));

    int generatedLoaderStartIndex = headerContents.Find(LoaderSectionStartMarker);
    int generatedLoaderEndIndex = headerContents.Find(LoaderSectionEndMarker);
    if (generatedLoaderStartIndex != INDEX_NONE && generatedLoaderEndIndex != INDEX_NONE) {
        // Append what was between the end of the property 
        // section and the beginning of the loading section
        int startIndex = propertiesSectionEndIndex + PropertiesSectionEndMarker.Len() + 1;
        int count = generatedLoaderStartIndex - startIndex;
        FString contentsBetweenPropertiesAndLoader = headerContents.Mid(startIndex, count);
        result.Append(contentsBetweenPropertiesAndLoader);

        startIndex = generatedLoaderStartIndex;
        count = (generatedLoaderEndIndex + LoaderSectionEndMarker.Len() + 1) - startIndex;
        FString currentLoaderSection = headerContents.Mid(startIndex, count);

        // Rebuild the loader section with the current blueprint path
        int widgetLineStartIndex = currentLoaderSection.Find(WidgetLineMarker);
        int endOfLineIndex = INDEX_NONE;
        if (!currentLoaderSection.RightChop(widgetLineStartIndex + 1).FindChar(TEXT('\n'), endOfLineIndex)) {
            UE_LOG(CodeGeneratorSub, Error, TEXT("No loader section start found in header"));
            return headerContents;
        }
        endOfLineIndex += widgetLineStartIndex + 1;
        FString newLoaderSection = currentLoaderSection.Left(widgetLineStartIndex);
        newLoaderSection.Append(WidgetLineMarker);
        newLoaderSection.Append("TEXT(\"" + blueprintReferencePath + "\");\n");
        FString remainingLoaderSection = currentLoaderSection.RightChop(endOfLineIndex + 1);
        newLoaderSection.Append(remainingLoaderSection);

        // Append the new loader section to the result
        result.Append(newLoaderSection);

        // Add the rest of the file
        FString afterLoaderSection = headerContents.RightChop(generatedLoaderEndIndex + LoaderSectionEndMarker.Len() + 1);
        result.Append(afterLoaderSection);
    } else {
        UE_LOG(CodeGeneratorSub, Error, TEXT("No loader section start found in header"));

        // Just write whatever was there before and don't update the loading section.
        result.Append(headerContents.RightChop(propertiesSectionEndIndex + PropertiesSectionEndMarker.Len() + 1));
    }

    return result;
}

FString UCodeGenerator::UpdateCppFile(const TArray<UWidget*>& namedWidgets, FString cppContents) {
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
    lookupTable->InitTable();
    UBlueprintSourceMap* sourceMap = NewObject<UBlueprintSourceMap>();
    sourceMap->LoadMapping(FPaths::GameSourceDir(), GetBlueprintSourceFilePath());
    TSet<FString> includes;
    for (UWidget* widget : namedWidgets) {
        UClass* widgetClass = GetFirstNonGeneratedParent(widget->GetClass());
        FString name = widgetClass->GetName();
        FString headerFilePath = lookupTable->GetIncludeFilePathFor(name);

        // If the header lookup is empty, check if it's a blueprint we made
        if (headerFilePath.IsEmpty()) {
            UBlueprint* blueprint = GetBlueprintForWidget(widget);
            if (blueprint != nullptr) {
                FBlueprintSourceModel entry = sourceMap->GetSourcePathsFor(blueprint);
                if (entry.IsValid()) {
                    FString gameSourceDir = FPaths::GameSourceDir();

                    // Make the header file path be the relative path of the header to the module source directory
                    FString relativeHeaderPath = entry.HeaderPath;
                    if (!FPaths::MakePathRelativeTo(relativeHeaderPath, *gameSourceDir)) {
                        UE_LOG(CodeGeneratorSub, Warning, TEXT("Could not find a relative path for header file %s"), *entry.HeaderPath);
                    } else {
                        // Remove the first part of the path because that will be the module name
                        int firstSlashIndex = -1;
                        if (relativeHeaderPath.FindChar(TEXT('/'), firstSlashIndex)) {
                            relativeHeaderPath = relativeHeaderPath.RightChop(firstSlashIndex + 1);
                        }

                        headerFilePath = relativeHeaderPath;
                    }
                }
            }
        }

        if (!headerFilePath.IsEmpty()) {
            if (!includes.Contains(headerFilePath)) {
                includes.Add(headerFilePath);
            }
        } else {
            UE_LOG(CodeGeneratorSub, Warning, TEXT("Could not find the include path for %s. You may need to restart the editor."), *name);
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
TArray<UWidget*> UCodeGenerator::GetNamedWidgets(const TArray<UWidget*> widgets) {
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

UClass* UCodeGenerator::GetFirstNonGeneratedParent(UClass* inputClass) {
    UClass* result = inputClass;
    while (result != nullptr && result->ClassGeneratedBy != nullptr) {
        result = result->GetSuperClass();
    }

    // This should never happen but just in case it does, do something reasonable
    if (result == nullptr) {
        // This is odd; there are no non-generated classes?
        // Just use the original class and let the user figure it out.
        // We hate our users here.
        UE_LOG(CodeGeneratorSub, Warning, TEXT("%s has no non-generated super classes! Using the base one."), *inputClass->GetName());
        result = inputClass;
    }

    return result;
}

UHeaderLookupTable* UCodeGenerator::GetHeaderLookupTable() {
    if (_headerLookupTable == nullptr) {
        _headerLookupTable = NewObject<UHeaderLookupTable>();
        _headerLookupTable->InitTable();        
    }
    return _headerLookupTable;
}

/**
 * Returns the blueprint that generated this widget or nullptr if it has none. 
 */
UBlueprint* UCodeGenerator::GetBlueprintForWidget(UWidget* widget) {  
    UClass* widgetClass = widget->GetClass();
    if (widgetClass != nullptr && widgetClass->ClassGeneratedBy != nullptr) {
        return Cast<UBlueprint>(widgetClass->ClassGeneratedBy);
    }

    return nullptr;
}

/**
 * Reports an issue to the user with a dialog that fades in/out.
 * @param message The message to show.
 * @param reason The reason for the notification.
 */
void UCodeGenerator::ShowNotification(FString message, ENotificationReason reason) {
    FNotificationInfo info(FText::FromString(message));
	info.FadeInDuration = 0.1f;
	info.FadeOutDuration = 0.5f;
	info.ExpireDuration = 5.0f;
	info.bUseThrobber = false;
	info.bUseSuccessFailIcons = true;
	info.bUseLargeFont = true;
	info.bFireAndForget = true;
	info.bAllowThrottleWhenFrameRateIsLow = false;

	auto notificationItem = FSlateNotificationManager::Get().AddNotification(info);
    if (reason == ENotificationReason::Error) {
	    notificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_Fail);
        UE_LOG(CodeGeneratorSub, Error, TEXT("%s"), *message);
    } else if (reason == ENotificationReason::Warning) {
	    notificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_None);
        UE_LOG(CodeGeneratorSub, Warning, TEXT("%s"), *message);
    } else if (reason == ENotificationReason::Success) {
	    notificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_Success);
        UE_LOG(CodeGeneratorSub, Display, TEXT("%s"), *message);
    }
	notificationItem->ExpireAndFadeout();
}

/**
 * @return Returns the absolute path to the blueprint source file.
 */
FString UCodeGenerator::GetBlueprintSourceFilePath() {
    return FPaths::Combine(FPaths::ProjectDir(), GetBlueprintSourceDirectory());
}
