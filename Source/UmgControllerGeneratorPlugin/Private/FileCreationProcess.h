#pragma once

#include "CoreMinimal.h"
#include <functional>
#include "FileCreationProcess.generated.h"

UCLASS()
class UFileCreationProcess : public UObject {
    GENERATED_BODY()

public:
    UFileCreationProcess(const FObjectInitializer& objectInitializer);
    virtual ~UFileCreationProcess() {}

    void Start(
        FString defaultName,
        std::function<void(FString headerFilePath, FString cppFilePath)> onFilesCreated,
        std::function<void(FString errorDescription)> onErrorOrCancelled);

private: // Callbacks
    UFUNCTION()
    void OnFileAdded(const FString& className, const FString& classPath, const FString& moduleName);

    UFUNCTION()
    void OnLiveCodingPatchComplete();

private:
    std::function<void(FString headerFilePath, FString cppFilePath)> _onFilesCreated;
    std::function<void(FString errorDescription)> _onErrorOrCancelled;

    // State for waiting for live coding compile to complete.
    // After adding a class, compilation immediately kicks off
    // if configured, so wait for that specific compile to complete
    // before notifying the caller the class has been created.
    bool _isWaitingForCompile = false;
    std::function<void()> _onPatchingComplete; // Our own delegate we set depending on what we want to do when patching is complete
    FString _cppPath;
    FString _headerPath;
};
