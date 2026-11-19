#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "CS6SpawnPlacement.h"
#include "Components/TimelineComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/LatentActionManager.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "Navigation/PathFollowingComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "UnrealClient.h"

using namespace CS6TemplateAccess;

ACS6TacticalMatch::ACS6TacticalMatch() { bRulesRunning=false; }
void ACS6TacticalMatch::BeginPlay() {
    Super::BeginPlay();
    bSmoke=FParse::Param(FCommandLine::Get(),TEXT("CS6Smoke"));
#if !UE_BUILD_SHIPPING
    bScenario=FParse::Param(FCommandLine::Get(),TEXT("CS6DefuseScenario")); bSmoke=bSmoke || bScenario;
    bWeaponScenario=FParse::Param(FCommandLine::Get(),TEXT("CS6WeaponScenario"));
    bDeathScenario=FParse::Param(FCommandLine::Get(),TEXT("CS6DeathScenario"));
    bShopScenario=FParse::Param(FCommandLine::Get(),TEXT("CS6ShopScenario"));
    bControlScenario=FParse::Param(FCommandLine::Get(),TEXT("CS6ControlScenario"));
    bScenario=bScenario || bWeaponScenario || bDeathScenario || bShopScenario || bControlScenario; bSmoke=bSmoke || bScenario;
    bFullMatch=FParse::Param(FCommandLine::Get(),TEXT("CS6FullMatch"));
    if(bFullMatch) {
        // Benchmark mode disables audio and breaks the kit's audio-component assumptions.
        // Use a fixed world clock after ordinary game/audio initialization instead.
        FApp::SetFixedDeltaTime(1.0/60.0); FApp::SetUseFixedTimeStep(true);
        UE_LOG(LogTemp,Display,TEXT("CS6 FULL clock fixedHz=60 audio=%d"),FApp::CanEverRenderAudio());
    }
#endif
    if(BombMesh) {
        Device=GetWorld()->SpawnActor<AStaticMeshActor>();
        Device->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        Device->GetStaticMeshComponent()->SetStaticMesh(BombMesh);
        Device->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Device->SetActorScale3D(FVector(0.35)); Device->SetActorHiddenInGame(true);
    }
}
void ACS6TacticalMatch::BootTemplate() {
    auto* PC=GetWorld()->GetFirstPlayerController(); auto* GM=GetWorld()->GetAuthGameMode();
    if(!PC || !PC->PlayerState || !GM || !TemplatePawn) return;
    SetNumber(PC->PlayerState,TEXT("Team"),0);
    // The user enum is reordered: numeric 0 is None, numeric 1 is Assaulter.
    SetNumber(PC->PlayerState,TEXT("Class"),1); SetNumber(PC,TEXT("CharacterClass"),1);
    CallNumber(PC,TEXT("SVR_SetTeam"),0);
    SetObject(PC,TEXT("PrimaryWeaponClass"),StartingRifle.Get());
    SetObject(PC,TEXT("SecondaryWeaponClass"),StartingPistol.Get());
    if(!ConfigureDefaultParts(PC,StartingRifle.Get(),true) || !ConfigureDefaultParts(PC,StartingPistol.Get(),false))UE_LOG(LogTemp,Error,TEXT("CS6 default weapon assembly setup failed"));
    Call(PC,TEXT("ROC_DeleteClassSelectionHUD"));
    Call(GM,TEXT("SpawnCharacterWithClass"),PC);
    Call(PC,TEXT("DisplayHUD"));
    CallNumber(Object(PC,TEXT("IngameHUD")),TEXT("RepresentingTeam"),0);
    PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false;
    PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput();
    auto* GS=GetWorld()->GetGameState();
    SetNumber(GS,TEXT("KillsTillVictory"),1000000);
    GetWorldTimerManager().ClearAllTimersForObject(GS);
    // CreateBots is only the first-spawn factory; SpawnBots consumes the existing respawn queue.
    int Bots=0;
    for(TActorIterator<APawn> It(GetWorld());It;++It) if(It->IsA(TemplatePawn) && !It->IsPlayerControlled()) ++Bots;
    if(Bots==0) { SetNumber(GM,TEXT("ABots"),4); SetNumber(GM,TEXT("BBots"),5); SetBool(GM,TEXT("NoABots"),false); SetBool(GM,TEXT("NoBBots"),false); Call(GM,TEXT("CreateBots")); }
    bBooted=true;
    UE_LOG(LogTemp,Display,TEXT("CS6 template boot: player=%s existingBots=%d"),*GetNameSafe(PC->GetPawn()),Bots);
}
void ACS6TacticalMatch::Enroll() {
    // Wait for the complete roster before committing any position or rule membership.
    struct FCandidate {AController* Controller;ACharacter* Pawn;int32 Team;bool Attacker;AActor* Start=nullptr;CS6Spawn::FPlacement Placement;};
    TArray<FCandidate> Candidates;
    TArray<AController*> Controllers;
    if(auto* PC=GetWorld()->GetFirstPlayerController()) Controllers.Add(PC);
    for(TActorIterator<AController> It(GetWorld());It;++It) if(!It->IsPlayerController()) Controllers.Add(*It);
    int32 TeamCounts[2]={0,0};
    for(auto* C:Controllers) {
        auto* P=Cast<ACharacter>(C->GetPawn());if(!IsValid(P) || !P->IsA(TemplatePawn))continue;
        FreezePawn(P,true);
        if(!C->PlayerState)continue;
        const int Team=int(Number(C->PlayerState,TEXT("Team"),-1)); if(Team!=0 && Team!=1) continue;
        ++TeamCounts[Team];Candidates.Add({C,P,Team,(Team==0)!=bSidesSwapped});
    }
    if(Candidates.Num()!=10 || TeamCounts[0]!=5 || TeamCounts[1]!=5 || !Candidates.ContainsByPredicate([](const FCandidate& C){return C.Controller->IsPlayerController();})) {
        Status=FString::Printf(TEXT("Waiting for roster: %d members (%d/%d)"),Candidates.Num(),TeamCounts[0],TeamCounts[1]);return;
    }
    TArray<AActor*> Starts[2];
    for(TActorIterator<AActor> It(GetWorld());It;++It) {
        if(It->ActorHasTag(TEXT("CS6.TeamA")))Starts[0].Add(*It);
        if(It->ActorHasTag(TEXT("CS6.TeamB")))Starts[1].Add(*It);
    }
    for(auto& Side:Starts)Side.Sort([](const AActor& A,const AActor& B){return A.GetName()<B.GetName();});
    if(Starts[0].Num()<5 || Starts[1].Num()<5){Status=TEXT("Missing team spawn points");return;}
    int32 SpawnIndex[2]={0,0};TArray<FVector> Reserved;
    for(auto& C:Candidates) {
        const int32 Side=C.Attacker?0:1;C.Start=Starts[Side][SpawnIndex[Side]++];
        if(!CS6Spawn::Plan(C.Pawn,C.Start->GetActorLocation(),Reserved,C.Placement)){Status=FString::Printf(TEXT("Spawn blocked: %s at %s %s"),*C.Pawn->GetName(),*C.Start->GetActorLocation().ToCompactString(),*C.Placement.Failure);return;}
        Reserved.Add(C.Placement.Center);
    }
    Combatants.Empty();BombCarrier=nullptr;
    for(auto& C:Candidates) {
        for(const TCHAR* Name:{TEXT("WantsToCrouch"),TEXT("Crouching"),TEXT("Sprinting"),TEXT("IsLeaning")})SetBool(C.Pawn,Name,false);
        if(auto* Timeline=Cast<UTimelineComponent>(Object(C.Pawn,TEXT("Crouch_TL")))){Timeline->Stop();Timeline->SetPlaybackPosition(0,false);}
        if(auto* Alpha=CastField<FNumericProperty>(Field(C.Pawn,TEXT("CrouchAlpha"))))Alpha->SetFloatingPointPropertyValue(Alpha->ContainerPtrToValuePtr<void>(C.Pawn),0);
        CS6Spawn::Commit(C.Pawn,C.Placement,C.Start->GetActorRotation());
        C.Controller->SetControlRotation(FRotator(0,C.Start->GetActorRotation().Yaw,0));
        RegisterPawn(C.Pawn,C.Attacker,!C.Attacker);
        if(!Roster.ContainsByPredicate([&C](const FMember& M){return M.Controller==C.Controller;})){FMember M;M.Controller=C.Controller;M.bTeamA=C.Team==0;Roster.Add(M);}
        InitializeEconomy(C.Pawn,C.Controller,C.Team==0);
        if(bSmoke || bFullMatch) {
            const float Height=C.Pawn->GetPawnViewLocation().Z-C.Placement.Ground.Z;
            UE_LOG(LogTemp,Display,TEXT("CS6 SPAWN CHECK pawn=%s swapped=%d gap=%.1f cameraHeight=%.1f penetrating=%d"),*C.Pawn->GetName(),bSidesSwapped,C.Pawn->GetActorLocation().Z-C.Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-C.Placement.Ground.Z,Height,CS6Spawn::IsBlocked(C.Pawn));
        }
    }
    if(bEconomyEnabled && Roster.ContainsByPredicate([](const FMember& M){return M.bNeedsInventory;})){Combatants.Empty();BombCarrier=nullptr;Status=TEXT("Waiting for equipment");return;}
    if(!bScenario) {
        TArray<APawn*> Attackers;for(const auto& C:Combatants)if(C.bAttacker)Attackers.Add(C.Pawn);
        if(!Attackers.IsEmpty())BombCarrier=Attackers[((TeamOneScore+TeamTwoScore)*3+1)%Attackers.Num()];
    }
    bReady=true;bRulesRunning=true;Status=TEXT("Prepare to buy");
    UE_LOG(LogTemp,Display,TEXT("CS6 enrolled 10 combatants; swapped=%d"),bSidesSwapped);
}
void ACS6TacticalMatch::FreezePawn(APawn* P,bool Freeze) {
    if(!IsValid(P)) return;
    P->SetCanBeDamaged(!Freeze);
    SetBool(P,TEXT("ShootingAllowed"),!Freeze); SetBool(P,TEXT("CanShoot"),!Freeze);
    if(auto* PC=Cast<APlayerController>(P->GetController())) {
        if(Freeze && !PC->IsMoveInputIgnored()) PC->SetIgnoreMoveInput(true);
        else if(!Freeze) PC->ResetIgnoreMoveInput();
    }
    if(auto* AI=Cast<AAIController>(P->GetController())) {
        if(Freeze) { AI->StopMovement(); if(auto* B=AI->GetBrainComponent()) if(!PausedBrains.Contains(AI)) {B->PauseLogic(TEXT("Bomb freeze")); PausedBrains.Add(AI);} }
        else if(PausedBrains.Contains(AI)) {if(auto* B=AI->GetBrainComponent()) B->ResumeLogic(TEXT("Round live")); PausedBrains.Remove(AI);}
    }
}
void ACS6TacticalMatch::Tick(float Dt) {
    Age+=Dt;
    if(!bBooted && Age>2) BootTemplate();
    if(bBooted && !bReady && Age>4 && Age>=NextEnroll) {NextEnroll=Age+0.25f;Enroll();}
    if(bScenario && bReady) DriveSmokeScenario();
    for(auto P:WorkingBots) if(P.IsValid()) HoldInteraction(P.Get());
    // Resolve clock/interaction completion before evaluating deaths in this frame.
    Super::Tick(Dt);
    if(bSmoke && Age>60) {UE_LOG(LogTemp,Error,TEXT("CS6 SMOKE FAIL: initialization timeout"));FPlatformMisc::RequestExitWithStatus(false,1);}
    if(bFullMatch && Age>5000) {UE_LOG(LogTemp,Error,TEXT("CS6 FULL MATCH FAIL: exceeded match duration"));FPlatformMisc::RequestExitWithStatus(false,1);}
    if(!bReady) return;
    if(bEconomyEnabled){ObserveEquipment();UpdateCS2Input();}
    if(!bCaptured && Age>25 && FParse::Param(FCommandLine::Get(),TEXT("CS6Capture"))) {bCaptured=true;FScreenshotRequest::RequestScreenshot(TEXT("D:/CS6/reports/military-runtime.png"),true,false);}
    auto* GS=GetWorld()->GetGameState();
    GetWorldTimerManager().ClearAllTimersForObject(GS);
    SetNumber(GS,TEXT("KillsTillVictory"),1000000);
    if(!Overlay) if(auto* PC=GetWorld()->GetFirstPlayerController()) {
        Overlay=CreateWidget<UCS6RoundOverlay>(PC); if(Overlay){Overlay->Match=this;Overlay->AddToViewport(50);Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);}
    }
    if(auto* PC=GetWorld()->GetFirstPlayerController()) if(auto* OldScore=Cast<UUserWidget>(Object(PC,TEXT("GameModeHUD")))) if(OldScore->IsInViewport())OldScore->RemoveFromParent();
    if(auto* PC=GetWorld()->GetFirstPlayerController()) if(auto* P=PC->GetPawn()) if(auto* W=Object(P,TEXT("CurrentWeapon")))
        for(const TCHAR* Name:{TEXT("AmmoWidget"),TEXT("WeaponDisplayWidget"),TEXT("WeaponNameWidget")}) {
            auto* Widget=Object(W,Name);
            if(auto* Component=Cast<USceneComponent>(Widget))Component->SetVisibility(false,true);
            else if(auto* UserWidget=Cast<UWidget>(Widget))UserWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    const bool Frozen=Phase==ECS6Phase::Freeze || Phase==ECS6Phase::Resolved;
    for(const auto& C:Combatants) if(C.bAlive) {
        if(!IsValid(C.Pawn) || Bool(C.Pawn,TEXT("Dead"))) {ReportDeath(C.Pawn);continue;}
        if(Frozen || (Phase==ECS6Phase::Live && PreviousPhase!=Phase)) FreezePawn(C.Pawn,Frozen);
    }
    PreviousPhase=Phase;
    if(auto* PC=GetWorld()->GetFirstPlayerController()) {
        auto* P=PC->GetPawn();
        if(!bScenario && !bEconomyEnabled) {
            if(PC->IsInputKeyDown(EKeys::F)) {if(!BombCarrier && Phase==ECS6Phase::Live) PickUpBomb(P); HoldInteraction(P);}
            else ReleaseInteraction(P);
        }
        if(bMatchOver && PC->WasInputKeyJustPressed(EKeys::R)) UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/CS6/Maps/M_MilitaryBase")));
    }
    if(!bScenario && Age>=NextAI) {NextAI=Age+0.5f; UpdateObjectives();}
    if(Phase==ECS6Phase::Resolved) {ResolvedAge+=Dt;if(!bMatchOver && ResolvedAge>5){bReady=false;bRulesRunning=false;StartNextRound();}}
    if((bSmoke || bFullMatch) && Age>=NextAudit) {NextAudit=Age+10;Audit();}
    if(Device) {
        auto* PC=GetWorld()->GetFirstPlayerController();
        const bool Held=bBombSelected && PC && PC->GetPawn()==BombCarrier;
        Device->SetActorHiddenInGame(!Held && (BombCarrier!=nullptr || Frozen));
        if(Held){FVector Eye;FRotator Look;PC->GetPlayerViewPoint(Eye,Look);Device->SetActorLocation(Eye+Look.RotateVector(FVector(55,16,-20)));Device->SetActorRotation(Look);}
        else {Device->SetActorLocation(BombLocation-FVector(0,0,65));Device->SetActorRotation(FRotator::ZeroRotator);}
    }
}
void ACS6TacticalMatch::RoundEnded_Implementation(bool Attackers,int32 Reason) {
    AwardRoundEconomy(Attackers,Reason);
    if(bControlScenario){bScenarioResolved=Attackers && Reason==int32(cs6::Reason::Eliminated);}
    else if(bShopScenario){}
    else if(bDeathScenario) {bScenarioResolved=Attackers && Reason==int32(cs6::Reason::Eliminated);}
    else if(bScenario && !bWeaponScenario) {bScenarioResolved=Reason==int32(cs6::Reason::Defused);ScenarioStep=3;UE_LOG(LogTemp,Display,TEXT("CS6 SCENARIO resolved defuse=%d"),bScenarioResolved);}
    Status=FString::Printf(TEXT("%s win  |  %s"),Attackers?TEXT("ATTACKERS"):TEXT("DEFENDERS"),Reason==3?TEXT("Device defused"):Reason==4?TEXT("Device detonated"):Reason==2?TEXT("Time expired"):TEXT("Team eliminated"));
    ResolvedAge=0;
    UE_LOG(LogTemp,Display,TEXT("CS6 round ended: reason=%d score=%d:%d"),Reason,TeamOneScore,TeamTwoScore);
}
void ACS6TacticalMatch::RoundReset_Implementation(bool Swapped) {
    bRoundBombPlanted=false;
    for(auto& Drop:DroppedGuns)if(Drop.Visual.IsValid())Drop.Visual->Destroy();DroppedGuns.Empty();
    if(bScenario && bScenarioResolved) bScenarioReset=true;
    if(bEconomyEnabled)for(auto& M:Roster) {
        const bool NewRole=M.bTeamA!=Swapped;
        if(M.Wallet.attacker!=NewRole){M.Wallet.resetHalf(NewRole);M.Snapshot=FCS6WeaponSnapshot{};}
        else {M.Wallet.startRound(M.bSurvived);if(!M.bSurvived)M.Snapshot=FCS6WeaponSnapshot{};}
        M.bNeedsInventory=true;M.bDeathDropPending=false;M.Contributions.Empty();M.LastAttacker.Reset();M.LastAttackerSlot=INDEX_NONE;M.PreviousHP=100;
    }
    auto* GM=GetWorld()->GetAuthGameMode(); auto* GS=GetWorld()->GetGameState();
    // Old unpossessed corpses have latent respawn/UI callbacks too. Retire all old
    // character instances, not just the controller's current spectator pawn.
    GetWorldTimerManager().ClearAllTimersForObject(GM);
    GetWorld()->GetLatentActionManager().RemoveActionsForObject(GM);
    GetWorld()->GetLatentActionManager().RemoveActionsForObject(GS);
    TArray<APawn*> Retired;
    for(TActorIterator<APawn> It(GetWorld());It;++It)if(It->IsA(TemplatePawn))Retired.Add(*It);
    for(auto* P:Retired){GetWorldTimerManager().ClearAllTimersForObject(P);GetWorld()->GetLatentActionManager().RemoveActionsForObject(P);Call(P,TEXT("DestroyWeaponAndAttached"));if(auto* C=P->GetController())C->UnPossess();P->Destroy();}
    TArray<AController*> Bots;
    for(auto& M:Roster) if(auto* C=M.Controller.Get()) {
        if(auto* P=C->GetPawn()) {GetWorldTimerManager().ClearAllTimersForObject(P);GetWorld()->GetLatentActionManager().RemoveActionsForObject(P);if(P->IsA(TemplatePawn))Call(P,TEXT("DestroyWeaponAndAttached"));C->UnPossess();P->Destroy();}
        if(!C->IsPlayerController()) Bots.Add(C);
    }
    // Kit guns, attachments and round drops are separate actors, including unowned pickups.
    TArray<AActor*> RoundItems;
    for(TActorIterator<AActor> It(GetWorld());It;++It) {
        const FString Path=It->GetClass()->GetPathName();
        if(Path.StartsWith(TEXT("/Game/MultiplayerFPS/Game/Weapons/")) || Path.StartsWith(TEXT("/Game/CS6/Weapons/")) || Path.StartsWith(TEXT("/Game/MultiplayerFPS/Game/Blueprints/Core/AI/Blueprints/Throwables/")))RoundItems.Add(*It);
    }
    for(auto* Item:RoundItems)if(IsValid(Item))Item->Destroy();
    UE_LOG(LogTemp,Display,TEXT("CS6 RESET removed round items=%d"),RoundItems.Num());
    SetControllers(GS,TEXT("AIInLoadout"),Bots);
    SetControllers(GS,TEXT("SpawnablePCs"),{});
    Call(GM,TEXT("SpawnBots"));
    if(auto* PC=GetWorld()->GetFirstPlayerController()) {SetNumber(PC->PlayerState,TEXT("Class"),1);SetNumber(PC,TEXT("CharacterClass"),1);CallNumber(PC,TEXT("SVR_SetTeam"),0);Call(PC,TEXT("CleanWidgets"));Call(GM,TEXT("SpawnCharacterWithClass"),PC);Call(PC,TEXT("DisplayHUD"));CallNumber(Object(PC,TEXT("IngameHUD")),TEXT("RepresentingTeam"),0);PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;}
    PausedBrains.Empty(); WorkingBots.Empty(); BotNavigation.Empty(); ResolvedAge=0;Status=TEXT("Prepare for the next round"); Enroll();
}
void ACS6TacticalMatch::Audit() {
    int Alive=0,Attackers=0,Defenders=0,Armed=0;
    int32 WeaponActors=0;
    for(TActorIterator<AActor> It(GetWorld());It;++It) if(IsValid(*It)) {
        const FString Path=It->GetClass()->GetPathName();
        // Attachments inherit the kit weapon base too; count only actual firearms.
        if((Path.StartsWith(TEXT("/Game/MultiplayerFPS/Game/Weapons/Primary/")) || Path.StartsWith(TEXT("/Game/MultiplayerFPS/Game/Weapons/Secondary/")) || Path.StartsWith(TEXT("/Game/CS6/Weapons/"))) && Field(*It,TEXT("MagazineCapacity"))) ++WeaponActors;
    }
    for(const auto& C:Combatants) if(C.bAlive && IsValid(C.Pawn)) {++Alive;C.bAttacker?++Attackers:++Defenders;if(Object(C.Pawn,TEXT("CurrentWeapon")))++Armed;}
    UE_LOG(LogTemp,Display,TEXT("CS6 AUDIT age=%.1f phase=%d combatants=%d alive=%d attackers=%d defenders=%d armed=%d score=%d:%d"),Age,int(Phase),Combatants.Num(),Alive,Attackers,Defenders,Armed,TeamOneScore,TeamTwoScore);
    UE_LOG(LogTemp,Display,TEXT("CS6 INVENTORY weapons=%d"),WeaponActors);
    if(!bReady)UE_LOG(LogTemp,Display,TEXT("CS6 ROSTER WAIT %s"),*Status);
    MaxAuditedFirearms=FMath::Max(MaxAuditedFirearms,WeaponActors);
    bool PlayerArmed=false;
    for(const auto& C:Combatants) if(IsValid(C.Pawn)) {
        auto* Weapon=Object(C.Pawn,TEXT("CurrentWeapon"));
        if(C.Pawn->IsPlayerControlled()) PlayerArmed=Weapon!=nullptr;
        UE_LOG(LogTemp,Display,TEXT("CS6 MEMBER pawn=%s player=%d alive=%d pos=%s weapon=%s"),*C.Pawn->GetName(),C.Pawn->IsPlayerControlled(),C.bAlive,*C.Pawn->GetActorLocation().ToCompactString(),*GetNameSafe(Weapon));
        if(const auto* State=BotNavigation.Find(C.Pawn.Get()))UE_LOG(LogTemp,Display,TEXT("CS6 BOT STATUS pawn=%s mode=%s distance=%.1f recoveries=%d"),*C.Pawn->GetName(),*State->Mode,State->Distance,State->Progress.recoveries);
        if(auto* Char=Cast<ACharacter>(C.Pawn)) {
            auto* M=Char->GetCharacterMovement();auto* Capsule=Char->GetCapsuleComponent();
            UE_LOG(LogTemp,Display,TEXT("CS6 CAPSULE pawn=%s collision=%d query=%d profile=%s actorCollision=%d maxSpeed=%.0f crouched=%.0f rvo=%d floor=%s penetrating=%d depth=%.1f"),*C.Pawn->GetName(),int(Capsule->GetCollisionEnabled()),Capsule->IsQueryCollisionEnabled(),*Capsule->GetCollisionProfileName().ToString(),C.Pawn->GetActorEnableCollision(),M->GetMaxSpeed(),M->MaxWalkSpeedCrouched,M->bUseRVOAvoidance,*GetNameSafe(M->CurrentFloor.HitResult.GetActor()),M->CurrentFloor.HitResult.bStartPenetrating,M->CurrentFloor.HitResult.PenetrationDepth);
        }
    }
    if(Age<15) bInitialRosterValid=Combatants.Num()==10 && Alive==10 && Armed==10 && PlayerArmed;
    if(bFullMatch && bMatchOver) {
        const bool Finished=FMath::Max(TeamOneScore,TeamTwoScore)==13 || (TeamOneScore==12 && TeamTwoScore==12);
        const bool Pass=bInitialRosterValid && bSidesSwapped && Finished && MaxAuditedFirearms<=20;
        UE_LOG(LogTemp,Display,TEXT("CS6 FULL MATCH %s score=%d:%d maxFirearms=%d"),Pass?TEXT("PASS"):TEXT("FAIL"),TeamOneScore,TeamTwoScore,MaxAuditedFirearms);
        FPlatformMisc::RequestExitWithStatus(false,Pass?0:1);
    }
    if(bSmoke && Age>35 && !bAuditDone) {
        bAuditDone=true;
        const bool ScenarioPassed=bControlScenario?bControlPassed:bShopScenario?bShopPassed:bDeathScenario?(bDeathPassed && bScenarioResolved && bScenarioReset && ScenarioStep==4):bWeaponScenario?bWeaponActionsPassed:(!bScenario || (bScenarioResolved && bScenarioReset && TeamTwoScore==1));
        const bool Pass=bInitialRosterValid && Combatants.Num()==10 && Alive>0 && BombSites.Num()==2 && WeaponActors<=20 && ScenarioPassed;
        UE_LOG(LogTemp,Display,TEXT("CS6 SMOKE %s"),Pass?TEXT("PASS"):TEXT("FAIL"));
        FPlatformMisc::RequestExitWithStatus(false,Pass?0:1);
    }
}
void ACS6TacticalMatch::DriveSmokeScenario() {
#if !UE_BUILD_SHIPPING
    if(bControlScenario) {DriveControlScenario();return;}
    if(bShopScenario) {DriveShopScenario();return;}
    if(bWeaponScenario) {DriveWeaponScenario();return;}
    if(bDeathScenario) {DriveDeathScenario();return;}
    if(ScenarioStep==0 && Phase==ECS6Phase::Live && BombSites.Num()) {
        for(const auto& C:Combatants) FreezePawn(C.Pawn,true);
        auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?PC->GetPawn():nullptr;
        if(!P || BombCarrier!=P)return;
        P->SetActorLocation(BombSites[0]->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);
        ScenarioInteractor=P;ScenarioStep=1;
        UE_LOG(LogTemp,Display,TEXT("CS6 SCENARIO begin plant"));
    }
    if(ScenarioStep==1 && Phase==ECS6Phase::Planted) {
        for(const auto& C:Combatants) if(!C.bAttacker && C.bAlive && IsValid(C.Pawn)) {
            C.Pawn->SetActorLocation(BombLocation+FVector(110,0,0),false,nullptr,ETeleportType::TeleportPhysics);
            ScenarioInteractor=C.Pawn;ScenarioStep=2;UE_LOG(LogTemp,Display,TEXT("CS6 SCENARIO begin defuse"));break;
        }
    }
    if(ScenarioStep==1 || ScenarioStep==2) if(auto* P=ScenarioInteractor.Get()) HoldInteraction(P);
#endif
}
void ACS6TacticalMatch::DriveWeaponScenario() {
#if !UE_BUILD_SHIPPING
    auto* PC=GetWorld()->GetFirstPlayerController(); auto* P=PC?PC->GetPawn():nullptr;
    if(!IsValid(P) || Phase!=ECS6Phase::Live)return;
    auto* Weapon=Object(P,TEXT("CurrentWeapon"));
    auto Input=[PC](FKey Key,EInputEvent Event){PC->InputKey(FInputKeyParams(Key,Event,Event==IE_Pressed?1.0:0.0));};
    const float Elapsed=Age-WeaponScenarioStart;
    if(ScenarioStep==0 && Age>20) {
        for(const auto& C:Combatants) if(C.Pawn!=P) FreezePawn(C.Pawn,true);
        PC->SetControlRotation(FRotator(-40,0,0));
        InitialMagazine=int32(Number(Weapon,TEXT("CurrentAmmo")));
        WeaponScenarioStart=Age; Input(EKeys::LeftMouseButton,IE_Pressed); ScenarioStep=1;
    } else if(ScenarioStep==1 && Elapsed>0.65f) {
        Input(EKeys::LeftMouseButton,IE_Released);
        const int32 After=int32(Number(Weapon,TEXT("CurrentAmmo")));
        bWeaponActionsPassed=InitialMagazine>After && After>=0;
        UE_LOG(LogTemp,Display,TEXT("CS6 WEAPON fired before=%d after=%d"),InitialMagazine,After);
        Input(EKeys::R,IE_Pressed); ScenarioStep=2;
    } else if(ScenarioStep==2 && Elapsed>1.f) {Input(EKeys::R,IE_Released);ScenarioStep=3;}
    else if(ScenarioStep==3 && Elapsed>7.f) {
        const int32 Ammo=int32(Number(Weapon,TEXT("CurrentAmmo"))),Capacity=int32(Number(Weapon,TEXT("MagazineCapacity")));
        bWeaponActionsPassed=bWeaponActionsPassed && Ammo==Capacity && Ammo>0;
        UE_LOG(LogTemp,Display,TEXT("CS6 WEAPON reloaded ammo=%d capacity=%d"),Ammo,Capacity);
        Input(EKeys::Two,IE_Pressed);ScenarioStep=4;
    } else if(ScenarioStep==4 && Elapsed>7.2f) {Input(EKeys::Two,IE_Released);ScenarioStep=5;}
    else if(ScenarioStep==5 && Elapsed>8.f) {
        const bool Pistol=Weapon && Weapon->IsA(StartingPistol.Get());
        bWeaponActionsPassed=bWeaponActionsPassed && Pistol;
        UE_LOG(LogTemp,Display,TEXT("CS6 WEAPON switched pistol=%d"),Pistol);
        Input(EKeys::One,IE_Pressed);ScenarioStep=6;
    } else if(ScenarioStep==6 && Elapsed>8.2f) {Input(EKeys::One,IE_Released);ScenarioStep=7;}
    else if(ScenarioStep==7 && Elapsed>9.f) {
        const bool Rifle=Weapon && Weapon->IsA(StartingRifle.Get());
        bWeaponActionsPassed=bWeaponActionsPassed && Rifle;
        UE_LOG(LogTemp,Display,TEXT("CS6 WEAPON switched rifle=%d"),Rifle);ScenarioStep=8;
    }
#endif
}
void ACS6TacticalMatch::DriveDeathScenario() {
#if !UE_BUILD_SHIPPING
    auto* PC=GetWorld()->GetFirstPlayerController(); if(!PC)return;
    auto Damage=[](APawn* Victim,APawn* Killer) {
        if(!IsValid(Victim) || !IsValid(Killer))return;
        auto* Weapon=Cast<AActor>(Object(Killer,TEXT("CurrentWeapon")));
        Victim->SetCanBeDamaged(true);
        UGameplayStatics::ApplyDamage(Victim,10000.f,Killer->GetController(),Weapon,Cast<UClass>(Object(Weapon,TEXT("DamageType"))));
    };
    if(ScenarioStep==0 && Phase==ECS6Phase::Live && Age>20) {
        for(const auto& C:Combatants) FreezePawn(C.Pawn,true);
        ScenarioVictim=PC->GetPawn();
        for(const auto& C:Combatants) if(!C.bAttacker && C.bAlive) {Damage(ScenarioVictim.Get(),C.Pawn);break;}
        ScenarioStep=1;
    // The supplied kit keeps its death camera for three seconds before spectating.
    } else if(ScenarioStep==1 && Age>24) {
        auto* Original=ScenarioVictim.Get();auto* Current=PC->GetPawn();
        const bool Dead=!IsValid(Original) || Bool(Original,TEXT("Dead"));
        const bool Spectating=!Current || (Current!=Original && !Current->IsA(TemplatePawn));
        const bool Dropped=!BombCarrier;
        bDeathPassed=Dead && Dropped && Spectating;
        UE_LOG(LogTemp,Display,TEXT("CS6 DEATH original_dead=%d dropped=%d spectator=%d"),Dead,Dropped,Spectating);
        UE_LOG(LogTemp,Display,TEXT("CS6 DEATH camera pawn=%s target=%s"),*GetNameSafe(Current),*GetNameSafe(PC->GetViewTarget()));
        for(const auto& C:Combatants) if(C.bAttacker && C.bAlive && IsValid(C.Pawn)) {
            C.Pawn->SetActorLocation(BombLocation+FVector(110,0,0),false,nullptr,ETeleportType::TeleportPhysics);
            ScenarioInteractor=C.Pawn;PickUpBomb(C.Pawn);break;
        }
        ScenarioStep=2;
    } else if(ScenarioStep==2 && Age>25) {
        auto* Current=PC->GetPawn();
        const bool NoRespawn=!Current || !Current->IsA(TemplatePawn);
        const bool Recovered=BombCarrier==ScenarioInteractor.Get() && IsValid(BombCarrier);
        bDeathPassed=bDeathPassed && NoRespawn && Recovered;
        UE_LOG(LogTemp,Display,TEXT("CS6 DEATH recovered=%d no_early_respawn=%d"),Recovered,NoRespawn);
        for(const auto& C:Combatants) if(!C.bAttacker && C.bAlive)Damage(C.Pawn,ScenarioInteractor.Get());
        ScenarioStep=3;
    } else if(ScenarioStep==3 && bScenarioReset && bReady && Age>35) {
        int32 Alive=0;for(const auto& C:Combatants)if(C.bAlive && IsValid(C.Pawn))++Alive;
        auto* P=PC->GetPawn();const bool Replaced=IsValid(P) && P->IsA(TemplatePawn) && P!=ScenarioVictim.Get() && !Bool(P,TEXT("Dead"));
        bDeathPassed=bDeathPassed && Alive==10 && Replaced && TeamOneScore==1 && TeamTwoScore==0;
        UE_LOG(LogTemp,Display,TEXT("CS6 DEATH reset_alive=%d player_replaced=%d score=%d:%d"),Alive,Replaced,TeamOneScore,TeamTwoScore);ScenarioStep=4;
    }
#endif
}
void ACS6TacticalMatch::EndPlay(const EEndPlayReason::Type Reason) {
    if(Overlay) Overlay->RemoveFromParent(); Super::EndPlay(Reason);
}

