#include "UI/SAssetTabGroupsPanel.h"

#include "AssetTabGroupsSubsystem.h"
#include "AssetTabGroupsStyle.h"
#include "AssetTabGroupsCompatibility.h"
#include "AssetThumbnail.h"
#include "ContentBrowserModule.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Repository/AssetTabGroupRepository.h"
#include "Session/AssetEditorSessionAdapter.h"
#include "Styling/CoreStyle.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/SoftObjectPath.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "AssetTabGroups"

#if ENGINE_MAJOR_VERSION < 5
#define ASSETTABGROUPS_BUTTON_TEXT_STYLE .TextStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.ButtonText"))
#else
#define ASSETTABGROUPS_BUTTON_TEXT_STYLE
#endif

namespace AssetTabGroupsPrivate
{
	const FName MemberNameColumnId(TEXT("Name"));
	const FName MemberTypeColumnId(TEXT("Type"));
	const FName MemberPackageColumnId(TEXT("Package"));
	const FName MemberStatusColumnId(TEXT("Status"));

	DECLARE_DELEGATE_RetVal_TwoParams(FReply, FOnGroupDragDetected, const FGeometry&, const FPointerEvent&);
	DECLARE_DELEGATE_ThreeParams(FOnGroupDropRequested, const FGuid&, const FGuid&, bool);
	DECLARE_DELEGATE_OneParam(FOnMemberTileSelectionRequested, const FPointerEvent&);

	class FAssetTabGroupDragDropOp final : public FDecoratedDragDropOp
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(FAssetTabGroupDragDropOp, FDecoratedDragDropOp)

		FGuid GroupId;
		FString GroupName;

		static TSharedRef<FAssetTabGroupDragDropOp> New(const FGuid& InGroupId, const FString& GroupName)
		{
			TSharedRef<FAssetTabGroupDragDropOp> Operation = MakeShared<FAssetTabGroupDragDropOp>();
			Operation->GroupId = InGroupId;
			Operation->GroupName = GroupName;
			Operation->Construct();
			return Operation;
		}

		virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
		{
			return SNew(SBorder)
				.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupPanelBorder")))
				.Padding(4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(GroupName))
				];
		}
	};

	static TSharedRef<SWidget> MakeGroupDragDot()
	{
		const FSlateBrush* DragHandleBrush = FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupDragHandle"));
		return SNew(SColorBlock)
			.Color(DragHandleBrush->TintColor.GetSpecifiedColor())
			.Size(FVector2D(3.0f, 3.0f));
	}

	class SAssetTabGroupDragHandle final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetTabGroupDragHandle)
			: _OnDragDetected()
		{
		}
			SLATE_EVENT(FOnGroupDragDetected, OnDragDetected)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnDragDetectedDelegate = InArgs._OnDragDetected;

			ChildSlot
			[
				SNew(SGridPanel)
				+ SGridPanel::Slot(0, 0)
				.Padding(1.0f)
				[
					MakeGroupDragDot()
				]
				+ SGridPanel::Slot(1, 0)
				.Padding(1.0f)
				[
					MakeGroupDragDot()
				]
				+ SGridPanel::Slot(0, 1)
				.Padding(1.0f)
				[
					MakeGroupDragDot()
				]
				+ SGridPanel::Slot(1, 1)
				.Padding(1.0f)
				[
					MakeGroupDragDot()
				]
				+ SGridPanel::Slot(0, 2)
				.Padding(1.0f)
				[
					MakeGroupDragDot()
				]
				+ SGridPanel::Slot(1, 2)
				.Padding(1.0f)
				[
					MakeGroupDragDot()
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}

			return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			return OnDragDetectedDelegate.IsBound()
				? OnDragDetectedDelegate.Execute(MyGeometry, MouseEvent)
				: FReply::Unhandled();
		}

	private:
		FOnGroupDragDetected OnDragDetectedDelegate;
	};

	class SAssetTabGroupRow final : public STableRow<TSharedPtr<FGuid>>
	{
	public:
		SLATE_BEGIN_ARGS(SAssetTabGroupRow)
			: _GroupId()
			, _OnDropRequested()
		{
		}
			SLATE_ARGUMENT(TSharedPtr<FGuid>, GroupId)
			SLATE_EVENT(FOnGroupDropRequested, OnDropRequested)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
		{
			GroupId = InArgs._GroupId;
			OnDropRequested = InArgs._OnDropRequested;

			TSharedRef<SOverlay> RowOverlay = SNew(SOverlay);
			RowOverlay->AddSlot()
			[
				InArgs._Content.Widget
			];
			RowOverlay->AddSlot()
			.Padding(1.0f)
			[
				SNew(SColorBlock)
				.Color_Lambda([this]()
				{
					return bDragOver
						? FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupDropIndicator"))->TintColor.GetSpecifiedColor().CopyWithNewOpacity(0.10f)
						: FLinearColor::Transparent;
				})
				.Visibility_Lambda([this]()
				{
					return bDragOver ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
			];
			RowOverlay->AddSlot()
			.VAlign(VAlign_Top)
			[
				SNew(SBox)
				.HeightOverride(2.0f)
				[
					SNew(SColorBlock)
					.Color(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupDropIndicator"))->TintColor.GetSpecifiedColor())
					.Visibility_Lambda([this]()
					{
						return bDragOver && !bDropAfter ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
				]
			];
			RowOverlay->AddSlot()
			.VAlign(VAlign_Bottom)
			[
				SNew(SBox)
				.HeightOverride(2.0f)
				[
					SNew(SColorBlock)
					.Color(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupDropIndicator"))->TintColor.GetSpecifiedColor())
					.Visibility_Lambda([this]()
					{
						return bDragOver && bDropAfter ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
				]
			];

			using FRowType = STableRow<TSharedPtr<FGuid>>;
			FRowType::Construct(
				FRowType::FArguments()
				.Style(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.TableRow"))
				[
					RowOverlay
				],
				OwnerTable);
		}

		virtual void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override
		{
			UpdateDragState(MyGeometry, DragDropEvent);
		}

		virtual void OnDragLeave(FDragDropEvent const& DragDropEvent) override
		{
			bDragOver = false;
			Invalidate(EInvalidateWidget::Paint);
			STableRow<TSharedPtr<FGuid>>::OnDragLeave(DragDropEvent);
		}

		virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			return UpdateDragState(MyGeometry, DragDropEvent)
				? FReply::Handled()
				: FReply::Unhandled();
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FAssetTabGroupDragDropOp> Operation = DragDropEvent.GetOperationAs<FAssetTabGroupDragDropOp>();
			if (!Operation.IsValid() || !GroupId.IsValid() || Operation->GroupId == *GroupId)
			{
				bDragOver = false;
				Invalidate(EInvalidateWidget::Paint);
				return FReply::Unhandled();
			}

			const bool bInsertAfter = bDropAfter;
			bDragOver = false;
			Invalidate(EInvalidateWidget::Paint);
			if (OnDropRequested.IsBound())
			{
				OnDropRequested.Execute(*GroupId, Operation->GroupId, bInsertAfter);
			}
			return FReply::Handled();
		}

	private:
		bool UpdateDragState(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
		{
			const TSharedPtr<FAssetTabGroupDragDropOp> Operation = DragDropEvent.GetOperationAs<FAssetTabGroupDragDropOp>();
			if (!Operation.IsValid() || !GroupId.IsValid() || Operation->GroupId == *GroupId)
			{
				bDragOver = false;
				Invalidate(EInvalidateWidget::Paint);
				return false;
			}

			const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
			bDragOver = true;
			bDropAfter = LocalPosition.Y >= MyGeometry.GetLocalSize().Y * 0.5f;
			Invalidate(EInvalidateWidget::Paint);
			return true;
		}

		TSharedPtr<FGuid> GroupId;
		FOnGroupDropRequested OnDropRequested;
		bool bDragOver = false;
		bool bDropAfter = false;
	};

	class SAssetTabGroupDetailsSurface final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetTabGroupDetailsSurface)
			: _OnGetMenuContent()
		{
		}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_EVENT(FOnGetContent, OnGetMenuContent)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnGetMenuContent = InArgs._OnGetMenuContent;

			ChildSlot
			[
				InArgs._Content.Widget
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnGetMenuContent.IsBound())
			{
				TSharedRef<SWidget> MenuWidget = OnGetMenuContent.Execute();
				FSlateApplication::Get().PushMenu(
					AsShared(),
					FWidgetPath(),
					MenuWidget,
					MouseEvent.GetScreenSpacePosition(),
					FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
				return FReply::Handled();
			}

			return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

	private:
		FOnGetContent OnGetMenuContent;
	};

	class SAssetTabGroupMemberTile final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetTabGroupMemberTile)
			: _OnDoubleClicked()
			, _OnGetMenuContent()
			, _OnSelectionRequested()
		{
		}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_EVENT(FOnClicked, OnDoubleClicked)
			SLATE_EVENT(FOnGetContent, OnGetMenuContent)
			SLATE_EVENT(FOnMemberTileSelectionRequested, OnSelectionRequested)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnDoubleClicked = InArgs._OnDoubleClicked;
			OnSelectionRequested = InArgs._OnSelectionRequested;

			ChildSlot
			[
				SAssignNew(MenuAnchor, SMenuAnchor)
				.Placement(MenuPlacement_MenuRight)
				.OnGetMenuContent(InArgs._OnGetMenuContent)
				[
					InArgs._Content.Widget
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				if (OnSelectionRequested.IsBound())
				{
					OnSelectionRequested.Execute(MouseEvent);
				}
				if (MenuAnchor.IsValid())
				{
					MenuAnchor->SetIsOpen(true);
					return FReply::Handled();
				}
			}

			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (OnSelectionRequested.IsBound())
				{
					OnSelectionRequested.Execute(MouseEvent);
				}
				return FReply::Handled();
			}

			return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnDoubleClicked.IsBound())
			{
				return OnDoubleClicked.Execute();
			}

			return SCompoundWidget::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);
		}

	private:
		FOnClicked OnDoubleClicked;
		FOnMemberTileSelectionRequested OnSelectionRequested;
		TSharedPtr<SMenuAnchor> MenuAnchor;
	};

	class SAssetTabGroupMemberRow final : public SMultiColumnTableRow<TSharedPtr<FAssetTabGroupMember>>
	{
	public:
		SLATE_BEGIN_ARGS(SAssetTabGroupMemberRow)
			: _Member()
			, _TypeText()
			, _PackageText()
			, _StatusText()
			, _bMissing(false)
			, _OnContextMenuRequested()
		{
		}
			SLATE_ARGUMENT(TSharedPtr<FAssetTabGroupMember>, Member)
			SLATE_ARGUMENT(FString, TypeText)
			SLATE_ARGUMENT(FString, PackageText)
			SLATE_ARGUMENT(FText, StatusText)
			SLATE_ARGUMENT(bool, bMissing)
			SLATE_EVENT(FSimpleDelegate, OnContextMenuRequested)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
		{
			Member = InArgs._Member;
			TypeText = InArgs._TypeText;
			PackageText = InArgs._PackageText;
			StatusText = InArgs._StatusText;
			bMissing = InArgs._bMissing;
			OnContextMenuRequested = InArgs._OnContextMenuRequested;

			using FRowType = SMultiColumnTableRow<TSharedPtr<FAssetTabGroupMember>>;
			FRowType::Construct(
				FRowType::FTableRowArgs()
				.Style(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.TableRow")),
				OwnerTable);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnContextMenuRequested.IsBound())
			{
				OnContextMenuRequested.Execute();
			}

			using FRowType = SMultiColumnTableRow<TSharedPtr<FAssetTabGroupMember>>;
			return FRowType::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
		{
			const FString DisplayName = Member.IsValid() && !Member->DisplayName.IsEmpty()
				? Member->DisplayName
				: (Member.IsValid() ? Member->AssetPath : FString());

			if (ColumnName == MemberNameColumnId)
			{
				return SNew(STextBlock)
					.Text(FText::FromString(DisplayName))
					.ToolTipText(Member.IsValid() ? FText::FromString(Member->AssetPath) : FText::GetEmpty())
					.ColorAndOpacity(bMissing ? AssetTabGroupsCompat::GetMissingForegroundColor() : FSlateColor::UseForeground());
			}

			if (ColumnName == MemberTypeColumnId)
			{
				return SNew(STextBlock)
					.Text(FText::FromString(TypeText))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground());
			}

			if (ColumnName == MemberPackageColumnId)
			{
				return SNew(STextBlock)
					.Text(FText::FromString(PackageText))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground());
			}

			if (ColumnName == MemberStatusColumnId)
			{
				return SNew(STextBlock)
					.Text(StatusText)
					.ColorAndOpacity(bMissing ? AssetTabGroupsCompat::GetMissingForegroundColor() : FSlateColor::UseSubduedForeground());
			}

			return SNew(STextBlock).Text(FText::GetEmpty());
		}

	private:
		TSharedPtr<FAssetTabGroupMember> Member;
		FString TypeText;
		FString PackageText;
		FText StatusText;
		bool bMissing = false;
		FSimpleDelegate OnContextMenuRequested;
	};
}

void SAssetTabGroupsPanel::Construct(const FArguments& InArgs)
{
	if (GEditor != nullptr)
	{
		Subsystem = GEditor->GetEditorSubsystem<UAssetTabGroupsSubsystem>();
	}

	if (Subsystem != nullptr)
	{
		Commands = MakeUnique<FAssetTabGroupCommands>(*Subsystem);
		GroupsChangedHandle = Subsystem->GetRepository().OnGroupsChanged().AddSP(
			this,
			&SAssetTabGroupsPanel::HandleGroupsChanged);
	}

	ThumbnailPool = MakeShared<FAssetThumbnailPool>(128);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupBorder")))
		.Padding(6.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.ButtonPanelBorder")))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.PanelBackground")))
					.Padding(FMargin(4.0f, 3.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
							ASSETTABGROUPS_BUTTON_TEXT_STYLE
							.Text(LOCTEXT("CreateEmptyGroup", "New Group"))
							.ToolTipText(LOCTEXT("CreateEmptyGroupTooltip", "Create an empty asset group."))
							.OnClicked_Lambda([this]()
							{
								CreateEmptyGroup();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
							ASSETTABGROUPS_BUTTON_TEXT_STYLE
							.Text(LOCTEXT("SaveAllOpenAssets", "Save All Open"))
							.ToolTipText(LOCTEXT("SaveAllOpenAssetsTooltip", "Save all currently open asset editors as a new group."))
							.OnClicked_Lambda([this]()
							{
								SaveAllOpenAssets();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
							ASSETTABGROUPS_BUTTON_TEXT_STYLE
							.Text(LOCTEXT("SaveSelectedOpenAssets", "Save Selected"))
							.ToolTipText(LOCTEXT("SaveSelectedOpenAssetsTooltip", "Choose currently open assets and save them as a new group."))
							.OnClicked_Lambda([this]()
							{
								SaveSelectedOpenAssets();
								return FReply::Handled();
							})
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 6.0f)
			[
				SNew(SSeparator)
				.SeparatorImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.Separator")))
				.Thickness(2.0f)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Orientation(EOrientation::Orient_Horizontal)
				.Style(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.Splitter"))
				.PhysicalSplitterHandleSize(2.0f)
				.HitDetectionSplitterHandleSize(ENGINE_MAJOR_VERSION < 5 ? 8.0f : 2.0f)
				+ SSplitter::Slot()
				.Value(0.28f)
				[
					SNew(SBorder)
					.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.GroupPanelBorder")))
					.Padding(1.0f)
					[
						SNew(SBorder)
						.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.PanelBackground")))
						.Padding(4.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2.0f, 0.0f, 2.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("GroupsHeading", "Asset Groups"))
								.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							]
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SAssignNew(GroupListView, SListView<TSharedPtr<FGuid>>)
								.ListItemsSource(&GroupItems)
								.OnGenerateRow(this, &SAssetTabGroupsPanel::GenerateGroupRow)
								.OnSelectionChanged(this, &SAssetTabGroupsPanel::GroupSelectionChanged)
								.OnContextMenuOpening_Lambda([this]()
								{
									const FGuid ContextGroupId = PendingGroupContextMenuId;
									PendingGroupContextMenuId.Invalidate();
									if (ContextGroupId.IsValid())
									{
										return TSharedPtr<SWidget>(MakeGroupMenu(ContextGroupId));
									}
									return TSharedPtr<SWidget>(MakeEmptyGroupMenu());
								})
								.SelectionMode(ESelectionMode::Single)
								.ClearSelectionOnClick(true)
								.ScrollbarVisibility(EVisibility::Visible)
							]
						]
					]
				]
				+ SSplitter::Slot()
				.Value(0.72f)
				[
					SNew(SBorder)
					.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.DetailPanelBorder")))
					.Padding(1.0f)
					[
						SNew(SBorder)
						.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.PanelBackground")))
						.Padding(6.0f)
						[
							SAssignNew(DetailsBox, SVerticalBox)
						]
					]
				]
			]
		]
	];

	Rebuild();
}

SAssetTabGroupsPanel::~SAssetTabGroupsPanel()
{
	if (Subsystem != nullptr && GroupsChangedHandle.IsValid())
	{
		Subsystem->GetRepository().OnGroupsChanged().Remove(GroupsChangedHandle);
	}
}

void SAssetTabGroupsPanel::Rebuild()
{
	if (!GroupListView.IsValid() || !DetailsBox.IsValid())
	{
		return;
	}

	PendingGroupContextMenuId.Invalidate();
	PendingMemberContextMenuGroupId.Invalidate();
	PendingMemberContextMenuAssetPath.Reset();
	PendingMemberContextMenuDisplayName.Reset();

	if (Subsystem == nullptr || Commands == nullptr)
	{
		DetailsBox->ClearChildren();
		DetailsBox->AddSlot()
		.FillHeight(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SubsystemUnavailable", "Asset Tab Groups subsystem is unavailable."))
		];
		return;
	}

	RebuildGroupItems();
	RebuildDetails();
}

