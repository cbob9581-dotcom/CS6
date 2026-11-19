#pragma once
#include "CoreMinimal.h"
class ACharacter;
namespace CS6Spawn {
struct FPlacement {FVector Center,Ground; float Radius=42,HalfHeight=96;FString Failure;};
bool Plan(ACharacter* Character,const FVector& Anchor,const TArray<FVector>& Reserved,FPlacement& Out);
void Commit(ACharacter* Character,const FPlacement& Placement,const FRotator& Rotation);
bool IsBlocked(ACharacter* Character);
}
