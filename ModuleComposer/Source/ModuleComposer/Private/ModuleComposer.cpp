// Copyright (c) 2026 Lucky Machida. All rights reserved.

#include "ModuleComposer.h"
#include "ModuleComposerStyle.h"
#include "ModuleComposerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "EditorActorFolders.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Brushes/SlateColorBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Widgets/SWindow.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "JsonObjectConverter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMeshActor.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Widgets/Input/SCheckBox.h"

#include "Engine/Selection.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonValue.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "HighResScreenshot.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"

static const FName ModuleComposerTabName("ModuleComposer");
#define LOCTEXT_NAMESPACE "FModuleComposerModule"

void FModuleComposerModule::StartupModule()
{
    PreviewMaterial =
        LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/ModuleComposer/M_WireframePreview.M_WireframePreview")
        );


    TransparentMaterial =
        LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/ModuleComposer/M_TransparentPreview.M_TransparentPreview")
        );
    FModuleComposerStyle::Initialize();
    FModuleComposerStyle::ReloadTextures();

    FModuleComposerCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FModuleComposerCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FModuleComposerModule::PluginButtonClicked),
        FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FModuleComposerModule::RegisterMenus));

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ModuleComposerTabName, FOnSpawnTab::CreateRaw(this, &FModuleComposerModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FModuleComposerTabTitle", "ModuleComposer"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FModuleComposerModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FModuleComposerStyle::Shutdown();
    FModuleComposerCommands::Unregister();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ModuleComposerTabName);
}