void SAssetTabGroupsPanel::RebuildGroupItems()
{
	for (auto MissingGroupIt = MissingAssetReasons.CreateIterator(); MissingGroupIt; ++MissingGroupIt)
	{
		const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(MissingGroupIt.Key());
		if (Group == nullptr)
		{
			MissingGroupIt.RemoveCurrent();
			continue;
		}

		for (auto MissingAssetIt = MissingGroupIt.Value().CreateIterator(); MissingAssetIt; ++MissingAssetIt)
		{
			if (!Group->ContainsAsset(MissingAssetIt.Key()))
			{
				MissingAssetIt.RemoveCurrent();
			}
		}

		if (MissingGroupIt.Value().Num() == 0)
		{
			MissingGroupIt.RemoveCurrent();
		}
	}

	const FGuid PreviousSelectedGroupId = SelectedGroupId;
	GroupItems.Reset();
	const TArray<FAssetTabGroup>& Groups = Subsystem->GetRepository().GetGroups();
	for (const FAssetTabGroup& Group : Groups)
	{
		GroupItems.Add(MakeShared<FGuid>(Group.Id));
	}

	if (!SelectedGroupId.IsValid() || Subsystem->GetRepository().FindGroup(SelectedGroupId) == nullptr)
	{
		SelectedGroupId = Groups.Num() > 0 ? Groups[0].Id : FGuid();
	}
	if (SelectedGroupId != PreviousSelectedGroupId)
	{
		SelectedMemberAssetPaths.Reset();
		MemberSelectionAnchorAssetPath.Reset();
	}

	GroupListView->RequestListRefresh();
	for (const TSharedPtr<FGuid>& GroupId : GroupItems)
	{
		if (GroupId.IsValid() && *GroupId == SelectedGroupId)
		{
			GroupListView->SetSelection(GroupId, ESelectInfo::Direct);
			break;
		}
	}
}

