#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
using namespace CS6TemplateAccess;

bool ACS6TacticalMatch::SpawnDroppedGun(APawn* P,cs6::Item Item,int32 Magazine,int32 Reserve) {
    const auto* D=Offer(Item);if(!IsValid(P) || !D)return false;
    UStaticMesh* DropMesh=D->VisualMesh;
    if(!DropMesh) {
        // Ground pickups use a static display only; held weapons keep the complete kit rig.
        switch(int32(Item)) {
            case 1: DropMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CS6/Weapons/Whole/SM_SM_Whole_BP_C1911_Chrome_C.SM_SM_Whole_BP_C1911_Chrome_C"));break;
            case 2: DropMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CS6/Weapons/Whole/SM_SM_Whole_BP_P350_C.SM_SM_Whole_BP_P350_C"));break;
            case 8: DropMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CS6/Weapons/Whole/SM_SM_Whole_BP_UMP_C.SM_SM_Whole_BP_UMP_C"));break;
            case 12: DropMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CS6/Weapons/Whole/SM_SM_Whole_BP_AKM_C.SM_SM_Whole_BP_AKM_C"));break;
            case 13: DropMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CS6/Weapons/Whole/SM_SM_Whole_BP_MK18_C.SM_SM_Whole_BP_MK18_C"));break;
            case 16: DropMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CS6/Weapons/Whole/SM_SM_Whole_BP_Hunter85_C.SM_SM_Whole_BP_Hunter85_C"));break;
            default:break;
        }
    }
    if(!DropMesh)return false;
    FVector Forward=P->GetActorForwardVector();Forward.Z=0;Forward.Normalize();
    FVector Position=P->GetActorLocation()+Forward*110;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CS6GunDrop),true,P);
    for(TActorIterator<APawn> It(GetWorld());It;++It)Query.AddIgnoredActor(*It);
    FHitResult Floor;if(GetWorld()->LineTraceSingleByChannel(Floor,Position+FVector(0,0,70),Position-FVector(0,0,500),ECC_Visibility,Query))Position=Floor.ImpactPoint;
    else return false;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FQuat Orientation=FRotator(0,P->GetActorRotation().Yaw,0).Quaternion()*D->VisualOffset.GetRotation()*FQuat(FVector::RightVector,PI*0.5f);
    auto* Visual=GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),Position,Orientation.Rotator(),Params);if(!Visual)return false;
    auto* Mesh=Visual->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetStaticMesh(DropMesh);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);
    Visual->SetActorScale3D(D->VisualOffset.GetScale3D());Visual->SetActorEnableCollision(false);
    FVector Origin,Extent;Visual->GetActorBounds(false,Origin,Extent);Visual->AddActorWorldOffset(FVector(0,0,Position.Z+3-(Origin.Z-Extent.Z)));
    FDroppedGun Drop;Drop.Visual=Visual;Drop.Item=Item;Drop.Magazine=FMath::Max(0,Magazine);Drop.Reserve=FMath::Max(0,Reserve);Drop.Position=Visual->GetActorLocation();DroppedGuns.Add(Drop);
    return true;
}
bool ACS6TacticalMatch::DropCurrentGun(APawn* P) {
    auto* M=Member(P);if(!M || Bool(P,TEXT("Dead")))return false;
    auto* Gun=Object(P,TEXT("CurrentWeapon"));if(!Gun)return false;
    const bool Primary=Gun==Object(P,TEXT("Primary Weapon"));const auto Item=Primary?M->Wallet.gear.primary:M->Wallet.gear.pistol;
    CaptureEquipment(*M);
    if(!SpawnDroppedGun(P,Item,int32(Number(Gun,TEXT("CurrentAmmo"))),ReserveAmmo(P,Gun)))return false;
    M->Wallet.drop(Primary?cs6::Slot::Primary:cs6::Slot::Pistol);
    (Primary?M->Snapshot.PrimaryAmmo:M->Snapshot.PistolAmmo)=-1;
    return Equip(*M,P,true);
}
bool ACS6TacticalMatch::PickUpNearbyGun(APawn* P) {
    auto* M=Member(P);if(!M || Bool(P,TEXT("Dead")) || (Phase!=ECS6Phase::Live && Phase!=ECS6Phase::Freeze && Phase!=ECS6Phase::Planted))return false;
    int32 Index=INDEX_NONE;float Best=180*180;
    for(int32 I=0;I<DroppedGuns.Num();++I)if(DroppedGuns[I].Visual.IsValid()) {
        const FVector Delta=DroppedGuns[I].Position-P->GetActorLocation();const float Distance=Delta.SizeSquared2D();
        if(Distance<Best && FMath::Abs(Delta.Z)<180 && FVector::DotProduct(Delta.GetSafeNormal2D(),P->GetActorForwardVector())>0.25f){Index=I;Best=Distance;}
    }
    if(Index==INDEX_NONE)return false;
    const FDroppedGun Drop=DroppedGuns[Index];const bool Primary=cs6::product(Drop.Item).slot==cs6::Slot::Primary;
    CaptureEquipment(*M);const auto Before=M->Wallet;const auto Saved=M->Snapshot;
    const auto Old=M->Wallet.pickUp(Drop.Item);
    (Primary?M->Snapshot.PrimaryAmmo:M->Snapshot.PistolAmmo)=Drop.Magazine;
    if(!Equip(*M,P,true)){M->Wallet=Before;M->Snapshot=Saved;Equip(*M,P,true);return false;}
    auto* Gun=Object(P,Primary?TEXT("Primary Weapon"):TEXT("Secondary Weapon"));SetReserveAmmo(P,Gun,Drop.Reserve);
    if(Old!=cs6::Item::None)SpawnDroppedGun(P,Old,Primary?Saved.PrimaryAmmo:Saved.PistolAmmo,Primary?Saved.PrimaryReserve:Saved.PistolReserve);
    if(Drop.Visual.IsValid())Drop.Visual->Destroy();DroppedGuns.RemoveAt(Index);
    UE_LOG(LogTemp,Display,TEXT("CS6 PICKUP item=%d magazine=%d reserve=%d"),int32(Drop.Item),Drop.Magazine,Drop.Reserve);return true;
}
void ACS6TacticalMatch::ProcessDeathDrops() {
    for(auto& M:Roster)if(M.bDeathDropPending) {
        M.bDeathDropPending=false;auto* P=M.Pawn.Get();if(!IsValid(P))continue;
        SpawnDroppedGun(P,M.Wallet.gear.primary,M.Snapshot.PrimaryAmmo,M.Snapshot.PrimaryReserve);
        SpawnDroppedGun(P,M.Wallet.gear.pistol,M.Snapshot.PistolAmmo,M.Snapshot.PistolReserve);
        Call(P,TEXT("DestroyWeaponAndAttached"));
    }
}