TSharedRef<SDockTab> FModuleComposerModule::OnSpawnPluginTab(
    const FSpawnTabArgs& SpawnTabArgs)
{
    LoadButtons();

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBox)
                .MinDesiredWidth(1000)
                .MinDesiredHeight(600)
                [
                    SNew(SBorder)
                        .Padding(8)
                        [
                            SNew(SVerticalBox)

                                +SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(2)
                                [
                                    SNew(SHorizontalBox)

                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2)
                                        [
                                            SNew(SButton)
                                                .Text(
                                                    FText::FromString(
                                                        "CREATE PRESET"))
                                                .OnClicked_Raw(
                                                    this,
                                                    &FModuleComposerModule::OnExportJsonButton)
                                        ]

                                    + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2)
                                        [
                                            SNew(SButton)
                                                .Text(
                                                    FText::FromString(
                                                        "Edit"))
                                                .OnClicked_Raw(
                                                    this,
                                                    &FModuleComposerModule::OnUpdateButton)
                                        ]

                                    + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2)
                                        [
                                            SNew(SButton)
                                                .Text(
                                                    FText::FromString(
                                                        "Delete"))
                                                .OnClicked_Raw(
                                                    this,
                                                    &FModuleComposerModule::OnDeleteButton)
                                        ]

                                    + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2)
                                        [
                                            SNew(SButton)
                                                .Text(
                                                    FText::FromString(
                                                        "Up"))
                                                .OnClicked_Raw(
                                                    this,
                                                    &FModuleComposerModule::OnMoveUpButton)
                                        ]

                                    + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2)
                                        [
                                            SNew(SButton)
                                                .Text(
                                                    FText::FromString(
                                                        "Dn"))
                                                .OnClicked_Raw(
                                                    this,
                                                    &FModuleComposerModule::OnMoveDownButton)
                                        ]

                                    + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .Padding(2)
                                        [
                                            SNew(SButton)
                                                .Text(
                                                    FText::FromString(
                                                        "Import"))
                                                .OnClicked_Raw(
                                                    this,
                                                    &FModuleComposerModule::OnImportButtons)
                                        ]
                                ]
                            +SVerticalBox::Slot()
                                .FillHeight(1.0f)
                                [
                                    SNew(SHorizontalBox)
                                        +SHorizontalBox::Slot()
                                        .FillWidth(5.0f)
                                        .Padding(5)
                                        [
                                            SNew(SVerticalBox)

                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                .Padding(2)
                                                [
                                                    SNew(STextBlock)
                                                        .Text(
                                                            FText::FromString(
                                                                "Presets"))
                                                ]

                                                + SVerticalBox::Slot()
                                                .AutoHeight()
                                                .Padding(2)
                                                [
                                                    SAssignNew(
                                                        SearchBox,
                                                        SSearchBox)
                                                        .OnTextChanged_Raw(
                                                            this,
                                                            &FModuleComposerModule::OnSearchChanged)
                                                ]

                                            + SVerticalBox::Slot()
                                                .FillHeight(1.0f)
                                                .Padding(2)
                                                [
                                                    SAssignNew(
                                                        ListView,
                                                        SListView<TSharedPtr<FButtonData>>)

                                                        .ListItemsSource(
                                                            &FilteredButtonItems)

                                                        .OnGenerateRow_Lambda(
                                                            [](TSharedPtr<FButtonData> Item,
                                                                const TSharedRef<STableViewBase>& Owner)
                                                            {
                                                                TSharedPtr<FSlateDynamicImageBrush> ThumbnailBrush;

                                                                if (Item.IsValid() &&
                                                                    !Item->ImagePath.IsEmpty())
                                                                {
                                                                    FString FullImagePath;

                                                                    if (FPaths::FileExists(
                                                                        Item->ImagePath))
                                                                    {
                                                                        FullImagePath =
                                                                            Item->ImagePath;
                                                                    }
                                                                    else
                                                                    {
                                                                        FullImagePath =
                                                                            FPaths::ProjectDir()
                                                                            / TEXT("ModuleComposer")
                                                                            / Item->ImagePath;
                                                                    }

                                                                    if (FPaths::FileExists(
                                                                        FullImagePath))
                                                                    {
                                                                        ThumbnailBrush =
                                                                            MakeShared<FSlateDynamicImageBrush>(
                                                                                FName(*FullImagePath),
                                                                                FVector2D(64, 64));
                                                                    }
                                                                }

                                                                return SNew(
                                                                    STableRow<TSharedPtr<FButtonData>>,
                                                                    Owner)
                                                                    [
                                                                        SNew(SHorizontalBox)

                                                                            + SHorizontalBox::Slot()
                                                                            .AutoWidth()
                                                                            .VAlign(VAlign_Center)
                                                                            .Padding(2)
                                                                            [
                                                                                SNew(SBox)
                                                                                    .WidthOverride(64)
                                                                                    .HeightOverride(64)
                                                                                    [
                                                                                        SNew(SImage)
                                                                                            .Image_Lambda(
                                                                                                [ThumbnailBrush]()
                                                                                                -> const FSlateBrush*
                                                                                                {
                                                                                                    return ThumbnailBrush.Get();
                                                                                                })
                                                                                    ]
                                                                            ]

                                                                        + SHorizontalBox::Slot()
                                                                            .FillWidth(1.0f)
                                                                            .VAlign(VAlign_Center)
                                                                            .Padding(8, 0, 2, 0)
                                                                            [
                                                                                SNew(STextBlock)
                                                                                    .Text(
                                                                                        Item.IsValid()
                                                                                        ? FText::FromString(
                                                                                            Item->Name)
                                                                                        : FText::GetEmpty())
                                                                            ]
                                                                    ];
                                                            })

                                                        .OnSelectionChanged(
                                                            SListView<TSharedPtr<FButtonData>>::FOnSelectionChanged::CreateRaw(
                                                                this,
                                                                &FModuleComposerModule::OnSelectionChanged))
                                                ]
                                        ]

                                    +SHorizontalBox::Slot()
                                        .FillWidth(7.0f)
                                        .Padding(5)
                                        [
                                            SNew(SScrollBox)
                                                .Orientation(Orient_Vertical)
                                                + SScrollBox::Slot()
                                                [
                                                    SNew(SVerticalBox)

                                                        + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SBorder)
                                                                .Padding(5)
                                                                [
                                                                    SNew(SBox)
                                                                        .WidthOverride(256)
                                                                        .HeightOverride(256)
                                                                        [
                                                                            SAssignNew(
                                                                                PreviewImage,
                                                                                SImage)
                                                                        ]
                                                                ]
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SVerticalBox)

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(STextBlock)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Position"))
                                                                ]

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SHorizontalBox)

                                                                        + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                PosXBox,
                                                                                SSpinBox<float>)
                                                                                .Value(0.0f)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnPositionChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                PosYBox,
                                                                                SSpinBox<float>)
                                                                                .Value(0.0f)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnPositionChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                PosZBox,
                                                                                SSpinBox<float>)
                                                                                .Value(0.0f)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnPositionChanged)
                                                                        ]
                                                                ]
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SVerticalBox)

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(STextBlock)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Rotation"))
                                                                ]

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SHorizontalBox)

                                                                        + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                RotPitchBox,
                                                                                SSpinBox<float>)
                                                                                .Value(0.0f)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnPositionChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                RotYawBox,
                                                                                SSpinBox<float>)
                                                                                .Value(0.0f)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnPositionChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                RotRollBox,
                                                                                SSpinBox<float>)
                                                                                .Value(0.0f)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnPositionChanged)
                                                                        ]
                                                                ]
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SVerticalBox)

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(STextBlock)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Array"))
                                                                ]

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                .Padding(0, 5, 0, 5)
                                                                [
                                                                    SNew(SHorizontalBox)

                                                                        + SHorizontalBox::Slot()
                                                                        .AutoWidth()
                                                                        .VAlign(VAlign_Center)
                                                                        .Padding(2)
                                                                        [
                                                                            SNew(STextBlock)
                                                                                .Text(
                                                                                    FText::FromString(
                                                                                        "Count"))
                                                                        ]

                                                                        + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayCountBox,
                                                                                SSpinBox<int32>)
                                                                                .Value(ArrayCount)
                                                                                .MinValue(1)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayCountChanged)
                                                                        ]
                                                                ]

                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                .Padding(0, 5, 0, 0)
                                                                [
                                                                    SNew(STextBlock)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Position Offset"))
                                                                ]

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SHorizontalBox)

                                                                        + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayPosXBox,
                                                                                SSpinBox<float>)
                                                                                .Value(
                                                                                    ArrayPositionOffset.X)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayPositionChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayPosYBox,
                                                                                SSpinBox<float>)
                                                                                .Value(
                                                                                    ArrayPositionOffset.Y)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayPositionChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayPosZBox,
                                                                                SSpinBox<float>)
                                                                                .Value(
                                                                                    ArrayPositionOffset.Z)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayPositionChanged)
                                                                        ]
                                                                ]

                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                .Padding(0, 5, 0, 0)
                                                                [
                                                                    SNew(STextBlock)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Rotation Offset"))
                                                                ]

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SHorizontalBox)

                                                                        + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayRotPitchBox,
                                                                                SSpinBox<float>)
                                                                                .Value(
                                                                                    ArrayRotationOffset.Pitch)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayRotationChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayRotYawBox,
                                                                                SSpinBox<float>)
                                                                                .Value(
                                                                                    ArrayRotationOffset.Yaw)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayRotationChanged)
                                                                        ]

                                                                    + SHorizontalBox::Slot()
                                                                        .FillWidth(1.0f)
                                                                        .Padding(2)
                                                                        [
                                                                            SAssignNew(
                                                                                ArrayRotRollBox,
                                                                                SSpinBox<float>)
                                                                                .Value(
                                                                                    ArrayRotationOffset.Roll)
                                                                                .OnValueChanged_Raw(
                                                                                    this,
                                                                                    &FModuleComposerModule::OnArrayRotationChanged)
                                                                        ]
                                                                ]
                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                .Padding(5)
                                                                [
                                                                    SNew(SButton)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Reset"))
                                                                        .OnClicked_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::OnResetButton)
                                                                ]
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SVerticalBox)

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(STextBlock)
                                                                        .Text(
                                                                            FText::FromString(
                                                                                "Preview"))
                                                                ]

                                                                + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SCheckBox)
                                                                        .Style(
                                                                            FAppStyle::Get(),
                                                                            "RadioButton")
                                                                        .IsChecked_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::IsPreviewOff)
                                                                        .OnCheckStateChanged_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::OnPreviewModeChanged,
                                                                            EPreviewMode::Off)
                                                                        [
                                                                            SNew(STextBlock)
                                                                                .Text(
                                                                                    FText::FromString(
                                                                                        "Off"))
                                                                        ]
                                                                ]

                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SCheckBox)
                                                                        .Style(
                                                                            FAppStyle::Get(),
                                                                            "RadioButton")
                                                                        .IsChecked_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::IsBoundingBoxPreview)
                                                                        .OnCheckStateChanged_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::OnPreviewModeChanged,
                                                                            EPreviewMode::BoundingBox)
                                                                        [
                                                                            SNew(STextBlock)
                                                                                .Text(
                                                                                    FText::FromString(
                                                                                        "Bounding Box"))
                                                                        ]
                                                                ]

                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SCheckBox)
                                                                        .Style(
                                                                            FAppStyle::Get(),
                                                                            "RadioButton")
                                                                        .IsChecked_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::IsWireframePreview)
                                                                        .OnCheckStateChanged_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::OnPreviewModeChanged,
                                                                            EPreviewMode::Wireframe)
                                                                        [
                                                                            SNew(STextBlock)
                                                                                .Text(
                                                                                    FText::FromString(
                                                                                        "Wireframe"))
                                                                        ]
                                                                ]

                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SCheckBox)
                                                                        .Style(
                                                                            FAppStyle::Get(),
                                                                            "RadioButton")
                                                                        .IsChecked_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::IsTransparentPreview)
                                                                        .OnCheckStateChanged_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::OnPreviewModeChanged,
                                                                            EPreviewMode::Transparent)
                                                                        [
                                                                            SNew(STextBlock)
                                                                                .Text(
                                                                                    FText::FromString(
                                                                                        "Transparent"))
                                                                        ]
                                                                ]

                                                            + SVerticalBox::Slot()
                                                                .AutoHeight()
                                                                [
                                                                    SNew(SCheckBox)
                                                                        .Style(
                                                                            FAppStyle::Get(),
                                                                            "RadioButton")
                                                                        .IsChecked_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::IsPreview)
                                                                        .OnCheckStateChanged_Raw(
                                                                            this,
                                                                            &FModuleComposerModule::OnPreviewModeChanged,
                                                                            EPreviewMode::Preview)
                                                                        [
                                                                            SNew(STextBlock)
                                                                                .Text(
                                                                                    FText::FromString(
                                                                                        "Default"))
                                                                        ]
                                                                ]
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SButton)
                                                                .Text(
                                                                    FText::FromString(
                                                                        "Spawn"))
                                                                .OnClicked_Raw(
                                                                    this,
                                                                    &FModuleComposerModule::OnExecutionButton)
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(10, 15, 10, 5)
                                                        [
                                                            SNew(SSeparator)
                                                        ]

                                                        + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(
                                                                    FText::FromString(
                                                                        "Delete Spawn Group"))
                                                        ]

                                                        + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SAssignNew(
                                                                SpawnGroupBox,
                                                                SEditableTextBox)
                                                                .HintText(
                                                                    FText::FromString(
                                                                        "20260726_143142"))
                                                        ]

                                                    + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        .Padding(5)
                                                        [
                                                            SNew(SButton)
                                                                .Text(
                                                                    FText::FromString(
                                                                        "Delete Spawn Group"))
                                                                .OnClicked_Raw(
                                                                    this,
                                                                    &FModuleComposerModule::OnDeleteSpawnGroupButton)
                                                        ]
                                                ]
                                        ]

                                    +SHorizontalBox::Slot()
                                        .FillWidth(1.0f)
                                        .Padding(5)
                                        [
                                            SNew(SVerticalBox)

                                                + SVerticalBox::Slot()
                                                .FillHeight(1.0f)
                                                [
                                                    SNew(SButton)
                                                        .HAlign(HAlign_Center)
                                                        .VAlign(VAlign_Center)
                                                        .OnClicked_Raw(
                                                            this,
                                                            &FModuleComposerModule::OnExportJsonButton)
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(
                                                                    FText::FromString(
                                                                        "\nC\nR\nE\nA\nT\nE\n\nP\nR\nE\nS\nE\nT"))
                                                                .Justification(
                                                                    ETextJustify::Center)
                                                        ]
                                                ]
                                        ]
                                ]
                        ]
                ]
        ];
}

