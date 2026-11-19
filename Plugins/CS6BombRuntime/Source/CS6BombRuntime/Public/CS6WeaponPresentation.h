#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CS6WeaponPresentation.generated.h"
class UStaticMeshComponent;
class USkeletalMeshComponent;
struct FCS6WeaponOffer;

// Uses the supplied assembled models while the kit skeleton drives moving parts.
UCLASS()
class CS6BOMBRUNTIME_API UCS6WeaponPresentation : public UActorComponent {
    GENERATED_BODY()
public:
    UCS6WeaponPresentation();
    bool Initialize(const FCS6WeaponOffer& Offer);
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick) override;
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FirstPerson;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ThirdPerson;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> FP;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> TP;
};
