#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundRules.h"
#include "CS6RoundDirector.generated.h"

class APawn;

UENUM(BlueprintType)
enum class ECS6Phase : uint8 { Freeze, Live, Planted, Resolved };
USTRUCT(BlueprintType)
struct FCS6Combatant {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Bomb") TObjectPtr<APawn> Pawn = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") bool bAttacker = false;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") bool bAlive = true;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") bool bDefuseKit = false;
};

// Add to a gameplay level. Template adapters register existing pawns;
// no character, weapon, animation or health implementation is replaced.
UCLASS(Blueprintable)
class CS6BOMBRUNTIME_API ACS6RoundDirector : public AActor {
    GENERATED_BODY()
public:
    ACS6RoundDirector();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") ECS6Phase Phase = ECS6Phase::Freeze;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") float SecondsRemaining = 15;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") int32 TeamOneScore = 0;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") int32 TeamTwoScore = 0;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") bool bMatchOver = false;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") bool bSidesSwapped = false;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") TObjectPtr<APawn> BombCarrier;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") FVector BombLocation = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") TArray<TObjectPtr<AActor>> BombSites;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") float PlantRadius = 220;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") float DefuseRadius = 160;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") TArray<FCS6Combatant> Combatants;
    UFUNCTION(BlueprintCallable, Category="Bomb") void RegisterPawn(APawn* Pawn, bool bAttacker, bool bHasKit);
    UFUNCTION(BlueprintCallable, Category="Bomb") bool PickUpBomb(APawn* Pawn);
    UFUNCTION(BlueprintCallable, Category="Bomb") virtual void ReportDeath(APawn* Pawn);
    // Call each frame while the interaction key is held; release to cancel.
    UFUNCTION(BlueprintCallable, Category="Bomb") bool HoldInteraction(APawn* Pawn);
    UFUNCTION(BlueprintCallable, Category="Bomb") void ReleaseInteraction(APawn* Pawn);
    UFUNCTION(BlueprintCallable, Category="Bomb") void StartNextRound();
    UFUNCTION(BlueprintPure, Category="Bomb") float InteractionProgress() const;
    bool IsInteracting(APawn* Pawn) const {return Pawn && Interactor.Get()==Pawn;}
    UFUNCTION(BlueprintNativeEvent, Category="Bomb") void RoundStateChanged();
    virtual void RoundStateChanged_Implementation() {}
    UFUNCTION(BlueprintNativeEvent, Category="Bomb") void RoundEnded(bool bAttackersWon, int32 Reason);
    virtual void RoundEnded_Implementation(bool bAttackersWon, int32 Reason) {}
    // Adapter respawns registered pawns, resets inventory/AI and re-registers them.
    UFUNCTION(BlueprintNativeEvent, Category="Bomb") void RoundReset(bool bSwapped);
    virtual void RoundReset_Implementation(bool bSwapped) {}
protected:
    bool bRulesRunning = true;
    virtual void ObjectiveCompleted(APawn* Pawn,bool bPlant) {}
private:
    cs6::Rules Rules;
    TWeakObjectPtr<APawn> Interactor;
    FVector InteractionOrigin;
    double LastHeld = -1;
    double InteractionDuration = 0;
    FCS6Combatant* Find(APawn* Pawn);
    void Publish();
};
