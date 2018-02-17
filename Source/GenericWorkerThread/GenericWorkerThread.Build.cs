/*
 GenericWorkerThread 0.0.1
 -----
 
*/

using UnrealBuildTool;
using System.IO;

public class GenericWorkerThread: ModuleRules
{
    public GenericWorkerThread(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Private include path
        PrivateIncludePaths.AddRange(new string[] {
            "GenericWorkerThread/Private"
        });

        // Base dependencies
		PublicDependencyModuleNames.AddRange( new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        } );

        // Additional dependencies
		PrivateDependencyModuleNames.AddRange( new string[] { } );
	}
}
