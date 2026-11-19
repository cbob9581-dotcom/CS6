#include "CS6TacticalMatch.h"
#include "CS6SpawnPlacement.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"

namespace {
bool ClearEscape(ACharacter* P,UNavigationSystemV1* Nav,const FVector& BlockedGoal,const TArray<FCS6Combatant>& Combatants,FVector& Out) {
    if(!P || !Nav)return false;
    auto* Capsule=P->GetCapsuleComponent();const FVector Start=P->GetActorLocation();
    FVector Away=(Start-BlockedGoal).GetSafeNormal2D();if(Away.IsNearlyZero())Away=-P->GetActorForwardVector();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CS6BotEscape),false,P);for(const auto& C:Combatants)if(C.Pawn)Query.AddIgnoredActor(C.Pawn);
    FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    for(float Radius:{300.f,500.f,750.f})for(float Angle:{0.f,60.f,-60.f,120.f,-120.f,180.f}) {
        const FVector Candidate=Start+Away.RotateAngleAxis(Angle,FVector::UpVector)*Radius;FNavLocation Projected;
        if(!Nav->ProjectPointToNavigation(Candidate,Projected,FVector(100,100,180)))continue;
        CS6Spawn::FPlacement Fit;if(!CS6Spawn::Plan(P,Projected.Location,{},Fit) || FMath::Abs(Fit.Center.Z-Start.Z)>45 || FVector::Dist2D(Start,Fit.Center)<190)continue;
        FHitResult Hit;const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()-8);
        if(!P->GetWorld()->SweepSingleByChannel(Hit,Start+FVector(0,0,8),Fit.Center+FVector(0,0,8),FQuat::Identity,Capsule->GetCollisionObjectType(),Shape,Query,Responses)){Out=Fit.Ground;return true;}
    }
    return false;
}
}

