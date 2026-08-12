#include "Commands/AssetTabGroupCommands.h"

#include "AssetTabGroupsSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/MessageDialog.h"
#include "Repository/AssetTabGroupRepository.h"
#include "Session/AssetEditorSessionAdapter.h"
#include "Widgets/Notifications/SNotificationList.h"

FAssetTabGroupCommands::FAssetTabGroupCommands(UAssetTabGroupsSubsystem& InSubsystem)
	: Subsystem(InSubsystem)
{
}

FGuid FAssetTabGroupCommands::CreateEmptyGroup(const FString& InName)
{
	const FGuid GroupId = Subsystem.GetRepository().CreateGroup(InName);
	if (GroupId.IsValid())
	{
		Notify(FText::Format(NSLOCTEXT("AssetTabGroups", "CreatedGroup", "Created group '{0}'."), FText::FromString(InName)));
	}
	return GroupId;
}

FGuid FAssetTabGroupCommands::CreateGroupFromOpenAssets(const FString& InName)
{
	FString ActiveAssetPath;
	const TArray<FAssetTabGroupMember> Members = GetOpenAssetMembers(Subsystem, &ActiveAssetPath);
	if (Members.Num() == 0)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "NoOpenAssets", "There are no open assets to save."), false);
		return FGuid();
	}

	return CreateGroupFromMembers(InName, Members, ActiveAssetPath);
}

FGuid FAssetTabGroupCommands::CreateGroupFromMembers(
	const FString& InName,
	const TArray<FAssetTabGroupMember>& InMembers,
	const FString& InActiveAssetPath)
{
	const FGuid GroupId = Subsystem.GetRepository().CreateGroup(InName);
	if (!GroupId.IsValid())
	{
		Notify(NSLOCTEXT("AssetTabGroups", "CreateGroupFailed", "Failed to create the asset group."), false);
		return FGuid();
	}

	if (!Subsystem.GetRepository().ReplaceMembers(GroupId, InMembers, InActiveAssetPath))
	{
		Subsystem.GetRepository().DeleteGroup(GroupId);
		Notify(NSLOCTEXT("AssetTabGroups", "PopulateGroupFailed", "Failed to populate the new asset group."), false);
		return FGuid();
	}

	const FAssetTabGroup* Group = Subsystem.GetRepository().FindGroup(GroupId);
	Notify(
		FText::Format(
			NSLOCTEXT("AssetTabGroups", "CreatedGroupWithAssets", "Created group '{0}' with {1} assets."),
			FText::FromString(Group != nullptr ? Group->Name : InName),
			FText::AsNumber(InMembers.Num())));
	return GroupId;
}

bool FAssetTabGroupCommands::AddOpenAssetsToGroup(const FGuid& GroupId, int32* OutAddedCount)
{
	FString ActiveAssetPath;
	const TArray<FAssetTabGroupMember> Members = GetOpenAssetMembers(Subsystem, &ActiveAssetPath);
	if (Members.Num() == 0)
	{
		if (OutAddedCount != nullptr)
		{
			*OutAddedCount = 0;
		}
		Notify(NSLOCTEXT("AssetTabGroups", "NoOpenAssetsToAdd", "There are no open assets to add."), false);
		return false;
	}

	const bool bResult = AddMembersToGroup(GroupId, Members, OutAddedCount);
	if (bResult && !ActiveAssetPath.IsEmpty())
	{
		const FAssetTabGroup* Group = Subsystem.GetRepository().FindGroup(GroupId);
		if (Group != nullptr && Group->ActiveAssetPath.IsEmpty() && Group->ContainsAsset(ActiveAssetPath))
		{
			Subsystem.GetRepository().SetActiveAsset(GroupId, ActiveAssetPath);
		}
	}
	return bResult;
}

bool FAssetTabGroupCommands::AddMembersToGroup(
	const FGuid& GroupId,
	const TArray<FAssetTabGroupMember>& InMembers,
	int32* OutAddedCount)
{
	const bool bResult = Subsystem.GetRepository().AddMembers(GroupId, InMembers, OutAddedCount);
	if (!bResult)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "AddAssetsFailed", "Failed to add assets to the group."), false);
	}
	else if (OutAddedCount != nullptr)
	{
		Notify(
			FText::Format(
				NSLOCTEXT("AssetTabGroups", "AddedAssets", "Added {0} new assets to the group."),
				FText::AsNumber(*OutAddedCount)));
	}
	return bResult;
}