FReply FModuleComposerModule::OnAddButton()
{
    TSharedRef<SWindow> Window =
        SNew(SWindow)
        .Title(FText::FromString("Add Button"))
        .ClientSize(FVector2D(300, 150))
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                [
                    SAssignNew(NameBox, SEditableTextBox)
                        .HintText(FText::FromString("Button Name"))
                ]

                + SVerticalBox::Slot()
                [
                    SNew(SButton)
                        .Text(FText::FromString("Select JSON"))
                        .OnClicked_Raw(this, &FModuleComposerModule::OnSelectJsonButton)
                ]


                + SVerticalBox::Slot()
                [
                    SAssignNew(JsonPathBox, SEditableTextBox)
                        .IsReadOnly(true)
                ]

                + SVerticalBox::Slot()
                [
                    SNew(SButton)
                        .Text(FText::FromString("Select Image"))
                        .OnClicked_Raw(this, &FModuleComposerModule::OnSelectImageButton)
                ]

                + SVerticalBox::Slot()
                [
                    SAssignNew(ImagePathBox, SEditableTextBox)
                        .IsReadOnly(true)
                ]

                + SVerticalBox::Slot()
                [
                    SNew(SButton)
                        .Text(FText::FromString("OK"))
                        .OnClicked_Raw(this, &FModuleComposerModule::OnAddConfirm)
                ]
        ];

    FSlateApplication::Get().AddWindow(Window);

    return FReply::Handled();
}

FReply FModuleComposerModule::OnAddConfirm()
{
    FString ImagePath =
        ImagePathBox->GetText().ToString();

    FString Name =
        NameBox->GetText().ToString();

    FString JsonPath =
        JsonPathBox->GetText().ToString();

    if (Name.IsEmpty())
    {
        return FReply::Handled();
    }

    if (JsonPath.IsEmpty())
    {
        return FReply::Handled();
    }

    FString DestPath;

    if (!ImagePath.IsEmpty())
    {
        FString ImageFolder =
            FPaths::ProjectDir() /
            TEXT("ModuleComposer");

        IFileManager::Get().MakeDirectory(
            *ImageFolder,
            true);

        FString FileName =
            FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S_"))
            + FPaths::GetCleanFilename(ImagePath);

        DestPath =
            ImageFolder / FileName;

        IFileManager::Get().Copy(
            *DestPath,
            *ImagePath);
    }

    TSharedPtr<FButtonData> Data =
        MakeShared<FButtonData>();


    Data->Name = Name;
    FString JsonFolder =
        FPaths::ProjectDir() /
        TEXT("ModuleComposer");

    IFileManager::Get().MakeDirectory(
        *JsonFolder,
        true);

    FString JsonDest =
        JsonFolder /
        FPaths::GetCleanFilename(JsonPath);

    IFileManager::Get().Copy(
        *JsonDest,
        *JsonPath);

    Data->JsonPath =
        FPaths::GetCleanFilename(JsonDest);
    Data->ImagePath = ImagePath.IsEmpty() ? TEXT("") : DestPath;

    ButtonItems.Add(Data);

    FilteredButtonItems = ButtonItems;

    ListView->RequestListRefresh();

    SaveButtons();

    ShowNotification(
        TEXT("Button added."),
        SNotificationItem::CS_Success
    );

    return FReply::Handled();
}

void FModuleComposerModule::SaveButtons()
{
    TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> JsonArray;

    for (auto& Item : ButtonItems)
    {
        if (!Item.IsValid())
        {
            continue;
        }
        TSharedRef<FJsonObject> Object =
            MakeShared<FJsonObject>();

        Object->SetStringField(
            TEXT("Name"),
            Item->Name
        );

        Object->SetStringField(
            TEXT("JsonPath"),
            FPaths::GetCleanFilename(Item->JsonPath)
        );

        Object->SetStringField(
            TEXT("ImagePath"),
            FPaths::GetCleanFilename(Item->ImagePath)
        );

        JsonArray.Add(
            MakeShared<FJsonValueObject>(Object)
        );
    }

    JsonObject->SetArrayField("Buttons", JsonArray);
    FString ButtonSaveDir =
        FPaths::ProjectDir() / TEXT("ModuleComposer");

    IFileManager::Get().MakeDirectory(
        *ButtonSaveDir,
        true);

    FString OutputString;

    TSharedRef<TJsonWriter<>> Writer =
        TJsonWriterFactory<>::Create(&OutputString);

    FJsonSerializer::Serialize(JsonObject, Writer);

    FFileHelper::SaveStringToFile(
        OutputString,
        *(FPaths::ProjectDir() / TEXT("ModuleComposer/Buttons.json"))
    );
}

void FModuleComposerModule::LoadButtons()
{
    ButtonItems.Empty();

    FString InputString;

    if (!FFileHelper::LoadFileToString(
        InputString,
        *(FPaths::ProjectDir() / TEXT("ModuleComposer/Buttons.json"))
    ))
    {
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;

    TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(InputString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* JsonArray;

    if (JsonObject->TryGetArrayField(TEXT("Buttons"), JsonArray))
    {
        for (const auto& Value : *JsonArray)
        {
            TSharedPtr<FJsonObject> Object =
                Value->AsObject();


            if (!Object.IsValid())
            {
                continue;
            }

            TSharedPtr<FButtonData> Data =
                MakeShared<FButtonData>();

            if (!Object->TryGetStringField(
                TEXT("Name"),
                Data->Name))
            {
                continue;
            }


            if (!Object->TryGetStringField(
                TEXT("JsonPath"),
                Data->JsonPath))
            {
                continue;
            }

            if (!Data->JsonPath.StartsWith(TEXT("Preset/")))
            {
                Data->JsonPath =
                    TEXT("Preset/") +
                    Data->JsonPath;
            }

            if (Data->JsonPath.IsEmpty())
            {
                continue;
            }

            Object->TryGetStringField(
                TEXT("ImagePath"),
                Data->ImagePath);

            if (!Data->ImagePath.IsEmpty())
            {
                if (!Data->ImagePath.IsEmpty())
                {
                    if (!Data->ImagePath.StartsWith(TEXT("Preset/")) &&
                        !FPaths::IsRelative(Data->ImagePath))
                    {
                        Data->ImagePath =
                            FPaths::ConvertRelativePathToFull(
                                Data->ImagePath);
                    }
                    else if (!Data->ImagePath.StartsWith(TEXT("Preset/")))
                    {
                        Data->ImagePath =
                            TEXT("Preset/") +
                            Data->ImagePath;
                    }
                }
            }
            ButtonItems.Add(Data);
        }
    }
    FilteredButtonItems = ButtonItems;
}

void FModuleComposerModule::OnSelectionChanged(
    TSharedPtr<FButtonData> Item,
    ESelectInfo::Type SelectInfo)
{
    SelectedItem = Item;


    if (!Item.IsValid())
    {
        return;
    }

    if (Item->ImagePath.IsEmpty())
    {
        PreviewBrush.Reset();

        if (PreviewImage.IsValid())
        {
            PreviewImage->SetImage(nullptr);
        }

        return;
    }
    FString FullImagePath;


    if (FPaths::FileExists(Item->ImagePath))
    {
        FullImagePath = Item->ImagePath;
    }
    else
    {
        FullImagePath =
            FPaths::ProjectDir()
            / TEXT("ModuleComposer")
            / Item->ImagePath;
    }


    PreviewBrush =
        MakeShared<FSlateDynamicImageBrush>(
            FName(*FullImagePath),
            FVector2D(256, 256)
        );

    DestroyPreviewActors();
    if (PreviewMode != EPreviewMode::Off)
    {
        if (LoadPreviewScene())
        {
            if (PreviewMode == EPreviewMode::BoundingBox)
            {
                SpawnBoundingBoxPreview();
            }
            else
            {
                SpawnPreviewActors();
            }
        }
    }

    if (PreviewImage.IsValid())
    {
        PreviewImage->SetImage(
            PreviewBrush.Get()
        );
    }
}

FReply FModuleComposerModule::OnExecutionButton()
{
    DestroyPreviewActors();

    if (!SelectedItem.IsValid())
    {
        return FReply::Handled();
    }

    FString JsonString;

    if (!FFileHelper::LoadFileToString(
        JsonString,
        *(FPaths::ProjectDir() / TEXT("ModuleComposer") / SelectedItem->JsonPath)))
    {
        return FReply::Handled();
    }

    FSceneData Scene;

    if (!FJsonObjectConverter::JsonObjectStringToUStruct(
        JsonString,
        &Scene,
        0,
        0))
    {
        return FReply::Handled();
    }

    FString SpawnTime =
        FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));

    if (SpawnGroupBox.IsValid())
    {
        SpawnGroupBox->SetText(
            FText::FromString(SpawnTime));
    }

    FVector Offset = FVector::ZeroVector;

    if (PosXBox.IsValid())
    {
        Offset.X = PosXBox->GetValue();
    }

    if (PosYBox.IsValid())
    {
        Offset.Y = PosYBox->GetValue();
    }

    if (PosZBox.IsValid())
    {
        Offset.Z = PosZBox->GetValue();
    }

    PreviewOffset = Offset;

    FRotator GlobalRotation =
        FRotator::ZeroRotator;

    if (RotPitchBox.IsValid())
    {
        GlobalRotation.Pitch =
            RotPitchBox->GetValue();
    }

    if (RotYawBox.IsValid())
    {
        GlobalRotation.Yaw =
            RotYawBox->GetValue();
    }

    if (RotRollBox.IsValid())
    {
        GlobalRotation.Roll =
            RotRollBox->GetValue();
    }

    PreviewRotation = GlobalRotation;

    int32 ExecutionArrayCount =
        FMath::Max(1, ArrayCount);

    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
            "AssetRegistry");

    FARFilter Filter;

    Filter.ClassNames.Add(
        UStaticMesh::StaticClass()->GetFName());

    Filter.PackagePaths.Add(
        FName(TEXT("/Game")));

    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;

    AssetRegistryModule.Get().GetAssets(
        Filter,
        Assets);

    UWorld* World =
        GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FReply::Handled();
    }

    for (const FMeshData& Mesh : Scene.Meshes)
    {
        UStaticMesh* FoundMesh = nullptr;

        for (const FAssetData& Asset : Assets)
        {
            if (Asset.AssetName.ToString() == Mesh.Name)
            {
                FoundMesh =
                    Cast<UStaticMesh>(
                        Asset.GetAsset());

                break;
            }
        }

        if (!FoundMesh)
        {
            continue;
        }

        for (int32 ArrayIndex = 0;
            ArrayIndex < ExecutionArrayCount;
            ArrayIndex++)
        {
            FVector FinalLocation;
            FRotator FinalRotation;
            FVector FinalScale;

            CalculateArrayTransform(
                Mesh,
                ArrayIndex,
                FinalLocation,
                FinalRotation,
                FinalScale);

            FActorSpawnParameters Params;

            AStaticMeshActor* Actor =
                World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(),
                    FinalLocation,
                    FinalRotation,
                    Params);

            if (!Actor)
            {
                continue;
            }

            Actor->GetStaticMeshComponent()
                ->SetStaticMesh(
                    FoundMesh);

            Actor->SetActorScale3D(
                FinalScale);

            Actor->SetActorLabel(
                Mesh.Name +
                TEXT("_") +
                SpawnTime +
                TEXT("_") +
                FString::FromInt(ArrayIndex));

            Actor->SetFolderPath(
                FName(*SpawnTime));
        }
    }

    ShowNotification(
        TEXT("Spawn completed."),
        SNotificationItem::CS_Success);

    return FReply::Handled();
}

