#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "CS6WeaponPresentation.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
using namespace CS6TemplateAccess;

namespace {
FString Export(UObject* Object,const TCHAR* Name) {
    FString Value;if(auto* P=Field(Object,Name))P->ExportTextItem_Direct(Value,P->ContainerPtrToValuePtr<void>(Object),nullptr,Object,PPF_None);return Value;
}
void Import(UObject* Object,const TCHAR* Name,const FString& Value) {
    if(auto* P=Field(Object,Name))P->ImportText_Direct(*Value,P->ContainerPtrToValuePtr<void>(Object),Object,PPF_None);
}
void DestroySlot(APawn* Pawn,bool Primary) {
    const TCHAR* Slot=Primary?TEXT("Primary Weapon"):TEXT("Secondary Weapon");
    auto* Gun=Cast<AActor>(Object(Pawn,Slot));if(!Gun)return;
    // Kit attachments are independent actors, not children owned by the gun actor.
    for(const TCHAR* Name:{TEXT("FPCurrentParts"),TEXT("TPCurrentParts")})
        if(auto* Array=CastField<FArrayProperty>(Field(Gun,Name)))
            if(auto* Inner=CastField<FObjectPropertyBase>(Array->Inner)) {
                FScriptArrayHelper Values(Array,Array->ContainerPtrToValuePtr<void>(Gun));
                for(int32 I=0;I<Values.Num();++I)if(auto* Part=Cast<AActor>(Inner->GetObjectPropertyValue(Values.GetRawPtr(I))))Part->Destroy();
            }
    if(Object(Pawn,TEXT("CurrentWeapon"))==Gun)SetObject(Pawn,TEXT("CurrentWeapon"),nullptr);
    SetObject(Pawn,Slot,nullptr);Gun->Destroy();
}
bool SpawnGun(APawn* Pawn,UClass* Class,bool Primary) {
    if(!Class)return true;
    auto* PC=Pawn->GetWorld()->GetFirstPlayerController();
    if(!ConfigureDefaultParts(PC,Class,Primary))return false;
    const TCHAR* Parts=Primary?TEXT("Primary Starting Parts"):TEXT("Secondary Starting Parts");
    auto* Source=CastField<FArrayProperty>(Field(PC,Parts));
    auto* Target=CastField<FArrayProperty>(Field(Pawn,Parts));
    if(!Source || !Target || !Source->SameType(Target))return false;
    Target->CopyCompleteValue(Target->ContainerPtrToValuePtr<void>(Pawn),Source->ContainerPtrToValuePtr<void>(PC));
    // Use the supplied Kit's complete spawn, montage, attach and notify pipeline.
    auto* Function=Pawn->FindFunction(TEXT("SpawnWeapon"));if(!Function)return false;
    FStructOnScope Params(Function);
    auto* Weapon=CastField<FObjectPropertyBase>(Function->FindPropertyByName(TEXT("WeaponToSpawn")));
    auto* Type=Function->FindPropertyByName(TEXT("WeaponType"));if(!Weapon || !Type)return false;
    Weapon->SetObjectPropertyValue_InContainer(Params.GetStructMemory(),Class);
    void* Value=Type->ContainerPtrToValuePtr<void>(Params.GetStructMemory());
    if(auto* Enum=CastField<FEnumProperty>(Type))Enum->GetUnderlyingProperty()->SetIntPropertyValue(Value,int64(Primary?0:1));
    else if(auto* Number=CastField<FNumericProperty>(Type))Number->SetIntPropertyValue(Value,int64(Primary?0:1));
    else return false;
    Pawn->ProcessEvent(Function,Params.GetStructMemory());
    return IsValid(Object(Pawn,Primary?TEXT("Primary Weapon"):TEXT("Secondary Weapon")));
}
FString Label(AController* C,int32 Index){return C && C->IsPlayerController()?TEXT("YOU"):FString::Printf(TEXT("BOT %02d"),Index);}
}
ACS6TacticalMatch::FMember* ACS6TacticalMatch::Member(APawn* Pawn){return IsValid(Pawn)?Roster.FindByPredicate([Pawn](const FMember& M){return M.Pawn==Pawn;}):nullptr;}
const ACS6TacticalMatch::FMember* ACS6TacticalMatch::Member(APawn* Pawn)const{return IsValid(Pawn)?Roster.FindByPredicate([Pawn](const FMember& M){return M.Pawn==Pawn;}):nullptr;}
const FCS6WeaponOffer* ACS6TacticalMatch::Offer(cs6::Item Item)const{return WeaponCatalog.FindByPredicate([Item](const FCS6WeaponOffer& O){return O.ItemKey==int32(Item) && O.WeaponClass;});}
int32 ACS6TacticalMatch::LocalCash()const {auto* PC=GetWorld()->GetFirstPlayerController();const auto* M=PC?Member(PC->GetPawn()):nullptr;if(!M && PC)M=Roster.FindByPredicate([PC](const FMember& E){return E.Controller==PC;});return M?M->Wallet.cash:0;}
const cs6::Gear* ACS6TacticalMatch::LocalGear()const {auto* PC=GetWorld()->GetFirstPlayerController();const auto* M=PC?Member(PC->GetPawn()):nullptr;return M?&M->Wallet.gear:nullptr;}
cs6::BuyContext ACS6TacticalMatch::LocalBuyContext()const {
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?PC->GetPawn():nullptr;const auto* M=Member(P);
    const bool Alive=IsValid(P) && M && !Bool(P,TEXT("Dead"));bool Zone=false;
    if(Alive)for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(M->Wallet.attacker?TEXT("CS6.TeamA"):TEXT("CS6.TeamB")) && FVector::Dist2D(P->GetActorLocation(),It->GetActorLocation())<=500 && FMath::Abs(P->GetActorLocation().Z-It->GetActorLocation().Z)<200){Zone=true;break;}
    const bool Time=Phase==ECS6Phase::Freeze || (Phase==ECS6Phase::Live && 115-SecondsRemaining<=20);
    return {Alive,Zone,Time};
}
void ACS6TacticalMatch::InitializeEconomy(APawn* P,AController* C,bool TeamA) {
    auto* M=Roster.FindByPredicate([C](const FMember& E){return E.Controller==C;});if(!M)return;M->Pawn=P;
    if(!M->bEconomyStarted){M->Wallet.resetHalf(TeamA!=bSidesSwapped);if(C->IsPlayerController() && FParse::Param(FCommandLine::Get(),TEXT("CS6ManualLoadout"))){M->Wallet.credit(7200);M->Wallet.gear.primary=M->Wallet.attacker?cs6::Item::AK47:cs6::Item::M4A4;}M->bEconomyStarted=true;}
    if(!bEconomyEnabled)return;
    if(M->bNeedsInventory) {
        if(!C->IsPlayerController()) {
            const cs6::BuyContext Buy{true,true,true};
            auto& W=M->Wallet;
            if(W.gear.armor<100 && W.cash>=1000)W.buy(cs6::Item::Helmet,Buy);
            else if(W.gear.armor<100 && W.cash>=650)W.buy(cs6::Item::Armor,Buy);
            if(W.gear.primary==cs6::Item::None) {
                for(cs6::Item Gun:{W.attacker?cs6::Item::AK47:cs6::Item::M4A4,W.attacker?cs6::Item::Galil:cs6::Item::Famas,cs6::Item::UMP})
                    if(Offer(Gun) && W.buy(Gun,Buy)==cs6::BuyResult::Ok)break;
            }
            if(!W.attacker && !W.gear.kit && W.cash>=400)W.buy(cs6::Item::DefuseKit,Buy);
        }
        if(Equip(*M,P,true))M->bNeedsInventory=false;else UE_LOG(LogTemp,Error,TEXT("CS6 EQUIP failed pawn=%s"),*P->GetName());
    }
    M->PreviousHP=float(Scalar(Object(P,TEXT("C_Health")),TEXT("Health"),100));
    SetPouchAmmo(P,4,M->Wallet.grenadeCount());
    P->OnTakeAnyDamage.AddUniqueDynamic(this,&ACS6TacticalMatch::OnCombatDamage);
    if(auto* F=Combatants.FindByPredicate([P](const FCS6Combatant& E){return E.Pawn==P;}))F->bDefuseKit=M->Wallet.gear.kit;
}
bool ACS6TacticalMatch::Equip(FMember& M,APawn* P,bool Restore) {
    const auto* Primary=Offer(M.Wallet.gear.primary);const auto* Pistol=Offer(M.Wallet.gear.pistol);
    if((M.Wallet.gear.primary!=cs6::Item::None && !Primary) || (M.Wallet.gear.pistol!=cs6::Item::None && !Pistol))return false;
    bool Changed=false;
    for(bool IsPrimary:{false,true}) {
        const auto* Definition=IsPrimary?Primary:Pistol;
        auto* Existing=Object(P,IsPrimary?TEXT("Primary Weapon"):TEXT("Secondary Weapon"));
        if(IsValid(Existing) && Definition && Existing->IsA(Definition->WeaponClass.Get()))continue;
        if(!Existing && !Definition)continue;
        Call(P,TEXT("CancelReload"));DestroySlot(P,IsPrimary);Changed=true;
        if(Definition && !SpawnGun(P,Definition->WeaponClass.Get(),IsPrimary))return false;
    }
    if(!IsValid(Object(P,TEXT("CurrentWeapon"))) || Changed) {
        SetObject(P,TEXT("CurrentWeapon"),Object(P,Primary?TEXT("Primary Weapon"):TEXT("Secondary Weapon")));
        if(Object(P,TEXT("CurrentWeapon")))Call(P,TEXT("OnRep_CurrentWeapon"));
    }
    if(Restore && !M.Snapshot.AmmoPouch.IsEmpty())Import(P,TEXT("PlayerAmmoPouch"),M.Snapshot.AmmoPouch);
    for(bool IsPrimary:{false,true}) {
        auto* Gun=Object(P,IsPrimary?TEXT("Primary Weapon"):TEXT("Secondary Weapon"));const auto* Definition=IsPrimary?Primary:Pistol;
        if(!Gun || !Definition)continue;
        SetScalar(Gun,TEXT("Damage"),Definition->Damage);SetScalar(Gun,TEXT("FireRate"),Definition->FireInterval);
        SetNumber(Gun,TEXT("MagazineCapacity"),Definition->Magazine);
        const int32 Ammo=IsPrimary?M.Snapshot.PrimaryAmmo:M.Snapshot.PistolAmmo;
        SetNumber(Gun,TEXT("CurrentAmmo"),Restore && Ammo>=0?Ammo:Definition->Magazine);
        if(!Restore || Ammo<0 || M.Snapshot.AmmoPouch.IsEmpty())SetReserveAmmo(P,Gun,Definition->Reserve);
    }
    return Object(P,TEXT("CurrentWeapon"))!=nullptr || (!Primary && !Pistol);
}
void ACS6TacticalMatch::CaptureEquipment(FMember& M) {
    auto* P=M.Pawn.Get();if(!IsValid(P))return;
    M.Snapshot.PrimaryAmmo=int32(Number(Object(P,TEXT("Primary Weapon")),TEXT("CurrentAmmo"),-1));
    M.Snapshot.PistolAmmo=int32(Number(Object(P,TEXT("Secondary Weapon")),TEXT("CurrentAmmo"),-1));
    M.Snapshot.PrimaryReserve=ReserveAmmo(P,Object(P,TEXT("Primary Weapon")));M.Snapshot.PistolReserve=ReserveAmmo(P,Object(P,TEXT("Secondary Weapon")));
    M.Snapshot.AmmoPouch=Export(P,TEXT("PlayerAmmoPouch"));
}
cs6::BuyResult ACS6TacticalMatch::BuyItem(cs6::Item Item,bool Refund) {
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?PC->GetPawn():nullptr;auto* M=Member(P);
    if(!bEconomyEnabled || !M)return cs6::BuyResult::Unavailable;
    const auto Slot=cs6::product(Item).slot;
    if(Slot==cs6::Slot::Grenade || ((Slot==cs6::Slot::Primary || Slot==cs6::Slot::Pistol) && !Offer(Item)))return cs6::BuyResult::Unavailable;
    CaptureEquipment(*M);const auto Before=M->Wallet;const auto Saved=M->Snapshot;const auto OldItem=Slot==cs6::Slot::Primary?Before.gear.primary:Before.gear.pistol;const int32 OldReserve=ReserveAmmo(P,Object(P,Slot==cs6::Slot::Primary?TEXT("Primary Weapon"):TEXT("Secondary Weapon")));
    const auto Result=Refund?M->Wallet.refund(Item,LocalBuyContext()):M->Wallet.buy(Item,LocalBuyContext());
    if(Result==cs6::BuyResult::Ok) {
        if(Slot==cs6::Slot::Primary)M->Snapshot.PrimaryAmmo=-1;
        if(Slot==cs6::Slot::Pistol)M->Snapshot.PistolAmmo=-1;
        if((Slot==cs6::Slot::Primary || Slot==cs6::Slot::Pistol) && !Equip(*M,P,true)){M->Wallet=Before;M->Snapshot=Saved;Equip(*M,P,true);return cs6::BuyResult::Unavailable;}
        if(!Refund && (Slot==cs6::Slot::Primary || Slot==cs6::Slot::Pistol) && OldItem!=cs6::Item::None)SpawnDroppedGun(P,OldItem,Slot==cs6::Slot::Primary?Saved.PrimaryAmmo:Saved.PistolAmmo,OldReserve);
        if(auto* F=Combatants.FindByPredicate([P](const FCS6Combatant& E){return E.Pawn==P;}))F->bDefuseKit=M->Wallet.gear.kit;
        BuyMessage=Refund?TEXT("Purchase refunded"):TEXT("Purchased");
        UE_LOG(LogTemp,Display,TEXT("CS6 BUY item=%d refund=%d cash=%d"),int32(Item),Refund,M->Wallet.cash);
    } else {
        switch(Result){case cs6::BuyResult::InsufficientFunds:BuyMessage=TEXT("Not enough money");break;case cs6::BuyResult::WrongSide:BuyMessage=TEXT("Unavailable for this side");break;case cs6::BuyResult::OutsideZone:BuyMessage=TEXT("Return to your buy zone");break;case cs6::BuyResult::TimeExpired:BuyMessage=TEXT("Buy time has ended");break;case cs6::BuyResult::Used:BuyMessage=TEXT("Used equipment cannot be refunded");break;default:BuyMessage=TEXT("Purchase unavailable");break;}
    }
    return Result;
}
float ACS6TacticalMatch::PawnMoveSpeed(APawn* P)const {
    const auto* M=Member(P);if(!bEconomyEnabled || !M)return 500;
    const bool Primary=Object(P,TEXT("CurrentWeapon"))==Object(P,TEXT("Primary Weapon"));
    const auto* D=Offer(Primary?M->Wallet.gear.primary:M->Wallet.gear.pistol);
    float Speed=(D?D->MoveSpeed:250)*2.54f;
    if(auto* PC=Cast<APlayerController>(P->GetController()))if(PC->IsInputKeyDown(EKeys::LeftShift))Speed*=0.52f;
    if(Bool(P,TEXT("Crouching")))Speed*=0.34f;
    return Speed;
}
float ACS6TacticalMatch::AdjustDamage(APawn* P,float Damage,const FDamageEvent& Event,AActor* Causer) {
    auto* M=Member(P);if(!bEconomyEnabled || !M)return Damage;
    CaptureEquipment(*M);
    bool Head=false;if(Event.IsOfType(FPointDamageEvent::ClassID))Head=static_cast<const FPointDamageEvent&>(Event).HitInfo.BoneName.ToString().Contains(TEXT("head"));
    float Ratio=0.5f;
    if(auto* Attacker=Cast<APawn>(Causer?Causer->GetOwner():nullptr))if(const auto* Killer=Member(Attacker)) {
        const auto* D=Offer(Object(Attacker,TEXT("CurrentWeapon"))==Object(Attacker,TEXT("Primary Weapon"))?Killer->Wallet.gear.primary:Killer->Wallet.gear.pistol);if(D)Ratio=D->ArmorPenetration;
    }
    return M->Wallet.absorb(Damage,Ratio,Head);
}
void ACS6TacticalMatch::OnCombatDamage(AActor* Victim,float Damage,const UDamageType* Type,AController* InstigatedBy,AActor* Causer) {
    if(!bEconomyEnabled)return;auto* P=Cast<APawn>(Victim);auto* M=Member(P);if(!M)return;
    const float HP=FMath::Max(0.f,float(Scalar(Object(P,TEXT("C_Health")),TEXT("Health"))));
    const float Actual=FMath::Max(0.f,M->PreviousHP-HP);M->PreviousHP=HP;
    if(InstigatedBy && InstigatedBy!=P->GetController()) {
        M->LastAttacker=InstigatedBy;
        auto* K=Member(InstigatedBy->GetPawn());if(!K)K=Roster.FindByPredicate([InstigatedBy](const FMember& E){return E.Controller==InstigatedBy;});if(K){K->Damage+=FMath::RoundToInt(Actual);M->LastAttackerSlot=int32(K-Roster.GetData());M->Contributions.FindOrAdd(M->LastAttackerSlot)+=Actual;}
    }
    if(HP<=0 || Bool(P,TEXT("Dead")))ReportDeath(P);
}
void ACS6TacticalMatch::ReportDeath(APawn* P) {
    auto* Victim=Combatants.FindByPredicate([P](const FCS6Combatant& C){return C.Pawn==P;});
    if(!Victim || !Victim->bAlive)return;
    if(bEconomyEnabled)if(auto* M=Member(P)) {
        ++M->Deaths;M->bSurvived=false;M->bDeathDropPending=true;auto* Attacker=M->LastAttacker.Get();
        auto* K=Roster.IsValidIndex(M->LastAttackerSlot)?&Roster[M->LastAttackerSlot]:nullptr;
        if(K) {
            const bool Friendly=K->bTeamA==M->bTeamA;const auto Gun=Object(K->Pawn.Get(),TEXT("CurrentWeapon"))==Object(K->Pawn.Get(),TEXT("Primary Weapon"))?K->Wallet.gear.primary:K->Wallet.gear.pistol;
            if(Friendly)K->Wallet.credit(-300);else {
                ++K->Kills;K->Wallet.credit(cs6::product(Gun).killReward);
                for(const auto& Contribution:M->Contributions)if(Contribution.Value>=40 && Roster.IsValidIndex(Contribution.Key)) {
                    auto& Assistant=Roster[Contribution.Key];if(&Assistant!=K && &Assistant!=M && Assistant.bTeamA!=M->bTeamA)++Assistant.Assists;
                }
            }
            FCS6KillNotice N;N.Killer=Label(Attacker,int32(K-Roster.GetData())+1);N.Victim=Label(M->Controller.Get(),int32(M-Roster.GetData())+1);N.Weapon=Offer(Gun)?Offer(Gun)->DisplayName.ToString():UTF8_TO_TCHAR(cs6::product(Gun).name);N.Time=Age;N.bFriendly=Friendly;Notices.Add(N);if(Notices.Num()>6)Notices.RemoveAt(0);
        }
    }
    Super::ReportDeath(P);
}
void ACS6TacticalMatch::AwardRoundEconomy(bool AttackersWon,int32 Reason) {
    if(!bEconomyEnabled)return;
    const bool Planted=bRoundBombPlanted || Phase==ECS6Phase::Planted || Reason==int32(cs6::Reason::Defused) || Reason==int32(cs6::Reason::Exploded);
    for(auto& M:Roster) {
        const auto* C=Combatants.FindByPredicate([&M](const FCS6Combatant& E){return E.Pawn==M.Pawn.Get();});
        M.bSurvived=C && C->bAlive && IsValid(C->Pawn) && !Bool(C->Pawn,TEXT("Dead"));
        if(M.bSurvived)CaptureEquipment(M);
        M.Wallet.awardRound(M.Wallet.attacker==AttackersWon,static_cast<cs6::Reason>(Reason),M.bSurvived,Planted);
    }
}

void ACS6TacticalMatch::ObjectiveCompleted(APawn* P,bool Plant) {
    if(Plant)bRoundBombPlanted=true;
    if(bEconomyEnabled)if(auto* M=Member(P)){M->Wallet.credit(300);UE_LOG(LogTemp,Display,TEXT("CS6 OBJECTIVE plant=%d reward=300 cash=%d"),Plant,M->Wallet.cash);}
}
