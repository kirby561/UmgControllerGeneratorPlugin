#include "BlueprintSourceMap.h"
#include "WidgetBlueprint.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h" 

DEFINE_LOG_CATEGORY_STATIC(BlueprintSourceMapSub, Log, All)

void UBlueprintSourceMap::LoadMapping(FString projectSourceDir, FString sourceMapDir) {
    _projectSourceDirectory = projectSourceDir;
    _sourceMapDir = sourceMapDir;

    FString expectedFilePath = GetFilePath();

    IFileManager& fileManager = IFileManager::Get();
    if (fileManager.FileExists(*expectedFilePath)) {
        FString fileContents;
        if (!FFileHelper::LoadFileToString(fileContents, *expectedFilePath)) {
            // The file is there but we couldn't load it. That's strange.
            UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to load %s for automatic update support."), *expectedFilePath);
            return;
        }

        // Parse into JSON
        _sourceMap.BlueprintSourceMap.Empty();
        if (!FJsonObjectConverter::JsonObjectStringToUStruct(fileContents, &_sourceMap, 0, 0)) {
            UE_LOG(BlueprintSourceMapSub, Error, TEXT("The blueprint source map was not deserialized from JSON properly. Could the file be corrupt?"));
            return;
        }
    }
}

void UBlueprintSourceMap::AddMapping(UBlueprint* blueprint, FString fullHeaderPath, FString fullCppPath) {
    FString relativeHeaderPath = fullHeaderPath;
    if (!FPaths::MakePathRelativeTo(relativeHeaderPath, *_projectSourceDirectory)) {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("Could not find a relative path for header file %s"), *fullHeaderPath);
        return;
    }

    FString relativeCppPath = fullCppPath;
    if (!FPaths::MakePathRelativeTo(relativeCppPath, *_projectSourceDirectory)) {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("Could not find a relative path for CPP file %s"), *fullCppPath);
        return;
    }

    FBlueprintSourceModel model;
    model.HeaderPath = relativeHeaderPath;
    model.CppPath = relativeCppPath;

    FString blueprintPath = blueprint->GetPathName();
    if (_sourceMap.BlueprintSourceMap.Contains(blueprintPath)) {
        _sourceMap.BlueprintSourceMap.Remove(blueprintPath);
        UE_LOG(BlueprintSourceMapSub, Warning, TEXT("Blueprint source map already included a mapping for %s"), *blueprintPath);
    }
    _sourceMap.BlueprintSourceMap.Add(blueprintPath, model);
}

FBlueprintSourceModel UBlueprintSourceMap::GetSourcePathsFor(UBlueprint* blueprint, bool absolutePaths) {
    FBlueprintSourceModel result;

    FString blueprintPath = blueprint->GetPathName();
    if (_sourceMap.BlueprintSourceMap.Contains(blueprintPath)) {
        result = _sourceMap.BlueprintSourceMap[blueprintPath];

        if (absolutePaths) {
            result.HeaderPath = FPaths::Combine(_projectSourceDirectory, result.HeaderPath);
            result.CppPath = FPaths::Combine(_projectSourceDirectory, result.CppPath);
        }
    } else {
        UE_LOG(BlueprintSourceMapSub, Warning, TEXT("Blueprint source map has no entry for %s"), *blueprintPath);
    }
    
    return result;
}

void UBlueprintSourceMap::SaveMapping() {
    FString jsonString = TEXT("");
    if (FJsonObjectConverter::UStructToJsonObjectString(
            _sourceMap,
            jsonString // out param
    )) {
        FString filePath = GetFilePath();
        if (!FFileHelper::SaveStringToFile(
            jsonString,
            *filePath
        )) {
            UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to save blueprint source mappings to file %s!"), *filePath);
        }
    } else {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to convert blueprint source mappings to json!"));
    }
}

void UBlueprintSourceMap::UpdateMappings() {
    // ?? TODO
}

FString UBlueprintSourceMap::GetFilePath() {
    return FPaths::Combine(_sourceMapDir, SourceMapFileName);
}