FReply FModuleComposerModule::OnResetButton()
{
    DestroyPreviewActors();

    PreviewOffset = FVector::ZeroVector;
    PreviewRotation = FRotator::ZeroRotator;

    ArrayCount = 1;
    ArrayPositionOffset = FVector::ZeroVector;
    ArrayRotationOffset = FRotator::ZeroRotator;
    ArrayScaleOffset = FVector::ZeroVector;

    if (PosXBox.IsValid())
    {
        PosXBox->SetValue(0.0f);
    }

    if (PosYBox.IsValid())
    {
        PosYBox->SetValue(0.0f);
    }

    if (PosZBox.IsValid())
    {
        PosZBox->SetValue(0.0f);
    }

    if (RotPitchBox.IsValid())
    {
        RotPitchBox->SetValue(0.0f);
    }

    if (RotYawBox.IsValid())
    {
        RotYawBox->SetValue(0.0f);
    }

    if (RotRollBox.IsValid())
    {
        RotRollBox->SetValue(0.0f);
    }

    if (ArrayCountBox.IsValid())
    {
        ArrayCountBox->SetValue(1);
    }

    if (ArrayPosXBox.IsValid())
    {
        ArrayPosXBox->SetValue(0.0f);
    }

    if (ArrayPosYBox.IsValid())
    {
        ArrayPosYBox->SetValue(0.0f);
    }

    if (ArrayPosZBox.IsValid())
    {
        ArrayPosZBox->SetValue(0.0f);
    }

    if (ArrayRotPitchBox.IsValid())
    {
        ArrayRotPitchBox->SetValue(0.0f);
    }

    if (ArrayRotYawBox.IsValid())
    {
        ArrayRotYawBox->SetValue(0.0f);
    }

    if (ArrayRotRollBox.IsValid())
    {
        ArrayRotRollBox->SetValue(0.0f);
    }

    if (ArrayScaleXBox.IsValid())
    {
        ArrayScaleXBox->SetValue(0.0f);
    }

    if (ArrayScaleYBox.IsValid())
    {
        ArrayScaleYBox->SetValue(0.0f);
    }

    if (ArrayScaleZBox.IsValid())
    {
        ArrayScaleZBox->SetValue(0.0f);
    }

    PreviewMode = EPreviewMode::Off;

    ShowNotification(
        TEXT("Reset completed."),
        SNotificationItem::CS_Success
    );

    return FReply::Handled();
}

FReply FModuleComposerModule::OnExportJsonButton()
{
    ExportSelectedActorsToJson();

    CaptureScreenshot();

    return FReply::Handled();
}

void FModuleComposerModule::ExportSelectedActorsToJson()
{
    USelection* SelectedActors = GEditor->GetSelectedActors();
    UE_LOG(LogTemp, Warning, TEXT("Selected Count = %d"), SelectedActors->Num());

    bool bBaseLocationSet = false;
    FVector BaseLocation;

    TArray<TSharedPtr<FJsonValue>> JsonArray;

    for (FSelectionIterator It(*SelectedActors); It; ++It)
    {
        AActor* Actor = Cast<AActor>(*It);
        if (!Actor)
        {
            continue;
        }
        AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actor);
        if (!SMActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("Not StaticMeshActor"));
        }
        if (!SMActor)
        {
            continue;
        }
        UStaticMeshComponent* MeshComp =
            SMActor->GetStaticMeshComponent();
        UStaticMesh* Mesh = MeshComp->GetStaticMesh();
        if (!Mesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("Mesh is null"));
            continue;
        }

        FString Name = Mesh->GetName();

        if (!bBaseLocationSet)
        {
            BaseLocation = Actor->GetActorLocation();
            bBaseLocationSet = true;
        }

        FVector RelativeLocation =
            Actor->GetActorLocation() - BaseLocation;

        FRotator Rotation =
            Actor->GetActorRotation();

        FVector Scale =
            Actor->GetActorScale3D();

        TSharedPtr<FJsonObject> MeshObject =
            MakeShared<FJsonObject>();

        MeshObject->SetStringField(
            TEXT("Name"),
            Name);

        TSharedPtr<FJsonObject> LocationObject =
            MakeShared<FJsonObject>();

        LocationObject->SetNumberField(TEXT("X"), RelativeLocation.X);
        LocationObject->SetNumberField(TEXT("Y"), RelativeLocation.Y);
        LocationObject->SetNumberField(TEXT("Z"), RelativeLocation.Z);

        MeshObject->SetObjectField(
            TEXT("Location"),
            LocationObject);

        TSharedPtr<FJsonObject> RotationObject =
            MakeShared<FJsonObject>();

        RotationObject->SetNumberField(TEXT("Pitch"), Rotation.Pitch);
        RotationObject->SetNumberField(TEXT("Yaw"), Rotation.Yaw);
        RotationObject->SetNumberField(TEXT("Roll"), Rotation.Roll);

        MeshObject->SetObjectField(
            TEXT("Rotation"),
            RotationObject);

        TSharedPtr<FJsonObject> ScaleObject =
            MakeShared<FJsonObject>();

        ScaleObject->SetNumberField(TEXT("X"), Scale.X);
        ScaleObject->SetNumberField(TEXT("Y"), Scale.Y);
        ScaleObject->SetNumberField(TEXT("Z"), Scale.Z);

        MeshObject->SetObjectField(
            TEXT("Scale"),
            ScaleObject);

        JsonArray.Add(
            MakeShared<FJsonValueObject>(MeshObject));
        UE_LOG(LogTemp, Warning, TEXT("Added : %s"), *Name);

    }

    TSharedPtr<FJsonObject> RootObject =
        MakeShared<FJsonObject>();

    RootObject->SetArrayField(
        TEXT("Meshes"),
        JsonArray);

    FString OutputString;

    TSharedRef<TJsonWriter<>> Writer =
        TJsonWriterFactory<>::Create(&OutputString);

    FJsonSerializer::Serialize(
        RootObject.ToSharedRef(),
        Writer);

    FString PresetDir =
        FPaths::ProjectDir() / TEXT("ModuleComposer/Preset");

    IFileManager::Get().MakeDirectory(
        *PresetDir,
        true);
    FString FileName =
        FString::Printf(
            TEXT("Meshes_%s.json"),
            *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    FString FilePath =
        PresetDir / FileName;

    ExportJsonPath = FilePath;
    UE_LOG(LogTemp, Warning, TEXT("Save Path = %s"), *FilePath);

    bool bSaved =
        FFileHelper::SaveStringToFile(
            OutputString,
            *FilePath);

    UE_LOG(LogTemp, Warning, TEXT("Save Result = %d"), bSaved);

}