void SAssetTabGroupsPanel::RebuildDetails()
{
	DetailsBox->ClearChildren();
	CurrentMemberItems.Reset();
	ActiveThumbnails.Reset();

	if (Subsystem == nullptr || !SelectedGroupId.IsValid())
	{
		DetailsBox->AddSlot()
		.FillHeight(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectGroupHint", "Select an asset group to view its assets."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(SelectedGroupId);
	if (Group == nullptr)
	{
		DetailsBox->AddSlot()
		.FillHeight(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("GroupUnavailable", "The selected asset group is unavailable."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	TSet<FString> ValidSelectedMemberPaths;
	for (const FAssetTabGroupMember& Member : Group->Members)
	{
		CurrentMemberItems.Add(MakeShared<FAssetTabGroupMember>(Member));
		if (SelectedMemberAssetPaths.Contains(Member.AssetPath))
		{
			ValidSelectedMemberPaths.Add(Member.AssetPath);
		}
	}
	SelectedMemberAssetPaths = MoveTemp(ValidSelectedMemberPaths);
	if (!MemberSelectionAnchorAssetPath.IsEmpty() && !SelectedMemberAssetPaths.Contains(MemberSelectionAnchorAssetPath))
	{
		MemberSelectionAnchorAssetPath.Reset();
	}

	DetailsBox->AddSlot()
	.FillHeight(1.0f)
	[
		MakeGroupDetails(*Group)
	];
}

void SAssetTabGroupsPanel::HandleGroupsChanged()
{
	Rebuild();
}

void SAssetTabGroupsPanel::GroupSelectionChanged(TSharedPtr<FGuid> GroupId, ESelectInfo::Type /*SelectInfo*/)
{
	if (!GroupId.IsValid())
	{
		return;
	}

	if (SelectedGroupId != *GroupId)
	{
		SelectedGroupId = *GroupId;
		SelectedMemberAssetPaths.Reset();
		MemberSelectionAnchorAssetPath.Reset();
	}
	RebuildDetails();
}

void SAssetTabGroupsPanel::HandleMemberTileSelection(
	const FGuid GroupId,
	const FString& AssetPath,
	const FPointerEvent& MouseEvent)
{
	if (!GroupId.IsValid() || GroupId != SelectedGroupId || AssetPath.IsEmpty())
	{
		return;
	}

	const bool bIsRightClick = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
	const bool bIsControlClick = MouseEvent.IsControlDown();
	const bool bIsShiftClick = MouseEvent.IsShiftDown();
	if (bIsRightClick)
	{
		if (!SelectedMemberAssetPaths.Contains(AssetPath))
		{
			SelectedMemberAssetPaths.Reset();
			SelectedMemberAssetPaths.Add(AssetPath);
		}
		MemberSelectionAnchorAssetPath = AssetPath;
	}
	else if (bIsShiftClick && !MemberSelectionAnchorAssetPath.IsEmpty())
	{
		int32 AnchorIndex = INDEX_NONE;
		int32 ClickedIndex = INDEX_NONE;
		for (int32 Index = 0; Index < CurrentMemberItems.Num(); ++Index)
		{
			if (CurrentMemberItems[Index].IsValid())
			{
				if (CurrentMemberItems[Index]->AssetPath == MemberSelectionAnchorAssetPath)
				{
					AnchorIndex = Index;
				}
				if (CurrentMemberItems[Index]->AssetPath == AssetPath)
				{
					ClickedIndex = Index;
				}
			}
		}

		if (AnchorIndex != INDEX_NONE && ClickedIndex != INDEX_NONE)
		{
			SelectedMemberAssetPaths.Reset();
			const int32 RangeStart = FMath::Min(AnchorIndex, ClickedIndex);
			const int32 RangeEnd = FMath::Max(AnchorIndex, ClickedIndex);
			for (int32 Index = RangeStart; Index <= RangeEnd; ++Index)
			{
				if (CurrentMemberItems[Index].IsValid())
				{
					SelectedMemberAssetPaths.Add(CurrentMemberItems[Index]->AssetPath);
				}
			}
		}
		else
		{
			SelectedMemberAssetPaths.Reset();
			SelectedMemberAssetPaths.Add(AssetPath);
		}
	}
	else if (bIsControlClick)
	{
		if (SelectedMemberAssetPaths.Contains(AssetPath))
		{
			SelectedMemberAssetPaths.Remove(AssetPath);
		}
		else
		{
			SelectedMemberAssetPaths.Add(AssetPath);
		}
		MemberSelectionAnchorAssetPath = AssetPath;
	}
	else
	{
		SelectedMemberAssetPaths.Reset();
		SelectedMemberAssetPaths.Add(AssetPath);
		MemberSelectionAnchorAssetPath = AssetPath;
	}

	if (DetailsBox.IsValid())
	{
		DetailsBox->Invalidate(EInvalidateWidget::Paint);
	}
}

bool SAssetTabGroupsPanel::IsMemberSelected(const FString& AssetPath) const
{
	return SelectedMemberAssetPaths.Contains(AssetPath);
}

void SAssetTabGroupsPanel::ReorderGroupFromDrop(
	const FGuid SourceGroupId,
	const FGuid TargetGroupId,
	const bool bDropAfter)
{
	if (Subsystem == nullptr || Commands == nullptr || !SourceGroupId.IsValid() || !TargetGroupId.IsValid() || SourceGroupId == TargetGroupId)
	{
		return;
	}

	const TArray<FAssetTabGroup>& Groups = Subsystem->GetRepository().GetGroups();
	const int32 SourceIndex = Groups.IndexOfByPredicate(
		[&SourceGroupId](const FAssetTabGroup& Group)
		{
			return Group.Id == SourceGroupId;
		});
	const int32 TargetIndex = Groups.IndexOfByPredicate(
		[&TargetGroupId](const FAssetTabGroup& Group)
		{
			return Group.Id == TargetGroupId;
		});
	if (SourceIndex == INDEX_NONE || TargetIndex == INDEX_NONE)
	{
		return;
	}

	int32 NewIndex = TargetIndex + (bDropAfter ? 1 : 0);
	if (SourceIndex < TargetIndex)
	{
		--NewIndex;
	}
	if (NewIndex != SourceIndex)
	{
		Commands->ReorderGroup(SourceGroupId, NewIndex);
	}
}

TSharedRef<ITableRow> SAssetTabGroupsPanel::GenerateGroupRow(
	TSharedPtr<FGuid> GroupId,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const FAssetTabGroup* Group = GroupId.IsValid() && Subsystem != nullptr
		? Subsystem->GetRepository().FindGroup(*GroupId)
		: nullptr;
	TSharedRef<SWidget> RowContent = SNew(STextBlock)
		.Text(LOCTEXT("InvalidGroupRow", "Unavailable group"));
	if (Group != nullptr)
	{
		RowContent = MakeGroupRowWidget(*Group);
	}

	return SNew(AssetTabGroupsPrivate::SAssetTabGroupRow, OwnerTable)
		.GroupId(GroupId)
		.OnDropRequested_Lambda([this](const FGuid& TargetGroupId, const FGuid& SourceGroupId, bool bDropAfter)
		{
			ReorderGroupFromDrop(SourceGroupId, TargetGroupId, bDropAfter);
		})
	[
		RowContent
	];
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeGroupRowWidget(const FAssetTabGroup& Group)
{
	const TMap<FString, FString>* MissingReasons = MissingAssetReasons.Find(Group.Id);
	const int32 MissingCount = MissingReasons != nullptr ? MissingReasons->Num() : 0;
	const FText CountText = MissingCount > 0
		? FText::Format(
			LOCTEXT("GroupRowCountWithMissing", "{0} assets  ({1} missing)"),
			FText::AsNumber(Group.Members.Num()),
			FText::AsNumber(MissingCount))
		: FText::Format(
			LOCTEXT("GroupRowCount", "{0} assets"),
			FText::AsNumber(Group.Members.Num()));

	return SNew(SBorder)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(4.0f, 5.0f))
		.OnMouseButtonDown_Lambda([this, GroupId = Group.Id](const FGeometry&, const FPointerEvent& MouseEvent)
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				PendingGroupContextMenuId = GroupId;
			}
			return FReply::Unhandled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(AssetTabGroupsPrivate::SAssetTabGroupDragHandle)
				.ToolTipText(LOCTEXT("GroupDragHandleTooltip", "Drag to reorder this asset group."))
				.OnDragDetected_Lambda([GroupId = Group.Id, GroupName = Group.Name](const FGeometry&, const FPointerEvent&)
				{
					return FReply::Handled().BeginDragDrop(
						AssetTabGroupsPrivate::FAssetTabGroupDragDropOp::New(GroupId, GroupName));
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SColorBlock)
				.Color(FAssetTabGroupRepository::GetColorForId(Group.ColorId))
				.Size(FVector2D(10.0f, 10.0f))
				.ASSETTABGROUPS_SColorBlock_IgnoreAlpha
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Group.Name))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(CountText)
					.ColorAndOpacity(MissingCount > 0 ? AssetTabGroupsCompat::GetMissingForegroundColor() : FSlateColor::UseSubduedForeground())
				]
			]
		];
}

TSharedRef<ITableRow> SAssetTabGroupsPanel::GenerateMemberRow(
	TSharedPtr<FAssetTabGroupMember> Member,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!Member.IsValid())
	{
		return SNew(STableRow<TSharedPtr<FAssetTabGroupMember>>, OwnerTable)
		.Style(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.TableRow"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("InvalidMemberRow", "Unavailable asset"))
		];
	}

	const FGuid GroupId = SelectedGroupId;
	const FString AssetPath = Member->AssetPath;
	const FString DisplayName = Member->DisplayName.IsEmpty() ? AssetPath : Member->DisplayName;
	const TMap<FString, FString>* GroupMissingReasons = MissingAssetReasons.Find(GroupId);
	const FString* MissingReason = GroupMissingReasons != nullptr
		? GroupMissingReasons->Find(AssetPath)
		: nullptr;
	const bool bMemberMissing = MissingReason != nullptr;

	FString TypeText = TEXT("Unknown");
	bool bAssetDataValid = false;
	if (!bMemberMissing)
	{
		const FSoftObjectPath SoftObjectPath(AssetPath);
		if (SoftObjectPath.IsValid())
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			const FAssetData AssetData = AssetTabGroupsCompat::GetAssetBySoftObjectPath(
				AssetRegistryModule.Get(),
				SoftObjectPath);
			if (AssetData.IsValid())
			{
				TypeText = AssetTabGroupsCompat::GetAssetClassString(AssetData);
				bAssetDataValid = true;
			}
		}
	}

	FText StatusText = LOCTEXT("AvailableMemberStatus", "Available");
	if (bMemberMissing)
	{
		StatusText = FText::Format(
			LOCTEXT("MissingMemberStatus", "Missing: {0}"),
			FText::FromString(*MissingReason));
	}
	else if (!bAssetDataValid)
	{
		StatusText = LOCTEXT("UnregisteredMemberStatus", "Unregistered");
	}

	return SNew(AssetTabGroupsPrivate::SAssetTabGroupMemberRow, OwnerTable)
		.Member(Member)
		.TypeText(TypeText)
		.PackageText(Member->PackagePath)
		.StatusText(StatusText)
		.bMissing(bMemberMissing)
		.OnContextMenuRequested_Lambda([this, GroupId, AssetPath, DisplayName]()
		{
			PendingMemberContextMenuGroupId = GroupId;
			PendingMemberContextMenuAssetPath = AssetPath;
			PendingMemberContextMenuDisplayName = DisplayName;
		});
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeMemberTile(const FAssetTabGroupMember& Member)
{
	const FGuid GroupId = SelectedGroupId;
	const FString AssetPath = Member.AssetPath;
	const FString DisplayName = Member.DisplayName.IsEmpty() ? AssetPath : Member.DisplayName;
	const TMap<FString, FString>* GroupMissingReasons = MissingAssetReasons.Find(GroupId);
	const FString* MissingReason = GroupMissingReasons != nullptr
		? GroupMissingReasons->Find(AssetPath)
		: nullptr;
	const bool bMemberMissing = MissingReason != nullptr;
	const FText MemberLabel = bMemberMissing
		? FText::Format(LOCTEXT("MissingTileMemberLabel", "[Missing] {0}"), FText::FromString(DisplayName))
		: FText::FromString(DisplayName);

	TSharedRef<SVerticalBox> VisualContent = SNew(SVerticalBox);
	bool bHasThumbnail = false;
	if (!bMemberMissing && ThumbnailPool.IsValid())
	{
		const FSoftObjectPath SoftObjectPath(AssetPath);
		if (SoftObjectPath.IsValid())
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			const FAssetData AssetData = AssetTabGroupsCompat::GetAssetBySoftObjectPath(
				AssetRegistryModule.Get(),
				SoftObjectPath);
			if (AssetData.IsValid())
			{
				TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(
					new FAssetThumbnail(AssetData, 128, 128, ThumbnailPool));
				ActiveThumbnails.Add(Thumbnail);
				VisualContent->AddSlot()
				.AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(128.0f)
					.HeightOverride(128.0f)
					[
						Thumbnail->MakeThumbnailWidget()
					]
				];
				bHasThumbnail = true;
			}
		}
	}

	if (!bHasThumbnail)
	{
		VisualContent->AddSlot()
		.AutoHeight()
		[
			SNew(SBox)
			.WidthOverride(128.0f)
			.HeightOverride(128.0f)
			[
				SNew(STextBlock)
				.Text(bMemberMissing
					? LOCTEXT("MissingTilePreview", "Missing")
					: LOCTEXT("UnavailableTilePreview", "No Preview"))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(bMemberMissing ? AssetTabGroupsCompat::GetMissingForegroundColor() : FSlateColor::UseSubduedForeground())
			]
		];
	}

	VisualContent->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 5.0f, 0.0f, 0.0f)
	[
		SNew(STextBlock)
		.Text(MemberLabel)
		.ToolTipText(FText::FromString(AssetPath))
		.AutoWrapText(true)
		.ColorAndOpacity(bMemberMissing ? AssetTabGroupsCompat::GetMissingForegroundColor() : FSlateColor::UseForeground())
	];

	VisualContent->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 4.0f, 0.0f, 0.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Member.PackagePath))
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];

	return SNew(SBox)
		.WidthOverride(160.0f)
		.Padding(4.0f)
		[
			SNew(SBorder)
			.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.PanelBackground")))
			.BorderBackgroundColor_Lambda([this, AssetPath, bMemberMissing]()
			{
				if (IsMemberSelected(AssetPath))
				{
					return FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.TileSelection"))->TintColor;
				}

				return FSlateColor(bMemberMissing
					? AssetTabGroupsCompat::GetMissingTileBackgroundColor()
					: AssetTabGroupsCompat::GetTileBackgroundColor());
			})
			.Padding(5.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(AssetTabGroupsPrivate::SAssetTabGroupMemberTile)
					.OnDoubleClicked_Lambda([this, GroupId, AssetPath]()
					{
						OpenMemberAsset(GroupId, AssetPath);
						return FReply::Handled();
					})
					.OnSelectionRequested_Lambda([this, GroupId, AssetPath](const FPointerEvent& MouseEvent)
					{
						HandleMemberTileSelection(GroupId, AssetPath, MouseEvent);
					})
					.OnGetMenuContent_Lambda([this, GroupId, AssetPath, DisplayName]()
					{
						return MakeTileMemberMenu(GroupId, AssetPath, DisplayName);
					})
					[
						VisualContent
					]
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				.Padding(2.0f)
				[
					SNew(SBorder)
					.BorderImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.TileSelection")))
					.Padding(2.0f)
					.Visibility_Lambda([this, AssetPath]()
					{
						return IsMemberSelected(AssetPath) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					[
						SNew(SBox)
						.WidthOverride(8.0f)
						.HeightOverride(8.0f)
						[
							SNew(SColorBlock)
							.Color(FLinearColor::White)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeGroupDetails(const FAssetTabGroup& Group)
{
	const TMap<FString, FString>* MissingReasons = MissingAssetReasons.Find(Group.Id);
	const int32 MissingCount = MissingReasons != nullptr ? MissingReasons->Num() : 0;

	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
	Header->AddSlot()
	.AutoWidth()
	.Padding(0.0f, 2.0f, 8.0f, 2.0f)
	.VAlign(VAlign_Center)
	[
		SNew(SColorBlock)
		.Color(FAssetTabGroupRepository::GetColorForId(Group.ColorId))
		.Size(FVector2D(12.0f, 12.0f))
		.ASSETTABGROUPS_SColorBlock_IgnoreAlpha
	];
	Header->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Group.Name))
		.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
	];
	Header->AddSlot()
	.FillWidth(1.0f)
	.Padding(12.0f, 0.0f)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(Group.Note.IsEmpty() ? LOCTEXT("NoGroupNote", "No note") : FText::FromString(Group.Note))
		.ToolTipText(Group.Note.IsEmpty() ? LOCTEXT("NoGroupNote", "No note") : FText::FromString(Group.Note))
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];
	Header->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(4.0f, 0.0f)
	[
		SNew(STextBlock)
		.Text(
			MissingCount > 0
				? FText::Format(
					LOCTEXT("DetailCountWithMissing", "{0} assets / {1} missing"),
					FText::AsNumber(Group.Members.Num()),
					FText::AsNumber(MissingCount))
				: FText::Format(
					LOCTEXT("DetailCount", "{0} assets"),
					FText::AsNumber(Group.Members.Num())))
		.ColorAndOpacity(MissingCount > 0 ? AssetTabGroupsCompat::GetMissingForegroundColor() : FSlateColor::UseSubduedForeground())
	];
	Header->AddSlot()
	.AutoWidth()
	.Padding(4.0f, 0.0f)
	[
		SNew(SButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		ASSETTABGROUPS_BUTTON_TEXT_STYLE
		.Text(ViewMode == EAssetTabGroupViewMode::List
			? LOCTEXT("SwitchToTileView", "Tile")
			: LOCTEXT("SwitchToListView", "List"))
		.ToolTipText(LOCTEXT("SwitchAssetViewTooltip", "Switch between list and tile views."))
		.OnClicked_Lambda([this]()
		{
			ToggleViewMode();
			return FReply::Handled();
		})
	];
	Header->AddSlot()
	.AutoWidth()
	.Padding(2.0f, 0.0f)
	[
		SNew(SButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		ASSETTABGROUPS_BUTTON_TEXT_STYLE
		.Text(LOCTEXT("OpenGroup", "Open"))
		.OnClicked_Lambda([this, GroupId = Group.Id]()
		{
			OpenGroup(GroupId);
			return FReply::Handled();
		})
	];
	Header->AddSlot()
	.AutoWidth()
	.Padding(2.0f, 0.0f)
	[
		SNew(SButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		ASSETTABGROUPS_BUTTON_TEXT_STYLE
		.Text(LOCTEXT("FocusGroup", "Focus"))
		.ToolTipText(LOCTEXT("FocusGroupTooltip", "Open this group and safely close clean asset editors outside it."))
		.OnClicked_Lambda([this, GroupId = Group.Id]()
		{
			FocusGroup(GroupId);
			return FReply::Handled();
		})
	];
	Header->AddSlot()
	.AutoWidth()
	.Padding(2.0f, 0.0f)
	[
		SNew(SButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		ASSETTABGROUPS_BUTTON_TEXT_STYLE
		.Text(LOCTEXT("AddOpenAssetsShort", "Add"))
		.OnClicked_Lambda([this, GroupId = Group.Id]()
		{
			AddOpenAssets(GroupId);
			return FReply::Handled();
		})
	];
	Header->AddSlot()
	.AutoWidth()
	.Padding(2.0f, 0.0f)
	[
		SNew(SButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		ASSETTABGROUPS_BUTTON_TEXT_STYLE
		.Text(LOCTEXT("AddSelectedContentBrowserAssetsShort", "Add Selected"))
		.ToolTipText(LOCTEXT("AddSelectedContentBrowserAssetsShortTooltip", "Add assets currently selected in the primary Content Browser."))
		.OnClicked_Lambda([this, GroupId = Group.Id]()
		{
			AddSelectedContentBrowserAssets(GroupId);
			return FReply::Handled();
		})
	];
	Header->AddSlot()
	.AutoWidth()
	.Padding(2.0f, 0.0f)
	[
		SNew(SButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		ASSETTABGROUPS_BUTTON_TEXT_STYLE
		.Text(LOCTEXT("UpdateOpenAssetsShort", "Update"))
		.OnClicked_Lambda([this, GroupId = Group.Id]()
		{
			UpdateFromOpenAssets(GroupId);
			return FReply::Handled();
		})
	];
	Header->AddSlot()
	.AutoWidth()
	[
		SNew(SComboButton)
		.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
		.OnGetMenuContent_Lambda([this, GroupId = Group.Id]()
		{
			return MakeGroupMenu(GroupId);
		})
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("...")))
		]
	];

	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
	Content->AddSlot()
	.AutoHeight()
	[
		Header
	];
	Content->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 6.0f)
	[
		SNew(SSeparator)
		.SeparatorImage(FAssetTabGroupsStyle::Get().GetBrush(TEXT("AssetTabGroups.Separator")))
		.Thickness(2.0f)
	];

	MemberListView.Reset();
	bool bUseDetailsSurfaceContextMenu = false;
	if (CurrentMemberItems.Num() == 0)
	{
		Content->AddSlot()
		.FillHeight(1.0f)
		[
			SAssignNew(MemberListView, SListView<TSharedPtr<FAssetTabGroupMember>>)
			.ListItemsSource(&CurrentMemberItems)
			.OnGenerateRow(this, &SAssetTabGroupsPanel::GenerateMemberRow)
			.OnContextMenuOpening_Lambda([this]() -> TSharedPtr<SWidget>
			{
				if (SelectedGroupId.IsValid())
				{
					return MakeGroupDetailsMenu(SelectedGroupId);
				}

				return TSharedPtr<SWidget>();
			})
			.HeaderRow(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberNameColumnId)
				.DefaultLabel(LOCTEXT("MemberNameColumn", "Name"))
				.FillWidth(0.38f)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberTypeColumnId)
				.DefaultLabel(LOCTEXT("MemberTypeColumn", "Type"))
				.FillWidth(0.16f)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberPackageColumnId)
				.DefaultLabel(LOCTEXT("MemberPackageColumn", "Package Path"))
				.FillWidth(0.28f)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberStatusColumnId)
				.DefaultLabel(LOCTEXT("MemberStatusColumn", "Status"))
				.FillWidth(0.18f)
			)
			.SelectionMode(ESelectionMode::Single)
			.ClearSelectionOnClick(true)
			.ScrollbarVisibility(EVisibility::Visible)
		];
	}
	else if (ViewMode == EAssetTabGroupViewMode::List)
	{
		Content->AddSlot()
		.FillHeight(1.0f)
		[
			SAssignNew(MemberListView, SListView<TSharedPtr<FAssetTabGroupMember>>)
			.ListItemsSource(&CurrentMemberItems)
			.OnGenerateRow(this, &SAssetTabGroupsPanel::GenerateMemberRow)
			.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FAssetTabGroupMember> Member)
			{
				if (Member.IsValid())
				{
					OpenMemberAsset(SelectedGroupId, Member->AssetPath);
				}
			})
			.OnContextMenuOpening_Lambda([this]() -> TSharedPtr<SWidget>
			{
				const FGuid ContextGroupId = PendingMemberContextMenuGroupId;
				const FString ContextAssetPath = PendingMemberContextMenuAssetPath;
				const FString ContextDisplayName = PendingMemberContextMenuDisplayName;
				PendingMemberContextMenuGroupId.Invalidate();
				PendingMemberContextMenuAssetPath.Reset();
				PendingMemberContextMenuDisplayName.Reset();

				if (ContextGroupId.IsValid() && ContextGroupId == SelectedGroupId && !ContextAssetPath.IsEmpty())
				{
					return MakeMemberMenu(ContextGroupId, ContextAssetPath, ContextDisplayName);
				}

				if (SelectedGroupId.IsValid())
				{
					return MakeGroupDetailsMenu(SelectedGroupId);
				}

				return TSharedPtr<SWidget>();
			})
			.HeaderRow(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberNameColumnId)
				.DefaultLabel(LOCTEXT("MemberNameColumn", "Name"))
				.FillWidth(0.38f)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberTypeColumnId)
				.DefaultLabel(LOCTEXT("MemberTypeColumn", "Type"))
				.FillWidth(0.16f)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberPackageColumnId)
				.DefaultLabel(LOCTEXT("MemberPackageColumn", "Package Path"))
				.FillWidth(0.28f)
				+ SHeaderRow::Column(AssetTabGroupsPrivate::MemberStatusColumnId)
				.DefaultLabel(LOCTEXT("MemberStatusColumn", "Status"))
				.FillWidth(0.18f)
			)
			.SelectionMode(ESelectionMode::Single)
			.ClearSelectionOnClick(true)
			.ScrollbarVisibility(EVisibility::Visible)
		];
	}
	else
	{
		bUseDetailsSurfaceContextMenu = true;
		TSharedRef<SWrapBox> TileBox = SNew(SWrapBox)
			.UseAllottedWidth(true)
			.InnerSlotPadding(FVector2D(4.0f, 4.0f));
		for (const TSharedPtr<FAssetTabGroupMember>& Member : CurrentMemberItems)
		{
			if (Member.IsValid())
			{
				TileBox->AddSlot()
				[
					MakeMemberTile(*Member)
				];
			}
		}

		Content->AddSlot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			.ScrollBarVisibility(EVisibility::Visible)
			+ SScrollBox::Slot()
			[
				TileBox
			]
		];
	}

	if (bUseDetailsSurfaceContextMenu)
	{
		return SNew(AssetTabGroupsPrivate::SAssetTabGroupDetailsSurface)
			.OnGetMenuContent_Lambda([this, GroupId = Group.Id]()
			{
				return MakeGroupDetailsMenu(GroupId);
			})
			[
				Content
			];
	}

	return Content;
}

