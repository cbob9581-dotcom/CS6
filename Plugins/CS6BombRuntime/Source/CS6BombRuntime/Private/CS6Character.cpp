#include "CS6Character.h"
#include "CS6TacticalMatch.h"
#include "EngineUtils.h"
#include "CameraRules.h"
#include "Camera/CameraTypes.h"
#include "Components/CapsuleComponent.h"

ACS6Character::ACS6Character(const FObjectInitializer& Initializer):Super(Initializer.SetDefaultSubobjectClass<UCS6CharacterMovement>(ACharacter::CharacterMovementComponentName)){}
FVector ACS6Character::GetPawnViewLocation()const {
    if(!GetWorld())return Super::GetPawnViewLocation();
    if(!CameraMatch.IsValid())for(TActorIterator<ACS6TacticalMatch> It(GetWorld());It;++It){CameraMatch=*It;break;}
    if(!CameraMatch.IsValid() || !CameraMatch->bEconomyEnabled)return Super::GetPawnViewLocation();
    const auto* Capsule=GetCapsuleComponent();const float Half=Capsule->GetScaledCapsuleHalfHeight();
    const auto* Defaults=GetClass()->GetDefaultObject<ACharacter>();
    const float Standing=Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()*Capsule->GetShapeScale();
    return GetActorLocation()+FVector(0,0,cs6::eyeHeight(Half,Standing)-Half);
}
void ACS6Character::CalcCamera(float DeltaTime,FMinimalViewInfo& OutResult) {
    Super::CalcCamera(DeltaTime,OutResult);
}
float ACS6Character::TakeDamage(float Damage,const FDamageEvent& Event,AController* InstigatedBy,AActor* Causer) {
    if(!CanBeDamaged())return 0;
    for(TActorIterator<ACS6TacticalMatch> It(GetWorld());It;++It)if(It->bEconomyEnabled){Damage=It->AdjustDamage(this,Damage,Event,Causer);break;}
    return Super::TakeDamage(Damage,Event,InstigatedBy,Causer);
}
float UCS6CharacterMovement::GetMaxSpeed()const {
    if(!CharacterOwner || !GetWorld())return Super::GetMaxSpeed();
    if(!Match.IsValid())for(TActorIterator<ACS6TacticalMatch> It(GetWorld());It;++It){Match=*It;break;}
    auto* Game=Match.Get();
    if(!Game || !Game->bEconomyEnabled || (MovementMode!=MOVE_Walking && MovementMode!=MOVE_Falling && MovementMode!=MOVE_NavWalking))return Super::GetMaxSpeed();
    return Game->PawnMoveSpeed(CharacterOwner);
}
