#pragma once

#include "CoreMinimal.h"

#include "Model/AssetTabGroupTypes.h"

struct ASSETTABGROUPS_API FAssetEditorSessionInfo
{
	FAssetTabGroupMember Member;
	TWeakObjectPtr<UObject> Asset;
	bool bIsDirty = false;
	bool bIsWorld = false;
	bool bIsTransient = false;
	bool bHasEditor = false;
	double LastActivationTime = 0.0;
};

class ASSETTABGROUPS_API FAssetEditorSessionAdapter
{
public:
	TArray<FAssetEditorSessionInfo> GetOpenAssetInfos();
	FString GetActiveAssetPath();

	UObject* ResolveAsset(const FString& InAssetPath, FString* OutResolvedAssetPath = nullptr) const;
	bool OpenAssetPath(
		const FString& InAssetPath,
		UObject*& OutAsset,
		FString* OutResolvedAssetPath = nullptr,
		FString* OutFailureReason = nullptr) const;
	bool FocusAsset(UObject* Asset) const;

	bool IsFocusClosePhaseBlocked() const;
	bool IsSafeToClose(UObject* Asset, FString& OutReason) const;
	bool TryCloseAssetIfSafe(UObject* Asset, FString& OutReason) const;

private:
	static FString GetAssetPath(UObject* Asset);
	static bool IsWorldAsset(UObject* Asset);
	static FAssetEditorSessionInfo MakeSessionInfo(UObject* Asset, double LastActivationTime);
	class UAssetEditorSubsystem* GetAssetEditorSubsystem() const;
};
