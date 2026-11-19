#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "CS6SpawnPlacement.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "Engine/LatentActionManager.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
using namespace CS6TemplateAccess;

void ACS6TacticalMatch::SetObservedFirstPersonView(APawn* Target,bool bEnable) {
    // Bot first-person rigs are never driven by the kit, so the observed view is a clean eye camera:
    // body, arms and weapon visuals are hidden instead of being forced onto a rig that is not posed.
    auto Apply=[this](APawn* Pawn,bool bOn) {
        if(!IsValid(Pawn))return;
        if(bOn)Pawn->Tags.AddUnique(TEXT("CS6.ObservedFP"));else Pawn->Tags.Remove(TEXT("CS6.ObservedFP"));
        if(auto* Body=Cast<ACharacter>(Pawn))Body->GetMesh()->SetVisibility(!bOn,false);
        if(auto* Arms=Cast<USkeletalMeshComponent>(Object(Pawn,TEXT("1P_Arms")))) {
            Arms->SetVisibility(!bOn,false);
            if(!bOn)Arms->SetOnlyOwnerSee(true);
        }
    };
    auto* Previous=FirstPersonViewPawn.Get();
    if(Previous && (Previous!=Target || !bEnable))Apply(Previous,false);
    FirstPersonViewPawn=nullptr;
    if(bEnable && IsValid(Target)) {Apply(Target,true);FirstPersonViewPawn=Target;}
}
void ACS6TacticalMatch::UpdateSpectating() {
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    // The kit can enqueue its delayed spectator transition after native death registration.
    // Retire callbacks on every dead character, including already unpossessed corpses.
    for(const auto& C:Combatants)if(!C.bAlive && IsValid(C.Pawn)){GetWorld()->GetLatentActionManager().RemoveActionsForObject(C.Pawn);GetWorldTimerManager().ClearAllTimersForObject(C.Pawn);}
    auto* Current=PC->GetPawn();
    const auto* Live=Combatants.FindByPredicate([Current](const FCS6Combatant& C){return C.Pawn==Current && C.bAlive;});
    if(Live && IsValid(Current) && !Bool(Current,TEXT("Dead"))){bSpectating=false;ObservedPawn.Reset();SetObservedFirstPersonView(nullptr,false);return;}
    if(!bSpectating) {
        if(Current){GetWorld()->GetLatentActionManager().RemoveActionsForObject(Current);PC->UnPossess();}
        bSpectating=true;SetBuyMenu(false);PC->ResetIgnoreMoveInput();PC->ResetIgnoreLookInput();
    }
    const bool FriendlyAttackers=(Number(PC->PlayerState,TEXT("Team"))==0)!=bSidesSwapped;
    TArray<APawn*> Targets;
    for(const auto& C:Combatants)if(C.bAlive && C.bAttacker==FriendlyAttackers && IsValid(C.Pawn) && !Bool(C.Pawn,TEXT("Dead")))Targets.Add(C.Pawn);
    if(Targets.IsEmpty())return;
    int32 Index=Targets.IndexOfByKey(ObservedPawn.Get());if(Index==INDEX_NONE)Index=0;
    if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))Index=(Index+1)%Targets.Num();
    if(PC->WasInputKeyJustPressed(EKeys::RightMouseButton))Index=(Index+Targets.Num()-1)%Targets.Num();
    if(PC->WasInputKeyJustPressed(EKeys::SpaceBar))bSpectateFirstPerson=!bSpectateFirstPerson;
    ObservedPawn=Targets[Index];
    if(PC->WasInputKeyJustPressed(EKeys::E) && TakeOverObservedBot())return;
    if(!SpectatorCamera)SpectatorCamera=GetWorld()->SpawnActor<ACameraActor>();
    FVector Eye;FRotator Aim;Targets[Index]->GetActorEyesViewPoint(Eye,Aim);
    FVector Position=Eye;
    if(!bSpectateFirstPerson) {
        const FVector Desired=Eye-FRotator(0,Aim.Yaw,0).Vector()*260+FVector(0,0,95);
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(CS6SpectatorCamera),false,Targets[Index]);
        Position=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Desired,ECC_Camera,Query)?Hit.ImpactPoint+Hit.ImpactNormal*14:Desired;
        Aim=(Eye-Position).Rotation();
    }
    SpectatorCamera->SetOwner(nullptr);
    SetObservedFirstPersonView(bSpectateFirstPerson?Targets[Index]:nullptr,bSpectateFirstPerson);
    SpectatorCamera->SetActorLocationAndRotation(Position,Aim);
    if(bControlScenario && bSpectateFirstPerson) {
        static bool Logged=false;if(!Logged) {
            auto* Arms=Cast<USkeletalMeshComponent>(Object(Targets[Index],TEXT("1P_Arms")));auto* Body=Cast<ACharacter>(Targets[Index]);
            const int32 ArmsVisible=Arms && Arms->IsVisible()?1:0,ArmsAnim=Arms && Arms->GetAnimInstance()?1:0;
            const int32 BodyVisible=Body && Body->GetMesh()->IsVisible()?1:0;
            UE_LOG(LogTemp,Display,TEXT("CS6 FP DIAG viewTarget=%s armsVisible=%d armsAnim=%d bodyVisible=%d camera=%s"),*GetNameSafe(PC->GetViewTarget()),ArmsVisible,ArmsAnim,BodyVisible,*Position.ToCompactString());Logged=true;
        }
    }
    AActor* ViewTarget=static_cast<AActor*>(SpectatorCamera);
    if(PC->GetViewTarget()!=ViewTarget)PC->SetViewTarget(ViewTarget);
}
bool ACS6TacticalMatch::TakeOverObservedBot() {
    auto* PC=GetWorld()->GetFirstPlayerController();auto* Old=ObservedPawn.Get();
    if(!PC || !bSpectating || !IsValid(Old))return false;
    auto* C=Combatants.FindByPredicate([Old](const FCS6Combatant& E){return E.Pawn==Old && E.bAlive;});
    auto* M=Member(Old);auto* AI=Cast<AAIController>(Old->GetController());
    const bool Friendly=(Number(PC->PlayerState,TEXT("Team"))==0)!=bSidesSwapped;
    if(!C || C->bAttacker!=Friendly || !M || !AI || Bool(Old,TEXT("Dead")))return false;
    const FVector Location=Old->GetActorLocation();const FRotator Rotation=Old->GetActorRotation();
    const float HP=float(Scalar(Object(Old,TEXT("C_Health")),TEXT("Health"),100));const bool Carrier=BombCarrier==Old;
    CaptureEquipment(*M);ReleaseInteraction(Old);
    AI->StopMovement();if(auto* Brain=AI->GetBrainComponent())Brain->PauseLogic(TEXT("Human takeover"));
    SetNumber(PC->PlayerState,TEXT("Class"),1);SetNumber(PC,TEXT("CharacterClass"),1);
    const auto* Primary=Offer(M->Wallet.gear.primary);const auto* Pistol=Offer(M->Wallet.gear.pistol);
    SetObject(PC,TEXT("PrimaryWeaponClass"),Primary?Primary->WeaponClass.Get():StartingRifle.Get());
    SetObject(PC,TEXT("SecondaryWeaponClass"),Pistol?Pistol->WeaponClass.Get():StartingPistol.Get());
    Call(GetWorld()->GetAuthGameMode(),TEXT("SpawnCharacterWithClass"),PC);auto* New=PC->GetPawn();
    if(!IsValid(New)){if(!PausedBrains.Contains(AI))if(auto* Brain=AI->GetBrainComponent())Brain->ResumeLogic(TEXT("Takeover cancelled"));UE_LOG(LogTemp,Error,TEXT("CS6 TAKEOVER failed to create local pawn"));return false;}
    CS6Spawn::FPlacement Placement;
    if(!CS6Spawn::Plan(Cast<ACharacter>(New),Location,{},Placement)) {PC->UnPossess();Call(New,TEXT("DestroyWeaponAndAttached"));New->Destroy();if(!PausedBrains.Contains(AI))if(auto* Brain=AI->GetBrainComponent())Brain->ResumeLogic(TEXT("Takeover placement cancelled"));return false;}
    if(SpectatorCamera)SpectatorCamera->SetOwner(nullptr);FirstPersonViewPawn.Reset();Old->Tags.Remove(TEXT("CS6.ObservedFP"));
    AI->UnPossess();GetWorld()->GetLatentActionManager().RemoveActionsForObject(Old);Call(Old,TEXT("DestroyWeaponAndAttached"));Old->Destroy();
    C->Pawn=New;M->Pawn=New;M->PreviousHP=HP;Equip(*M,New,true);SetScalar(Object(New,TEXT("C_Health")),TEXT("Health"),HP);
    CS6Spawn::Commit(Cast<ACharacter>(New),Placement,FRotator(0,Rotation.Yaw,0));
    PC->SetControlRotation(FRotator(0,Rotation.Yaw,0));PC->ResetIgnoreMoveInput();PC->ResetIgnoreLookInput();PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
    New->OnTakeAnyDamage.AddUniqueDynamic(this,&ACS6TacticalMatch::OnCombatDamage);
    if(Carrier)BombCarrier=New;
    FreezePawn(New,Phase==ECS6Phase::Freeze || Phase==ECS6Phase::Resolved);
    bSpectating=false;ObservedPawn.Reset();PC->SetViewTarget(New);
    UE_LOG(LogTemp,Display,TEXT("CS6 TAKEOVER health=%.1f carrier=%d roster=%d"),HP,Carrier,Combatants.Num());return true;
}
