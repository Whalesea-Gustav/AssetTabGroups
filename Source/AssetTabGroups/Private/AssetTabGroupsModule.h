#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

class FAssetTabGroupsModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OpenAssetTabGroups();
	TSharedRef<SDockTab> SpawnAssetTabGroupsTab(const FSpawnTabArgs& Args);

	TSharedPtr<class FUICommandList> PluginCommands;
};
