#include "CS6TemplateBridge.h"
#include "UObject/UnrealType.h"
#include "NavigationSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "HAL/PlatformProcess.h"
#include "Async/TaskGraphInterfaces.h"
#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UObjectIterator.h"
#include "MetasoundEditorSubsystem.h"
#include "MetasoundUObjectRegistry.h"
#include "MetasoundAssetBase.h"
#include "EdGraph/EdGraphSchema.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

bool UCS6TemplateBridge::UseComplexStaticCollision(UStaticMesh* Mesh) {
#if WITH_EDITOR
    if(!Mesh || !Mesh->GetBodySetup())return false;
    auto* Body=Mesh->GetBodySetup();Body->Modify();
    Body->CollisionTraceFlag=CTF_UseComplexAsSimple;
    Body->InvalidatePhysicsData();Body->CreatePhysicsMeshes();Mesh->MarkPackageDirty();return true;
#else
    return false;
#endif
}
TArray<UPrimitiveComponent*> UCS6TemplateBridge::GetPawnBlockers(UObject* Context,FVector Location) {
    TArray<UPrimitiveComponent*> Result;
    auto* World=GEngine->GetWorldFromContextObject(Context,EGetWorldErrorMode::ReturnNull);
    if(!World)return Result;
    TArray<FOverlapResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CS6CapsuleValidation),false);
    World->OverlapMultiByChannel(Hits,Location,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96),Params);
    for(const auto& Hit:Hits)if(auto* C=Hit.GetComponent())if(C->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Block)Result.AddUnique(C);
    return Result;
}

bool UCS6TemplateBridge::RebuildMetaSoundEditorGraph(UObject* Sound) {
#if WITH_EDITOR
    auto* Asset=Metasound::IMetasoundUObjectRegistry::Get().GetObjectAsAssetBase(Sound);if(!Asset)return false;
    // The frontend document owns audio behavior. Recreate only its obsolete editor graph cache.
    if(auto* OldGraph=Asset->GetGraph()) {
        OldGraph->SetFlags(RF_Transient);
        if(!OldGraph->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors | REN_NonTransactional))return false;
    }
    auto* GraphProperty=FindFProperty<FObjectPropertyBase>(Sound->GetClass(),TEXT("Graph"));
    if(!GraphProperty)return false;
    // Restore the factory's uninitialized editor-cache state. SetGraph itself only accepts a populated graph.
    GraphProperty->SetObjectPropertyValue_InContainer(Sound,nullptr);
    Asset->GetModifyContext().SetDocumentModified();
    auto& Subsystem=UMetaSoundEditorSubsystem::GetChecked();
    Subsystem.InitEdGraph(*Sound);
    Subsystem.RegisterGraphWithFrontend(*Sound,true);
    return Asset->GetGraph() && Asset->GetGraph()->Nodes.Num()>0;
#else
    return false;
#endif
}

TMap<FString,int32> UCS6TemplateBridge::AuditLoadedBlueprintLinks() {
    TMap<FString,int32> Result;
#if WITH_EDITOR
    for(TObjectIterator<UBlueprint> It;It;++It) {
        if(!It->GetPathName().StartsWith(TEXT("/Game/"))) continue;
        TArray<UEdGraph*> Graphs; It->GetAllGraphs(Graphs);int32 Broken=0;
        for(auto* Graph:Graphs) for(UEdGraphNode* Node:Graph->Nodes) if(Node) for(auto* Pin:Node->Pins) if(Pin)
            for(auto* Link:Pin->LinkedTo) if(!Link || !Link->LinkedTo.Contains(Pin)) ++Broken;
        if(Broken)Result.Add(It->GetPathName(),Broken);
    }
#endif
    return Result;
}
bool UCS6TemplateBridge::RebuildBlueprint(UObject* Object) {
#if WITH_EDITOR
    auto* BP=Cast<UBlueprint>(Object);if(!BP)return false;
    FBlueprintEditorUtils::RefreshAllNodes(BP);
    FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::SkipGarbageCollection);
    return BP->Status==BS_UpToDate || BP->Status==BS_UpToDateWithWarnings;
