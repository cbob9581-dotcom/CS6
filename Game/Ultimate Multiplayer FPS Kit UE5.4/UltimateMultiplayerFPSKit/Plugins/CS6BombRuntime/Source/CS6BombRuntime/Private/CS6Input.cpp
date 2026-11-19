#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
using namespace CS6TemplateAccess;

void ACS6TacticalMatch::SetBuyMenu(bool Open) {
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    const auto Context=LocalBuyContext();
    if(Open && (!Context.alive || !Context.inBuyZone || !Context.inBuyTime)){BuyMessage=!Context.inBuyZone?TEXT("Return to your buy zone"):TEXT("Buying unavailable");return;}
    if(Open)SetBombSelected(false);
    bBuyMenuOpen=Open;
    if(Overlay)Overlay->SetVisibility(Open?ESlateVisibility::Visible:ESlateVisibility::HitTestInvisible);
    if(Open && Overlay){PC->SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(Overlay->TakeWidget()).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));PC->SetIgnoreLookInput(true);}
    else {PC->SetInputMode(FInputModeGameOnly());PC->ResetIgnoreLookInput();}
    PC->bShowMouseCursor=Open;
}
void ACS6TacticalMatch::SetPauseMenu(bool Open) {
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    if(Open && bBuyMenuOpen)SetBuyMenu(false);
    bPauseMenuOpen=Open;UGameplayStatics::SetGamePaused(this,Open);
    if(Overlay)Overlay->SetVisibility(Open?ESlateVisibility::Visible:ESlateVisibility::HitTestInvisible);
    if(Open && Overlay){PC->SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(Overlay->TakeWidget()).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));PC->SetIgnoreLookInput(true);}
    else {PC->SetInputMode(FInputModeGameOnly());PC->ResetIgnoreLookInput();}
    PC->bShowMouseCursor=Open;
}
void ACS6TacticalMatch::SetBombSelected(bool Selected) {
    if(bBombSelected==Selected)return;
    bBombSelected=Selected;
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?PC->GetPawn():nullptr;
    if(Selected && P) {
        Call(P,TEXT("CancelReload"));
        auto Hide=[&](UPrimitiveComponent* C){if(C){ObjectiveVisibility.FindOrAdd(C)=C->bHiddenInGame;C->SetHiddenInGame(true);}};
        Hide(Cast<UPrimitiveComponent>(Object(P,TEXT("1P_Arms"))));
        for(const TCHAR* Slot:{TEXT("Primary Weapon"),TEXT("Secondary Weapon")})if(auto* Gun=Cast<AActor>(Object(P,Slot))) {
            TInlineComponentArray<UPrimitiveComponent*> Components(Gun);for(auto* C:Components)Hide(C);
            for(const TCHAR* Parts:{TEXT("FPCurrentParts"),TEXT("TPCurrentParts")})
                if(auto* Array=CastField<FArrayProperty>(Field(Gun,Parts)))if(auto* Inner=CastField<FObjectPropertyBase>(Array->Inner)) {
                    FScriptArrayHelper Values(Array,Array->ContainerPtrToValuePtr<void>(Gun));
                    for(int32 I=0;I<Values.Num();++I)if(auto* Part=Cast<AActor>(Inner->GetObjectPropertyValue(Values.GetRawPtr(I)))) {
                        TInlineComponentArray<UPrimitiveComponent*> PartComponents(Part);for(auto* C:PartComponents)Hide(C);
                    }
                }
        }
    } else {
        for(const auto& Entry:ObjectiveVisibility)if(Entry.Key.IsValid())Entry.Key->SetHiddenInGame(Entry.Value);
        ObjectiveVisibility.Empty();
    }
}
void ACS6TacticalMatch::UpdateCS2Input() {
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;auto* P=PC->GetPawn();
    const TSet<FName> Removed={TEXT("Sprinting"),TEXT("Lean Left"),TEXT("Lean Right"),TEXT("Cycle Sights"),TEXT("Toggle NVGs"),TEXT("Show Scoreboard"),TEXT("Pause Menu"),TEXT("Interact"),TEXT("Throw"),TEXT("Melee"),TEXT("Change Firing Mode")};
    for(AActor* InputOwner:{static_cast<AActor*>(PC),static_cast<AActor*>(P)})if(InputOwner) {
        TInlineComponentArray<UInputComponent*> Inputs(InputOwner);
        for(auto* Input:Inputs)for(int32 I=Input->GetNumActionBindings()-1;I>=0;--I)if(Removed.Contains(Input->GetActionBinding(I).GetActionName()))Input->RemoveActionBinding(I);
    }
    for(const TCHAR* Name:{TEXT("IngameHUD"),TEXT("GameModeHUD"),TEXT("KillerInfoHUD"),TEXT("ScoreboardHUD"),TEXT("ClassSelectionHud"),TEXT("CustomizationHUD")})if(auto* Old=Cast<UUserWidget>(Object(PC,Name)))if(Old->IsInViewport())Old->RemoveFromParent();
    if(PC->WasInputKeyJustPressed(EKeys::B))SetBuyMenu(!bBuyMenuOpen);
    if(PC->WasInputKeyJustPressed(EKeys::Escape))SetPauseMenu(!bPauseMenuOpen);
    const bool WasSpectating=bSpectating;UpdateSpectating();P=PC->GetPawn();
    if(WasSpectating && !bSpectating)return; // The takeover E press must not also pick up a nearby drop.
    if(P && !Bool(P,TEXT("Dead"))) {
        SetBool(P,TEXT("Sprinting"),false);SetBool(P,TEXT("IsLeaning"),false);SetScalar(P,TEXT("Lean"),0);SetScalar(P,TEXT("LeanHands"),0);
        auto* Local=Member(P);
        const bool Attacker=Local && Local->Wallet.attacker;
        if(bBombSelected && (BombCarrier!=P || Phase==ECS6Phase::Resolved))SetBombSelected(false);
        if(PC->WasInputKeyJustPressed(EKeys::Five) && BombCarrier==P && !bBuyMenuOpen)SetBombSelected(true);
        if(PC->WasInputKeyJustPressed(EKeys::One) || PC->WasInputKeyJustPressed(EKeys::Two) || PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))SetBombSelected(false);
        if(!bBuyMenuOpen && !bScenario) {
            if(PC->WasInputKeyJustPressed(EKeys::G)) {
                if(bBombSelected && BombCarrier==P && Phase==ECS6Phase::Live) {
                    ReleaseInteraction(P);BombLocation=P->GetActorLocation()+P->GetActorForwardVector()*100;
                    BombCarrier=nullptr;BombPickupAfter=Age+1;SetBombSelected(false);
                } else if(!bBombSelected)DropCurrentGun(P);
            }
            if(Attacker && !BombCarrier && Phase==ECS6Phase::Live && Age>=BombPickupAfter)PickUpBomb(P);
            const bool Plant=Attacker && bBombSelected && PC->IsInputKeyDown(EKeys::LeftMouseButton);
            const bool Defuse=!Attacker && PC->IsInputKeyDown(EKeys::E);
            if(Plant || Defuse)HoldInteraction(P);else ReleaseInteraction(P);
            if(PC->WasInputKeyJustPressed(EKeys::E) && !IsInteracting(P))PickUpNearbyGun(P);
        } else ReleaseInteraction(P);
        // Preserve Kit fire/reload cooldown flags; gate only objective and menu actions.
        const bool Block=bBuyMenuOpen || bBombSelected || IsInteracting(P);
        if(ObjectivePawn.Get()!=P){bObjectiveBlocked=false;ObjectivePawn=P;}
        if(Block && !bObjectiveBlocked){bSavedShootingAllowed=Bool(P,TEXT("ShootingAllowed"));bSavedCanAim=Bool(P,TEXT("CanAim"));}
        if(Block){SetBool(P,TEXT("ShootingAllowed"),false);SetBool(P,TEXT("CanAim"),false);}
        else if(bObjectiveBlocked){SetBool(P,TEXT("ShootingAllowed"),(Phase==ECS6Phase::Live || Phase==ECS6Phase::Planted));SetBool(P,TEXT("CanAim"),bSavedCanAim);}
        bObjectiveBlocked=Block;
    } else {SetBombSelected(false);bObjectiveBlocked=false;}
    if(bBuyMenuOpen){const auto C=LocalBuyContext();if(!C.alive || !C.inBuyZone || !C.inBuyTime)SetBuyMenu(false);}
}
void ACS6TacticalMatch::ObserveEquipment() {
    ProcessDeathDrops();
    for(auto& M:Roster)if(auto* P=M.Pawn.Get()) {
        auto* W=Cast<AActor>(Object(P,TEXT("CurrentWeapon")));if(!W)continue;
        const int32 Ammo=int32(Number(W,TEXT("CurrentAmmo")));
        if(M.LastWeapon==W && M.LastMagazine>=0 && Ammo<M.LastMagazine)M.Wallet.markUsed(W==Object(P,TEXT("Primary Weapon"))?M.Wallet.gear.primary:M.Wallet.gear.pistol);
        M.LastWeapon=W;M.LastMagazine=Ammo;
    }
}
void UCS6RoundOverlay::NativeConstruct(){Super::NativeConstruct();SetIsFocusable(true);}
FReply UCS6RoundOverlay::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E) {
    if(Match && E.GetKey()==EKeys::Escape){if(Match->bPauseMenuOpen)Match->SetPauseMenu(false);else Match->SetBuyMenu(false);return FReply::Handled();}
    if(Match && Match->bBuyMenuOpen && E.GetKey()==EKeys::B){Match->SetBuyMenu(false);return FReply::Handled();}
    return Super::NativeOnKeyDown(G,E);
}
FReply UCS6RoundOverlay::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) {
    if(!Match || (!Match->bBuyMenuOpen && !Match->bPauseMenuOpen))return Super::NativeOnMouseButtonDown(G,E);
    const FVector2D P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
    for(const auto& Hit:ShopHits)if(Hit.Rect.ContainsPoint(P)) {
        if(Hit.Item<0) {
            if(Hit.Item==-1)Match->SetPauseMenu(false);
            else if(Hit.Item==-4)UKismetSystemLibrary::QuitGame(Match,GetOwningPlayer(),EQuitPreference::Quit,false);
            else {auto* Settings=UGameUserSettings::GetGameUserSettings();Settings->SetOverallScalabilityLevel(Hit.Item==-2?1:3);Settings->ApplySettings(false);}
        } else Match->BuyItem(static_cast<cs6::Item>(Hit.Item),Hit.bRefund || E.GetEffectingButton()==EKeys::RightMouseButton);
        return FReply::Handled();
    }
    return FReply::Handled();
}