void SAssetTabGroupsPanel::ToggleViewMode()
{
	ViewMode = ViewMode == EAssetTabGroupViewMode::List
		? EAssetTabGroupViewMode::Tile
		: EAssetTabGroupViewMode::List;
	SelectedMemberAssetPaths.Reset();
	MemberSelectionAnchorAssetPath.Reset();
	RebuildDetails();
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeGroupMenu(const FGuid GroupId)
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("AssetTabGroupActions", LOCTEXT("GroupActionsHeading", "Group"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuOpenGroup", "Open Group"),
			LOCTEXT("MenuOpenGroupTooltip", "Open all available assets in this group."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				OpenGroup(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuFocusGroup", "Focus Group (Safe)"),
			LOCTEXT("MenuFocusGroupTooltip", "Open the group and safely close clean asset editors outside it."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				FocusGroup(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuMoveGroupUp", "Move Group Up"),
			LOCTEXT("MenuMoveGroupUpTooltip", "Move this group one position up."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				const int32 GroupIndex = Subsystem->GetRepository().GetGroups().IndexOfByPredicate(
					[&GroupId](const FAssetTabGroup& Group)
					{
						return Group.Id == GroupId;
					});
				if (GroupIndex > 0)
				{
					Commands->ReorderGroup(GroupId, GroupIndex - 1);
				}
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuMoveGroupDown", "Move Group Down"),
			LOCTEXT("MenuMoveGroupDownTooltip", "Move this group one position down."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				const TArray<FAssetTabGroup>& Groups = Subsystem->GetRepository().GetGroups();
				const int32 GroupIndex = Groups.IndexOfByPredicate(
					[&GroupId](const FAssetTabGroup& Group)
					{
						return Group.Id == GroupId;
					});
				if (GroupIndex >= 0 && GroupIndex + 1 < Groups.Num())
				{
					Commands->ReorderGroup(GroupId, GroupIndex + 1);
				}
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuAddOpenAssets", "Add Open Assets..."),
			LOCTEXT("MenuAddOpenAssetsTooltip", "Choose open assets to append to this group."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				AddOpenAssets(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuUpdateOpenAssets", "Update from Current Open Assets"),
			LOCTEXT("MenuUpdateOpenAssetsTooltip", "Replace this group's members with the currently open assets."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				UpdateFromOpenAssets(GroupId);
			})));
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AssetTabGroupMetadata", LOCTEXT("GroupMetadataHeading", "Metadata"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuRenameGroup", "Edit Title"),
			LOCTEXT("MenuRenameGroupTooltip", "Change the group title."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				RenameGroup(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuEditNote", "Edit Note"),
			LOCTEXT("MenuEditNoteTooltip", "Change the group note."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				EditGroupNote(GroupId);
			})));
		MenuBuilder.AddSubMenu(
			LOCTEXT("MenuChangeColor", "Change Color"),
			LOCTEXT("MenuChangeColorTooltip", "Choose the group color."),
			FNewMenuDelegate::CreateLambda([this, GroupId](FMenuBuilder& ColorMenu)
			{
				ColorMenu.AddWidget(MakeColorMenu(GroupId), FText::GetEmpty(), true);
			}));
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AssetTabGroupDestructive", LOCTEXT("GroupDestructiveHeading", "管理"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuRemoveMissingMembers", "Remove Recorded Missing Assets"),
			LOCTEXT("MenuRemoveMissingMembersTooltip", "Remove assets that failed during the last restore attempt."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				RemoveMissingMembers(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuDeleteGroup", "Delete Group"),
			LOCTEXT("MenuDeleteGroupTooltip", "Delete group metadata without closing or deleting assets."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				DeleteGroup(GroupId);
			})));
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeEmptyGroupMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("AssetTabGroupsEmptyArea", LOCTEXT("EmptyGroupAreaHeading", "Asset Groups"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("MenuCreateGroupFromEmptyArea", "New Group"),
		LOCTEXT("MenuCreateGroupFromEmptyAreaTooltip", "Create a new empty asset group."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			CreateEmptyGroup();
		})));
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeGroupDetailsMenu(const FGuid GroupId)
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("AssetTabGroupsDetailsActions", LOCTEXT("GroupDetailsActionsHeading", "Group Files"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuAddContentBrowserFiles", "Add Files..."),
			LOCTEXT("MenuAddContentBrowserFilesTooltip", "Choose assets from the Content Browser and add them to this group."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				AddContentBrowserAssets(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuAddSelectedContentBrowserFiles", "Add Selected Content Browser Assets"),
			LOCTEXT("MenuAddSelectedContentBrowserFilesTooltip", "Add the assets currently selected in the primary Content Browser."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				AddSelectedContentBrowserAssets(GroupId);
			})));
		MenuBuilder.AddMenuSeparator();
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuOpenGroupFiles", "Open Group Files"),
			LOCTEXT("MenuOpenGroupFilesTooltip", "Open all available assets in this group."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				OpenGroup(GroupId);
			})));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MenuFocusGroupFiles", "Focus Group Files"),
			LOCTEXT("MenuFocusGroupFilesTooltip", "Open this group and safely close clean asset editors outside it."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
			{
				FocusGroup(GroupId);
			})));
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void SAssetTabGroupsPanel::OpenMemberAsset(const FGuid GroupId, const FString& AssetPath)
{
	if (Commands != nullptr)
	{
		Commands->OpenAsset(AssetPath);
	}

	if (Subsystem != nullptr)
	{
		Subsystem->GetRepository().SetActiveAsset(GroupId, AssetPath);
	}
}

