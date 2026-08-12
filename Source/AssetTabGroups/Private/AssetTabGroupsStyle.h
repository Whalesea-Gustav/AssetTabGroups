#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ISlateStyle;

class FAssetTabGroupsStyle final
{
public:
	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get();
	static FName GetStyleSetName();

private:
	static TSharedRef<FSlateStyleSet> Create();

	static TSharedPtr<FSlateStyleSet> StyleInstance;
};
