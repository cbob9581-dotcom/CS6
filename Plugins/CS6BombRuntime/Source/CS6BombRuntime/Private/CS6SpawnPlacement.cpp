#include "CS6SpawnPlacement.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "NavigationSystem.h"

bool CS6Spawn::Plan(ACharacter* Character,const FVector& Anchor,const TArray<FVector>& Reserved,FPlacement& Out) {
    if(!IsValid(Character))return false;
    auto* World=Character->GetWorld();auto* Capsule=Character->GetCapsuleComponent();
    auto* Defaults=Character->GetClass()->GetDefaultObject<ACharacter>();
    Out.Radius=Defaults->GetCapsuleComponent()->GetUnscaledCapsuleRadius()*Capsule->GetShapeScale();
    Out.HalfHeight=Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()*Capsule->GetShapeScale();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CS6SafeSpawn),true,Character);
    for(TActorIterator<APawn> It(World);It;++It)Query.AddIgnoredActor(*It);
    const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    const FVector Offsets[]={FVector::ZeroVector,FVector(150,0,0),FVector(-150,0,0),FVector(0,150,0),FVector(0,-150,0),FVector(200,200,0),FVector(-200,-200,0)};
    int32 GroundMiss=0,Blocked=0,NavMiss=0;FString LastBlocker;
    for(const FVector& Offset:Offsets) {
        const FVector XY=Anchor+Offset;FHitResult Hit;
        if(!World->LineTraceSingleByChannel(Hit,XY+FVector(0,0,140),XY-FVector(0,0,180),ECC_Visibility,Query) || Hit.ImpactNormal.Z<0.75f){++GroundMiss;continue;}
        if(FMath::Abs(Hit.ImpactPoint.Z-Anchor.Z)>140)continue;
        const FVector Desired=Hit.ImpactPoint+FVector(0,0,Out.HalfHeight+10);
        bool Occupied=false;
        for(const FVector& Other:Reserved)if(FVector::DistSquared2D(Other,Desired)<FMath::Square(Out.Radius*2+20)){Occupied=true;break;}
        if(Occupied)continue;
        TArray<FOverlapResult> Hits;
        if(World->OverlapMultiByChannel(Hits,Desired,FQuat::Identity,Capsule->GetCollisionObjectType(),FCollisionShape::MakeCapsule(Out.Radius,Out.HalfHeight),Query,Responses)) {
            ++Blocked;for(const auto& H:Hits)if(H.bBlockingHit)LastBlocker=GetNameSafe(H.GetActor());continue;
        }
        FNavLocation NavPoint;
        if(Nav && !Nav->ProjectPointToNavigation(Hit.ImpactPoint,NavPoint,FVector(90,90,180))){++NavMiss;continue;}
        Out.Center=Desired;Out.Ground=Hit.ImpactPoint;return true;
    }
    Out.Failure=FString::Printf(TEXT("groundMiss=%d blocked=%d navMiss=%d last=%s"),GroundMiss,Blocked,NavMiss,*LastBlocker);return false;
}
void CS6Spawn::Commit(ACharacter* Character,const FPlacement& P,const FRotator& Rotation) {
    auto* Defaults=Character->GetClass()->GetDefaultObject<ACharacter>();auto* Movement=Character->GetCharacterMovement();
    Character->UnCrouch();Movement->bWantsToCrouch=false;Movement->StopMovementImmediately();
    Character->GetCapsuleComponent()->SetCapsuleSize(Defaults->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),true);
    Character->GetMesh()->SetRelativeTransform(Defaults->GetMesh()->GetRelativeTransform());
    Character->SetActorLocationAndRotation(P.Center,FRotator(0,Rotation.Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
    Movement->SetMovementMode(MOVE_Walking);Movement->bForceNextFloorCheck=true;
}
bool CS6Spawn::IsBlocked(ACharacter* Character) {
    auto* Capsule=Character->GetCapsuleComponent();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CS6PlacedCapsule),false,Character);
    for(TActorIterator<APawn> It(Character->GetWorld());It;++It)Query.AddIgnoredActor(*It);
    return Character->GetWorld()->OverlapBlockingTestByChannel(Character->GetActorLocation(),Capsule->GetComponentQuat(),Capsule->GetCollisionObjectType(),FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()),Query,FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()));
}