void SAssetTabGroupsPanel::OpenSelectedMembers(const FGuid GroupId)
{
	if (Commands == nullptr || Subsystem == nullptr)
	{
		return;
	}

	TArray<FString> SelectedPaths;
	for (const TSharedPtr<FAssetTabGroupMember>& Member : CurrentMemberItems)
	{
		if (Member.IsValid() && SelectedMemberAssetPaths.Contains(Member->AssetPath))
		{
			SelectedPaths.Add(Member->AssetPath);
		}
	}

	FString LastOpenedAssetPath;
	for (const FString& AssetPath : SelectedPaths)
	{
		if (Commands->OpenAsset(AssetPath))
		{
			LastOpenedAssetPath = AssetPath;
		}
	}
	if (!LastOpenedAssetPath.IsEmpty())
	{
		Subsystem->GetRepository().SetActiveAsset(GroupId, LastOpenedAssetPath);
	}
}

void SAssetTabGroupsPanel::RemoveSelectedMembers(const FGuid GroupId)
{
	if (Commands == nullptr)
	{
		return;
	}

	TArray<FString> SelectedPaths;
	FString FirstDisplayName;
	for (const TSharedPtr<FAssetTabGroupMember>& Member : CurrentMemberItems)
	{
		if (Member.IsValid() && SelectedMemberAssetPaths.Contains(Member->AssetPath))
		{
			SelectedPaths.Add(Member->AssetPath);
			if (FirstDisplayName.IsEmpty())
			{
				FirstDisplayName = Member->DisplayName.IsEmpty() ? Member->AssetPath : Member->DisplayName;
			}
		}
	}
	if (SelectedPaths.Num() == 0)
	{
		return;
	}

	const FText ConfirmationText = SelectedPaths.Num() == 1
		? FText::Format(
			LOCTEXT("RemoveSelectedMemberConfirmation", "Remove '{0}' from this group?"),
			FText::FromString(FirstDisplayName))
		: FText::Format(
			LOCTEXT("RemoveSelectedMembersConfirmation", "Remove {0} selected assets from this group?"),
			FText::AsNumber(SelectedPaths.Num()));
	if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmationText) != EAppReturnType::Yes)
	{
		return;
	}

	if (Commands->RemoveMembersFromGroup(GroupId, SelectedPaths))
	{
		SelectedMemberAssetPaths.Reset();
		MemberSelectionAnchorAssetPath.Reset();
	}
}

void SAssetTabGroupsPanel::AddContentBrowserAssets(const FGuid GroupId)
{
	if (Commands == nullptr)
	{
		return;
	}

	FContentBrowserModule* ContentBrowserModule = FModuleManager::LoadModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));
	if (ContentBrowserModule == nullptr)
	{
		Commands->Notify(LOCTEXT("ContentBrowserUnavailable", "The Content Browser module is unavailable."), false);
		return;
	}

	FOpenAssetDialogConfig DialogConfig;
	DialogConfig.DialogTitleOverride = LOCTEXT("AddGroupFilesDialogTitle", "Add Files to Asset Group");
	DialogConfig.bAllowMultipleSelection = true;
	DialogConfig.WindowSizeOverride = FVector2D(800.0f, 600.0f);

	const TArray<FAssetData> SelectedAssets = ContentBrowserModule->Get().CreateModalOpenAssetDialog(DialogConfig);
	AddAssetDataToGroup(GroupId, SelectedAssets);
}

void SAssetTabGroupsPanel::AddSelectedContentBrowserAssets(const FGuid GroupId)
{
	if (Commands == nullptr)
	{
		return;
	}

	FContentBrowserModule* ContentBrowserModule = FModuleManager::LoadModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));
	if (ContentBrowserModule == nullptr)
	{
		Commands->Notify(LOCTEXT("ContentBrowserUnavailable", "The Content Browser module is unavailable."), false);
		return;
	}

	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule->Get().GetSelectedAssets(SelectedAssets);
	if (SelectedAssets.Num() == 0)
	{
		Commands->Notify(LOCTEXT("NoSelectedContentBrowserAssets", "There are no assets selected in the primary Content Browser."), false);
		return;
	}

	AddAssetDataToGroup(GroupId, SelectedAssets);
}

