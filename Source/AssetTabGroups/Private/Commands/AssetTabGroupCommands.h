#pragma once

#include "CoreMinimal.h"

#include "Model/AssetTabGroupTypes.h"

class UAssetTabGroupsSubsystem;

struct FAssetTabGroupOpenFailure
{
	FAssetTabGroupMember Member;
	FString Reason;
};

struct FAssetTabGroupOperationResult
{
	bool bSucceeded = false;
	int32 OpenedAssetCount = 0;
	int32 MissingAssetCount = 0;
	int32 AddedAssetCount = 0;
	int32 ClosedAssetCount = 0;
	int32 KeptAssetCount = 0;
	bool bClosePhaseSkipped = false;
	TArray<FString> MissingAssetPaths;
	TArray<FAssetTabGroupOpenFailure> FailedAssets;
};

class ASSETTABGROUPS_API FAssetTabGroupCommands
{
public:
	explicit FAssetTabGroupCommands(UAssetTabGroupsSubsystem& InSubsystem);

	FGuid CreateEmptyGroup(const FString& InName);
	FGuid CreateGroupFromOpenAssets(const FString& InName);
	FGuid CreateGroupFromMembers(const FString& InName, const TArray<FAssetTabGroupMember>& InMembers, const FString& InActiveAssetPath);

	bool AddOpenAssetsToGroup(const FGuid& GroupId, int32* OutAddedCount = nullptr);
	bool AddMembersToGroup(const FGuid& GroupId, const TArray<FAssetTabGroupMember>& InMembers, int32* OutAddedCount = nullptr);
	bool UpdateGroupFromOpenAssets(const FGuid& GroupId);
	bool RemoveMemberFromGroup(const FGuid& GroupId, const FString& AssetPath);
	bool RemoveMembersFromGroup(const FGuid& GroupId, const TArray<FString>& AssetPaths, int32* OutRemovedCount = nullptr);
	bool DeleteGroup(const FGuid& GroupId);
	bool RenameGroup(const FGuid& GroupId, const FString& InName);
	bool SetGroupNote(const FGuid& GroupId, const FString& InNote);
	bool SetGroupColor(const FGuid& GroupId, int32 ColorId);
	bool SetGroupCollapsed(const FGuid& GroupId, bool bCollapsed);
	bool ReorderGroup(const FGuid& GroupId, int32 NewIndex);

	FAssetTabGroupOperationResult OpenGroup(const FGuid& GroupId);
	FAssetTabGroupOperationResult FocusGroup(const FGuid& GroupId);
	bool OpenAsset(const FString& AssetPath);

	void Notify(const FText& Message, bool bSuccess = true) const;

private:
	FAssetTabGroupOperationResult OpenGroupInternal(const FAssetTabGroup& Group, bool bNotify);
	static TArray<FAssetTabGroupMember> GetOpenAssetMembers(UAssetTabGroupsSubsystem& Subsystem, FString* OutActiveAssetPath);
	static FString MakeOpenResultMessage(const FAssetTabGroupOperationResult& Result, const FText& ActionName);

	UAssetTabGroupsSubsystem& Subsystem;
};