bool FAssetTabGroupCommands::UpdateGroupFromOpenAssets(const FGuid& GroupId)
{
	FString ActiveAssetPath;
	const TArray<FAssetTabGroupMember> Members = GetOpenAssetMembers(Subsystem, &ActiveAssetPath);
	if (Members.Num() == 0)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "NoOpenAssetsToUpdate", "There are no open assets to update the group from."), false);
		return false;
	}

	const bool bResult = Subsystem.GetRepository().ReplaceMembers(GroupId, Members, ActiveAssetPath);
	Notify(
		bResult
			? FText::Format(NSLOCTEXT("AssetTabGroups", "UpdatedGroup", "Updated group with {0} open assets."), FText::AsNumber(Members.Num()))
			: NSLOCTEXT("AssetTabGroups", "UpdateGroupFailed", "Failed to update the group."),
		bResult);
	return bResult;
}

bool FAssetTabGroupCommands::RemoveMemberFromGroup(const FGuid& GroupId, const FString& AssetPath)
{
	bool bRemoved = false;
	const bool bResult = Subsystem.GetRepository().RemoveMember(GroupId, AssetPath, &bRemoved);
	if (!bResult || !bRemoved)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "RemoveAssetFailed", "The asset is no longer a member of this group."), false);
	}
	return bResult && bRemoved;
}

bool FAssetTabGroupCommands::RemoveMembersFromGroup(
	const FGuid& GroupId,
	const TArray<FString>& AssetPaths,
	int32* OutRemovedCount)
{
	int32 RemovedCount = 0;
	const bool bResult = Subsystem.GetRepository().RemoveMembers(GroupId, AssetPaths, &RemovedCount);
	if (OutRemovedCount != nullptr)
	{
		*OutRemovedCount = RemovedCount;
	}

	if (!bResult)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "RemoveAssetsFailed", "Failed to remove the selected assets from the group."), false);
		return false;
	}

	if (RemovedCount == 0)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "NoSelectedAssetsRemoved", "The selected assets are no longer members of this group."), false);
		return false;
	}

	Notify(
		FText::Format(
			NSLOCTEXT("AssetTabGroups", "RemovedAssets", "Removed {0} assets from the group."),
			FText::AsNumber(RemovedCount)));
	return true;
}

bool FAssetTabGroupCommands::DeleteGroup(const FGuid& GroupId)
{
	const bool bResult = Subsystem.GetRepository().DeleteGroup(GroupId);
	if (!bResult)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "DeleteGroupFailed", "Failed to delete the group."), false);
	}
	return bResult;
}

bool FAssetTabGroupCommands::RenameGroup(const FGuid& GroupId, const FString& InName)
{
	const bool bResult = Subsystem.GetRepository().RenameGroup(GroupId, InName);
	if (!bResult)
	{
		Notify(FText::FromString(Subsystem.GetRepository().GetLastError()), false);
	}
	return bResult;
}

bool FAssetTabGroupCommands::SetGroupNote(const FGuid& GroupId, const FString& InNote)
{
	return Subsystem.GetRepository().SetGroupNote(GroupId, InNote);
}

bool FAssetTabGroupCommands::SetGroupColor(const FGuid& GroupId, int32 ColorId)
{
	return Subsystem.GetRepository().SetGroupColor(GroupId, ColorId);
}

bool FAssetTabGroupCommands::SetGroupCollapsed(const FGuid& GroupId, bool bCollapsed)
{
	return Subsystem.GetRepository().SetGroupCollapsed(GroupId, bCollapsed);
}

bool FAssetTabGroupCommands::ReorderGroup(const FGuid& GroupId, int32 NewIndex)
{
	return Subsystem.GetRepository().ReorderGroup(GroupId, NewIndex);
}

FAssetTabGroupOperationResult FAssetTabGroupCommands::OpenGroup(const FGuid& GroupId)
{
	const FAssetTabGroup* Group = Subsystem.GetRepository().FindGroup(GroupId);
	if (Group == nullptr)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "OpenGroupFailed", "The asset group no longer exists."), false);
		return FAssetTabGroupOperationResult();
	}

	const FAssetTabGroupOperationResult Result = OpenGroupInternal(*Group, true);
	return Result;
}

