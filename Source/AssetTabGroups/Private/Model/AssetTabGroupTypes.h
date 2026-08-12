#pragma once

#include "CoreMinimal.h"

struct ASSETTABGROUPS_API FAssetTabGroupMember
{
	FString AssetPath;
	FString DisplayName;
	FString PackagePath;

	bool IsValid() const
	{
		return !AssetPath.IsEmpty();
	}

	bool operator==(const FAssetTabGroupMember& Other) const
	{
		return AssetPath == Other.AssetPath;
	}
};

struct ASSETTABGROUPS_API FAssetTabGroup
{
	FGuid Id;
	FString Name;
	FString Note;
	int32 ColorId = 0;
	bool bCollapsed = false;
	TArray<FAssetTabGroupMember> Members;
	FString ActiveAssetPath;
	int64 CreatedAtEpochMs = 0;
	int64 UpdatedAtEpochMs = 0;
	int64 LastUsedAtEpochMs = 0;

	bool IsValid() const
	{
		return Id.IsValid() && !Name.IsEmpty();
	}

	bool ContainsAsset(const FString& InAssetPath) const
	{
		return Members.ContainsByPredicate(
			[&InAssetPath](const FAssetTabGroupMember& Member)
			{
				return Member.AssetPath == InAssetPath;
			});
	}
};

struct ASSETTABGROUPS_API FAssetTabGroupWorkspace
{
	int32 SchemaVersion = 1;
	TArray<FAssetTabGroup> Groups;
	bool bFocusSafetyNoticeAcknowledged = false;
};
