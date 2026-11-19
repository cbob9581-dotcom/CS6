#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

namespace CS6TemplateAccess {
inline FProperty* Field(UObject* O,const TCHAR* Name) { return O ? O->GetClass()->FindPropertyByName(Name) : nullptr; }
inline int64 Number(UObject* O,const TCHAR* Name,int64 Fallback=0) {
    if(auto* P=CastField<FNumericProperty>(Field(O,Name))) return P->GetSignedIntPropertyValue(P->ContainerPtrToValuePtr<void>(O));
    return Fallback;
}
inline double Scalar(UObject* O,const TCHAR* Name,double Fallback=0) {
    if(auto* P=CastField<FNumericProperty>(Field(O,Name))) {
        const void* Value=P->ContainerPtrToValuePtr<void>(O);
        return P->IsFloatingPoint()?P->GetFloatingPointPropertyValue(Value):double(P->GetSignedIntPropertyValue(Value));
    }
    return Fallback;
}
inline void SetNumber(UObject* O,const TCHAR* Name,int64 Value) {
    if(auto* P=CastField<FNumericProperty>(Field(O,Name))) P->SetIntPropertyValue(P->ContainerPtrToValuePtr<void>(O),Value);
}
inline void SetScalar(UObject* O,const TCHAR* Name,double Value) {
    if(auto* P=CastField<FNumericProperty>(Field(O,Name))) {
        void* Target=P->ContainerPtrToValuePtr<void>(O);
        if(P->IsFloatingPoint())P->SetFloatingPointPropertyValue(Target,Value);else P->SetIntPropertyValue(Target,int64(Value));
    }
}
inline void SetBool(UObject* O,const TCHAR* Name,bool Value) {
    if(auto* P=CastField<FBoolProperty>(Field(O,Name))) P->SetPropertyValue_InContainer(O,Value);
}
inline bool Bool(UObject* O,const TCHAR* Name) {
    if(auto* P=CastField<FBoolProperty>(Field(O,Name))) return P->GetPropertyValue_InContainer(O);
    return false;
}
inline UObject* Object(UObject* O,const TCHAR* Name) {
    if(auto* P=CastField<FObjectPropertyBase>(Field(O,Name))) return P->GetObjectPropertyValue_InContainer(O);
    return nullptr;
}
inline void SetObject(UObject* O,const TCHAR* Name,UObject* Value) {
    if(auto* P=CastField<FObjectPropertyBase>(Field(O,Name))) P->SetObjectPropertyValue_InContainer(O,Value);
}
inline bool Call(UObject* O,const TCHAR* Name,UObject* Argument=nullptr) {
    auto* F=O?O->FindFunction(Name):nullptr; if(!F) { UE_LOG(LogTemp,Error,TEXT("CS6 missing template function %s"),Name); return false; }
    FStructOnScope Params(F);
    if(Argument) for(TFieldIterator<FProperty> It(F);It && It->HasAnyPropertyFlags(CPF_Parm);++It) {
        if(auto* P=CastField<FObjectPropertyBase>(*It)) { P->SetObjectPropertyValue_InContainer(Params.GetStructMemory(),Argument); break; }
    }
    O->ProcessEvent(F,Params.GetStructMemory()); return true;
}
inline bool CallNumber(UObject* O,const TCHAR* Name,int64 Value) {
    auto* F=O?O->FindFunction(Name):nullptr;if(!F)return false;
    FStructOnScope Params(F);
    for(TFieldIterator<FProperty> It(F);It && It->HasAnyPropertyFlags(CPF_Parm);++It) {
        if(auto* P=CastField<FNumericProperty>(*It)){P->SetIntPropertyValue(P->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),Value);break;}
    }
    O->ProcessEvent(F,Params.GetStructMemory());return true;
}
inline void SetControllers(UObject* O,const TCHAR* Name,const TArray<AController*>& Values) {
    if(auto* P=CastField<FArrayProperty>(Field(O,Name))) {
        auto* Inner=CastField<FObjectPropertyBase>(P->Inner); if(!Inner) return;
        FScriptArrayHelper Array(P,P->ContainerPtrToValuePtr<void>(O)); Array.EmptyValues();
        for(auto* C:Values) Inner->SetObjectPropertyValue(Array.GetRawPtr(Array.AddValue()),C);
    }
}
inline FProperty* PrefixField(const UStruct* Struct,const FString& Prefix) {
    for(TFieldIterator<FProperty> It(Struct);It;++It) if(It->GetName()==Prefix || It->GetName().StartsWith(Prefix+TEXT("_")))return *It;
    return nullptr;
}
inline int32 ReserveAmmo(UObject* Pawn,UObject* Weapon) {
    auto* P=CastField<FArrayProperty>(Field(Pawn,TEXT("PlayerAmmoPouch")));
    auto* Struct=P?CastField<FStructProperty>(P->Inner):nullptr;
    if(!Struct || !Weapon)return -1;
    auto* Type=CastField<FNumericProperty>(PrefixField(Struct->Struct,TEXT("AmmoType")));
    auto* Ammo=CastField<FNumericProperty>(PrefixField(Struct->Struct,TEXT("CurrentAmmo")));
    if(!Type || !Ammo)return -1;
    FScriptArrayHelper Array(P,P->ContainerPtrToValuePtr<void>(Pawn));
    for(int32 I=0;I<Array.Num();++I) if(Type->GetSignedIntPropertyValue(Type->ContainerPtrToValuePtr<void>(Array.GetRawPtr(I)))==Number(Weapon,TEXT("AmmoType")))
        return int32(Ammo->GetSignedIntPropertyValue(Ammo->ContainerPtrToValuePtr<void>(Array.GetRawPtr(I))));
    return -1;
}
inline bool ConfigureDefaultParts(APlayerController* PC,UClass* Weapon,bool Primary) {
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/MultiplayerFPS/Game/Blueprints/Customization/Data/DT_WeaponSelection.DT_WeaponSelection"));
    if(!Table || !Weapon)return false;
    auto* Select=CastField<FObjectPropertyBase>(PrefixField(Table->GetRowStruct(),TEXT("WeaponSelect")));
    auto* Function=PC->FindFunction(Primary?TEXT("AddDefaultStartingPartsPrimary"):TEXT("AddDefaultStartingPartsSecondary"));
    if(!Select || !Function)return false;
    for(const auto& Row:Table->GetRowMap()) if(auto* Base=Cast<UClass>(Select->GetObjectPropertyValue_InContainer(Row.Value));Base && Weapon->IsChildOf(Base)) {
        if(auto* P=CastField<FArrayProperty>(Field(PC,Primary?TEXT("Primary Starting Parts"):TEXT("Secondary Starting Parts"))))FScriptArrayHelper(P,P->ContainerPtrToValuePtr<void>(PC)).EmptyValues();
        FStructOnScope Params(Function);int32 Count=0;
        for(TFieldIterator<FProperty> It(Function);It && It->HasAnyPropertyFlags(CPF_Parm);++It) if(auto* Param=CastField<FObjectPropertyBase>(*It)) {
            auto* Default=CastField<FObjectPropertyBase>(PrefixField(Table->GetRowStruct(),TEXT("Default")+Param->GetName()));
            if(Default) {auto* Part=Default->GetObjectPropertyValue_InContainer(Row.Value);Param->SetObjectPropertyValue_InContainer(Params.GetStructMemory(),Part);if(Part)++Count;}
        }
        PC->ProcessEvent(Function,Params.GetStructMemory());
        UE_LOG(LogTemp,Display,TEXT("CS6 DEFAULT PARTS weapon=%s row=%s count=%d"),*Weapon->GetName(),*Row.Key.ToString(),Count);
        return true;
    }
    return false;
}
inline void SetPouchAmmo(UObject* Pawn,int32 AmmoType,int32 Amount) {
    auto* P=CastField<FArrayProperty>(Field(Pawn,TEXT("PlayerAmmoPouch")));auto* S=P?CastField<FStructProperty>(P->Inner):nullptr;if(!S)return;
    auto* Type=CastField<FNumericProperty>(PrefixField(S->Struct,TEXT("AmmoType")));
    auto* Current=CastField<FNumericProperty>(PrefixField(S->Struct,TEXT("CurrentAmmo")));
    auto* Maximum=CastField<FNumericProperty>(PrefixField(S->Struct,TEXT("MaxAmmoAbleToHold")));
    if(!Type || !Current)return;
    FScriptArrayHelper Array(P,P->ContainerPtrToValuePtr<void>(Pawn));
    for(int32 I=0;I<Array.Num();++I)if(Type->GetSignedIntPropertyValue(Type->ContainerPtrToValuePtr<void>(Array.GetRawPtr(I)))==AmmoType) {
        Current->SetIntPropertyValue(Current->ContainerPtrToValuePtr<void>(Array.GetRawPtr(I)),int64(Amount));
        if(Maximum)Maximum->SetIntPropertyValue(Maximum->ContainerPtrToValuePtr<void>(Array.GetRawPtr(I)),int64(Amount));
        break;
    }
}
inline void SetReserveAmmo(UObject* Pawn,UObject* Gun,int32 Amount){SetPouchAmmo(Pawn,int32(Number(Gun,TEXT("AmmoType"))),Amount);}
}
