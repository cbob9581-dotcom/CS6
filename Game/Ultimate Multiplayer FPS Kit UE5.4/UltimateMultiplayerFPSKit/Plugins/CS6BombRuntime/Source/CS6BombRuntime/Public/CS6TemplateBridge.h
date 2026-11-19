#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CS6TemplateBridge.generated.h"

UCLASS()
class CS6BOMBRUNTIME_API UCS6TemplateBridge : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="CS6|Development")
    static TMap<FString,FString> ReadTemplateProperties(UObject* Object);
    UFUNCTION(BlueprintCallable, Category="CS6|Development", meta=(WorldContext="Context"))
    static bool BuildNavigation(UObject* Context);
    UFUNCTION(BlueprintCallable, Category="CS6|Development", meta=(WorldContext="Context"))
    static FVector FindGround(UObject* Context, FVector Start, FVector End);
    UFUNCTION(BlueprintCallable, Category="CS6|Development")
    static TArray<FString> InspectBlueprintGraph(UObject* Blueprint, FString GraphName);
    UFUNCTION(BlueprintCallable, Category="CS6|Development")
    static TMap<FString,int32> AuditLoadedBlueprintLinks();
    UFUNCTION(BlueprintCallable, Category="CS6|Development")
    static bool RebuildBlueprint(UObject* Blueprint);
    UFUNCTION(BlueprintCallable, Category="CS6|Development")
    static bool RebuildMetaSoundEditorGraph(UObject* Sound);
    UFUNCTION(BlueprintCallable, Category="CS6|Development")
    static bool UseComplexStaticCollision(class UStaticMesh* Mesh);
    UFUNCTION(BlueprintCallable, Category="CS6|Development", meta=(WorldContext="Context"))
    static TArray<class UPrimitiveComponent*> GetPawnBlockers(UObject* Context,FVector Location);
};
