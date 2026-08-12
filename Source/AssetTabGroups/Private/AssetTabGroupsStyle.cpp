#include "AssetTabGroupsStyle.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"

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
	Style->SetParentStyleName(FAppStyle::GetAppStyleSetName());
#else
	const ISlateStyle& EditorStyle = FEditorStyle::Get();
	Style->SetParentStyleName(FEditorStyle::GetStyleSetName());
#endif

	Style->Set(TEXT("AssetTabGroups.GroupBorder"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("ToolPanel.GroupBorder"))));

#if ENGINE_MAJOR_VERSION >= 5
	Style->Set(TEXT("AssetTabGroups.PanelBackground"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("Brushes.Panel"))));
#else
	Style->Set(TEXT("AssetTabGroups.PanelBackground"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("ToolPanel.GroupBorder"))));
#endif

	Style->Set(TEXT("AssetTabGroups.Separator"), new FSlateBrush(*EditorStyle.GetBrush(TEXT("Separator"))));
	Style->Set(TEXT("AssetTabGroups.Splitter"), EditorStyle.GetWidgetStyle<FSplitterStyle>(TEXT("Splitter")));
	Style->Set(TEXT("AssetTabGroups.TableRow"), EditorStyle.GetWidgetStyle<FTableRowStyle>(TEXT("TableView.Row")));

#if ENGINE_MAJOR_VERSION >= 5
	Style->Set(TEXT("AssetTabGroups.FlatButton"), EditorStyle.GetWidgetStyle<FButtonStyle>(TEXT("SimpleButton")));
#else
	Style->Set(TEXT("AssetTabGroups.FlatButton"), EditorStyle.GetWidgetStyle<FButtonStyle>(TEXT("FlatButton")));
#endif

	return Style;
}