void FModuleComposerModule::CaptureScreenshot()
{
    FString ImageDir =
        FPaths::ProjectDir()
        / TEXT("ModuleComposer/Preset");

    IFileManager::Get().MakeDirectory(
        *ImageDir,
        true);


    FString ImageName =
        FString::Printf(
            TEXT("Meshes_%s.png"),
            *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));


    ExportImagePath =
        ImageDir / ImageName;


    FScreenshotRequest::RequestScreenshot(
        ExportImagePath,
        false,
        false);


    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(
            this,
            &FModuleComposerModule::CheckScreenshotFinished),
        0.1f);
}

bool FModuleComposerModule::CheckScreenshotFinished(float DeltaTime)
{
    if (!FPaths::FileExists(ExportImagePath))
    {
        return true;
    }


    OpenExportRegisterWindow();


    return false;
}

void FModuleComposerModule::OpenExportRegisterWindow()
{
    ExportWindow =
        SNew(SWindow)
        .Title(FText::FromString("Register Preset"))
        .ClientSize(FVector2D(400, 500))
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(
                        ExportNameBox,
                        SEditableTextBox)
                        .HintText(
                            FText::FromString("Name"))
                ]


            + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(
                        ExportPreviewImage,
                        SImage)
                ]


                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SButton)
                        .Text(
                            FText::FromString("Select Image"))
                        .OnClicked_Raw(
                            this,
                            &FModuleComposerModule::OnSelectImageButton)
                ]


            + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(
                        ExportJsonPathBox,
                        SEditableTextBox)
                        .Text(
                            FText::FromString(
                                ExportJsonPath))
                        .IsReadOnly(true)
                ]


            + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(
                        ExportImagePathBox,
                        SEditableTextBox)
                        .Text(
                            FText::FromString(
                                ExportImagePath))
                        .IsReadOnly(true)
                ]


            + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SButton)
                        .Text(
                            FText::FromString("Register"))
                        .OnClicked_Raw(
                            this,
                            &FModuleComposerModule::OnExportRegisterConfirm)
                ]
        ];


    FSlateApplication::Get()
        .AddWindow(
            ExportWindow.ToSharedRef());

    if (FPaths::FileExists(ExportImagePath))
    {
        ExportPreviewBrush =
            MakeShared<FSlateDynamicImageBrush>(
                FName(*ExportImagePath),
                FVector2D(256, 256));


        if (ExportPreviewImage.IsValid())
        {
            ExportPreviewImage->SetImage(
                ExportPreviewBrush.Get());
        }
    }

}

FReply FModuleComposerModule::OnExportRegisterConfirm()
{
    FString Name =
        ExportNameBox->GetText().ToString();


    if (Name.IsEmpty())
    {
        return FReply::Handled();
    }


    TSharedPtr<FButtonData> Data =
        MakeShared<FButtonData>();


    Data->Name = Name;
    Data->JsonPath =
        TEXT("Preset/") +
        FPaths::GetCleanFilename(ExportJsonPath);

    Data->ImagePath =
        TEXT("Preset/") +
        FPaths::GetCleanFilename(ExportImagePath);


    ButtonItems.Add(Data);

    FilteredButtonItems = ButtonItems;


    ListView->RequestListRefresh();


    SaveButtons();


    if (ExportWindow.IsValid())
    {
        ExportWindow->RequestDestroyWindow();
    }


    ShowNotification(
        TEXT("Preset registered."),
        SNotificationItem::CS_Success);


    return FReply::Handled();
}


void FModuleComposerModule::CalculateArrayTransform(
    const FMeshData& Mesh,
    int32 ArrayIndex,
    FVector& OutLocation,
    FRotator& OutRotation,
    FVector& OutScale)
{
    FVector RotatedLocation =
        PreviewRotation.RotateVector(
            Mesh.Location);

    OutLocation =
        RotatedLocation +
        PreviewOffset +
        ArrayPositionOffset * static_cast<float>(ArrayIndex);

    FRotator ArrayRotation =
        ArrayRotationOffset *
        static_cast<float>(ArrayIndex);

    FQuat GlobalQuat =
        PreviewRotation.Quaternion();

    FQuat ArrayQuat =
        ArrayRotation.Quaternion();

    FQuat MeshQuat =
        Mesh.Rotation.Quaternion();

    FQuat FinalQuat =
        GlobalQuat *
        ArrayQuat *
        MeshQuat;

    OutRotation =
        FinalQuat.Rotator();

    OutScale =
        Mesh.Scale +
        ArrayScaleOffset *
        static_cast<float>(ArrayIndex);
}

void FModuleComposerModule::SpawnPreviewActors()
{
    UWorld* World =
        GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return;
    }

    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
            "AssetRegistry");

    FARFilter Filter;

    Filter.ClassNames.Add(
        UStaticMesh::StaticClass()->GetFName());

    Filter.PackagePaths.Add(
        FName(TEXT("/Game")));

    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;

    AssetRegistryModule.Get().GetAssets(
        Filter,
        Assets);

    int32 PreviewArrayCount =
        FMath::Max(1, ArrayCount);

    for (const FMeshData& Mesh : PreviewScene.Meshes)
    {
        UStaticMesh* FoundMesh = nullptr;

        for (const FAssetData& Asset : Assets)
        {
            if (Asset.AssetName.ToString() == Mesh.Name)
            {
                FoundMesh =
                    Cast<UStaticMesh>(
                        Asset.GetAsset());

                break;
            }
        }

        if (!FoundMesh)
        {
            continue;
        }

        for (int32 ArrayIndex = 0;
            ArrayIndex < PreviewArrayCount;
            ArrayIndex++)
        {
            FVector FinalLocation;
            FRotator FinalRotation;
            FVector FinalScale;

            CalculateArrayTransform(
                Mesh,
                ArrayIndex,
                FinalLocation,
                FinalRotation,
                FinalScale);

            AStaticMeshActor* Actor =
                World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(),
                    FinalLocation,
                    FinalRotation);

            if (!Actor)
            {
                continue;
            }

            UStaticMeshComponent* MeshComponent =
                Actor->GetStaticMeshComponent();

            MeshComponent->SetStaticMesh(
                FoundMesh);

            switch (PreviewMode)
            {
            case EPreviewMode::Wireframe:
            {
                if (PreviewMaterial)
                {
                    const int32 MaterialCount =
                        MeshComponent->GetNumMaterials();

                    for (int32 Index = 0;
                        Index < MaterialCount;
                        Index++)
                    {
                        MeshComponent->SetMaterial(
                            Index,
                            PreviewMaterial);
                    }
                }

                break;
            }

            case EPreviewMode::Transparent:
            {
                if (TransparentMaterial)
                {
                    const int32 MaterialCount =
                        MeshComponent->GetNumMaterials();

                    for (int32 Index = 0;
                        Index < MaterialCount;
                        Index++)
                    {
                        MeshComponent->SetMaterial(
                            Index,
                            TransparentMaterial);
                    }
                }

                break;
            }

            case EPreviewMode::Preview:
            {
                break;
            }

            default:
                break;
            }

            Actor->SetActorScale3D(
                FinalScale);

            PreviewActors.Add(
                TWeakObjectPtr<AStaticMeshActor>(
                    Actor));
        }
    }
}

