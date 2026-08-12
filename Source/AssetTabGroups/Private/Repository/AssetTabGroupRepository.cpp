#include "Repository/AssetTabGroupRepository.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace AssetTabGroupRepositoryPrivate
{
	static TSharedPtr<FJsonObject> MemberToJson(const FAssetTabGroupMember& Member)
	{
		TSharedPtr<FJsonObject> JsonMember = MakeShared<FJsonObject>();
		JsonMember->SetStringField(TEXT("assetPath"), Member.AssetPath);
		JsonMember->SetStringField(TEXT("displayName"), Member.DisplayName);
		JsonMember->SetStringField(TEXT("packagePath"), Member.PackagePath);
		return JsonMember;
	}

	static FAssetTabGroupMember MemberFromJson(const TSharedPtr<FJsonObject>& JsonMember)
	{
		FAssetTabGroupMember Member;
		if (JsonMember.IsValid())
		{
			JsonMember->TryGetStringField(TEXT("assetPath"), Member.AssetPath);
			JsonMember->TryGetStringField(TEXT("displayName"), Member.DisplayName);
			JsonMember->TryGetStringField(TEXT("packagePath"), Member.PackagePath);
		}
		return Member;
	}

	static TSharedPtr<FJsonObject> GroupToJson(const FAssetTabGroup& Group)
	{
		TSharedPtr<FJsonObject> JsonGroup = MakeShared<FJsonObject>();
		JsonGroup->SetStringField(TEXT("id"), Group.Id.ToString(EGuidFormats::DigitsWithHyphens));
		JsonGroup->SetStringField(TEXT("name"), Group.Name);
		JsonGroup->SetStringField(TEXT("note"), Group.Note);
		JsonGroup->SetNumberField(TEXT("colorId"), Group.ColorId);
		JsonGroup->SetBoolField(TEXT("collapsed"), Group.bCollapsed);
		JsonGroup->SetStringField(TEXT("activeAssetPath"), Group.ActiveAssetPath);
		JsonGroup->SetNumberField(TEXT("createdAtEpochMs"), static_cast<double>(Group.CreatedAtEpochMs));
		JsonGroup->SetNumberField(TEXT("updatedAtEpochMs"), static_cast<double>(Group.UpdatedAtEpochMs));
		JsonGroup->SetNumberField(TEXT("lastUsedAtEpochMs"), static_cast<double>(Group.LastUsedAtEpochMs));

		TArray<TSharedPtr<FJsonValue>> JsonMembers;
		JsonMembers.Reserve(Group.Members.Num());
		for (const FAssetTabGroupMember& Member : Group.Members)
		{
			JsonMembers.Add(MakeShared<FJsonValueObject>(MemberToJson(Member)));
		}
		JsonGroup->SetArrayField(TEXT("members"), JsonMembers);
		return JsonGroup;
	}

	static FAssetTabGroup GroupFromJson(const TSharedPtr<FJsonObject>& JsonGroup)
	{
		FAssetTabGroup Group;
		if (!JsonGroup.IsValid())
		{
			return Group;
		}

		FString IdString;
		if (JsonGroup->TryGetStringField(TEXT("id"), IdString))
		{
			FGuid::Parse(IdString, Group.Id);
		}

		JsonGroup->TryGetStringField(TEXT("name"), Group.Name);
		JsonGroup->TryGetStringField(TEXT("note"), Group.Note);
		JsonGroup->TryGetNumberField(TEXT("colorId"), Group.ColorId);
		JsonGroup->TryGetBoolField(TEXT("collapsed"), Group.bCollapsed);
		JsonGroup->TryGetStringField(TEXT("activeAssetPath"), Group.ActiveAssetPath);

		double NumberValue = 0.0;
		if (JsonGroup->TryGetNumberField(TEXT("createdAtEpochMs"), NumberValue))
		{
			Group.CreatedAtEpochMs = static_cast<int64>(NumberValue);
		}
		if (JsonGroup->TryGetNumberField(TEXT("updatedAtEpochMs"), NumberValue))
		{
			Group.UpdatedAtEpochMs = static_cast<int64>(NumberValue);
		}
		if (JsonGroup->TryGetNumberField(TEXT("lastUsedAtEpochMs"), NumberValue))
		{
			Group.LastUsedAtEpochMs = static_cast<int64>(NumberValue);
		}

		const TArray<TSharedPtr<FJsonValue>>* JsonMembers = nullptr;
		if (JsonGroup->TryGetArrayField(TEXT("members"), JsonMembers) && JsonMembers != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& JsonMember : *JsonMembers)
			{
				if (JsonMember.IsValid() && JsonMember->Type == EJson::Object)
				{
					Group.Members.Add(MemberFromJson(JsonMember->AsObject()));
				}
			}
		}

		return Group;
	}
}