void ACS6TacticalMatch::UpdateObjectives() {
    WorkingBots.Empty();
    if(Phase!=ECS6Phase::Live && Phase!=ECS6Phase::Planted)return;
    if(BombCarrier)BombLocation=BombCarrier->GetActorLocation();
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    int32 Ordinal=0;
    for(const auto& C:Combatants) {
        auto* P=C.Pawn.Get();if(!C.bAlive || !IsValid(P))continue;
        auto* AI=Cast<AAIController>(P->GetController());if(!AI)continue;
        const int32 Index=Ordinal++;auto& State=BotNavigation.FindOrAdd(P);
        if(auto* Character=Cast<ACharacter>(P))Character->StopJumping();
        APawn* Seen=nullptr;float Nearest=FMath::Square(4500.f);
        for(const auto& Enemy:Combatants)if(Enemy.bAlive && Enemy.bAttacker!=C.bAttacker && IsValid(Enemy.Pawn)) {
            const float D=FVector::DistSquared(P->GetActorLocation(),Enemy.Pawn->GetActorLocation());
            if(D<Nearest && AI->LineOfSightTo(Enemy.Pawn)){Nearest=D;Seen=Enemy.Pawn;}
        }
        bool CanPlant=false;
        if(C.bAttacker && BombCarrier==P && Phase==ECS6Phase::Live)
            for(auto Site:BombSites)if(IsValid(Site) && FVector::Dist(P->GetActorLocation(),Site->GetActorLocation())<=PlantRadius){CanPlant=true;break;}
        const bool CanDefuse=Phase==ECS6Phase::Planted && !C.bAttacker && FVector::Dist(P->GetActorLocation(),BombLocation)<=DefuseRadius;
        if(Seen) {State.LastEnemyPosition=Seen->GetActorLocation();State.LastEnemySeen=Age;}
        if(Seen && !CanPlant && !CanDefuse) {
            State.Mode=TEXT("combat");State.Area.update(Age,0,false);State.AreaAnchor=P->GetActorLocation();State.bHasGoal=false;State.Progress.update(Age,false,false,false,false);State.LastPosition=P->GetActorLocation();
            AI->SetFocus(Seen);
            if(PausedBrains.Contains(AI)){if(auto* Brain=AI->GetBrainComponent())Brain->ResumeLogic(TEXT("Visible opponent"));PausedBrains.Remove(AI);}
            continue;
        }
        AI->ClearFocus(EAIFocusPriority::Gameplay);
        if(!PausedBrains.Contains(AI)){if(auto* Brain=AI->GetBrainComponent())Brain->PauseLogic(TEXT("Tactical navigation"));PausedBrains.Add(AI);}
        if(CanPlant || CanDefuse) {
            State.Area.update(Age,0,false);State.AreaAnchor=P->GetActorLocation();State.Mode=CanPlant?TEXT("planting"):TEXT("defusing");State.Progress.update(Age,false,false,false,false);
            AI->StopMovement();HoldInteraction(P);WorkingBots.Add(P);continue;
        }
        FVector Goal=P->GetActorLocation();
        const int32 MainSite=(TeamOneScore+TeamTwoScore)%FMath::Max(1,BombSites.Num());
        if(Phase==ECS6Phase::Planted) {
            Goal=BombLocation;
            if(C.bAttacker)Goal+=FVector((Index%2?1:-1)*280,(Index%3-1)*260,0);
        } else if(C.bAttacker && !BombCarrier) {Goal=BombLocation;PickUpBomb(P);}
        else if(BombSites.Num()) {
            const int32 SiteIndex=C.bAttacker?((BombCarrier==P || Index%3!=0)?MainSite:(MainSite+1)%BombSites.Num()):Index%BombSites.Num();
            if(auto* Site=BombSites[SiteIndex].Get())Goal=Site->GetActorLocation();
        }
        // Search the last observed location briefly; unseen opponents are not tracked.
        if(Age-State.LastEnemySeen<4 && FVector::Dist2D(P->GetActorLocation(),State.LastEnemyPosition)>180 && BombCarrier!=P)
            Goal=State.LastEnemyPosition;
        const float AreaDistance=FVector::Dist2D(P->GetActorLocation(),State.AreaAnchor);
        const bool WantsObjective=FVector::Dist2D(P->GetActorLocation(),Goal)>160;
        const bool AreaStalled=State.Area.update(Age,AreaDistance,WantsObjective);
        if(AreaDistance>200 || !WantsObjective)State.AreaAnchor=P->GetActorLocation();
        if(AreaStalled) {
            FVector Escape;if(ClearEscape(Cast<ACharacter>(P),Nav,Goal,Combatants,Escape)) {
                State.RecoveryGoal=Escape;State.RecoveryUntil=Age+5;State.bHasGoal=false;
                UE_LOG(LogTemp,Display,TEXT("CS6 BOT BACKTRACK pawn=%s physical=1 goal=%s"),*P->GetName(),*Escape.ToCompactString());
            }
            State.AreaAnchor=P->GetActorLocation();
        }
        if(Age<State.RecoveryUntil)Goal=State.RecoveryGoal;
        State.Distance=FVector::Dist2D(P->GetActorLocation(),Goal);
        const bool WantsMove=State.Distance>160;
        const bool GoalChanged=!State.bHasGoal || FVector::DistSquared2D(State.Goal,Goal)>FMath::Square(140.f);
        const bool Moved=FVector::DistSquared2D(State.LastPosition,P->GetActorLocation())>FMath::Square(25.f);
        if(Moved || !State.bHasGoal)State.LastPosition=P->GetActorLocation();
        auto* Following=AI->GetPathFollowingComponent();const auto Path=Following?Following->GetPath():FNavPathSharedPtr();
        const bool HasPath=Following && Following->GetStatus()==EPathFollowingStatus::Moving && Path.IsValid() && !Path->IsPartial();
        const auto Action=State.Progress.update(Age,Moved,WantsMove,HasPath,GoalChanged);
        State.bHasGoal=true;State.Goal=Goal;
        if(Action==cs6::NavAction::Hold){State.Mode=TEXT("guarding");AI->StopMovement();continue;}
        if(Action==cs6::NavAction::KeepPath){State.Mode=Age<State.RecoveryUntil?TEXT("recovering"):TEXT("moving");continue;}
        if(Action==cs6::NavAction::Recover) {
            AI->StopMovement();FVector Escape;
            if(ClearEscape(Cast<ACharacter>(P),Nav,Goal,Combatants,Escape)) {
                Goal=Escape;State.RecoveryGoal=Goal;State.RecoveryUntil=Age+3.5f;State.Goal=Goal;
            }
            if(auto* Character=Cast<ACharacter>(P))if(Character->GetCharacterMovement()->IsMovingOnGround())Character->Jump();
            State.Mode=TEXT("recovering");
            UE_LOG(LogTemp,Display,TEXT("CS6 BOT RECOVER pawn=%s attempts=%d from=%s goal=%s"),*P->GetName(),State.Progress.recoveries,*P->GetActorLocation().ToCompactString(),*Goal.ToCompactString());
        } else State.Mode=TEXT("moving");
        FNavLocation Projected;
        if(Nav && Nav->ProjectPointToNavigation(Goal,Projected,FVector(200,200,250)))Goal=Projected.Location;
        const auto Request=AI->MoveToLocation(Goal,90,true,true,true,false,nullptr,false);
        if(Request==EPathFollowingRequestResult::Failed)State.Mode=TEXT("path-failed");
    }
}
