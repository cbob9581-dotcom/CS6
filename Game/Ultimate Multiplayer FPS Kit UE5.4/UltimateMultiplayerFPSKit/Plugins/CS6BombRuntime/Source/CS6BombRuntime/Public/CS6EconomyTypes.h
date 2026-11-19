#pragma once
#include "CoreMinimal.h"
#include "CS6EconomyTypes.generated.h"
class AActor;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FCS6WeaponOffer {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 ItemKey=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<AActor> WeaponClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<AActor> VisualClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMesh> VisualMesh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FTransform VisualOffset=FTransform::Identity;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float Damage=30;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float FireInterval=0.1f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float ArmorPenetration=0.5f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MoveSpeed=240;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Magazine=20;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Reserve=100;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bScoped=false;
};

struct FCS6WeaponSnapshot {
    int32 PrimaryAmmo=-1,PistolAmmo=-1,PrimaryReserve=-1,PistolReserve=-1;
    FString AmmoPouch;
};
struct FCS6KillNotice {FString Killer,Victim,Weapon;float Time=0;bool bFriendly=false;};
