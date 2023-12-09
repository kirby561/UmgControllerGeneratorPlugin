#pragma once

#include "CoreMinimal.h"
#include "BlueprintSourceMap.generated.h"

USTRUCT()
struct FBlueprintSourceModel {
    GENERATED_BODY()

    UPROPERTY() // Path to a header file relative to the source directory of the project
    FString HeaderPath = TEXT("");

    UPROPERTY() // Path to a cpp file relative to the source directory of the project
    FString CppPath = TEXT("");

    bool IsValid() { return !HeaderPath.IsEmpty() && !CppPath.IsEmpty(); }
};

USTRUCT()
struct FBlueprintSourceMapModel {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FString, FBlueprintSourceModel> BlueprintSourceMap;
};

/**
 * This is a simple mapping from Blueprint Reference Path to
 * the header and cpp file of its parent class (for auto-generated classes only). 
 */
UCLASS()
class UBlueprintSourceMap : public UObject {
    GENERATED_BODY()

public:
    UBlueprintSourceMap(const FObjectInitializer& objectInitializer) : UObject(objectInitializer) { }
    virtual ~UBlueprintSourceMap() { }

    /**
     * Loads the mappings from the file in sourceMapDir.
     * The mappings should be relative to the given project source directory.
     */
    void LoadMapping(FString projectSourceDir, FString sourceMapDir);

    /**
     * Adds a mapping for the given blueprint.
     * @param blueprint The blueprint who's parent corresponds to the given files.
     * @param fullHeaderPath The full path to the header file.
     * @param fullCppPath The full path to the cpp file.
     */
    void AddMapping(class UBlueprint* blueprint, FString fullHeaderPath, FString fullCppPath);

    /**
     * Returns a structure containing the source files corresponding to the given blueprint.
     * Note: While we store the mappings as relative, this will return absolute paths by default.
     *       use absolutePaths = false to get the relative ones.
     */
    FBlueprintSourceModel GetSourcePathsFor(class UBlueprint* blueprint, bool absolutePaths = true);

    /**
     * Saves the current mapping to disk. 
     */
    void SaveMapping();

    /**
     * If you move or rename a file, call UpdateMappings to try to correct them all.
     * Note: This assumes that the name of each file is the name of the class without
     *       the U prefix. If it's not, you will need to manually update the mapping in
     *       the BlueprintSourceMap.json file.
     */
    void UpdateMappings(const TArray<UBlueprint*>& files, FString nameSuffix);

private: // Methods
    FString GetFilePath();

private:
    UPROPERTY()
    FBlueprintSourceMapModel _sourceMap;

    FString _projectSourceDirectory;
    FString _sourceMapDir;

    // This class' mapping is serialized to the following file in the plugin directory.
    const static inline FString SourceMapFileName = TEXT("BlueprintSourceMap.json");
};
