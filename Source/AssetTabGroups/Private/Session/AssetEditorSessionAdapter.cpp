#include "Session/AssetEditorSessionAdapter.h"

#include "AssetTabGroupsCompatibility.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/SoftObjectPath.h"

TArray<FAssetEditorSessionInfo> FAssetEditorSessionAdapter::GetOpenAssetInfos()
{
	TArray<FAssetEditorSessionInfo> Result;
	UAssetEditorSubsystem* AssetEditorSubsystem = GetAssetEditorSubsystem();
	if (AssetEditorSubsystem == nullptr)
	{
		return Result;
	}

	TMap<FString, int32> PathToResultIndex;
	for (UObject* Asset : AssetEditorSubsystem->GetAllEditedAssets())
	{
		if (Asset == nullptr)
		{
			continue;
		}

		const FString AssetPath = GetAssetPath(Asset);
		if (AssetPath.IsEmpty())
		{
			continue;
		}

		double LastActivationTime = 0.0;
		for (IAssetEditorInstance* EditorInstance : AssetEditorSubsystem->FindEditorsForAsset(Asset))
		{
			if (EditorInstance != nullptr)
			{
				LastActivationTime = FMath::Max(LastActivationTime, EditorInstance->GetLastActivationTime());
			}
		}

		const int32* ExistingIndex = PathToResultIndex.Find(AssetPath);
		if (ExistingIndex != nullptr)
		{
			FAssetEditorSessionInfo& ExistingInfo = Result[*ExistingIndex];
			ExistingInfo.bIsDirty |= Asset->GetOutermost() != nullptr && Asset->GetOutermost()->IsDirty();
			ExistingInfo.bIsWorld |= IsWorldAsset(Asset);
			ExistingInfo.bIsTransient |= Asset->HasAnyFlags(RF_Transient);
			ExistingInfo.bHasEditor = true;
			ExistingInfo.LastActivationTime = FMath::Max(ExistingInfo.LastActivationTime, LastActivationTime);
			continue;
		}

		PathToResultIndex.Add(AssetPath, Result.Num());
		Result.Add(MakeSessionInfo(Asset, LastActivationTime));
	}

	return Result;
}

FString FAssetEditorSessionAdapter::GetActiveAssetPath()
{
	const TArray<FAssetEditorSessionInfo> OpenAssets = GetOpenAssetInfos();
	FString ActiveAssetPath;
	double BestActivationTime = 0.0;
	bool bHasActivationTime = false;

	for (const FAssetEditorSessionInfo& Info : OpenAssets)
	{
		if (!bHasActivationTime || Info.LastActivationTime > BestActivationTime)
		{
			bHasActivationTime = true;
			BestActivationTime = Info.LastActivationTime;
			ActiveAssetPath = Info.Member.AssetPath;
		}
	}

	return ActiveAssetPath;
}

UObject* FAssetEditorSessionAdapter::ResolveAsset(const FString& InAssetPath, FString* OutResolvedAssetPath) const
{
	if (OutResolvedAssetPath != nullptr)
	{
		OutResolvedAssetPath->Reset();
	}

	FSoftObjectPath SoftObjectPath(InAssetPath);
	if (!SoftObjectPath.IsValid())
	{
		return nullptr;
	}

	SoftObjectPath.FixupCoreRedirects();
	UObject* Asset = SoftObjectPath.ResolveObject();

	if (Asset == nullptr)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		const FAssetData AssetData = AssetTabGroupsCompat::GetAssetBySoftObjectPath(
			AssetRegistryModule.Get(),
			SoftObjectPath);
		if (AssetData.IsValid())
		{
			Asset = AssetData.GetAsset();
		}
	}

	if (Asset == nullptr)
	{
		Asset = SoftObjectPath.TryLoad();
	}

	if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(Asset))
	{
		Asset = Redirector->DestinationObject;
	}

	if (Asset != nullptr && OutResolvedAssetPath != nullptr)
	{
		*OutResolvedAssetPath = GetAssetPath(Asset);
	}

	return Asset;
}

