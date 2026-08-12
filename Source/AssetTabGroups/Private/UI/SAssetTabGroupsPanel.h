#pragma once

#include "CoreMinimal.h"
#include "AssetTabGroupsCompatibility.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"

#include "Commands/AssetTabGroupCommands.h"

class ITableRow;
class STableViewBase;
class SListViewBase;
template<typename ItemType> class SListView;
class SVerticalBox;
class FAssetThumbnail;
class FAssetThumbnailPool;
class UAssetTabGroupsSubsystem;
struct FPointerEvent;

enum class EAssetTabGroupViewMode : uint8
{
	List,
	Tile
};

class ASSETTABGROUPS_API SAssetTabGroupsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetTabGroupsPanel)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAssetTabGroupsPanel() override;

private:
	void Rebuild();
	void RebuildGroupItems();
	void RebuildDetails();
	void HandleGroupsChanged();
	void GroupSelectionChanged(TSharedPtr<FGuid> GroupId, ESelectInfo::Type SelectInfo);
	void ReorderGroupFromDrop(const FGuid SourceGroupId, const FGuid TargetGroupId, bool bDropAfter);
	TSharedRef<ITableRow> GenerateGroupRow(TSharedPtr<FGuid> GroupId, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateMemberRow(TSharedPtr<FAssetTabGroupMember> Member, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<SWidget> MakeMemberTile(const FAssetTabGroupMember& Member);
	void HandleMemberTileSelection(const FGuid GroupId, const FString& AssetPath, const FPointerEvent& MouseEvent);
	bool IsMemberSelected(const FString& AssetPath) const;
	void OpenSelectedMembers(const FGuid GroupId);
	void RemoveSelectedMembers(const FGuid GroupId);
	void ToggleViewMode();

	TSharedRef<SWidget> MakeGroupRowWidget(const FAssetTabGroup& Group);
	TSharedRef<SWidget> MakeGroupDetails(const FAssetTabGroup& Group);
	TSharedRef<SWidget> MakeGroupMenu(const FGuid GroupId);
	TSharedRef<SWidget> MakeEmptyGroupMenu();
	TSharedRef<SWidget> MakeGroupDetailsMenu(const FGuid GroupId);
	TSharedRef<SWidget> MakeMemberMenu(const FGuid GroupId, const FString& AssetPath, const FString& DisplayName);
	TSharedRef<SWidget> MakeTileMemberMenu(const FGuid GroupId, const FString& AssetPath, const FString& DisplayName);
	TSharedRef<SWidget> MakeColorMenu(const FGuid GroupId);

	void OpenGroup(const FGuid GroupId);
	void FocusGroup(const FGuid GroupId);
	void OpenMemberAsset(const FGuid GroupId, const FString& AssetPath);
	void AddContentBrowserAssets(const FGuid GroupId);
	void AddSelectedContentBrowserAssets(const FGuid GroupId);
	void AddAssetDataToGroup(const FGuid GroupId, const TArray<FAssetData>& AssetData);
	void ApplyRestoreResult(const FGuid GroupId, const FAssetTabGroupOperationResult& Result);
	void ShowRestoreResult(const FText& ActionName, const FAssetTabGroupOperationResult& Result) const;
	void RemoveMissingMembers(const FGuid GroupId);

	void CreateEmptyGroup();
	void SaveAllOpenAssets();
	void SaveSelectedOpenAssets();
	void AddOpenAssets(const FGuid GroupId);
	void UpdateFromOpenAssets(const FGuid GroupId);
	void RenameGroup(const FGuid GroupId);
	void EditGroupNote(const FGuid GroupId);
	void DeleteGroup(const FGuid GroupId);

	bool PromptForText(const FText& Title, const FText& Prompt, FString& InOutText) const;
	bool PromptForOpenAssetSelection(TArray<FAssetTabGroupMember>& OutMembers, FString& OutActiveAssetPath);

	UAssetTabGroupsSubsystem* Subsystem = nullptr;
	TUniquePtr<FAssetTabGroupCommands> Commands;
	TSharedPtr<SListView<TSharedPtr<FGuid>>> GroupListView;
	TSharedPtr<SListView<TSharedPtr<FAssetTabGroupMember>>> MemberListView;
	TSharedPtr<SVerticalBox> DetailsBox;
	TArray<TSharedPtr<FGuid>> GroupItems;
	TArray<TSharedPtr<FAssetTabGroupMember>> CurrentMemberItems;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TArray<TSharedPtr<FAssetThumbnail>> ActiveThumbnails;
	TSet<FString> SelectedMemberAssetPaths;
	FString MemberSelectionAnchorAssetPath;
	FGuid SelectedGroupId;
	FGuid PendingGroupContextMenuId;
	FGuid PendingMemberContextMenuGroupId;
	FString PendingMemberContextMenuAssetPath;
	FString PendingMemberContextMenuDisplayName;
	EAssetTabGroupViewMode ViewMode = EAssetTabGroupViewMode::List;
	TMap<FGuid, TMap<FString, FString>> MissingAssetReasons;
	FDelegateHandle GroupsChangedHandle;
};
