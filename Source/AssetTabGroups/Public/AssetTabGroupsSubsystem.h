#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "AssetTabGroupsSubsystem.generated.h"

class FAssetEditorSessionAdapter;
class FAssetTabGroupRepository;

UCLASS()
class ASSETTABGROUPS_API UAssetTabGroupsSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UAssetTabGroupsSubsystem();
	UAssetTabGroupsSubsystem(FVTableHelper& Helper);

	virtual ~UAssetTabGroupsSubsystem() override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FAssetTabGroupRepository& GetRepository();
	const FAssetTabGroupRepository& GetRepository() const;

	FAssetEditorSessionAdapter& GetSessionAdapter();
	const FAssetEditorSessionAdapter& GetSessionAdapter() const;

private:
	TUniquePtr<FAssetTabGroupRepository> Repository;
	TUniquePtr<FAssetEditorSessionAdapter> SessionAdapter;
};