FAssetTabGroupOperationResult FAssetTabGroupCommands::FocusGroup(const FGuid& GroupId)
{
	const FAssetTabGroup* Group = Subsystem.GetRepository().FindGroup(GroupId);
	if (Group == nullptr)
	{
		Notify(NSLOCTEXT("AssetTabGroups", "FocusGroupFailed", "The asset group no longer exists."), false);
		return FAssetTabGroupOperationResult();
	}
	const FAssetTabGroup GroupSnapshot = *Group;

	if (!Subsystem.GetRepository().HasAcknowledgedFocusSafetyNotice())
	{
		const EAppReturnType::Type Choice = FMessageDialog::Open(
			EAppMsgType::YesNo,
			NSLOCTEXT(
				"AssetTabGroups",
				"FocusSafetyWarning",
				"Focus Group will try to close clean, non-world asset editors outside the selected group.\n\n"
				"Unsaved, world, PIE, simulation, and uncertain editors will be kept.\n\n"
				"Do you want to continue?"));
		if (Choice != EAppReturnType::Yes)
		{
			Notify(NSLOCTEXT("AssetTabGroups", "FocusCancelled", "Focus Group was cancelled."), false);
			return FAssetTabGroupOperationResult();
		}
		Subsystem.GetRepository().AcknowledgeFocusSafetyNotice();
	}

	const FAssetTabGroupOperationResult OpenResult = OpenGroupInternal(GroupSnapshot, false);
	FAssetTabGroupOperationResult Result = OpenResult;

	TSet<FString> TargetPaths;
	for (const FAssetTabGroupMember& Member : GroupSnapshot.Members)
	{
		TargetPaths.Add(Member.AssetPath);
		FString ResolvedPath;
		Subsystem.GetSessionAdapter().ResolveAsset(Member.AssetPath, &ResolvedPath);
		if (!ResolvedPath.IsEmpty())
		{
			TargetPaths.Add(ResolvedPath);
		}
	}

	const TArray<FAssetEditorSessionInfo> OpenAssets = Subsystem.GetSessionAdapter().GetOpenAssetInfos();
	if (Subsystem.GetSessionAdapter().IsFocusClosePhaseBlocked())
	{
		Result.bClosePhaseSkipped = true;
		for (const FAssetEditorSessionInfo& Info : OpenAssets)
		{
			if (!TargetPaths.Contains(Info.Member.AssetPath))
			{
				++Result.KeptAssetCount;
			}
		}
	}
	else
	{
		for (const FAssetEditorSessionInfo& Info : OpenAssets)
		{
			if (TargetPaths.Contains(Info.Member.AssetPath))
			{
				continue;
			}

			FString CloseReason;
			if (Subsystem.GetSessionAdapter().TryCloseAssetIfSafe(Info.Asset.Get(), CloseReason))
			{
				++Result.ClosedAssetCount;
			}
			else
			{
				++Result.KeptAssetCount;
			}
		}
	}

	if (!GroupSnapshot.ActiveAssetPath.IsEmpty())
	{
		UObject* ActiveAsset = nullptr;
		if (Subsystem.GetSessionAdapter().OpenAssetPath(GroupSnapshot.ActiveAssetPath, ActiveAsset))
		{
			Subsystem.GetSessionAdapter().FocusAsset(ActiveAsset);
		}
	}

	Subsystem.GetRepository().TouchLastUsed(GroupId);
	Notify(
		FText::FromString(
			FString::Printf(
				TEXT("Focused group: opened %d, unavailable %d, closed %d, kept %d%s."),
				Result.OpenedAssetCount,
				Result.MissingAssetCount,
				Result.ClosedAssetCount,
				Result.KeptAssetCount,
				Result.bClosePhaseSkipped ? TEXT(" (PIE/simulation protection active)") : TEXT(""))));
	return Result;
}

