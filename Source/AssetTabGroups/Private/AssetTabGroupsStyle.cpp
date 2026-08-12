#include "AssetTabGroupsStyle.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 5
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

TSharedPtr<FSlateStyleSet> FAssetTabGroupsStyle::StyleInstance;

void FAssetTabGroupsStyle::Initialize()
{
	if (StyleInstance.IsValid())
	{
		return;
	}

	StyleInstance = Create();
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FAssetTabGroupsStyle::Shutdown()
{
	if (!StyleInstance.IsValid())
	{
		return;
	}

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	StyleInstance.Reset();
}

const ISlateStyle& FAssetTabGroupsStyle::Get()
{
	check(StyleInstance.IsValid());
	return *StyleInstance;
}

FName FAssetTabGroupsStyle::GetStyleSetName()
{
	static const FName StyleSetName(TEXT("AssetTabGroupsStyle"));
	return StyleSetName;
}

TSharedRef<FSlateStyleSet> FAssetTabGroupsStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

#if ENGINE_MAJOR_VERSION >= 5
	const ISlateStyle& EditorStyle = FAppStyle::Get();
#else
	const ISlateStyle& EditorStyle = FEditorStyle::Get();
#endif

	Style->Set(TEXT("AssetTabGroups.GroupBorder"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("ToolPanel.GroupBorder"))));

#if ENGINE_MAJOR_VERSION >= 5
	Style->Set(TEXT("AssetTabGroups.PanelBackground"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("Brushes.Panel"))));
	Style->Set(TEXT("AssetTabGroups.Separator"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("Separator"))));
	Style->Set(TEXT("AssetTabGroups.Splitter"), EditorStyle.GetWidgetStyle<FSplitterStyle>(TEXT("Splitter")));
	Style->Set(TEXT("AssetTabGroups.ButtonPanelBorder"), new FSlateColorBrush(FLinearColor(FColor(72, 72, 72))));
	Style->Set(TEXT("AssetTabGroups.GroupPanelBorder"), new FSlateColorBrush(FLinearColor(FColor(48, 48, 48))));
	Style->Set(TEXT("AssetTabGroups.DetailPanelBorder"), new FSlateColorBrush(FLinearColor(FColor(88, 88, 88))));
	Style->Set(TEXT("AssetTabGroups.GroupDragHandle"), new FSlateColorBrush(FLinearColor(FColor(150, 150, 150))));
	Style->Set(TEXT("AssetTabGroups.GroupDropIndicator"), new FSlateColorBrush(FLinearColor(FColor(80, 140, 220))));
	Style->Set(TEXT("AssetTabGroups.TileSelection"), new FSlateColorBrush(FLinearColor(FColor(58, 126, 220, 180))));
#else
	Style->Set(TEXT("AssetTabGroups.PanelBackground"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("ToolPanel.GroupBorder"))));
	Style->Set(TEXT("AssetTabGroups.Separator"), new FSlateColorBrush(FLinearColor(FColor(34, 34, 34))));
	Style->Set(
		TEXT("AssetTabGroups.Splitter"),
		FSplitterStyle()
			.SetHandleNormalBrush(FSlateColorBrush(FLinearColor(FColor(32, 32, 32))))
			.SetHandleHighlightBrush(FSlateColorBrush(FLinearColor(FColor(96, 96, 96)))));
	Style->Set(TEXT("AssetTabGroups.ButtonPanelBorder"), new FSlateColorBrush(FLinearColor(FColor(64, 64, 64))));
	Style->Set(TEXT("AssetTabGroups.GroupPanelBorder"), new FSlateColorBrush(FLinearColor(FColor(40, 40, 40))));
	Style->Set(TEXT("AssetTabGroups.DetailPanelBorder"), new FSlateColorBrush(FLinearColor(FColor(80, 80, 80))));
	Style->Set(TEXT("AssetTabGroups.GroupDragHandle"), new FSlateColorBrush(FLinearColor(FColor(128, 128, 128))));
	Style->Set(TEXT("AssetTabGroups.GroupDropIndicator"), new FSlateColorBrush(FLinearColor(FColor(70, 120, 190))));
	Style->Set(TEXT("AssetTabGroups.TileSelection"), new FSlateColorBrush(FLinearColor(FColor(50, 110, 190, 180))));
#endif
	Style->Set(TEXT("AssetTabGroups.TableRow"), EditorStyle.GetWidgetStyle<FTableRowStyle>(TEXT("TableView.Row")));

#if ENGINE_MAJOR_VERSION >= 5
	Style->Set(TEXT("AssetTabGroups.FlatButton"), EditorStyle.GetWidgetStyle<FButtonStyle>(TEXT("SimpleButton")));
#else
	Style->Set(TEXT("AssetTabGroups.FlatButton"), EditorStyle.GetWidgetStyle<FButtonStyle>(TEXT("FlatButton")));

	FTextBlockStyle ButtonTextStyle = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText"));
	ButtonTextStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Style->Set(TEXT("AssetTabGroups.ButtonText"), ButtonTextStyle);
#endif

	return Style;
}