bool FModuleComposerModule::LoadPreviewScene()
{
    if (!SelectedItem.IsValid())
    {
        return false;
    }

    FString JsonString;

    if (!FFileHelper::LoadFileToString(
        JsonString,
        *(FPaths::ProjectDir() / TEXT("ModuleComposer") / SelectedItem->JsonPath)))
    {
        return false;
    }


    PreviewScene.Meshes.Empty();


    return FJsonObjectConverter::JsonObjectStringToUStruct(
        JsonString,
        &PreviewScene,
        0,
        0);
}

void FModuleComposerModule::OnPositionChanged(float Value)
{
    if (PreviewMode == EPreviewMode::Off)
    {
        return;
    }

    if (!SelectedItem.IsValid())
    {
        return;
    }

    PreviewOffset.X =
        PosXBox.IsValid()
        ? PosXBox->GetValue()
        : 0;

    PreviewOffset.Y =
        PosYBox.IsValid()
        ? PosYBox->GetValue()
        : 0;

    PreviewOffset.Z =
        PosZBox.IsValid()
        ? PosZBox->GetValue()
        : 0;

    PreviewRotation = FRotator::ZeroRotator;

    if (RotPitchBox.IsValid())
    {
        PreviewRotation.Pitch =
            RotPitchBox->GetValue();
    }

    if (RotYawBox.IsValid())
    {
        PreviewRotation.Yaw =
            RotYawBox->GetValue();
    }

    if (RotRollBox.IsValid())
    {
        PreviewRotation.Roll =
            RotRollBox->GetValue();
    }


    DestroyPreviewActors();

    if (PreviewMode == EPreviewMode::BoundingBox)
    {
        SpawnBoundingBoxPreview();
    }
    else
    {
        SpawnPreviewActors();
    }
}

void FModuleComposerModule::OnArrayCountChanged(int32 Value)
{
    ArrayCount = FMath::Max(1, Value);

    if (PreviewMode == EPreviewMode::Off)
    {
        return;
    }

    if (!SelectedItem.IsValid())
    {
        return;
    }

    DestroyPreviewActors();

    if (PreviewMode == EPreviewMode::BoundingBox)
    {
        SpawnBoundingBoxPreview();
    }
    else
    {
        SpawnPreviewActors();
    }
}


void FModuleComposerModule::OnArrayPositionChanged(float Value)
{
    if (ArrayPosXBox.IsValid())
    {
        ArrayPositionOffset.X =
            ArrayPosXBox->GetValue();
    }

    if (ArrayPosYBox.IsValid())
    {
        ArrayPositionOffset.Y =
            ArrayPosYBox->GetValue();
    }

    if (ArrayPosZBox.IsValid())
    {
        ArrayPositionOffset.Z =
            ArrayPosZBox->GetValue();
    }

    if (PreviewMode == EPreviewMode::Off)
    {
        return;
    }

    if (!SelectedItem.IsValid())
    {
        return;
    }

    DestroyPreviewActors();

    if (PreviewMode == EPreviewMode::BoundingBox)
    {
        SpawnBoundingBoxPreview();
    }
    else
    {
        SpawnPreviewActors();
    }
}

void FModuleComposerModule::OnArrayRotationChanged(float Value)
{
    if (ArrayRotPitchBox.IsValid())
    {
        ArrayRotationOffset.Pitch =
            ArrayRotPitchBox->GetValue();
    }

    if (ArrayRotYawBox.IsValid())
    {
        ArrayRotationOffset.Yaw =
            ArrayRotYawBox->GetValue();
    }

    if (ArrayRotRollBox.IsValid())
    {
        ArrayRotationOffset.Roll =
            ArrayRotRollBox->GetValue();
    }

    if (PreviewMode == EPreviewMode::Off)
    {
        return;
    }

    if (!SelectedItem.IsValid())
    {
        return;
    }

    DestroyPreviewActors();

    if (PreviewMode == EPreviewMode::BoundingBox)
    {
        SpawnBoundingBoxPreview();
    }
    else
    {
        SpawnPreviewActors();
    }
}

void FModuleComposerModule::OnArrayScaleChanged(float Value)
{
    if (ArrayScaleXBox.IsValid())
    {
        ArrayScaleOffset.X =
            ArrayScaleXBox->GetValue();
    }

    if (ArrayScaleYBox.IsValid())
    {
        ArrayScaleOffset.Y =
            ArrayScaleYBox->GetValue();
    }

    if (ArrayScaleZBox.IsValid())
    {
        ArrayScaleOffset.Z =
            ArrayScaleZBox->GetValue();
    }

    if (PreviewMode == EPreviewMode::Off)
    {
        return;
    }

    if (!SelectedItem.IsValid())
    {
        return;
    }

    DestroyPreviewActors();

    if (PreviewMode == EPreviewMode::BoundingBox)
    {
        SpawnBoundingBoxPreview();
    }
    else
    {
        SpawnPreviewActors();
    }
}

void FModuleComposerModule::SpawnBoundingBoxPreview()
{
    UWorld* World =
        GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return;
    }

    FVector Min(
        TNumericLimits<float>::Max());

    FVector Max(
        TNumericLimits<float>::Lowest());

    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
            "AssetRegistry");

    FARFilter Filter;

    Filter.ClassNames.Add(
        UStaticMesh::StaticClass()->GetFName());

    Filter.PackagePaths.Add(
        FName(TEXT("/Game")));

    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;

    AssetRegistryModule.Get().GetAssets(
        Filter,
        Assets);

    int32 PreviewArrayCount =
        FMath::Max(1, ArrayCount);

    for (const FMeshData& Mesh : PreviewScene.Meshes)
    {
        UStaticMesh* FoundMesh = nullptr;

        for (const FAssetData& Asset : Assets)
        {
            if (Asset.AssetName.ToString() == Mesh.Name)
            {
                FoundMesh =
                    Cast<UStaticMesh>(
                        Asset.GetAsset());

                break;
            }
        }

        if (!FoundMesh)
        {
            continue;
        }

        FBoxSphereBounds Bounds =
            FoundMesh->GetBounds();

        FVector Extent =
            Bounds.BoxExtent;

        for (int32 ArrayIndex = 0;
            ArrayIndex < PreviewArrayCount;
            ArrayIndex++)
        {
            FVector FinalLocation;
            FRotator FinalRotation;
            FVector FinalScale;

            CalculateArrayTransform(
                Mesh,
                ArrayIndex,
                FinalLocation,
                FinalRotation,
                FinalScale);

            FVector ScaledExtent =
                Extent *
                FinalScale.GetAbs();

            FMatrix RotationMatrix =
                FRotationMatrix(
                    FinalRotation);

            FVector WorldExtent;

            WorldExtent.X =
                FMath::Abs(RotationMatrix.M[0][0]) *
                ScaledExtent.X
                +
                FMath::Abs(RotationMatrix.M[0][1]) *
                ScaledExtent.Y
                +
                FMath::Abs(RotationMatrix.M[0][2]) *
                ScaledExtent.Z;

            WorldExtent.Y =
                FMath::Abs(RotationMatrix.M[1][0]) *
                ScaledExtent.X
                +
                FMath::Abs(RotationMatrix.M[1][1]) *
                ScaledExtent.Y
                +
                FMath::Abs(RotationMatrix.M[1][2]) *
                ScaledExtent.Z;

            WorldExtent.Z =
                FMath::Abs(RotationMatrix.M[2][0]) *
                ScaledExtent.X
                +
                FMath::Abs(RotationMatrix.M[2][1]) *
                ScaledExtent.Y
                +
                FMath::Abs(RotationMatrix.M[2][2]) *
                ScaledExtent.Z;

            Min.X =
                FMath::Min(
                    Min.X,
                    FinalLocation.X - WorldExtent.X);

            Min.Y =
                FMath::Min(
                    Min.Y,
                    FinalLocation.Y - WorldExtent.Y);

            Min.Z =
                FMath::Min(
                    Min.Z,
                    FinalLocation.Z - WorldExtent.Z);

            Max.X =
                FMath::Max(
                    Max.X,
                    FinalLocation.X + WorldExtent.X);

            Max.Y =
                FMath::Max(
                    Max.Y,
                    FinalLocation.Y + WorldExtent.Y);

            Max.Z =
                FMath::Max(
                    Max.Z,
                    FinalLocation.Z + WorldExtent.Z);
        }
    }

    if (Min.X == TNumericLimits<float>::Max())
    {
        return;
    }

    FVector BoxSize =
        Max - Min;

    FVector BoxCenter =
        (Min + Max) * 0.5f;

    UStaticMesh* CubeMesh =
        LoadObject<UStaticMesh>(
            nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (!CubeMesh)
    {
        return;
    }

    AStaticMeshActor* Actor =
        World->SpawnActor<AStaticMeshActor>(
            BoxCenter,
            FRotator::ZeroRotator);

    if (!Actor)
    {
        return;
    }

    UStaticMeshComponent* MeshComponent =
        Actor->GetStaticMeshComponent();

    MeshComponent->SetStaticMesh(
        CubeMesh);

    if (TransparentMaterial)
    {
        MeshComponent->SetMaterial(
            0,
            TransparentMaterial);
    }

    Actor->SetActorScale3D(
        BoxSize / 100.f);

    PreviewActors.Add(
        TWeakObjectPtr<AStaticMeshActor>(
            Actor));
}