FAssetTabGroupRepository::FAssetTabGroupRepository()
{
	Workspace.SchemaVersion = 1;
}

void FAssetTabGroupRepository::Initialize()
{
	Workspace = FAssetTabGroupWorkspace();
	LastError.Reset();

	LoadFromDisk();
	NormalizeWorkspace();
}

FAssetTabGroup* FAssetTabGroupRepository::FindGroup(const FGuid& GroupId)
{
	return Workspace.Groups.FindByPredicate(
		[&GroupId](FAssetTabGroup& Group)
		{
			return Group.Id == GroupId;
		});
}

const FAssetTabGroup* FAssetTabGroupRepository::FindGroup(const FGuid& GroupId) const
{
	return Workspace.Groups.FindByPredicate(
		[&GroupId](const FAssetTabGroup& Group)
		{
			return Group.Id == GroupId;
		});
}

FGuid FAssetTabGroupRepository::CreateGroup(const FString& InName, const FString& InNote)
{
	FAssetTabGroup NewGroup;
	NewGroup.Id = FGuid::NewGuid();
	NewGroup.Name = MakeUniqueGroupName(Workspace.Groups, InName.TrimStartAndEnd());
	NewGroup.Note = InNote.TrimStartAndEnd();
	NewGroup.ColorId = Workspace.Groups.Num() % 5;
	NewGroup.CreatedAtEpochMs = GetNowEpochMilliseconds();
	NewGroup.UpdatedAtEpochMs = NewGroup.CreatedAtEpochMs;
	NewGroup.LastUsedAtEpochMs = NewGroup.CreatedAtEpochMs;

	const FGuid NewGroupId = NewGroup.Id;
	Workspace.Groups.Add(MoveTemp(NewGroup));
	CommitChange();
	return NewGroupId;
}

bool FAssetTabGroupRepository::DeleteGroup(const FGuid& GroupId)
{
	const int32 RemovedCount = Workspace.Groups.RemoveAll(
		[&GroupId](const FAssetTabGroup& Group)
		{
			return Group.Id == GroupId;
		});

	return RemovedCount > 0 ? CommitChange() : false;
}

bool FAssetTabGroupRepository::RenameGroup(const FGuid& GroupId, const FString& InName)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	const FString NewName = InName.TrimStartAndEnd();
	if (Group == nullptr || NewName.IsEmpty())
	{
		LastError = TEXT("A group name cannot be empty.");
		return false;
	}

	const bool bNameTaken = Workspace.Groups.ContainsByPredicate(
		[&GroupId, &NewName](const FAssetTabGroup& OtherGroup)
		{
			return OtherGroup.Id != GroupId && OtherGroup.Name.Equals(NewName, ESearchCase::IgnoreCase);
		});
	if (bNameTaken)
	{
		LastError = TEXT("A group with this name already exists.");
		return false;
	}

	Group->Name = NewName;
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::SetGroupNote(const FGuid& GroupId, const FString& InNote)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	Group->Note = InNote.TrimStartAndEnd();
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::SetGroupColor(const FGuid& GroupId, int32 InColorId)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	Group->ColorId = FMath::Clamp(InColorId, 0, 4);
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::SetGroupCollapsed(const FGuid& GroupId, bool bInCollapsed)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	Group->bCollapsed = bInCollapsed;
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::AddMembers(const FGuid& GroupId, const TArray<FAssetTabGroupMember>& InMembers, int32* OutAddedCount)
{
	if (OutAddedCount != nullptr)
	{
		*OutAddedCount = 0;
	}

	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	for (const FAssetTabGroupMember& NewMember : InMembers)
	{
		if (NewMember.IsValid() && !Group->ContainsAsset(NewMember.AssetPath))
		{
			Group->Members.Add(NewMember);
			if (OutAddedCount != nullptr)
			{
				++(*OutAddedCount);
			}
		}
	}

	if (OutAddedCount == nullptr || *OutAddedCount > 0)
	{
		Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
		return CommitChange();
	}

	return true;
}

