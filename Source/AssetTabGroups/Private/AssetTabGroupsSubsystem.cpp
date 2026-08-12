#include "AssetTabGroupsSubsystem.h"

#include "Repository/AssetTabGroupRepository.h"
#include "Session/AssetEditorSessionAdapter.h"

UAssetTabGroupsSubsystem::UAssetTabGroupsSubsystem()
	: Super()
{
}

UAssetTabGroupsSubsystem::UAssetTabGroupsSubsystem(FVTableHelper& Helper)
	: Super(Helper)
{
}

UAssetTabGroupsSubsystem::~UAssetTabGroupsSubsystem() = default;

void UAssetTabGroupsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Repository = MakeUnique<FAssetTabGroupRepository>();
	Repository->Initialize();

	SessionAdapter = MakeUnique<FAssetEditorSessionAdapter>();
}

void UAssetTabGroupsSubsystem::Deinitialize()
{
	SessionAdapter.Reset();
	Repository.Reset();

	Super::Deinitialize();
}

FAssetTabGroupRepository& UAssetTabGroupsSubsystem::GetRepository()
{
	check(Repository.IsValid());
	return *Repository;
}

const FAssetTabGroupRepository& UAssetTabGroupsSubsystem::GetRepository() const
{
	check(Repository.IsValid());
	return *Repository;
}

FAssetEditorSessionAdapter& UAssetTabGroupsSubsystem::GetSessionAdapter()
{
	check(SessionAdapter.IsValid());
	return *SessionAdapter;
}

const FAssetEditorSessionAdapter& UAssetTabGroupsSubsystem::GetSessionAdapter() const
{
	check(SessionAdapter.IsValid());
	return *SessionAdapter;
}
