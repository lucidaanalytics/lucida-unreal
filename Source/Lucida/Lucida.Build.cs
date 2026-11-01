using System.IO;
using UnrealBuildTool;

public class Lucida : ModuleRules
{
	public Lucida(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "HTTP" });

		// The engine-free core is the header-only C++ SDK (sdks/cpp). A copy vendored into
		// Source/Lucida/ThirdParty/include wins; otherwise the plugin is used from inside the repo.
		string Vendored = Path.Combine(ModuleDirectory, "ThirdParty", "include");
		string Repo = Path.GetFullPath(Path.Combine(PluginDirectory, "..", "..", "cpp", "include"));
		PrivateIncludePaths.Add(Directory.Exists(Vendored) ? Vendored : Repo);
	}
}