bool FAssetTabGroupRepository::ReplaceMembers(const FGuid& GroupId, const TArray<FAssetTabGroupMember>& InMembers, const FString& InActiveAssetPath)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	Group->Members = InMembers;
	NormalizeMembers(Group->Members);
	Group->ActiveAssetPath = InActiveAssetPath;
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::RemoveMember(const FGuid& GroupId, const FString& InAssetPath, bool* bOutRemoved)
{
	if (bOutRemoved != nullptr)
	{
		*bOutRemoved = false;
	}

	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	const int32 RemovedCount = Group->Members.RemoveAll(
		[&InAssetPath](const FAssetTabGroupMember& Member)
		{
			return Member.AssetPath == InAssetPath;
		});
	if (RemovedCount == 0)
	{
		return true;
	}

	if (bOutRemoved != nullptr)
	{
		*bOutRemoved = true;
	}
	if (Group->ActiveAssetPath == InAssetPath)
	{
		Group->ActiveAssetPath = Group->Members.Num() > 0 ? Group->Members[0].AssetPath : FString();
	}
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::SetActiveAsset(const FGuid& GroupId, const FString& InAssetPath)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr || (!InAssetPath.IsEmpty() && !Group->ContainsAsset(InAssetPath)))
	{
		return false;
	}

	Group->ActiveAssetPath = InAssetPath;
	Group->UpdatedAtEpochMs = GetNowEpochMilliseconds();
	return CommitChange();
}

bool FAssetTabGroupRepository::ReorderGroup(const FGuid& GroupId, int32 NewIndex)
{
	const int32 ExistingIndex = Workspace.Groups.IndexOfByPredicate(
		[&GroupId](const FAssetTabGroup& Group)
		{
			return Group.Id == GroupId;
		});
	if (ExistingIndex == INDEX_NONE)
	{
		return false;
	}

	FAssetTabGroup Group = Workspace.Groups[ExistingIndex];
	Workspace.Groups.RemoveAt(ExistingIndex);
	NewIndex = FMath::Clamp(NewIndex, 0, Workspace.Groups.Num());
	Workspace.Groups.Insert(MoveTemp(Group), NewIndex);
	return CommitChange();
}

bool FAssetTabGroupRepository::TouchLastUsed(const FGuid& GroupId)
{
	FAssetTabGroup* Group = FindGroup(GroupId);
	if (Group == nullptr)
	{
		return false;
	}

	const int64 Now = GetNowEpochMilliseconds();
	Group->LastUsedAtEpochMs = Now;
	Group->UpdatedAtEpochMs = Now;
	return CommitChange();
}

bool FAssetTabGroupRepository::AcknowledgeFocusSafetyNotice()
{
	if (Workspace.bFocusSafetyNoticeAcknowledged)
	{
		return true;
	}

	Workspace.bFocusSafetyNoticeAcknowledged = true;
	return CommitChange();
}

int64 FAssetTabGroupRepository::GetNowEpochMilliseconds()
{
	const FDateTime Now = FDateTime::UtcNow();
	return Now.ToUnixTimestamp() * 1000 + Now.GetMillisecond();
}

FLinearColor FAssetTabGroupRepository::GetColorForId(int32 ColorId)
{
	static const FLinearColor Colors[] =
	{
		FLinearColor(0.16f, 0.45f, 0.90f),
		FLinearColor(0.18f, 0.70f, 0.36f),
		FLinearColor(0.86f, 0.22f, 0.20f),
		FLinearColor(0.95f, 0.55f, 0.12f),
		FLinearColor(0.55f, 0.30f, 0.82f)
	};

	return Colors[FMath::Clamp(ColorId, 0, 4)];
}

bool FAssetTabGroupRepository::LoadFromDisk()
{
	const FString Filename = GetWorkspaceFilename();
	if (!IFileManager::Get().FileExists(*Filename))
	{
		return true;
	}

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *Filename))
	{
		LastError = FString::Printf(TEXT("Failed to read Asset Tab Groups workspace: %s"), *Filename);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		LastError = FString::Printf(TEXT("Failed to parse Asset Tab Groups workspace: %s"), *Filename);
		return false;
	}

	RootObject->TryGetNumberField(TEXT("schemaVersion"), Workspace.SchemaVersion);
	RootObject->TryGetBoolField(TEXT("focusSafetyNoticeAcknowledged"), Workspace.bFocusSafetyNoticeAcknowledged);

	const TArray<TSharedPtr<FJsonValue>>* JsonGroups = nullptr;
	if (RootObject->TryGetArrayField(TEXT("groups"), JsonGroups) && JsonGroups != nullptr)
	{
		for (const TSharedPtr<FJsonValue>& JsonGroup : *JsonGroups)
		{
			if (JsonGroup.IsValid() && JsonGroup->Type == EJson::Object)
			{
				Workspace.Groups.Add(AssetTabGroupRepositoryPrivate::GroupFromJson(JsonGroup->AsObject()));
			}
		}
	}

	return true;
}