bool FAssetTabGroupCommands::OpenAsset(const FString& AssetPath)
{
	UObject* Asset = nullptr;
	FString FailureReason;
	if (!Subsystem.GetSessionAdapter().OpenAssetPath(AssetPath, Asset, nullptr, &FailureReason))
	{
		Notify(
			FText::Format(
				NSLOCTEXT("AssetTabGroups", "OpenAssetFailed", "Could not open asset '{0}': {1}"),
				FText::FromString(AssetPath),
				FText::FromString(FailureReason)),
			false);
		return false;
	}

	const bool bFocused = Subsystem.GetSessionAdapter().FocusAsset(Asset);
	Notify(
		bFocused
			? FText::Format(NSLOCTEXT("AssetTabGroups", "OpenedAsset", "Opened asset '{0}'."), FText::FromString(Asset->GetName()))
			: NSLOCTEXT("AssetTabGroups", "OpenedAssetNoFocus", "Asset opened, but its editor could not be focused."),
		bFocused);
	return true;
}

void FAssetTabGroupCommands::Notify(const FText& Message, bool bSuccess) const
{
	FNotificationInfo NotificationInfo(Message);
	NotificationInfo.ExpireDuration = 4.0f;
	NotificationInfo.bUseThrobber = false;

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	if (NotificationItem.IsValid())
	{
		NotificationItem->SetCompletionState(
			bSuccess
				? SNotificationItem::CS_Success
				: SNotificationItem::CS_Fail);
	}
}

FAssetTabGroupOperationResult FAssetTabGroupCommands::OpenGroupInternal(const FAssetTabGroup& Group, bool bNotify)
{
	FAssetTabGroupOperationResult Result;
	Result.bSucceeded = true;

	TMap<FString, UObject*> ResolvedAssets;
	TArray<UObject*> OrderedResolvedAssets;
	for (const FAssetTabGroupMember& Member : Group.Members)
	{
		UObject* Asset = nullptr;
		FString ResolvedPath;
		FString FailureReason;
		if (Subsystem.GetSessionAdapter().OpenAssetPath(Member.AssetPath, Asset, &ResolvedPath, &FailureReason))
		{
			++Result.OpenedAssetCount;
			OrderedResolvedAssets.Add(Asset);
			ResolvedAssets.Add(Member.AssetPath, Asset);
			if (!ResolvedPath.IsEmpty())
			{
				ResolvedAssets.Add(ResolvedPath, Asset);
			}
		}
		else
		{
			++Result.MissingAssetCount;
			Result.MissingAssetPaths.Add(Member.AssetPath);
			FAssetTabGroupOpenFailure& Failure = Result.FailedAssets.AddDefaulted_GetRef();
			Failure.Member = Member;
			Failure.Reason = FailureReason.IsEmpty()
				? TEXT("The asset could not be opened.")
				: FailureReason;
		}
	}

	UObject* ActiveAsset = ResolvedAssets.FindRef(Group.ActiveAssetPath);
	if (ActiveAsset == nullptr && OrderedResolvedAssets.Num() > 0)
	{
		ActiveAsset = OrderedResolvedAssets[0];
	}
	if (ActiveAsset != nullptr)
	{
		Subsystem.GetSessionAdapter().FocusAsset(ActiveAsset);
	}

	Subsystem.GetRepository().TouchLastUsed(Group.Id);
	if (bNotify)
	{
		Notify(FText::FromString(MakeOpenResultMessage(Result, NSLOCTEXT("AssetTabGroups", "OpenAction", "Opened group"))), Result.MissingAssetCount == 0);
	}
	return Result;
}

TArray<FAssetTabGroupMember> FAssetTabGroupCommands::GetOpenAssetMembers(
	UAssetTabGroupsSubsystem& InSubsystem,
	FString* OutActiveAssetPath)
{
	TArray<FAssetTabGroupMember> Members;
	FAssetEditorSessionAdapter& SessionAdapter = InSubsystem.GetSessionAdapter();
	const TArray<FAssetEditorSessionInfo> OpenAssets = SessionAdapter.GetOpenAssetInfos();
	for (const FAssetEditorSessionInfo& Info : OpenAssets)
	{
		Members.Add(Info.Member);
	}

	if (OutActiveAssetPath != nullptr)
	{
		*OutActiveAssetPath = SessionAdapter.GetActiveAssetPath();
	}
	return Members;
}

FString FAssetTabGroupCommands::MakeOpenResultMessage(
	const FAssetTabGroupOperationResult& Result,
	const FText& ActionName)
{
	return FString::Printf(
		TEXT("%s: %d opened, %d unavailable."),
		*ActionName.ToString(),
		Result.OpenedAssetCount,
		Result.MissingAssetCount);
}
