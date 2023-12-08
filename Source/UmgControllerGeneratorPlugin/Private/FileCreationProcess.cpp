#include "FileCreationProcess.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Blueprint/UserWidget.h"
#include "GameProjectUtils.h"
#include "AddToProjectConfig.h"
#include "ILiveCodingModule.h"

DEFINE_LOG_CATEGORY_STATIC(FileCreationProcessSub, Log, All)

// This is defined in a private class so we need to define it
enum class EClassDomain : uint8 { Blueprint, Native };

UFileCreationProcess::UFileCreationProcess(const FObjectInitializer& objectInitializer) : UObject(objectInitializer) {
    ILiveCodingModule* liveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
    if (liveCoding != nullptr) {
        liveCoding->GetOnPatchCompleteDelegate().AddUObject(this, &UFileCreationProcess::OnLiveCodingPatchComplete);
    }
}

void UFileCreationProcess::Start(
        FString defaultName,
        std::function<void(FString headerFilePath, FString cppFilePath)> onFilesCreated,
        std::function<void(FString errorDescription)> onErrorOrCancelled) {
    _onFilesCreated = onFilesCreated;
    _onErrorOrCancelled = onErrorOrCancelled;

    // Abandon any previous request (shouldn't be possible anyway since even if you close the dialog the compile continues)
    if (_isWaitingForCompile) {
        _isWaitingForCompile = false;
        _cppPath = TEXT("");
        _headerPath = TEXT("");
        _onPatchingComplete = nullptr;
    }
    
    // ?? TODO: Maybe pass the window in so we know when it closed since there's no event to get notified
    //          if cancel was pressed. This is what OpenAddToProjectDialog does internally:
    /*
	TSharedRef<SWindow> AddCodeWindow =
		SNew(SWindow)
		.Title( WindowTitle )
		.ClientSize( WindowSize )
		.SizingRule( ESizingRule::FixedSize )
		.SupportsMinimize(false) .SupportsMaximize(false);
    */

    // This is dumb but internally if the SNewClassDialog is given an empty prefix,
    // it will add "My" at the beginning of the class name, which we don't want.
    // If the default name is empty, it puts "Class" in, which we also don't want.
    // Since we don't necessarily have a prefix, make the first letter of the name be the prefix.
    // If we only have one character then the My can't be helped. A longer term fix
    // for all this would be to just create our own dialog class.
    FString hackDefaultPrefix = TEXT("");
    FString hackDefaultName = defaultName;
    if (defaultName.Len() > 1) {
        hackDefaultPrefix = defaultName.Left(1);
        hackDefaultName = defaultName.RightChop(1);
    }

    // Show the new class dialog
    FString sourcePath = FPaths::GameSourceDir();
    FAddToProjectConfig config;
	FOnAddedToProject delegate;
	delegate.BindUObject(this, &UFileCreationProcess::OnFileAdded);
	config
		.WindowTitle(FText::FromString(TEXT("Pick header/cpp location")))
        .DefaultClassPrefix(hackDefaultPrefix)
		.DefaultClassName(hackDefaultName)
        .ParentClass(UUserWidget::StaticClass())
		.InitialPath(sourcePath)
		.OnAddedToProject(delegate);
	GameProjectUtils::OpenAddToProjectDialog(config, EClassDomain::Native);

    // ...Afterwards we wait for an OnFileAdded. There is no callback we can listen for
    //    if the user cancels the dialog without modifying the engine source or potentially
    //    passing in our own window (although this could also have a race between when the
    //    window closes and when OnFileAdded gets called)
}

void UFileCreationProcess::OnFileAdded(const FString& className, const FString& classPath, const FString& moduleName) {
    // For some reason it only returns where the header file is placed, not the cpp but it is predictable so we
    // can determine where the cpp file was based on if we're in the Public folder or not
    UE_LOG(FileCreationProcessSub, Display, TEXT("OnFileAdded: %s, %s, %s"), *className, *classPath, *moduleName);

    FString headerPath = classPath + className + TEXT(".h");
    FString cppPath = classPath + className + TEXT(".cpp");

    IFileManager& fileManager = IFileManager::Get();
    if (!fileManager.FileExists(*headerPath)) {
        // We expect the header file to be there, this is an error
        FString errorDescription = FString::Printf(TEXT("Unable to find the newly created header at %s"), *headerPath);
        UE_LOG(FileCreationProcessSub, Error, TEXT("%s"), *errorDescription);
        _onErrorOrCancelled(errorDescription);
        return;
    }

    if (!fileManager.FileExists(*cppPath)) {
        FString classPathWithoutTrailingSlash = classPath;
        if (classPathWithoutTrailingSlash.EndsWith(TEXT("\\")) || classPathWithoutTrailingSlash.EndsWith(TEXT("/"))) {
            classPathWithoutTrailingSlash = classPathWithoutTrailingSlash.LeftChop(1);
        }
        // If the cpp doesn't exist, it may be in the private folder - check there too
        FString parentDir = FPaths::GetPath(classPathWithoutTrailingSlash);
        FString privateCppPath = FPaths::Combine(parentDir, TEXT("Private"), className + TEXT(".cpp"));

        if (fileManager.FileExists(*privateCppPath)) {
            cppPath = privateCppPath;
        } else {
            // We expect the cpp file to be in one of those 2 locations, this is an error
            FString errorDescription = FString::Printf(TEXT("Unable to find the newly created cpp at %s"), *cppPath);
            UE_LOG(FileCreationProcessSub, Error, TEXT("%s"), *errorDescription);
            _onErrorOrCancelled(errorDescription);
            return;
        }
    }

    // Now we have a cpp/header file, wait for the live coding to finish
    // (if it's enabled) and notify the caller the classes are created
    ILiveCodingModule* liveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
    if (liveCoding != nullptr && liveCoding->AutomaticallyCompileNewClasses()) {
        _isWaitingForCompile = true;
        _cppPath = cppPath;
        _headerPath = headerPath;
        _onPatchingComplete = [this]() { // Callback when patching is complete. Note: Even if you close the window it continues in the background.
            UE_LOG(FileCreationProcessSub, Display, TEXT("OnPatchingComplete: %s, %s"), *_headerPath, *_cppPath);

            // Notify the caller that the patch is done
            _onFilesCreated(_headerPath, _cppPath);

            // Clear state
            _isWaitingForCompile = false;
            _cppPath = TEXT("");
            _headerPath = TEXT("");
            _onPatchingComplete = nullptr;
        };
    } else {
        // Live coding isn't enabled so we can callback right away
        _onFilesCreated(_headerPath, _cppPath);
    }
}

void UFileCreationProcess::OnLiveCodingPatchComplete() {
    UE_LOG(FileCreationProcessSub, Display, TEXT("OnLiveCodingPatchComplete"));
    if (_onPatchingComplete) {
        _onPatchingComplete();
    }
}