FReply FModuleComposerModule::OnImportButtons()
{
    TArray<FString> Files;

    int32 AddedCount = 0;

    IDesktopPlatform* DesktopPlatform =
        FDesktopPlatformModule::Get();

    if (DesktopPlatform)
    {
        DesktopPlatform->OpenFileDialog(
            nullptr,
            TEXT("Select Buttons.json"),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("JSON (*.json)|*.json"),
            EFileDialogFlags::None,
            Files
        );
    }


    if (Files.Num() == 0)
    {
        return FReply::Handled();
    }

    FString SourceFolder =
        FPaths::GetPath(Files[0]);

    FString DestinationFolder =
        FPaths::ProjectDir()
        / TEXT("ModuleComposer/Preset");


    IFileManager::Get().MakeDirectory(
        *DestinationFolder,
        true);

    FString JsonString;

    if (!FFileHelper::LoadFileToString(
        JsonString,
        *Files[0]))
    {
        return FReply::Handled();
    }

    TSharedPtr<FJsonObject> JsonObject;

    TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(
        Reader,
        JsonObject))
    {
        return FReply::Handled();
    }

    const TArray<TSharedPtr<FJsonValue>>* JsonArray;

    if (!JsonObject->TryGetArrayField(
        TEXT("Buttons"),
        JsonArray))
    {
        return FReply::Handled();
    }
    for (const auto& Value : *JsonArray)
    {
        TSharedPtr<FJsonObject> Object =
            Value->AsObject();

        if (!Object.IsValid())
        {
            continue;
        }

        TSharedPtr<FButtonData> Data =
            MakeShared<FButtonData>();

        if (!Object->TryGetStringField(
            TEXT("Name"),
            Data->Name))
        {
            continue;
        }
        if (!Object->TryGetStringField(
            TEXT("JsonPath"),
            Data->JsonPath))
        {
            continue;
        }

        Object->TryGetStringField(
            TEXT("ImagePath"),
            Data->ImagePath);

        FString SourceJsonPath =
            SourceFolder /
            TEXT("Preset") /
            FPaths::GetCleanFilename(Data->JsonPath);


        FString DestinationJsonPath =
            DestinationFolder /
            FPaths::GetCleanFilename(Data->JsonPath);


        IFileManager::Get().Copy(
            *DestinationJsonPath,
            *SourceJsonPath);


        Data->JsonPath =
            TEXT("Preset/") +
            FPaths::GetCleanFilename(Data->JsonPath);

        if (!Data->ImagePath.IsEmpty())
        {
            FString SourceImagePath =
                SourceFolder /
                TEXT("Preset") /
                FPaths::GetCleanFilename(Data->ImagePath);


            FString DestinationImagePath =
                DestinationFolder /
                FPaths::GetCleanFilename(Data->ImagePath);


            IFileManager::Get().Copy(
                *DestinationImagePath,
                *SourceImagePath);


            Data->ImagePath =
                TEXT("Preset/") +
                FPaths::GetCleanFilename(Data->ImagePath);
        }

        bool bExists = false;

        for (const auto& Existing : ButtonItems)
        {
            if (!Existing.IsValid())
            {
                continue;
            }

            if (Existing->Name == Data->Name)
            {
                bExists = true;
                break;
            }
        }

        if (!bExists)
        {
            ButtonItems.Add(Data);

            AddedCount++;
        }
    }
    FilteredButtonItems = ButtonItems;

    ListView->RequestListRefresh();

    SaveButtons();

    ShowNotification(
        FString::Printf(
            TEXT("%d presets imported."),
            AddedCount),
        SNotificationItem::CS_Success
    );
    return FReply::Handled();
}

void FModuleComposerModule::ShowNotification(
    const FString& Message,
    SNotificationItem::ECompletionState State)
{
    FNotificationInfo Info(
        FText::FromString(Message)
    );

    Info.ExpireDuration = 3.5f;
    Info.FadeOutDuration = 0.5f;

    TSharedPtr<SNotificationItem> Notification =
        FSlateNotificationManager::Get().AddNotification(Info);

    if (Notification.IsValid())
    {
        Notification->SetCompletionState(State);
    }
}

void FModuleComposerModule::OnSearchChanged(const FText& Text)
{
    FString SearchText =
        Text.ToString();

    FilteredButtonItems.Empty();

    if (SearchText.IsEmpty())
    {
        FilteredButtonItems = ButtonItems;
    }
    else
    {
        for (const TSharedPtr<FButtonData>& Item : ButtonItems)
        {
            if (!Item.IsValid())
            {
                continue;
            }

            if (Item->Name.Contains(SearchText))
            {
                FilteredButtonItems.Add(Item);
            }
        }
    }

    if (ListView.IsValid())
    {
        ListView->RequestListRefresh();
    }
}

FReply FModuleComposerModule::OnDeleteButton()
{
    if (!SelectedItem.IsValid())
    {
        return FReply::Handled();
    }

    DestroyPreviewActors();

    TSharedPtr<FButtonData> DeletedItem =
        SelectedItem;

    ButtonItems.Remove(DeletedItem);
    FilteredButtonItems.Remove(DeletedItem);

    SelectedItem.Reset();

    if (ListView.IsValid())
    {
        ListView->ClearSelection();
        ListView->RequestListRefresh();
    }

    if (PreviewImage.IsValid())
    {
        PreviewBrush.Reset();
        PreviewImage->SetImage(nullptr);
    }

    SaveButtons();

    ShowNotification(
        TEXT("Button deleted."),
        SNotificationItem::CS_Success
    );

    return FReply::Handled();
}

FReply FModuleComposerModule::OnUpdateButton()
{
    if (!SelectedItem.IsValid())
    {
        return FReply::Handled();
    }

    UpdateImagePath = SelectedItem->ImagePath;

    SAssignNew(UpdateWindow, SWindow)
        .Title(FText::FromString("Update Button"))
        .ClientSize(FVector2D(300, 180))
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SAssignNew(NameBox, SEditableTextBox)
                        .Text(FText::FromString(SelectedItem->Name))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SAssignNew(JsonPathBox, SEditableTextBox)
                        .IsReadOnly(true)
                        .Text(FText::FromString(SelectedItem->JsonPath))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SButton)
                        .Text(FText::FromString("Select Image"))
                        .OnClicked_Raw(
                            this,
                            &FModuleComposerModule::OnSelectImageButton)
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SAssignNew(ImagePathBox, SEditableTextBox)
                        .IsReadOnly(true)
                        .Text(FText::FromString(SelectedItem->ImagePath))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SButton)
                        .Text(FText::FromString("OK"))
                        .OnClicked_Raw(
                            this,
                            &FModuleComposerModule::OnUpdateConfirm)
                ]
        ];

    FSlateApplication::Get().AddWindow(
        UpdateWindow.ToSharedRef());

    FSlateApplication::Get().SetKeyboardFocus(
        NameBox);

    return FReply::Handled();
}