bool FAssetEditorSessionAdapter::OpenAssetPath(
	const FString& InAssetPath,
	UObject*& OutAsset,
	FString* OutResolvedAssetPath,
	FString* OutFailureReason) const
{
	if (OutFailureReason != nullptr)
	{
		OutFailureReason->Reset();
	}

	OutAsset = ResolveAsset(InAssetPath, OutResolvedAssetPath);
	if (OutAsset == nullptr)
	{
		if (OutFailureReason != nullptr)
		{
			*OutFailureReason = FSoftObjectPath(InAssetPath).IsValid()
				? TEXT("The asset could not be found or loaded.")
				: TEXT("The saved asset path is invalid.");
		}
		return false;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GetAssetEditorSubsystem();
	if (AssetEditorSubsystem == nullptr)
	{
		if (OutFailureReason != nullptr)
		{
			*OutFailureReason = TEXT("The asset editor subsystem is unavailable.");
		}
		return false;
	}

	if (AssetEditorSubsystem->FindEditorForAsset(OutAsset, false) != nullptr)
	{
		return true;
	}

	if (!AssetEditorSubsystem->OpenEditorForAsset(OutAsset))
	{
		if (OutFailureReason != nullptr)
		{
			*OutFailureReason = TEXT("The asset resolved, but no compatible editor could be opened.");
		}
		return false;
	}

	return true;
}

bool FAssetEditorSessionAdapter::FocusAsset(UObject* Asset) const
{
	if (Asset == nullptr)
	{
		return false;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GetAssetEditorSubsystem();
	return AssetEditorSubsystem != nullptr && AssetEditorSubsystem->FindEditorForAsset(Asset, true) != nullptr;
}

bool FAssetEditorSessionAdapter::IsFocusClosePhaseBlocked() const
{
	return GEditor != nullptr
		&& (GEditor->PlayWorld != nullptr || GEditor->bIsSimulatingInEditor);
}

bool FAssetEditorSessionAdapter::IsSafeToClose(UObject* Asset, FString& OutReason) const
{
	OutReason.Reset();
	if (Asset == nullptr || !IsValid(Asset))
	{
		OutReason = TEXT("The asset is no longer valid.");
		return false;
	}
	if (IsFocusClosePhaseBlocked())
	{
		OutReason = TEXT("PIE or simulation is active.");
		return false;
	}
	if (Asset->HasAnyFlags(RF_Transient))
	{
		OutReason = TEXT("Transient assets are protected.");
		return false;
	}
	if (IsWorldAsset(Asset))
	{
		OutReason = TEXT("World and level assets are protected.");
		return false;
	}
	if (Asset->GetOutermost() != nullptr && Asset->GetOutermost()->IsDirty())
	{
		OutReason = TEXT("The asset has unsaved changes.");
		return false;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GetAssetEditorSubsystem();
	if (AssetEditorSubsystem == nullptr || AssetEditorSubsystem->FindEditorForAsset(Asset, false) == nullptr)
	{
		OutReason = TEXT("No active asset editor was found.");
		return false;
	}

	return true;
}

bool FAssetEditorSessionAdapter::TryCloseAssetIfSafe(UObject* Asset, FString& OutReason) const
{
	if (!IsSafeToClose(Asset, OutReason))
	{
		return false;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GetAssetEditorSubsystem();
	const int32 ClosedEditorCount = AssetEditorSubsystem != nullptr
		? AssetEditorSubsystem->CloseAllEditorsForAsset(Asset)
		: 0;

	if (ClosedEditorCount > 0)
	{
		OutReason = TEXT("Closed.");
		return true;
	}

	OutReason = TEXT("The asset editor did not close.");
	return false;
}

FString FAssetEditorSessionAdapter::GetAssetPath(UObject* Asset)
{
	return Asset != nullptr ? FSoftObjectPath(Asset).ToString() : FString();
}

bool FAssetEditorSessionAdapter::IsWorldAsset(UObject* Asset)
{
	return Asset != nullptr
		&& (Asset->IsA<UWorld>() || (Asset->GetOutermost() != nullptr && Asset->GetOutermost()->ContainsMap()));
}

FAssetEditorSessionInfo FAssetEditorSessionAdapter::MakeSessionInfo(UObject* Asset, double LastActivationTime)
{
	FAssetEditorSessionInfo Info;
	Info.Asset = Asset;
	Info.LastActivationTime = LastActivationTime;
	Info.bIsDirty = Asset != nullptr && Asset->GetOutermost() != nullptr && Asset->GetOutermost()->IsDirty();
	Info.bIsWorld = IsWorldAsset(Asset);
	Info.bIsTransient = Asset != nullptr && Asset->HasAnyFlags(RF_Transient);
	Info.bHasEditor = true;

	if (Asset != nullptr)
	{
		Info.Member.AssetPath = GetAssetPath(Asset);
		Info.Member.DisplayName = Asset->GetName();
		Info.Member.PackagePath = Asset->GetOutermost() != nullptr ? Asset->GetOutermost()->GetName() : FString();
	}

	return Info;
}

UAssetEditorSubsystem* FAssetEditorSessionAdapter::GetAssetEditorSubsystem() const
{
	return GEditor != nullptr ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
}
