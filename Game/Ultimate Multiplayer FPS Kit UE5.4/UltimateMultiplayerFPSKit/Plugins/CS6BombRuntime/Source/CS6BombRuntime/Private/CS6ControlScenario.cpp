#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UnrealClient.h"
using namespace CS6TemplateAccess;

void ACS6TacticalMatch::DriveControlScenario() {
#if !UE_BUILD_SHIPPING
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC || !bEconomyEnabled)return;
    if(ScenarioStep==0 && Phase==ECS6Phase::Live && Age>20) {
        for(const auto& C:Combatants)FreezePawn(C.Pawn,true);
        ScenarioVictim=PC->GetPawn();
        auto* P=PC->GetPawn();P->SetCanBeDamaged(true);
        if(auto* Arms=Cast<USkeletalMeshComponent>(Object(P,TEXT("1P_Arms")))) {
            auto* Cam=Cast<UCameraComponent>(Object(P,TEXT("1P_Camera")));
            const FString Msg=TEXT("CS6 FP HUMAN armsParent=")+GetNameSafe(Arms->GetAttachParent())+TEXT(" armsZ=")+FString::SanitizeFloat(Arms->GetComponentLocation().Z)
                +TEXT(" camZ=")+FString::SanitizeFloat(Cam?Cam->GetComponentLocation().Z:-1.0)+TEXT(" rel=")+Arms->GetRelativeTransform().GetLocation().ToCompactString();
            UE_LOG(LogTemp,Display,TEXT("%s"),*Msg);
        }
        for(const auto& C:Combatants)if(!C.bAttacker && C.bAlive) {auto* Gun=Cast<AActor>(Object(C.Pawn,TEXT("CurrentWeapon")));UGameplayStatics::ApplyDamage(P,1000,C.Pawn->GetController(),Gun,Cast<UClass>(Object(Gun,TEXT("DamageType"))));break;}
        ScenarioStep=1;
    } else if(ScenarioStep==1 && Age>21) {
        bSpectateFirstPerson=false;UpdateSpectating();FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/cs2-spectator-tp.png"),true,false);ScenarioStep=2;
    } else if(ScenarioStep==2 && Age>22) {
        bSpectateFirstPerson=true;UpdateSpectating();ScenarioStep=3;
    } else if(ScenarioStep==3 && Age>24) {
        if(auto* Seen=ObservedPawn.Get()) {
            auto* Arms=Cast<USkeletalMeshComponent>(Object(Seen,TEXT("1P_Arms")));auto* Cam=Cast<UCameraComponent>(Object(Seen,TEXT("1P_Camera")));
            const int32 ArmsVisible=Arms && Arms->IsVisible()?1:0,ArmsOwnerNoSee=Arms && Arms->bOwnerNoSee?1:0,ArmsHidden=Arms && Arms->bHiddenInGame?1:0;
            const FString Msg=TEXT("CS6 FP VIEW viewTarget=")+GetNameSafe(PC->GetViewTarget())+TEXT(" pawn=")+GetNameSafe(Seen)
                +TEXT(" armsVisible=")+FString::FromInt(ArmsVisible)+TEXT(" ownerNoSee=")+FString::FromInt(ArmsOwnerNoSee)+TEXT(" hidden=")+FString::FromInt(ArmsHidden)
                +TEXT(" camZ=")+FString::SanitizeFloat(Cam?Cam->GetComponentLocation().Z:-1.0)+TEXT(" armsZ=")+FString::SanitizeFloat(Arms?Arms->GetComponentLocation().Z:-1.0)
                +TEXT(" actorHidden=")+FString::FromInt(Seen->IsHidden()?1:0)+TEXT(" armsParent=")+GetNameSafe(Arms?Arms->GetAttachParent():nullptr);
            UE_LOG(LogTemp,Display,TEXT("%s"),*Msg);
        }
        FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/cs2-spectator-fp.png"),true,false);
        UpdateSpectating();bControlPassed=bSpectating && !PC->GetPawn() && ObservedPawn.IsValid();
        UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL spectating=%d"),bControlPassed);
        auto* Target=ObservedPawn.Get();if(!Target)return;
        SetScalar(Object(Target,TEXT("C_Health")),TEXT("Health"),63);
        auto* Gun=Object(Target,TEXT("CurrentWeapon"));SetNumber(Gun,TEXT("CurrentAmmo"),7);
        InitialMagazine=0;for(const auto& C:Combatants)InitialMagazine+=C.bAlive;
        PC->InputKey(FInputKeyParams(EKeys::E,IE_Pressed,1.0));WeaponScenarioStart=Age;ScenarioStep=4;
    } else if(ScenarioStep==4 && Age-WeaponScenarioStart>0.2f && (PC->GetPawn() || Age-WeaponScenarioStart>2.f)) {
        PC->InputKey(FInputKeyParams(EKeys::E,IE_Released,0.0));
        const bool Taken=PC->GetPawn()!=nullptr && !bSpectating;int32 After=0;for(const auto& C:Combatants)After+=C.bAlive;
        const int32 Alive=InitialMagazine;const auto* D=Offer(cs6::Item::Glock);auto* Class=D?D->WeaponClass.Get():nullptr;
        auto* P=PC->GetPawn();auto* NewGun=Object(P,TEXT("CurrentWeapon"));
        const bool Health=Scalar(Object(P,TEXT("C_Health")),TEXT("Health"))==63;
        const bool Inventory=NewGun && NewGun->GetClass()==Class && Number(NewGun,TEXT("CurrentAmmo"))==7;
        bControlPassed=bControlPassed && Taken && Alive==After && Health && Inventory;
        UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL success=%d same_alive=%d health_kept=%d inventory_kept=%d"),Taken,Alive==After,Health,Inventory);
        // Enemy rejection must not create or replace a local pawn.
        auto* Original=P;bSpectating=true;
        for(const auto& C:Combatants)if(C.bAlive && !C.bAttacker){ObservedPawn=C.Pawn;break;}
        const bool Rejected=!TakeOverObservedBot() && PC->GetPawn()==Original;
        bSpectating=false;ObservedPawn.Reset();bControlPassed=bControlPassed && Rejected;
        UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL enemy_rejected=%d"),Rejected);
        // Damage/kill awards must follow the controlled bot's wallet, not the dead human row.
        auto* Controlled=Member(P);const int32 BeforeCash=Controlled?Controlled->Wallet.cash:0;
        for(const auto& C:Combatants)if(C.bAlive && !C.bAttacker) {
            C.Pawn->SetCanBeDamaged(true);auto* TestWeapon=Cast<AActor>(Object(P,TEXT("CurrentWeapon")));
            UGameplayStatics::ApplyDamage(C.Pawn,1000,PC,TestWeapon,Cast<UClass>(Object(TestWeapon,TEXT("DamageType"))));break;
        }
        const bool Award=Controlled && Controlled->Wallet.cash==BeforeCash+300 && Controlled->Kills==1;
        bControlPassed=bControlPassed && Award;UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL controlled_wallet_award=%d"),Award);
        ScenarioInteractor=P;ScenarioStep=5;
    } else if(ScenarioStep==5 && Age>28) {
        const bool Stable=PC->GetPawn()==ScenarioInteractor.Get() && IsValid(PC->GetPawn()) && !Bool(PC->GetPawn(),TEXT("Dead"));
        bControlPassed=bControlPassed && Stable;
        UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL delayed_callback_safe=%d"),Stable);
        ScenarioStep=6;
    } else if(ScenarioStep==6 && Age>29) {
        auto* P=PC->GetPawn();auto* TestWeapon=Cast<AActor>(Object(P,TEXT("CurrentWeapon")));
        for(const auto& C:Combatants)if(C.bAlive && !C.bAttacker){C.Pawn->SetCanBeDamaged(true);UGameplayStatics::ApplyDamage(C.Pawn,1000,PC,TestWeapon,Cast<UClass>(Object(TestWeapon,TEXT("DamageType"))));}
        ScenarioStep=7;
    } else if(ScenarioStep==7 && bScenarioReset && bReady) {
        int32 Alive=0;for(const auto& C:Combatants)Alive+=C.bAlive;
        const auto* Human=Member(PC->GetPawn());int32 Bots=0;for(const auto& M:Roster)if(M.Controller.IsValid() && !M.Controller->IsPlayerController() && M.Controller->GetPawn()==M.Pawn.Get())++Bots;
        const bool Restored=Alive==10 && Bots==9 && Human && Human->Controller==PC;
        bControlPassed=bControlPassed && Restored;
        UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL next_round_restored=%d"),Restored);
        UE_LOG(LogTemp,Display,TEXT("CS6 CONTROL %s"),bControlPassed?TEXT("PASS"):TEXT("FAIL"));ScenarioStep=8;
    }
#endif
}