#else
    return false;
#endif
}

TArray<FString> UCS6TemplateBridge::InspectBlueprintGraph(UObject* Object, FString GraphName) {
    TArray<FString> Result;
#if WITH_EDITOR
    auto* BP=Cast<UBlueprint>(Object);
    if (!BP) return Result;
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    for (auto* Graph: Graphs) {
        if (!GraphName.IsEmpty() && !Graph->GetName().Contains(GraphName)) continue;
        for (UEdGraphNode* Node: Graph->Nodes) {
            if (!Node) continue;
            FString Line=Graph->GetName()+TEXT(" | ")+Node->NodeGuid.ToString()+TEXT(" | ")+Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
            for (auto* Pin: Node->Pins) {
                if (!Pin) continue;
                Line+=TEXT("\n  ")+Pin->PinName.ToString()+TEXT("=")+Pin->DefaultValue;
                if(Pin->DefaultObject) Line+=Pin->DefaultObject->GetPathName();
                for(auto* Link:Pin->LinkedTo) if(Link && Link->GetOwningNode())
                    Line+=TEXT(" -> ")+Link->GetOwningNode()->NodeGuid.ToString()+TEXT(".")+Link->PinName.ToString();
            }
            Result.Add(Line);
        }
    }
#endif
    return Result;
}

TMap<FString,FString> UCS6TemplateBridge::ReadTemplateProperties(UObject* Object) {
    TMap<FString,FString> Result;
    if (!IsValid(Object)) return Result;
    for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It) {
        FString Value;
        It->ExportText_InContainer(0,Value,Object,Object,Object,PPF_None);
        Result.Add(It->GetName(),Value);
        UEnum* Enum=nullptr;
        if(auto* Byte=CastField<FByteProperty>(*It))Enum=Byte->Enum;
        if(auto* EnumProperty=CastField<FEnumProperty>(*It))Enum=EnumProperty->GetEnum();
        if(Enum) {
            FString Labels;
            for(int32 I=0;I<Enum->NumEnums();++I)Labels+=FString::Printf(TEXT("%s=%lld:%s; "),*Enum->GetNameStringByIndex(I),Enum->GetValueByIndex(I),*Enum->GetDisplayNameTextByIndex(I).ToString());
            Result.Add(TEXT("Enum:")+It->GetName(),Labels);
        }
    }
    for (TFieldIterator<UFunction> It(Object->GetClass()); It; ++It) {
        FString Args;
        for (TFieldIterator<FProperty> P(*It); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
            Args += P->GetName()+TEXT(":")+P->GetCPPType()+TEXT("; ");
        Result.Add(TEXT("Function:")+It->GetName(),Args);
    }
    return Result;
}
FVector UCS6TemplateBridge::FindGround(UObject* Context,FVector Start,FVector End) {
    UWorld* World=GEngine->GetWorldFromContextObject(Context,EGetWorldErrorMode::ReturnNull);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CS6SpawnGround),true);
    if (World && World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params) && Hit.ImpactNormal.Z>0.8)
        return Hit.ImpactPoint;
    return FVector(0,0,-1000000000);
}
bool UCS6TemplateBridge::BuildNavigation(UObject* Context) {
#if WITH_EDITOR
    UWorld* World=GEngine->GetWorldFromContextObject(Context,EGetWorldErrorMode::ReturnNull);
    auto* Nav=World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!Nav) return false;
    Nav->Build();
    const double Deadline=FPlatformTime::Seconds()+90.0;
    do {
        Nav->Tick(0.05f);
        FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
        if (!Nav->IsNavigationBuildInProgress()) return true;
        FPlatformProcess::Sleep(0.01f);
    } while (FPlatformTime::Seconds()<Deadline);
#endif
    return false;
}
