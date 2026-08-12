#pragma once

#include "CoreMinimal.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#else
#include "AssetData.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5
#include "Styling/StyleColors.h"
#else
#include "EditorStyleSet.h"
#endif

namespace AssetTabGroupsCompat
{
	inline FString GetAssetObjectPathString(const FAssetData& AssetData)
	{
#if ENGINE_MAJOR_VERSION >= 5
		return AssetData.GetObjectPathString();
#else
		return AssetData.ObjectPath.ToString();
#endif
	}

	inline FString GetAssetClassString(const FAssetData& AssetData)
	{
#if ENGINE_MAJOR_VERSION >= 5
		if (AssetData.AssetClassPath.IsValid())
		{
			return AssetData.AssetClassPath.ToString();
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return AssetData.AssetClass.ToString();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
#else
		return AssetData.AssetClass.ToString();
#endif
	}

	inline FAssetData GetAssetBySoftObjectPath(IAssetRegistry& AssetRegistry, const FSoftObjectPath& SoftObjectPath)
	{
#if ENGINE_MAJOR_VERSION >= 5
		return AssetRegistry.GetAssetByObjectPath(SoftObjectPath);
#else
		return AssetRegistry.GetAssetByObjectPath(FName(*SoftObjectPath.ToString()));
#endif
	}

	inline FLinearColor GetTileBackgroundColor()
	{
#if ENGINE_MAJOR_VERSION >= 5
		return FStyleColors::Panel.GetSpecifiedColor();
#else
		if (const FSlateBrush* PanelBrush = FEditorStyle::Get().GetBrush(TEXT("ToolPanel.GroupBorder")))
		{
			return PanelBrush->TintColor.GetSpecifiedColor();
		}

		return FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);
#endif
	}

	inline FLinearColor GetMissingTileBackgroundColor()
	{
#if ENGINE_MAJOR_VERSION >= 5
		return FStyleColors::Error.GetSpecifiedColor().CopyWithNewOpacity(0.18f);
#else
		return FLinearColor(0.20f, 0.05f, 0.05f, 1.0f);
#endif
	}

	inline FSlateColor GetMissingForegroundColor()
	{
#if ENGINE_MAJOR_VERSION >= 5
		return FSlateColor(FStyleColors::Error);
#else
		return FSlateColor(FLinearColor::Red);
#endif
	}
}

#if ENGINE_MAJOR_VERSION >= 5
#define ASSETTABGROUPS_SColorBlock_IgnoreAlpha AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
#else
#define ASSETTABGROUPS_SColorBlock_IgnoreAlpha IgnoreAlpha(true)
#endif
