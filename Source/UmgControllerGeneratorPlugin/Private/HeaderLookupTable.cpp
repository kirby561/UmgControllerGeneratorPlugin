#include "HeaderLookupTable.h"
#include "UhtManifestModel.h"
#include <functional>
#include "Misc/Paths.h" 	
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h" 	
#include "Misc/ConfigCacheIni.h"

DEFINE_LOG_CATEGORY_STATIC(HeaderLookupTableSub, Log, All)
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

void UHeaderLookupTable::InitTable() {
    // Get all the various info we need to find the UHT manifest
    FString platform = TEXT(TOSTRING(UBT_COMPILED_PLATFORM));
    FString target = TEXT(TOSTRING(UE_TARGET_NAME));
    FString ubtTarget = TEXT(TOSTRING(UBT_COMPILED_TARGET));
    FString intermediateDir = FPaths::ProjectIntermediateDir();
    EBuildConfiguration buildConfig = FApp::GetBuildConfiguration();
    FString buildConfigStr = LexToString(buildConfig);

    // Build the UHT manifest file path
    FString uhtManifestFileName = target + TEXT(".uhtmanifest");
    FString uhtPath = FPaths::Combine(intermediateDir, TEXT("Build"), platform, target, buildConfigStr, uhtManifestFileName);

    // Load the file
    FString uhtManifestContents;
    if (!FFileHelper::LoadFileToString(uhtManifestContents, *uhtPath)) {
        UE_LOG(HeaderLookupTableSub, Error, TEXT("Failed to load the UHT Manifest for header file lookup support."));
        return;
    }

    // Parse into JSON
    FUhtManifestModel manifestModel;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(uhtManifestContents, &manifestModel, 0, 0)) {
        UE_LOG(HeaderLookupTableSub, Error, TEXT("The UHT Manifest file was not deserialized from JSON properly. Could the file be corrupt?"));
        return;
    }

    // Build the table from the includes available in the dependent modules
    _lookupTable.Empty();
    double timeBefore = FPlatformTime::Seconds();
    for (const FUhtModuleModel& moduleModel : manifestModel.Modules) {
        // Function to use to add each type of header array
        std::function addHeaders = [this, &moduleModel] (const TArray<FString>& headerArray) {
            FString moduleBasePath = moduleModel.BaseDirectory;
            for (const FString& headerPath : headerArray) {
                // Use the header name as the file name that maps to it.
                // This is an assumption but will work ~100% of the time.
                FString className = FPaths::GetBaseFilename(headerPath);

                if (headerPath.StartsWith(moduleBasePath)) {
                    FString abbreviatedHeaderPath = headerPath.RightChop(moduleBasePath.Len());

                    // If it starts with a slash, remove it
                    const FString backslash = TEXT("\\");
                    if (abbreviatedHeaderPath.StartsWith(TEXT("\\"))) {
                        abbreviatedHeaderPath = abbreviatedHeaderPath.RightChop(backslash.Len());
                    }

                    // If it's in a public folder, omit the public part
                    const FString publicPrefix = TEXT("Public\\");
                    if (abbreviatedHeaderPath.StartsWith(publicPrefix)) {
                        abbreviatedHeaderPath = abbreviatedHeaderPath.RightChop(publicPrefix.Len());
                    }

                    // If it's in a private folder, omit the private part
                    const FString privatePrefix = TEXT("Private\\");
                    if (abbreviatedHeaderPath.StartsWith(privatePrefix)) {
                        abbreviatedHeaderPath = abbreviatedHeaderPath.RightChop(privatePrefix.Len());
                    }

                    // Replace all backslashes with forward slashes
                    const FString forwardSlash = TEXT("/");
                    abbreviatedHeaderPath = abbreviatedHeaderPath.Replace(*backslash, *forwardSlash);

                    if (!_lookupTable.Contains(className)) {
                        _lookupTable.Add(className, abbreviatedHeaderPath);
                    } else {
                        UE_LOG(HeaderLookupTableSub, Warning, TEXT("Header file %s was already added for module %s"), *abbreviatedHeaderPath, *moduleModel.Name);    
                    }
                } else {
                    UE_LOG(HeaderLookupTableSub, Warning, TEXT("Header file does not start with the base path at %s in module %s"), *headerPath, *moduleModel.Name);
                }
            }
        };
        addHeaders(moduleModel.PrivateHeaders);
        addHeaders(moduleModel.InternalHeaders);
        addHeaders(moduleModel.PublicHeaders);
    }
    double timeAfter = FPlatformTime::Seconds();
    double elapsedTimeMs = (timeAfter - timeBefore) * 1000.0;
    UE_LOG(HeaderLookupTableSub, Display, TEXT("Initialized header lookup table in %f ms"), elapsedTimeMs);
}

FString UHeaderLookupTable::GetIncludeFilePathFor(FString className) {
    if (_lookupTable.Contains(className)) {
        return _lookupTable[className];
    }
    return FString();
}
