#include "BlueprintSourceMap.h"
#include "WidgetBlueprint.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h" 

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

void UBlueprintSourceMap::SaveMapping() {
    FString jsonString = TEXT("");
    if (FJsonObjectConverter::UStructToJsonObjectString(_sourceMap, jsonString)) {
        FString filePath = GetFilePath();
        if (!FFileHelper::SaveStringToFile(jsonString, *filePath)) {
            UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to save blueprint source mappings to file %s!"), *filePath);
        }
    } else {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("Failed to convert blueprint source mappings to json!"));
    }
}

void UBlueprintSourceMap::UpdateMappings(const TArray<UBlueprint*>& filesToUpdate, FString nameSuffix) {
    // Build a filemap of the source directory
    IFileManager& fileManager = IFileManager::Get();
    FFileMapBuilder mapBuilder;
    if (!fileManager.IterateDirectoryRecursively(*_projectSourceDirectory, mapBuilder)) {
        UE_LOG(BlueprintSourceMapSub, Error, TEXT("UpdateMappings: Could not iterate directory at %s"), *_projectSourceDirectory);
        return;
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

    // ?? TODO: This would be a good place to make sure all the existing mappings still exist and
    //          remove any that aren't valid anymore.

    // Save
    SaveMapping();
}

FString UBlueprintSourceMap::GetFilePath() {
    return FPaths::Combine(_sourceMapDir, SourceMapFileName);
}
