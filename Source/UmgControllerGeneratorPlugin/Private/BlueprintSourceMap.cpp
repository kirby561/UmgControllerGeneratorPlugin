#include "BlueprintSourceMap.h"
#include "WidgetBlueprint.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(BlueprintSourceMapSub, Log, All)

/**
 * Helper class to build a file map for us when updating header references. 
 */
class FFileMapBuilder : public IPlatformFile::FDirectoryVisitor {
public:
    // A map of file name (with extension) to full file path.
    TMap<FString, FString> FileMap;

    virtual bool Visit(const TCHAR* filenameOrDirectory, bool isDirectory) override {
        if (!isDirectory) {
            FString name = FPaths::GetCleanFilename(filenameOrDirectory);
            if (!FileMap.Contains(name)) {
                FileMap.Add(name, filenameOrDirectory);
            } else {
                UE_LOG(BlueprintSourceMapSub, Warning, TEXT("Duplicate filename when building file map: %s at %s"), *name, filenameOrDirectory);
            }
        }
        return true; // keep going
    }
};

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

bool UBlueprintSourceMap::SaveMapping() {
    FString jsonString = TEXT("");
    if (FJsonObjectConverter::UStructToJsonObjectString(_sourceMap, jsonString)) {
        FString filePath = GetFilePath();

        // Make sure the directory exists and create it if it doesn't
        FString directory = FPaths::GetPath(filePath);
        if (!FPaths::DirectoryExists(directory)) {
            IFileManager& fileManager = IFileManager::Get();

            if (!fileManager.MakeDirectory(*directory, true)) {
                UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to save blueprint source mappings to file. The directory did not exist and we could not create it: %s!"), *filePath);
                return false;
            }
        }

        if (!FFileHelper::SaveStringToFile(jsonString, *filePath)) {
            UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to save blueprint source mappings to file %s!"), *filePath);
            return false;
        }
    } else {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to convert blueprint source mappings to json!"));
        return false;
    }

    return true;
}

bool UBlueprintSourceMap::UpdateMappings(const TArray<UBlueprint*>& filesToUpdate, FString nameSuffix) {
    // Build a filemap of the source directory
    IFileManager& fileManager = IFileManager::Get();
    FFileMapBuilder mapBuilder;
    if (!fileManager.IterateDirectoryRecursively(*_projectSourceDirectory, mapBuilder)) {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("UpdateMappings: Could not iterate directory at %s"), *_projectSourceDirectory);
        return false;
    }

    for (UBlueprint* blueprint : filesToUpdate) {
        // Get the class name
        FString name = blueprint->GetName();
	    FString wbpPrefix = TEXT("WBP_");
	    if (name.StartsWith(wbpPrefix)) {
		    name = name.RightChop(wbpPrefix.Len());
	    }
        name += nameSuffix;

        FString expectedHeaderName = name + TEXT(".h");
        FString expectedCppName = name + TEXT(".cpp");

        // First check if it's already in the right place
        FString pathName = blueprint->GetPathName();
        bool cppExists = false;
        bool headerExists = false;
        FBlueprintSourceModel newPaths;
        if (_sourceMap.BlueprintSourceMap.Contains(pathName)) {
            FBlueprintSourceModel sourcePaths = _sourceMap.BlueprintSourceMap[pathName];

            FString fullHeaderPath = FPaths::Combine(_projectSourceDirectory, sourcePaths.HeaderPath);
            if (fileManager.FileExists(*fullHeaderPath)) {
                headerExists = true;
                newPaths.HeaderPath = sourcePaths.HeaderPath;
            }

            FString fullCppPath = FPaths::Combine(_projectSourceDirectory, sourcePaths.CppPath);
            if (fileManager.FileExists(*fullCppPath)) {
                cppExists = true;
                newPaths.CppPath = sourcePaths.CppPath;
            }
        }

        // If one of them needs an update, update it.
        // Note if one of them is still there, it will already be in newPaths.
        if (!headerExists || !cppExists) {
            if (!headerExists) {
                if (mapBuilder.FileMap.Contains(expectedHeaderName)) {
                    FString newHeaderPath = mapBuilder.FileMap[expectedHeaderName];
                    if (!FPaths::MakePathRelativeTo(newHeaderPath, *_projectSourceDirectory)) {
                        UE_LOG(BlueprintSourceMapSub, Error, TEXT("UpdateMappings: Could not find a relative path for header file %s"), *newHeaderPath);
                        continue;
                    }
                    newPaths.HeaderPath = newHeaderPath;
                }
            }

            if (!cppExists) {
                if (mapBuilder.FileMap.Contains(expectedCppName)) {
                    FString newCppPath = mapBuilder.FileMap[expectedCppName];
                    if (!FPaths::MakePathRelativeTo(newCppPath, *_projectSourceDirectory)) {
                        UE_LOG(BlueprintSourceMapSub, Error, TEXT("UpdateMappings: Could not find a relative path for cpp file %s"), *newCppPath);
                        continue;
                    }
                    newPaths.CppPath = newCppPath;
                }
            }

            // Add the updated entry, replacing the old one if needed
            if (_sourceMap.BlueprintSourceMap.Contains(pathName))
                _sourceMap.BlueprintSourceMap.Remove(pathName);
            _sourceMap.BlueprintSourceMap.Add(pathName, newPaths);
        }
    }

    // Take this opportunity to check that all the blueprints in our mapping
    // still exist as well and remove them from the table if they do not.
    TArray<FString> keysToRemove;
    for (auto& entry : _sourceMap.BlueprintSourceMap) {
        if (!DoesBlueprintExist(entry.Key)) {
            keysToRemove.Add(entry.Key);
        }
    }

    for (const FString& key : keysToRemove) {
        _sourceMap.BlueprintSourceMap.Remove(key);
    }

    // Save
    return SaveMapping();
}

FString UBlueprintSourceMap::GetFilePath() {
    return FPaths::Combine(_sourceMapDir, SourceMapFileName);
}

/**
 * Returns true if a blueprint with the given reference path exists and false if it does not.
 * @param blueprintPath The reference path to the blueprint (For example "/Game/SomeSubFolder/WBP_AnotherExample.WBP_AnotherExample")
 */
bool UBlueprintSourceMap::DoesBlueprintExist(const FString& blueprintPath) {
    UBlueprint* blueprintObject = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *blueprintPath));
    return (blueprintObject != nullptr);
}