FReply FModuleComposerModule::OnUpdateConfirm()
{
    if (!SelectedItem.IsValid())
    {
        return FReply::Handled();
    }

    SelectedItem->Name =
        NameBox->GetText().ToString();

    SelectedItem->JsonPath =
        JsonPathBox->GetText().ToString();

    if (!UpdateImagePath.IsEmpty())
    {
        if (UpdateImagePath != SelectedItem->ImagePath)
        {
            FString ImageFolder =
                FPaths::ProjectDir() /
                TEXT("ModuleComposer");

            IFileManager::Get().MakeDirectory(
                *ImageFolder,
                true);

            FString FileName =
                FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S_"))
                + FPaths::GetCleanFilename(UpdateImagePath);

            FString DestPath =
                ImageFolder / FileName;

            IFileManager::Get().Copy(
                *DestPath,
                *UpdateImagePath);

            SelectedItem->ImagePath = DestPath;
        }
    }
    else
    {
        SelectedItem->ImagePath.Empty();
    }


    ListView->RebuildList();

    SaveButtons();

    ShowNotification(
        TEXT("Button updated."),
        SNotificationItem::CS_Success
    );

    if (UpdateWindow.IsValid())
    {
        UpdateWindow->RequestDestroyWindow();
    }

    return FReply::Handled();
}

FReply FModuleComposerModule::OnMoveUpButton()
{
    if (!SelectedItem.IsValid())
    {
        return FReply::Handled();
    }

    int32 Index = ButtonItems.IndexOfByKey(SelectedItem);

    if (Index <= 0)
    {
        return FReply::Handled();
    }

    ButtonItems.Swap(Index, Index - 1);

    FilteredButtonItems = ButtonItems;

    ListView->RebuildList();

    ListView->SetSelection(ButtonItems[Index - 1]);

    SaveButtons();

    ShowNotification(
        TEXT("Button moved up."),
        SNotificationItem::CS_Success
    );

    return FReply::Handled();
}

FReply FModuleComposerModule::OnMoveDownButton()
{
    if (!SelectedItem.IsValid())
    {
        return FReply::Handled();
    }

    int32 Index = ButtonItems.IndexOfByKey(SelectedItem);

    if (Index == INDEX_NONE || Index >= ButtonItems.Num() - 1)
    {
        return FReply::Handled();
    }

    ButtonItems.Swap(Index, Index + 1);

    FilteredButtonItems = ButtonItems;

    ListView->RebuildList();

    ListView->SetSelection(ButtonItems[Index + 1]);

    SaveButtons();

    ShowNotification(
        TEXT("Button moved down."),
        SNotificationItem::CS_Success
    );

    return FReply::Handled();
}

FReply FModuleComposerModule::OnSelectJsonButton()
{
    TArray<FString> Files;

    IDesktopPlatform* DesktopPlatform =
        FDesktopPlatformModule::Get();

    if (DesktopPlatform)
    {
        void* ParentWindowHandle = nullptr;

        DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            TEXT("Select JSON"),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("JSON file (*.json)|*.json"),
            EFileDialogFlags::None,
            Files
        );
    }


    if (Files.Num() > 0)
    {
        JsonPathBox->SetText(
            FText::FromString(Files[0])
        );
    }

    return FReply::Handled();
}

ECheckBoxState FModuleComposerModule::IsPreviewOff() const
{
    return PreviewMode == EPreviewMode::Off
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

ECheckBoxState FModuleComposerModule::IsBoundingBoxPreview() const
{
    return PreviewMode == EPreviewMode::BoundingBox
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

ECheckBoxState FModuleComposerModule::IsWireframePreview() const
{
    return PreviewMode == EPreviewMode::Wireframe
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

ECheckBoxState FModuleComposerModule::IsTransparentPreview() const
{
    return PreviewMode == EPreviewMode::Transparent
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

ECheckBoxState FModuleComposerModule::IsPreview() const
{
    return PreviewMode == EPreviewMode::Preview
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

void FModuleComposerModule::OnPreviewModeChanged(
    ECheckBoxState NewState,
    EPreviewMode Mode)
{
    if (NewState != ECheckBoxState::Checked)
    {
        return;
    }


    PreviewMode = Mode;


    DestroyPreviewActors();

    if (Mode != EPreviewMode::Off)
    {
        if (LoadPreviewScene())
        {
            if (Mode == EPreviewMode::BoundingBox)
            {
                SpawnBoundingBoxPreview();
            }
            else
            {
                SpawnPreviewActors();
            }
        }
    }
}

void FModuleComposerModule::DestroyPreviewActors()
{
    UWorld* World =
        GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return;
    }

    for (auto& Actor : PreviewActors)
    {
        if (Actor.IsValid())
        {
            World->DestroyActor(Actor.Get());
        }
    }

    PreviewActors.Empty();
}



FReply FModuleComposerModule::OnSelectImageButton()
{
    TArray<FString> Files;

    IDesktopPlatform* DesktopPlatform =
        FDesktopPlatformModule::Get();

    if (DesktopPlatform)
    {
        DesktopPlatform->OpenFileDialog(
            nullptr,
            TEXT("Select Image"),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("Image (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg"),
            EFileDialogFlags::None,
            Files);
    }

    if (Files.Num() > 0)
    {
        if (ExportImagePathBox.IsValid())
        {
            ExportImagePathBox->SetText(
                FText::FromString(Files[0]));

            ExportImagePath = Files[0];

            ExportPreviewBrush =
                MakeShared<FSlateDynamicImageBrush>(
                    FName(*ExportImagePath),
                    FVector2D(256, 256));


            if (ExportPreviewImage.IsValid())
            {
                ExportPreviewImage->SetImage(
                    ExportPreviewBrush.Get());
            }
        }
        else
        {
            ImagePathBox->SetText(
                FText::FromString(Files[0]));

            UpdateImagePath = Files[0];
        }
    }

    return FReply::Handled();
}

FReply FModuleComposerModule::OnDeleteSpawnGroupButton()
{
    if (!SpawnGroupBox.IsValid())
    {
        return FReply::Handled();
    }

    FString SpawnGroup =
        SpawnGroupBox->GetText().ToString();

    if (SpawnGroup.IsEmpty())
    {
        return FReply::Handled();
    }

    UWorld* World =
        GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FReply::Handled();
    }

    TArray<AActor*> ActorsToDelete;

    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        AStaticMeshActor* Actor = *It;

        if (!Actor)
        {
            continue;
        }

        if (Actor->GetActorLabel().Contains(SpawnGroup))
        {
            ActorsToDelete.Add(Actor);
        }
    }

    for (AActor* Actor : ActorsToDelete)
    {
        World->DestroyActor(Actor);
    }

    TArray<FFolder> FoldersToDelete;

    FActorFolders::Get().ForEachFolder(
        *World,
        [&SpawnGroup, &FoldersToDelete](const FFolder& Folder)
        {
            if (Folder.GetPath().ToString() == SpawnGroup)
            {
                FoldersToDelete.Add(Folder);
            }

            return true;
        }
    );

    for (const FFolder& Folder : FoldersToDelete)
    {
        FActorFolders::Get().DeleteFolder(
            *World,
            Folder
        );
    }

    ShowNotification(
        FString::Printf(
            TEXT("%d actors deleted."),
            ActorsToDelete.Num()
        ),
        SNotificationItem::CS_Success
    );

    return FReply::Handled();
}

void FModuleComposerModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ModuleComposerTabName);
}

void FModuleComposerModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            Section.AddMenuEntryWithCommandList(FModuleComposerCommands::Get().OpenPluginWindow, PluginCommands);
        }
    }

    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
            {
                FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FModuleComposerCommands::Get().OpenPluginWindow));
                Entry.SetCommandList(PluginCommands);
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModuleComposerModule, ModuleComposer)