void SAssetTabGroupsPanel::AddAssetDataToGroup(const FGuid GroupId, const TArray<FAssetData>& AssetData)
{
	if (Subsystem == nullptr || Commands == nullptr || !GroupId.IsValid())
	{
		return;
	}

	TArray<FAssetTabGroupMember> Members;
	for (const FAssetData& Asset : AssetData)
	{
		const FString AssetPath = AssetTabGroupsCompat::GetAssetObjectPathString(Asset);
		if (!Asset.IsValid() || AssetPath.IsEmpty())
		{
			continue;
		}

		FAssetTabGroupMember& NewMember = Members.AddDefaulted_GetRef();
		NewMember.AssetPath = AssetPath;
		NewMember.DisplayName = Asset.AssetName.ToString();
		if (NewMember.DisplayName.IsEmpty())
		{
			NewMember.DisplayName = AssetPath;
		}
		NewMember.PackagePath = Asset.PackagePath.ToString();
	}

	if (Members.Num() == 0)
	{
		Commands->Notify(LOCTEXT("NoValidContentBrowserAssets", "No valid assets were selected."), false);
		return;
	}

	int32 AddedCount = 0;
	if (Commands->AddMembersToGroup(GroupId, Members, &AddedCount))
	{
		const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
		if (Group != nullptr && Group->ActiveAssetPath.IsEmpty() && Members[0].IsValid() && Group->ContainsAsset(Members[0].AssetPath))
		{
			Subsystem->GetRepository().SetActiveAsset(GroupId, Members[0].AssetPath);
		}
	}
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeMemberMenu(
	const FGuid GroupId,
	const FString& AssetPath,
	const FString& DisplayName)
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("MenuOpenMember", "Open Asset"),
		LOCTEXT("MenuOpenMemberTooltip", "Open and focus this asset editor."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, GroupId, AssetPath]()
		{
			OpenMemberAsset(GroupId, AssetPath);
		})));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("MenuRemoveMember", "Remove from Group"),
		LOCTEXT("MenuRemoveMemberTooltip", "Remove this asset reference from the group without closing its editor."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, GroupId, AssetPath, DisplayName]()
		{
			if (FMessageDialog::Open(
					EAppMsgType::YesNo,
					FText::Format(
						LOCTEXT("RemoveMemberConfirmation", "Remove '{0}' from this group?"),
						FText::FromString(DisplayName))) == EAppReturnType::Yes)
			{
				Commands->RemoveMemberFromGroup(GroupId, AssetPath);
			}
		})));
	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeTileMemberMenu(
	const FGuid GroupId,
	const FString& AssetPath,
	const FString& DisplayName)
{
	if (SelectedMemberAssetPaths.Num() <= 1)
	{
		return MakeMemberMenu(GroupId, AssetPath, DisplayName);
	}

	const FText SelectedCountText = FText::AsNumber(SelectedMemberAssetPaths.Num());
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("AssetTabGroupSelectedMembers", LOCTEXT("SelectedMembersHeading", "Selected Assets"));
	MenuBuilder.AddMenuEntry(
		FText::Format(LOCTEXT("MenuOpenSelectedMembers", "Open Selected Assets ({0})"), SelectedCountText),
		LOCTEXT("MenuOpenSelectedMembersTooltip", "Open and focus all currently selected assets."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
		{
			OpenSelectedMembers(GroupId);
		})));
	MenuBuilder.AddMenuEntry(
		FText::Format(LOCTEXT("MenuRemoveSelectedMembers", "Remove Selected From Group ({0})"), SelectedCountText),
		LOCTEXT("MenuRemoveSelectedMembersTooltip", "Remove all currently selected asset references from this group."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, GroupId]()
		{
			RemoveSelectedMembers(GroupId);
		})));
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAssetTabGroupsPanel::MakeColorMenu(const FGuid GroupId)
{
	TSharedRef<SVerticalBox> ColorBox = SNew(SVerticalBox);
	static const TCHAR* ColorNames[] = { TEXT("Blue"), TEXT("Green"), TEXT("Red"), TEXT("Orange"), TEXT("Purple") };
	for (int32 ColorId = 0; ColorId < UE_ARRAY_COUNT(ColorNames); ++ColorId)
	{
		ColorBox->AddSlot()
		.AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(FAssetTabGroupsStyle::Get(), TEXT("AssetTabGroups.FlatButton"))
			.OnClicked_Lambda([this, GroupId, ColorId]()
			{
				Commands->SetGroupColor(GroupId, ColorId);
				return FReply::Handled();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SColorBlock)
					.Color(FAssetTabGroupRepository::GetColorForId(ColorId))
					.Size(FVector2D(10.0f, 10.0f))
					.ASSETTABGROUPS_SColorBlock_IgnoreAlpha
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ColorNames[ColorId]))
				]
			]
		];
	}
	return ColorBox;
}

void SAssetTabGroupsPanel::OpenGroup(const FGuid GroupId)
{
	const FAssetTabGroupOperationResult Result = Commands->OpenGroup(GroupId);
	ApplyRestoreResult(GroupId, Result);
	ShowRestoreResult(LOCTEXT("OpenGroupAction", "Open Group"), Result);
}

void SAssetTabGroupsPanel::FocusGroup(const FGuid GroupId)
{
	const FAssetTabGroupOperationResult Result = Commands->FocusGroup(GroupId);
	ApplyRestoreResult(GroupId, Result);
	ShowRestoreResult(LOCTEXT("FocusGroupAction", "Focus Group"), Result);
}

void SAssetTabGroupsPanel::ApplyRestoreResult(
	const FGuid GroupId,
	const FAssetTabGroupOperationResult& Result)
{
	if (Subsystem == nullptr)
	{
		return;
	}
	if (!Result.bSucceeded)
	{
		return;
	}

	TMap<FString, FString>& GroupMissingAssetReasons = MissingAssetReasons.FindOrAdd(GroupId);
	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
	if (Group != nullptr)
	{
		for (const FAssetTabGroupMember& Member : Group->Members)
		{
			GroupMissingAssetReasons.Remove(Member.AssetPath);
		}
	}

	for (const FAssetTabGroupOpenFailure& Failure : Result.FailedAssets)
	{
		GroupMissingAssetReasons.Add(Failure.Member.AssetPath, Failure.Reason);
	}

	if (GroupMissingAssetReasons.Num() == 0)
	{
		MissingAssetReasons.Remove(GroupId);
	}
	Rebuild();
}

void SAssetTabGroupsPanel::ShowRestoreResult(
	const FText& ActionName,
	const FAssetTabGroupOperationResult& Result) const
{
	if (Result.FailedAssets.Num() == 0)
	{
		return;
	}

	FString Message = FString::Printf(
		TEXT("%s completed with %d unavailable assets.\n\n"),
		*ActionName.ToString(),
		Result.FailedAssets.Num());
	for (const FAssetTabGroupOpenFailure& Failure : Result.FailedAssets)
	{
		const FString DisplayName = Failure.Member.DisplayName.IsEmpty()
			? Failure.Member.AssetPath
			: Failure.Member.DisplayName;
		Message += FString::Printf(
			TEXT("- %s\n  Path: %s\n  Reason: %s\n\n"),
			*DisplayName,
			*Failure.Member.AssetPath,
			*Failure.Reason);
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

void SAssetTabGroupsPanel::RemoveMissingMembers(const FGuid GroupId)
{
	const TMap<FString, FString>* GroupMissingAssetReasons = MissingAssetReasons.Find(GroupId);
	if (GroupMissingAssetReasons == nullptr || GroupMissingAssetReasons->Num() == 0)
	{
		Commands->Notify(LOCTEXT("NoMissingMembers", "This group has no recorded missing assets."), false);
		return;
	}

	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
	const FText GroupName = Group != nullptr
		? FText::FromString(Group->Name)
		: LOCTEXT("UnknownGroup", "this group");
	const int32 MissingCount = GroupMissingAssetReasons->Num();
	if (FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format(
				LOCTEXT("RemoveMissingMembersConfirmation", "Remove {0} unavailable assets from '{1}'?"),
				FText::AsNumber(MissingCount),
				GroupName)) != EAppReturnType::Yes)
	{
		return;
	}

	TArray<FString> MissingPaths;
	GroupMissingAssetReasons->GenerateKeyArray(MissingPaths);
	for (const FString& AssetPath : MissingPaths)
	{
		Commands->RemoveMemberFromGroup(GroupId, AssetPath);
	}
	MissingAssetReasons.Remove(GroupId);
	Rebuild();
	Commands->Notify(
		FText::Format(
			LOCTEXT("RemovedMissingMembers", "Removed {0} unavailable assets from the group."),
			FText::AsNumber(MissingCount)));
}

void SAssetTabGroupsPanel::CreateEmptyGroup()
{
	if (Subsystem == nullptr || Commands == nullptr)
	{
		return;
	}

	FString Name = TEXT("New Group");
	if (PromptForText(LOCTEXT("CreateGroupDialogTitle", "Create Asset Group"), LOCTEXT("GroupNamePrompt", "Group name"), Name))
	{
		SelectedGroupId = Commands->CreateEmptyGroup(Name);
		Rebuild();
	}
}

void SAssetTabGroupsPanel::SaveAllOpenAssets()
{
	FString Name = TEXT("Open Assets");
	if (PromptForText(LOCTEXT("SaveAllDialogTitle", "Save All Open Assets"), LOCTEXT("GroupNamePrompt", "Group name"), Name))
	{
		SelectedGroupId = Commands->CreateGroupFromOpenAssets(Name);
		Rebuild();
	}
}

void SAssetTabGroupsPanel::SaveSelectedOpenAssets()
{
	TArray<FAssetTabGroupMember> Members;
	FString ActiveAssetPath;
	if (!PromptForOpenAssetSelection(Members, ActiveAssetPath))
	{
		return;
	}

	FString Name = TEXT("Selected Assets");
	if (PromptForText(LOCTEXT("SaveSelectedDialogTitle", "Save Selected Open Assets"), LOCTEXT("GroupNamePrompt", "Group name"), Name))
	{
		SelectedGroupId = Commands->CreateGroupFromMembers(Name, Members, ActiveAssetPath);
		Rebuild();
	}
}

void SAssetTabGroupsPanel::AddOpenAssets(const FGuid GroupId)
{
	TArray<FAssetTabGroupMember> Members;
	FString ActiveAssetPath;
	if (!PromptForOpenAssetSelection(Members, ActiveAssetPath))
	{
		return;
	}

	int32 AddedCount = 0;
	if (Commands->AddMembersToGroup(GroupId, Members, &AddedCount))
	{
		const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
		if (Group != nullptr && Group->ActiveAssetPath.IsEmpty() && Group->ContainsAsset(ActiveAssetPath))
		{
			Subsystem->GetRepository().SetActiveAsset(GroupId, ActiveAssetPath);
		}
	}
}

void SAssetTabGroupsPanel::UpdateFromOpenAssets(const FGuid GroupId)
{
	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
	const FText GroupName = Group != nullptr ? FText::FromString(Group->Name) : LOCTEXT("UnknownGroup", "this group");
	if (FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format(
				LOCTEXT("UpdateGroupConfirmation", "Replace all members of '{0}' with the currently open assets?"),
				GroupName)) == EAppReturnType::Yes)
	{
		Commands->UpdateGroupFromOpenAssets(GroupId);
	}
}