bool FAssetTabGroupRepository::SaveToDisk()
{
	const FString Filename = GetWorkspaceFilename();
	const FString Directory = FPaths::GetPath(Filename);
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		LastError = FString::Printf(TEXT("Failed to create Asset Tab Groups directory: %s"), *Directory);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetNumberField(TEXT("schemaVersion"), Workspace.SchemaVersion);
	RootObject->SetBoolField(TEXT("focusSafetyNoticeAcknowledged"), Workspace.bFocusSafetyNoticeAcknowledged);

	TArray<TSharedPtr<FJsonValue>> JsonGroups;
	JsonGroups.Reserve(Workspace.Groups.Num());
	for (const FAssetTabGroup& Group : Workspace.Groups)
	{
		JsonGroups.Add(MakeShared<FJsonValueObject>(AssetTabGroupRepositoryPrivate::GroupToJson(Group)));
	}
	RootObject->SetArrayField(TEXT("groups"), JsonGroups);

	FString JsonText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
	if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		LastError = TEXT("Failed to serialize Asset Tab Groups workspace.");
		return false;
	}
	Writer->Close();

	if (!FFileHelper::SaveStringToFile(JsonText, *Filename))
	{
		LastError = FString::Printf(TEXT("Failed to write Asset Tab Groups workspace: %s"), *Filename);
		return false;
	}

	LastError.Reset();
	return true;
}

bool FAssetTabGroupRepository::CommitChange()
{
	NormalizeWorkspace();
	const bool bSaved = SaveToDisk();
	GroupsChanged.Broadcast();
	return bSaved;
}

void FAssetTabGroupRepository::NormalizeWorkspace()
{
	Workspace.SchemaVersion = 1;

	TSet<FGuid> UsedIds;
	for (int32 Index = Workspace.Groups.Num() - 1; Index >= 0; --Index)
	{
		FAssetTabGroup& Group = Workspace.Groups[Index];
		NormalizeGroup(Group);
		if (!Group.Id.IsValid() || UsedIds.Contains(Group.Id))
		{
			Workspace.Groups.RemoveAt(Index);
			continue;
		}
		UsedIds.Add(Group.Id);
	}
}

void FAssetTabGroupRepository::NormalizeGroup(FAssetTabGroup& Group)
{
	if (!Group.Id.IsValid())
	{
		Group.Id = FGuid::NewGuid();
	}
	if (Group.Name.TrimStartAndEnd().IsEmpty())
	{
		Group.Name = TEXT("New Group");
	}
	Group.Name = Group.Name.TrimStartAndEnd();
	Group.ColorId = FMath::Clamp(Group.ColorId, 0, 4);

	const int64 Now = GetNowEpochMilliseconds();
	if (Group.CreatedAtEpochMs <= 0)
	{
		Group.CreatedAtEpochMs = Now;
	}
	if (Group.UpdatedAtEpochMs <= 0)
	{
		Group.UpdatedAtEpochMs = Group.CreatedAtEpochMs;
	}
	if (Group.LastUsedAtEpochMs <= 0)
	{
		Group.LastUsedAtEpochMs = Group.CreatedAtEpochMs;
	}

	NormalizeMembers(Group.Members);
	if (!Group.ActiveAssetPath.IsEmpty() && !Group.ContainsAsset(Group.ActiveAssetPath))
	{
		Group.ActiveAssetPath.Reset();
	}
	if (Group.ActiveAssetPath.IsEmpty() && Group.Members.Num() > 0)
	{
		Group.ActiveAssetPath = Group.Members[0].AssetPath;
	}
}

void FAssetTabGroupRepository::NormalizeMembers(TArray<FAssetTabGroupMember>& Members)
{
	TSet<FString> SeenPaths;
	for (int32 Index = Members.Num() - 1; Index >= 0; --Index)
	{
		FAssetTabGroupMember& Member = Members[Index];
		Member.AssetPath = Member.AssetPath.TrimStartAndEnd();
		if (Member.AssetPath.IsEmpty() || SeenPaths.Contains(Member.AssetPath))
		{
			Members.RemoveAt(Index);
			continue;
		}
		SeenPaths.Add(Member.AssetPath);
	}
}

FString FAssetTabGroupRepository::GetWorkspaceFilename()
{
	return FPaths::ProjectSavedDir() / TEXT("AssetTabGroups/Workspace.json");
}

FString FAssetTabGroupRepository::MakeUniqueGroupName(const TArray<FAssetTabGroup>& Groups, const FString& BaseName)
{
	const FString EffectiveBaseName = BaseName.IsEmpty() ? TEXT("New Group") : BaseName;
	FString Candidate = EffectiveBaseName;
	int32 Suffix = 2;

	while (Groups.ContainsByPredicate(
		[&Candidate](const FAssetTabGroup& Group)
		{
			return Group.Name.Equals(Candidate, ESearchCase::IgnoreCase);
		}))
	{
		Candidate = FString::Printf(TEXT("%s %d"), *EffectiveBaseName, Suffix++);
	}

	return Candidate;
}
