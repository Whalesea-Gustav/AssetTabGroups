#pragma once

#include "CoreMinimal.h"

#include "Model/AssetTabGroupTypes.h"

DECLARE_MULTICAST_DELEGATE(FAssetTabGroupsChanged);

class ASSETTABGROUPS_API FAssetTabGroupRepository
{
public:
	FAssetTabGroupRepository();

	void Initialize();

	const FAssetTabGroupWorkspace& GetWorkspace() const
	{
		return Workspace;
	}

	const TArray<FAssetTabGroup>& GetGroups() const
	{
		return Workspace.Groups;
	}

	FAssetTabGroup* FindGroup(const FGuid& GroupId);
	const FAssetTabGroup* FindGroup(const FGuid& GroupId) const;

	FGuid CreateGroup(const FString& InName, const FString& InNote = FString());
	bool DeleteGroup(const FGuid& GroupId);
	bool RenameGroup(const FGuid& GroupId, const FString& InName);
	bool SetGroupNote(const FGuid& GroupId, const FString& InNote);
	bool SetGroupColor(const FGuid& GroupId, int32 InColorId);
	bool SetGroupCollapsed(const FGuid& GroupId, bool bInCollapsed);
	bool AddMembers(const FGuid& GroupId, const TArray<FAssetTabGroupMember>& InMembers, int32* OutAddedCount = nullptr);
	bool ReplaceMembers(const FGuid& GroupId, const TArray<FAssetTabGroupMember>& InMembers, const FString& InActiveAssetPath);
	bool RemoveMember(const FGuid& GroupId, const FString& InAssetPath, bool* bOutRemoved = nullptr);
	bool SetActiveAsset(const FGuid& GroupId, const FString& InAssetPath);
	bool ReorderGroup(const FGuid& GroupId, int32 NewIndex);
	bool TouchLastUsed(const FGuid& GroupId);
	bool AcknowledgeFocusSafetyNotice();

	bool HasAcknowledgedFocusSafetyNotice() const
	{
		return Workspace.bFocusSafetyNoticeAcknowledged;
	}

	const FString& GetLastError() const
	{
		return LastError;
	}

	FAssetTabGroupsChanged& OnGroupsChanged()
	{
		return GroupsChanged;
	}

	static int64 GetNowEpochMilliseconds();
	static FLinearColor GetColorForId(int32 ColorId);

private:
	bool LoadFromDisk();
	bool SaveToDisk();
	bool CommitChange();
	void NormalizeWorkspace();
	void NormalizeGroup(FAssetTabGroup& Group);
	static void NormalizeMembers(TArray<FAssetTabGroupMember>& Members);
	static FString GetWorkspaceFilename();
	static FString MakeUniqueGroupName(const TArray<FAssetTabGroup>& Groups, const FString& BaseName);

	FAssetTabGroupWorkspace Workspace;
	FString LastError;
	FAssetTabGroupsChanged GroupsChanged;
};
