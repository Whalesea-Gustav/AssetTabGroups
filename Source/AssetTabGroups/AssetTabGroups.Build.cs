using UnrealBuildTool;

public class AssetTabGroups : ModuleRules
{
	public AssetTabGroups(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		if (Target.Version.MajorVersion < 5)
		{
			PrivateDependencyModuleNames.Add("EditorStyle");
		}
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AssetRegistry",
				"ContentBrowser",
				"EditorSubsystem",
				"InputCore",
				"Json",
				"JsonUtilities",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				"WorkspaceMenuStructure"
			}
		);
	}
}
