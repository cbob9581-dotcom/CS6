#pragma once
#include "CoreMinimal.h"
#include "CS6RoundDirector.h"
#include "NavigationProgress.h"
#include "EconomyRules.h"
#include "CS6EconomyTypes.h"
#include "Blueprint/UserWidget.h"
#include "CS6TacticalMatch.generated.h"

class AController;
class AAIController;
class AStaticMeshActor;
class UStaticMesh;
class USkeletalMesh;
class ACameraActor;

UCLASS()
class CS6BOMBRUNTIME_API UCS6RoundOverlay : public UUserWidget {
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<class ACS6TacticalMatch> Match;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
        FSlateWindowElementList& OutDrawElements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    struct FShopHit {FSlateRect Rect;int32 Item=0;bool bRefund=false;};
    mutable TArray<FShopHit> ShopHits;
};

// Adapter for the supplied FPS Kit: keep its character, loadout, damage and combat AI.
UCLASS()
class CS6BOMBRUNTIME_API ACS6TacticalMatch : public ACS6RoundDirector {
    GENERATED_BODY()
    friend class UCS6RoundOverlay;
public:
    ACS6TacticalMatch();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void RoundReset_Implementation(bool bSwapped) override;
    virtual void RoundEnded_Implementation(bool bAttackersWon,int32 Reason) override;
    UPROPERTY(EditAnywhere,Category="Template") TSubclassOf<APawn> TemplatePawn;
    UPROPERTY(EditAnywhere,Category="Template") TSubclassOf<AActor> StartingRifle;
    UPROPERTY(EditAnywhere,Category="Template") TSubclassOf<AActor> StartingPistol;
    UPROPERTY(EditAnywhere,Category="Bomb") TObjectPtr<UStaticMesh> BombMesh;
    UPROPERTY(BlueprintReadOnly,Category="Bomb") FString Status=TEXT("Preparing match");
    UPROPERTY(BlueprintReadOnly,Category="Bomb") bool bReady=false;
    UPROPERTY(EditAnywhere,Category="CS2") bool bEconomyEnabled=false;
    UPROPERTY(EditAnywhere,Category="CS2") TArray<FCS6WeaponOffer> WeaponCatalog;
    bool bBuyMenuOpen=false,bPauseMenuOpen=false;
    bool bRoundBombPlanted=false;
    bool bBombSelected=false;
    FString BuyMessage;
    int32 LocalCash()const;
    const cs6::Gear* LocalGear()const;
    cs6::BuyContext LocalBuyContext()const;
    cs6::BuyResult BuyItem(cs6::Item Item,bool bRefund=false);
    const FCS6WeaponOffer* Offer(cs6::Item Item)const;
    float AdjustDamage(APawn* Victim,float Damage,const struct FDamageEvent& Event,AActor* Causer);
    float PawnMoveSpeed(APawn* Pawn)const;
    const TArray<FCS6KillNotice>& KillNotices()const{return Notices;}
private:
    struct FMember {
        TWeakObjectPtr<AController> Controller;TWeakObjectPtr<APawn> Pawn;
        bool bTeamA=false,bEconomyStarted=false,bSurvived=false,bNeedsInventory=true,bDeathDropPending=false;
        cs6::Economy Wallet;FCS6WeaponSnapshot Snapshot;
        int32 Kills=0,Deaths=0,Assists=0,Damage=0;
        float PreviousHP=100;
        int32 LastMagazine=-1;
        TWeakObjectPtr<AActor> LastWeapon;
        TWeakObjectPtr<AController> LastAttacker;
        int32 LastAttackerSlot=INDEX_NONE;
        TMap<int32,float> Contributions;
    };
    TArray<FMember> Roster;
    TArray<FCS6KillNotice> Notices;
    struct FDroppedGun {TWeakObjectPtr<AActor> Visual;cs6::Item Item=cs6::Item::None;int32 Magazine=0,Reserve=0;FVector Position=FVector::ZeroVector;};
    TArray<FDroppedGun> DroppedGuns;
    UPROPERTY() TObjectPtr<ACameraActor> SpectatorCamera;
    TWeakObjectPtr<APawn> ObservedPawn;
    TWeakObjectPtr<APawn> FirstPersonViewPawn;
    bool bSpectating=false,bSpectateFirstPerson=false;
    TSet<TWeakObjectPtr<AAIController>> PausedBrains;
    TSet<TWeakObjectPtr<APawn>> WorkingBots;
    struct FBotNavigation {
        cs6::NavigationProgress Progress;cs6::AreaProgress Area;FVector AreaAnchor=FVector::ZeroVector;
        FVector Goal=FVector::ZeroVector,LastPosition=FVector::ZeroVector,RecoveryGoal=FVector::ZeroVector,LastEnemyPosition=FVector::ZeroVector;
        float RecoveryUntil=0,LastEnemySeen=-100,Distance=0;
        bool bHasGoal=false;
        FString Mode=TEXT("preparing");
    };
    TMap<TWeakObjectPtr<APawn>,FBotNavigation> BotNavigation;
    UPROPERTY() TObjectPtr<UCS6RoundOverlay> Overlay;
    UPROPERTY() TObjectPtr<AStaticMeshActor> Device;
    float Age=0, NextAI=0, ResolvedAge=0, NextAudit=10, NextEnroll=0;
    bool bBooted=false,bSmoke=false,bAuditDone=false,bCaptured=false,bInitialRosterValid=false;
    bool bScenario=false,bScenarioResolved=false,bScenarioReset=false;
    bool bWeaponScenario=false,bWeaponActionsPassed=false;
    bool bDeathScenario=false,bDeathPassed=false;
    bool bShopScenario=false,bShopPassed=false;
    bool bControlScenario=false,bControlPassed=false;
    FVector ShopOriginalPosition=FVector::ZeroVector;
    bool bFullMatch=false;
    int32 MaxAuditedFirearms=0;
    TWeakObjectPtr<APawn> ScenarioVictim;
    float WeaponScenarioStart=0;
    int32 InitialMagazine=0;
    int32 ScenarioStep=0;
    TWeakObjectPtr<APawn> ScenarioInteractor;
    ECS6Phase PreviousPhase=ECS6Phase::Freeze;
    void BootTemplate();
    void Enroll();
    void FreezePawn(APawn* Pawn,bool bFreeze);
    void UpdateObjectives();
    void Audit();
    void DriveSmokeScenario();
    void DriveWeaponScenario();
    void DriveDeathScenario();
    void DriveShopScenario();
    void DriveControlScenario();
    FMember* Member(APawn* Pawn);
    const FMember* Member(APawn* Pawn)const;
    void InitializeEconomy(APawn* Pawn,AController* Controller,bool bTeamA);
    bool Equip(FMember& Member,APawn* Pawn,bool bRestoreAmmo);
    void CaptureEquipment(FMember& Member);
    bool SpawnDroppedGun(APawn* Pawn,cs6::Item Item,int32 Magazine,int32 Reserve);
    bool DropCurrentGun(APawn* Pawn);
    bool PickUpNearbyGun(APawn* Pawn);
    void ProcessDeathDrops();
    void AwardRoundEconomy(bool bAttackersWon,int32 Reason);
    virtual void ObjectiveCompleted(APawn* Pawn,bool bPlant) override;
    void UpdateCS2Input();
    void SetBombSelected(bool Selected);
    bool bObjectiveBlocked=false;
    bool bSavedShootingAllowed=true,bSavedCanAim=true;
    TWeakObjectPtr<APawn> ObjectivePawn;
    TMap<TWeakObjectPtr<class UPrimitiveComponent>,bool> ObjectiveVisibility;
    float BombPickupAfter=0;
    void SetBuyMenu(bool bOpen);
    void SetPauseMenu(bool bOpen);
    void ObserveEquipment();
    void UpdateSpectating();
    void SetObservedFirstPersonView(APawn* Target,bool bEnable);
    bool TakeOverObservedBot();
    UFUNCTION() void OnCombatDamage(AActor* Victim,float Damage,const class UDamageType* Type,AController* InstigatedBy,AActor* Causer);
public:
    virtual void ReportDeath(APawn* Pawn) override;
};
