#include "AssetTabGroupsModule.h"

#include "AssetTabGroupsStyle.h"
#include "UI/SAssetTabGroupsPanel.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "AssetTabGroups"

namespace AssetTabGroups
{
	static const FName TabName(TEXT("AssetTabGroups"));
}

void FAssetTabGroupsModule::StartupModule()
{
	FAssetTabGroupsStyle::Initialize();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		AssetTabGroups::TabName,
		FOnSpawnTab::CreateRaw(this, &FAssetTabGroupsModule::SpawnAssetTabGroupsTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Asset Tab Groups"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Open the Asset Tab Groups panel."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetTabGroupsModule::RegisterMenus));
}

void FAssetTabGroupsModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AssetTabGroups::TabName);
	FAssetTabGroupsStyle::Shutdown();
}

void FAssetTabGroupsModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
	FToolMenuSection& Section = WindowMenu->FindOrAddSection(TEXT("WindowLayout"));
	Section.AddMenuEntry(
		"AssetTabGroups.Open",
		LOCTEXT("OpenMenuEntry", "Asset Tab Groups"),
		LOCTEXT("OpenMenuEntryTooltip", "Open the Asset Tab Groups panel."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FAssetTabGroupsModule::OpenAssetTabGroups)));
}

void FAssetTabGroupsModule::OpenAssetTabGroups()
{
	FGlobalTabmanager::Get()->TryInvokeTab(AssetTabGroups::TabName);
}

TSharedRef<SDockTab> FAssetTabGroupsModule::SpawnAssetTabGroupsTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("TabLabel", "Asset Tab Groups"));

	Tab->SetContent(SNew(SAssetTabGroupsPanel));
	return Tab;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetTabGroupsModule, AssetTabGroups)
