#include "Modules/ModuleManager.h"
#if WITH_EDITOR
#include "Misc/CoreDelegates.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UObjectThreadContext.h"
#include "UObject/Package.h"
#endif
class FCS6BombModule : public IModuleInterface {
#if WITH_EDITOR
    FDelegateHandle EnsureHandle;
    void TraceEnsure() {
        auto* Context=FUObjectThreadContext::Get().GetSerializeContext();
        auto* O=Context?Context->SerializedObject:nullptr;
        UE_LOG(LogTemp,Warning,TEXT("CS6 SERIALIZATION ENSURE object=%s package=%s"),*GetPathNameSafe(O),O?*O->GetOutermost()->GetName():TEXT("None"));
    }
#endif
public:
    virtual void StartupModule() override {
#if WITH_EDITOR
        if(FParse::Param(FCommandLine::Get(),TEXT("CS6TraceLoads"))) EnsureHandle=FCoreDelegates::OnHandleSystemEnsure.AddRaw(this,&FCS6BombModule::TraceEnsure);
#endif
    }
    virtual void ShutdownModule() override {
#if WITH_EDITOR
        if(EnsureHandle.IsValid())FCoreDelegates::OnHandleSystemEnsure.Remove(EnsureHandle);
#endif
    }
};
IMPLEMENT_MODULE(FCS6BombModule, CS6BombRuntime)
