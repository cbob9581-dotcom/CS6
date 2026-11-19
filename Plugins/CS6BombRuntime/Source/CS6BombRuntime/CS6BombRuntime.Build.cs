using UnrealBuildTool;
public class CS6BombRuntime : ModuleRules
{
    public CS6BombRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "NavigationSystem", "AIModule", "UMG", "Slate", "SlateCore", "InputCore" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "MetasoundEditor", "MetasoundEngine", "MetasoundFrontend" });
    }
}
