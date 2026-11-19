#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "UnrealClient.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/PlayerCameraManager.h"
using namespace CS6TemplateAccess;

void ACS6TacticalMatch::DriveShopScenario() {
#if !UE_BUILD_SHIPPING
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?PC->GetPawn():nullptr;auto* M=Member(P);if(!M || !bEconomyEnabled)return;
    auto AuditCamera=[&](const TCHAR* Stage,bool Crouch) {
        auto* Character=Cast<ACharacter>(P);const float Ground=Character->GetCharacterMovement()->CurrentFloor.HitResult.ImpactPoint.Z;
        const float Height=PC->PlayerCameraManager->GetCameraLocation().Z-Ground;
        const bool Pass=Crouch?(Height>=95 && Height<=140):(Height>=150 && Height<=195);
        bShopPassed=bShopPassed && Pass;
        UE_LOG(LogTemp,Display,TEXT("CS6 CAMERA CHECK stage=%s height=%.1f half=%.1f pass=%d"),Stage,Height,Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),Pass);
    };
    if(ScenarioStep==2) {
        if(InitialMagazine==0 && Age>10){SetBuyMenu(false);AuditCamera(TEXT("standing"),false);InitialMagazine=1;}
        else if(InitialMagazine==1 && Age>11){PC->InputKey(FInputKeyParams(EKeys::LeftControl,IE_Pressed,1.0));InitialMagazine=2;}
        else if(InitialMagazine==2 && Age>13){AuditCamera(TEXT("crouching"),true);FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/model-camera-crouch.png"),true,false);PC->InputKey(FInputKeyParams(EKeys::LeftControl,IE_Released,0.0));InitialMagazine=3;}
        else if(InitialMagazine==3 && Age>16){AuditCamera(TEXT("standing-restored"),false);FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/model-camera-standing.png"),true,false);InitialMagazine=4;}
    }
    auto AuditModels=[&](const TCHAR* Stage) {
        int32 Strays=0,CurrentParts=0,Legacy=0;
        auto* Current=Object(P,TEXT("CurrentWeapon"));
        for(const TCHAR* Slot:{TEXT("Primary Weapon"),TEXT("Secondary Weapon")})if(auto* Gun=Cast<AActor>(Object(P,Slot))) {
            auto* FP=Cast<USkeletalMeshComponent>(Object(Gun,TEXT("FPMesh")));
            TInlineComponentArray<UStaticMeshComponent*> Parts(Gun);
            for(auto* Part:Parts)if(Part->GetAttachParent()==FP && Part->IsVisible() && !Part->bHiddenInGame && !Gun->IsHidden()) {
                if(Gun==Current)++CurrentParts;else ++Strays;
            }
            if(FP && FP->IsVisible() && !FP->bHiddenInGame && !Gun->IsHidden())++Legacy;
        }
        const bool Pass=Strays==0 && Legacy==0 && CurrentParts==1;bShopPassed=bShopPassed && Pass;
        UE_LOG(LogTemp,Display,TEXT("CS6 MODEL CHECK stage=%s current=%s parts=%d stray=%d legacy=%d pass=%d"),Stage,*GetNameSafe(Current),CurrentParts,Strays,Legacy,Pass);
    };
    if(ScenarioStep==0 && Age>7) {
        SetBuyMenu(true);
        const bool Armor=BuyItem(cs6::Item::Armor)==cs6::BuyResult::Ok && M->Wallet.cash==150 && M->Wallet.gear.armor==100;
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP armor_cash=%d armor=%d"),M->Wallet.cash,M->Wallet.gear.armor);
        const bool Refund=BuyItem(cs6::Item::Armor,true)==cs6::BuyResult::Ok && M->Wallet.cash==800 && M->Wallet.gear.armor==0;
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP refund_cash=%d armor=%d"),M->Wallet.cash,M->Wallet.gear.armor);
        M->Wallet.credit(5000);const bool Bought=BuyItem(cs6::Item::AK47)==cs6::BuyResult::Ok;
        auto* Gun=Object(P,TEXT("CurrentWeapon"));const auto* D=Offer(cs6::Item::AK47);
        const bool Equipped=Bought && Gun && D && Gun->IsA(D->WeaponClass);
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP rifle_equipped=%d cash=%d"),Equipped,M->Wallet.cash);
        bShopPassed=Armor && Refund && Equipped && M->Wallet.cash==3100;
        ShopOriginalPosition=P->GetActorLocation();ScenarioStep=1;
    } else if(ScenarioStep==1 && Age>9) {
        bCaptured=true;FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/cs2-shop.png"),true,false);ScenarioStep=2;
    } else if(ScenarioStep==2 && Phase==ECS6Phase::Live && Age>20) {
        SetBuyMenu(false);for(const auto& C:Combatants)if(C.Pawn!=P)FreezePawn(C.Pawn,true);
        PC->InputKey(FInputKeyParams(EKeys::LeftMouseButton,IE_Pressed,1.0));WeaponScenarioStart=Age;ScenarioStep=3;
    } else if(ScenarioStep==3 && Age-WeaponScenarioStart>0.7f) {
        PC->InputKey(FInputKeyParams(EKeys::LeftMouseButton,IE_Released,0.0));ScenarioStep=4;
    } else if(ScenarioStep==4 && Age-WeaponScenarioStart>1.2f) {
        const bool Refused=BuyItem(cs6::Item::AK47,true)==cs6::BuyResult::Used;
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP fired_refund_rejected=%d"),Refused);bShopPassed=bShopPassed && Refused;
        P->SetActorLocation(ShopOriginalPosition+FVector(2500,2500,0),false,nullptr,ETeleportType::TeleportPhysics);
        const bool Zone=BuyItem(cs6::Item::P250)==cs6::BuyResult::OutsideZone;
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP zone_rejected=%d"),Zone);bShopPassed=bShopPassed && Zone;
        P->SetActorLocation(ShopOriginalPosition,false,nullptr,ETeleportType::TeleportPhysics);
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP %s"),bShopPassed?TEXT("PASS"):TEXT("FAIL"));ScenarioStep=5;
    } else if(ScenarioStep==5 && Age>25) {
        FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/cs2-rifle.png"),true,false);
        PC->InputKey(FInputKeyParams(EKeys::R,IE_Pressed,1.0));ScenarioStep=6;
    } else if(ScenarioStep==6 && Age>26.3f) {
        PC->InputKey(FInputKeyParams(EKeys::R,IE_Released,0.0));
        FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/cs2-reload.png"),true,false);ScenarioStep=7;
    } else if(ScenarioStep==7 && Age>32) {
        const bool Reload=Number(Object(P,TEXT("CurrentWeapon")),TEXT("CurrentAmmo"))==30;
        bShopPassed=bShopPassed && Reload;UE_LOG(LogTemp,Display,TEXT("CS6 SHOP supplied_model_reload=%d"),Reload);
        const int32 Ammo=int32(Number(Object(P,TEXT("CurrentWeapon")),TEXT("CurrentAmmo")));const int32 Reserve=ReserveAmmo(P,Object(P,TEXT("CurrentWeapon")));
        const bool Dropped=DropCurrentGun(P) && M->Wallet.gear.primary==cs6::Item::None;
        const bool Picked=PickUpNearbyGun(P) && M->Wallet.gear.primary==cs6::Item::AK47 && Number(Object(P,TEXT("CurrentWeapon")),TEXT("CurrentAmmo"))==Ammo && ReserveAmmo(P,Object(P,TEXT("CurrentWeapon")))==Reserve;
        const bool NoRefund=BuyItem(cs6::Item::AK47,true)==cs6::BuyResult::NotOwned;
        bShopPassed=bShopPassed && Dropped && Picked && NoRefund;
        UE_LOG(LogTemp,Display,TEXT("CS6 SHOP drop=%d pickup_kept_ammo=%d dropped_refund_rejected=%d"),Dropped,Picked,NoRefund);ScenarioStep=8;
    } else if(ScenarioStep==8 && Age>33) {
        PC->SetControlRotation(FRotator(-35,P->GetActorRotation().Yaw,0));PC->InputKey(FInputKeyParams(EKeys::Two,IE_Pressed,1.0));ScenarioStep=9;
    } else if(ScenarioStep==9 && Age>35) {
        PC->InputKey(FInputKeyParams(EKeys::Two,IE_Released,0.0));AuditModels(TEXT("pistol"));FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/model-pistol-whole.png"),true,false);ScenarioStep=10;
    } else if(ScenarioStep==10 && Age>35.2f) {
        PC->InputKey(FInputKeyParams(EKeys::One,IE_Pressed,1.0));ScenarioStep=11;
    } else if(ScenarioStep==11 && Age>37) {
        PC->InputKey(FInputKeyParams(EKeys::One,IE_Released,0.0));AuditModels(TEXT("rifle"));FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/model-rifle-whole.png"),true,false);ScenarioStep=12;
    } else if(ScenarioStep==12 && Age>37.2f) {
        PC->InputKey(FInputKeyParams(EKeys::Two,IE_Pressed,1.0));ScenarioStep=13;
    } else if(ScenarioStep==13 && Age>39) {
        PC->InputKey(FInputKeyParams(EKeys::Two,IE_Released,0.0));AuditModels(TEXT("pistol-repeat"));FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/model-pistol-repeat-whole.png"),true,false);ScenarioStep=14;
    }
#endif
}
