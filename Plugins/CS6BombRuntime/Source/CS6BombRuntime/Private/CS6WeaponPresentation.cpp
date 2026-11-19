#include "CS6WeaponPresentation.h"
#include "CS6EconomyTypes.h"
#include "CS6TemplateAccess.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
using namespace CS6TemplateAccess;
UCS6WeaponPresentation::UCS6WeaponPresentation(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;}
bool UCS6WeaponPresentation::Initialize(const FCS6WeaponOffer& Offer) {
    FP=Cast<USkeletalMeshComponent>(Object(GetOwner(),TEXT("FPMesh")));TP=Cast<USkeletalMeshComponent>(Object(GetOwner(),TEXT("TPMesh")));
    if(!Offer.VisualMesh || !FP || !TP)return false;
    for(bool First:{true,false}) {
        auto* Skeleton=First?FP.Get():TP.Get();auto* Asset=Skeleton->GetSkeletalMeshAsset();if(!Asset || Asset->GetRefSkeleton().GetNum()==0)return false;
        Skeleton->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        const auto& Ref=Asset->GetRefSkeleton();const FName RootBone=Ref.GetBoneName(0);
        auto* Whole=NewObject<UStaticMeshComponent>(GetOwner());GetOwner()->AddInstanceComponent(Whole);
        Whole->SetStaticMesh(Offer.VisualMesh);Whole->SetCollisionEnabled(ECollisionEnabled::NoCollision);Whole->SetCanEverAffectNavigation(false);
        Whole->SetOnlyOwnerSee(First);Whole->SetOwnerNoSee(!First);Whole->SetCastShadow(!First);
        Whole->ComponentTags.Add(First?TEXT("CS6.WholeFP"):TEXT("CS6.WholeTP"));
        Whole->AttachToComponent(Skeleton,FAttachmentTransformRules::KeepRelativeTransform,RootBone);
        Whole->SetRelativeTransform(Offer.VisualOffset*Ref.GetRefBonePose()[0].Inverse());Whole->RegisterComponent();
        (First?FirstPerson:ThirdPerson).Add(Whole);
    }
    UE_LOG(LogTemp,Display,TEXT("CS6 PRESENT item=%d fp=1 tp=1 animated=0 rigid=1 mesh=%s"),Offer.ItemKey,*GetNameSafe(Offer.VisualMesh));return true;
}
void UCS6WeaponPresentation::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick) {
    Super::TickComponent(Dt,Type,Tick);auto* Pawn=Cast<APawn>(GetOwner()->GetOwner());
    if(!Pawn){GetOwner()->SetActorHiddenInGame(true);return;}
    const bool Current=Object(Pawn,TEXT("CurrentWeapon"))==GetOwner(),Alive=!Bool(Pawn,TEXT("Dead"));
    const bool LocalHuman=Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled();
    const bool Observing=Pawn->ActorHasTag(TEXT("CS6.ObservedFP"));
    GetOwner()->SetActorHiddenInGame(!Alive || !Current);
    if(FP){FP->SetVisibility(false,false);FP->SetHiddenInGame(true,false);}if(TP){TP->SetVisibility(false,false);TP->SetHiddenInGame(true,false);}
    const bool ShowFP=Alive && Current && LocalHuman,ShowTP=Alive && Current && !LocalHuman && !Observing;
    for(auto& Whole:FirstPerson){Whole->SetVisibility(ShowFP,false);Whole->SetHiddenInGame(!ShowFP,false);}
    for(auto& Whole:ThirdPerson){Whole->SetVisibility(ShowTP,false);Whole->SetHiddenInGame(!ShowTP,false);}
    for(const TCHAR* FieldName:{TEXT("FPCurrentParts"),TEXT("TPCurrentParts")})if(auto* ArrayProp=CastField<FArrayProperty>(Field(GetOwner(),FieldName)))if(auto* Inner=CastField<FObjectPropertyBase>(ArrayProp->Inner)) {
        FScriptArrayHelper Array(ArrayProp,ArrayProp->ContainerPtrToValuePtr<void>(GetOwner()));
        for(int32 I=0;I<Array.Num();++I)if(auto* Part=Cast<AActor>(Inner->GetObjectPropertyValue(Array.GetRawPtr(I))))Part->SetActorHiddenInGame(true);
    }
    TInlineComponentArray<UWidgetComponent*> Widgets(GetOwner());for(auto* Widget:Widgets)Widget->SetVisibility(false,true);
}
