#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CS6Character.generated.h"
class ACS6TacticalMatch;

UCLASS()
class CS6BOMBRUNTIME_API UCS6CharacterMovement : public UCharacterMovementComponent {
    GENERATED_BODY()
public:
    virtual float GetMaxSpeed()const override;
private:
    mutable TWeakObjectPtr<ACS6TacticalMatch> Match;
};

// Native parent for the supplied Blueprint character. Animation and weapon logic stay in the kit.
UCLASS()
class CS6BOMBRUNTIME_API ACS6Character : public ACharacter {
    GENERATED_BODY()
public:
    ACS6Character(const FObjectInitializer& Initializer=FObjectInitializer::Get());
    virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* InstigatedBy,AActor* Causer) override;
    virtual FVector GetPawnViewLocation()const override;
    virtual void CalcCamera(float DeltaTime,struct FMinimalViewInfo& OutResult) override;
private:
    mutable TWeakObjectPtr<ACS6TacticalMatch> CameraMatch;
};
