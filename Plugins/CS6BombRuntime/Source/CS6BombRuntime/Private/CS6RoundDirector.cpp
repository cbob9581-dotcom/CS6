#include "CS6RoundDirector.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ACS6RoundDirector::ACS6RoundDirector() { PrimaryActorTick.bCanEverTick = true; }
FCS6Combatant* ACS6RoundDirector::Find(APawn* P) {
    return Combatants.FindByPredicate([P](const FCS6Combatant& C){ return C.Pawn == P; });
}
void ACS6RoundDirector::RegisterPawn(APawn* P, bool A, bool K) {
    if (!IsValid(P) || Rules.phase != cs6::Phase::Freeze) return;
    if (auto* C = Find(P)) { C->bAttacker=A; C->bDefuseKit=K; C->bAlive=true; return; }
    FCS6Combatant C; C.Pawn=P; C.bAttacker=A; C.bDefuseKit=K; Combatants.Add(C);
    if (A && !BombCarrier) { BombCarrier=P; BombLocation=P->GetActorLocation(); }
}
bool ACS6RoundDirector::PickUpBomb(APawn* P) {
    auto* C = Find(P);
    if (!C || !C->bAlive || !C->bAttacker || BombCarrier || Rules.phase != cs6::Phase::Live) return false;
    if (FVector::Dist(P->GetActorLocation(), BombLocation) > 180) return false;
    BombCarrier=P; return true;
}
void ACS6RoundDirector::ReportDeath(APawn* P) {
    auto* C=Find(P); if (!C || !C->bAlive) return;
    C->bAlive=false;
    // Registered dead pawns no longer take gameplay damage.
    if(IsValid(P)) P->SetCanBeDamaged(false);
    ReleaseInteraction(P);
    if (BombCarrier==P) { if(IsValid(P)) BombLocation=P->GetActorLocation(); BombCarrier=nullptr; }
    int A=0,D=0;
    for (const auto& R:Combatants) if (R.bAlive && IsValid(R.Pawn)) { if (R.bAttacker) ++A; else ++D; }
    Rules.setAlive(A,D); Publish();
}
bool ACS6RoundDirector::HoldInteraction(APawn* P) {
    auto* C=Find(P); if (!C || !C->bAlive || !IsValid(P)) return false;
    if(auto* Character=Cast<ACharacter>(P))if(!Character->GetCharacterMovement()->IsMovingOnGround()){ReleaseInteraction(P);return false;}
    if (P->GetVelocity().Size2D()>15){ReleaseInteraction(P);return false;}
    if (Interactor.Get()==P) { LastHeld=GetWorld()->GetTimeSeconds(); return true; }
    bool Started=false;
    const int Id=Combatants.IndexOfByPredicate([P](const FCS6Combatant& R){return R.Pawn==P;});
    if (C->bAttacker && BombCarrier==P) {
        for (auto Site:BombSites) if (IsValid(Site) && FVector::Dist(P->GetActorLocation(),Site->GetActorLocation())<=PlantRadius) {
            Started=Rules.beginPlant(Id,true); InteractionDuration=3.2; break;
        }
    } else if (!C->bAttacker && FVector::Dist(P->GetActorLocation(),BombLocation)<=DefuseRadius) {
        Started=Rules.beginDefuse(Id,C->bDefuseKit); InteractionDuration=C->bDefuseKit?5:10;
    }
    if (Started) { Interactor=P; InteractionOrigin=P->GetActorLocation(); LastHeld=GetWorld()->GetTimeSeconds(); }
    return Started;
}
void ACS6RoundDirector::ReleaseInteraction(APawn* P) {
    if (Interactor.Get()!=P) return;
    Rules.cancel(Rules.actor); Interactor.Reset(); LastHeld=-1;
}
float ACS6RoundDirector::InteractionProgress() const {
    return Rules.actor<0 || InteractionDuration<=0 ? 0.f : FMath::Clamp(1.f-float(Rules.interaction/InteractionDuration),0.f,1.f);
}
void ACS6RoundDirector::Tick(float Dt) {
    Super::Tick(Dt);
    if (!bRulesRunning) return;
    if (Rules.actor>=0) {
        auto* P=Interactor.Get();
        if (!IsValid(P) || GetWorld()->GetTimeSeconds()-LastHeld>0.15 || FVector::Dist(P->GetActorLocation(),InteractionOrigin)>32) {
            Rules.cancel(Rules.actor); Interactor.Reset();
        }
    }
    const auto Before=Rules.phase;
    Rules.advance(Dt);
    if (Before==cs6::Phase::Live && Rules.phase==cs6::Phase::Planted) {
        BombLocation=InteractionOrigin; BombCarrier=nullptr;ObjectiveCompleted(Interactor.Get(),true);
    }
    if(Before==cs6::Phase::Planted && Rules.phase==cs6::Phase::Resolved && Rules.reason==cs6::Reason::Defused)ObjectiveCompleted(Interactor.Get(),false);
    if (Rules.actor<0) Interactor.Reset();
    Publish();
}
void ACS6RoundDirector::Publish() {
    auto NewPhase=static_cast<ECS6Phase>(Rules.phase);
    const bool Changed=NewPhase!=Phase;
    Phase=NewPhase; SecondsRemaining=float(Rules.remaining);
    TeamOneScore=Rules.score[0]; TeamTwoScore=Rules.score[1]; bMatchOver=Rules.matchOver; bSidesSwapped=Rules.swapped;
    if (Changed) {
        RoundStateChanged();
        if (Rules.phase==cs6::Phase::Resolved) RoundEnded(Rules.winner==cs6::Team::Attackers,static_cast<int32>(Rules.reason));
    }
}
void ACS6RoundDirector::StartNextRound() {
    if (Rules.phase!=cs6::Phase::Resolved || Rules.matchOver) return;
    Rules.resetRound(); Combatants.Empty(); BombCarrier=nullptr; Interactor.Reset();
    LastHeld=-1; BombLocation=FVector::ZeroVector; Publish(); RoundReset(Rules.swapped);
}