void SAssetTabGroupsPanel::RenameGroup(const FGuid GroupId)
{
	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
	if (Group == nullptr)
	{
		return;
	}

	FString Name = Group->Name;
	if (PromptForText(LOCTEXT("RenameGroupDialogTitle", "Rename Asset Group"), LOCTEXT("GroupNamePrompt", "Group name"), Name))
	{
		Commands->RenameGroup(GroupId, Name);
	}
}

void SAssetTabGroupsPanel::EditGroupNote(const FGuid GroupId)
{
	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
	if (Group == nullptr)
	{
		return;
	}

	FString Note = Group->Note;
	if (PromptForText(LOCTEXT("EditNoteDialogTitle", "Edit Group Note"), LOCTEXT("GroupNotePrompt", "Note"), Note))
	{
		Commands->SetGroupNote(GroupId, Note);
	}
}

void SAssetTabGroupsPanel::DeleteGroup(const FGuid GroupId)
{
	const FAssetTabGroup* Group = Subsystem->GetRepository().FindGroup(GroupId);
	if (Group == nullptr)
	{
		return;
	}

	if (FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format(
				LOCTEXT("DeleteGroupConfirmation", "Delete group '{0}'?\n\nThis will not close editors or delete assets."),
				FText::FromString(Group->Name))) == EAppReturnType::Yes)
	{
		if (Commands->DeleteGroup(GroupId))
		{
			SelectedGroupId.Invalidate();
			MissingAssetReasons.Remove(GroupId);
			Rebuild();
		}
	}
}

bool SAssetTabGroupsPanel::PromptForText(const FText& Title, const FText& Prompt, FString& InOutText) const
{
	bool bAccepted = false;
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(Title)
		.SizingRule(ESizingRule::Autosized)
		.SupportsMinimize(false)
		.SupportsMaximize(false);
	TWeakPtr<SWindow> WeakWindow = Window;

	TSharedRef<SEditableTextBox> TextBox = SNew(SEditableTextBox)
		.Text(FText::FromString(InOutText))
		.SelectAllTextWhenFocused(true)
		.OnTextCommitted_Lambda([&InOutText, &bAccepted, WeakWindow](const FText& Text, ETextCommit::Type CommitType)
		{
			if (CommitType == ETextCommit::OnEnter)
			{
				InOutText = Text.ToString();
				bAccepted = true;
				if (TSharedPtr<SWindow> WindowPin = WeakWindow.Pin())
				{
					WindowPin->RequestDestroyWindow();
				}
			}
		});

	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
	Content->AddSlot()
	.AutoHeight()
	.Padding(10.0f, 10.0f, 10.0f, 4.0f)
	[
		SNew(STextBlock)
		.Text(Prompt)
	];
	Content->AddSlot()
	.AutoHeight()
	.Padding(10.0f, 0.0f, 10.0f, 8.0f)
	[
		TextBox
	];

	TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	Buttons->AddSlot()
	.FillWidth(1.0f)
	[
		SNew(SSpacer)
	];
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(4.0f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Accept", "OK"))
		.OnClicked_Lambda([&InOutText, &bAccepted, TextBox, WeakWindow]()
		{
			InOutText = TextBox->GetText().ToString();
			bAccepted = true;
			if (TSharedPtr<SWindow> WindowPin = WeakWindow.Pin())
			{
				WindowPin->RequestDestroyWindow();
			}
			return FReply::Handled();
		})
	];
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(4.0f, 4.0f, 10.0f, 4.0f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Cancel", "Cancel"))
		.OnClicked_Lambda([WeakWindow]()
		{
			if (TSharedPtr<SWindow> WindowPin = WeakWindow.Pin())
			{
				WindowPin->RequestDestroyWindow();
			}
			return FReply::Handled();
		})
	];
	Content->AddSlot()
	.AutoHeight()
	[
		Buttons
	];

	Window->SetContent(Content);
	FSlateApplication::Get().AddModalWindow(Window, FGlobalTabmanager::Get()->GetRootWindow().ToSharedRef(), false);
	return bAccepted && !InOutText.TrimStartAndEnd().IsEmpty();
}

bool SAssetTabGroupsPanel::PromptForOpenAssetSelection(
	TArray<FAssetTabGroupMember>& OutMembers,
	FString& OutActiveAssetPath)
{
	OutMembers.Reset();
	OutActiveAssetPath.Reset();
	if (Subsystem == nullptr)
	{
		return false;
	}

	FAssetEditorSessionAdapter& SessionAdapter = Subsystem->GetSessionAdapter();
	const TArray<FAssetEditorSessionInfo> OpenAssets = SessionAdapter.GetOpenAssetInfos();
	if (OpenAssets.Num() == 0)
	{
		Commands->Notify(LOCTEXT("NoOpenAssetsSelection", "There are no open assets to select."), false);
		return false;
	}

	const FString ActiveAssetPath = SessionAdapter.GetActiveAssetPath();
	TArray<TSharedPtr<SCheckBox>> CheckBoxes;
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (const FAssetEditorSessionInfo& Info : OpenAssets)
	{
		TSharedPtr<SCheckBox> CheckBox;
		const FString DisplayName = Info.Member.DisplayName.IsEmpty() ? Info.Member.AssetPath : Info.Member.DisplayName;
		const FString AssetPath = Info.Member.AssetPath;
		Rows->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 2.0f)
		[
			SAssignNew(CheckBox, SCheckBox)
			.IsChecked(AssetPath == ActiveAssetPath ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(DisplayName))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Info.Member.PackagePath))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
		CheckBoxes.Add(CheckBox);
	}

	bool bAccepted = false;
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SelectOpenAssetsTitle", "Select Open Assets"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		.ClientSize(FVector2D(620.0f, 480.0f));
	TWeakPtr<SWindow> WeakWindow = Window;

	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
	Content->AddSlot()
	.AutoHeight()
	.Padding(10.0f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("SelectOpenAssetsDescription", "Select the open assets to include. The active asset is selected by default."))
	];
	Content->AddSlot()
	.FillHeight(1.0f)
	.Padding(4.0f)
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			Rows
		]
	];

	TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(8.0f, 4.0f)
	[
		SNew(SButton)
		.Text(LOCTEXT("SelectAll", "Select All"))
		.OnClicked_Lambda([&CheckBoxes]()
		{
			for (const TSharedPtr<SCheckBox>& CheckBox : CheckBoxes)
			{
				if (CheckBox.IsValid())
				{
					CheckBox->SetIsChecked(ECheckBoxState::Checked);
				}
			}
			return FReply::Handled();
		})
	];
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(0.0f, 4.0f)
	[
		SNew(SButton)
		.Text(LOCTEXT("ClearAll", "Clear"))
		.OnClicked_Lambda([&CheckBoxes]()
		{
			for (const TSharedPtr<SCheckBox>& CheckBox : CheckBoxes)
			{
				if (CheckBox.IsValid())
				{
					CheckBox->SetIsChecked(ECheckBoxState::Unchecked);
				}
			}
			return FReply::Handled();
		})
	];
	Buttons->AddSlot()
	.FillWidth(1.0f)
	[
		SNew(SSpacer)
	];
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(4.0f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Accept", "OK"))
		.OnClicked_Lambda([&bAccepted, WeakWindow]()
		{
			bAccepted = true;
			if (TSharedPtr<SWindow> WindowPin = WeakWindow.Pin())
			{
				WindowPin->RequestDestroyWindow();
			}
			return FReply::Handled();
		})
	];
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(4.0f, 4.0f, 8.0f, 4.0f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Cancel", "Cancel"))
		.OnClicked_Lambda([WeakWindow]()
		{
			if (TSharedPtr<SWindow> WindowPin = WeakWindow.Pin())
			{
				WindowPin->RequestDestroyWindow();
			}
			return FReply::Handled();
		})
	];
	Content->AddSlot()
	.AutoHeight()
	[
		Buttons
	];

	Window->SetContent(Content);
	FSlateApplication::Get().AddModalWindow(Window, FGlobalTabmanager::Get()->GetRootWindow().ToSharedRef(), false);

	if (!bAccepted)
	{
		return false;
	}

	for (int32 Index = 0; Index < OpenAssets.Num(); ++Index)
	{
		if (CheckBoxes.IsValidIndex(Index)
			&& CheckBoxes[Index].IsValid()
			&& CheckBoxes[Index]->IsChecked() == true)
		{
			OutMembers.Add(OpenAssets[Index].Member);
		}
	}

	if (OutMembers.Num() == 0)
	{
		Commands->Notify(LOCTEXT("NoSelectedAssets", "Select at least one open asset."), false);
		return false;
	}

	OutActiveAssetPath = ActiveAssetPath;
	return true;
}

#undef ASSETTABGROUPS_BUTTON_TEXT_STYLE
#undef LOCTEXT_NAMESPACE
