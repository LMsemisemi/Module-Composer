// Copyright (c) 2026 Lucky Machida. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SSpinBox.h"
#include "Modules/ModuleManager.h"

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "JsonObjectConverter.h"

#include "HAL/FileManager.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "ModuleComposer.generated.h"
//↑これは絶対に最後のincludeにしないといけない。おまじない。

class FToolBarBuilder;
class FMenuBuilder;

class AStaticMeshActor;
class UMaterialInterface;



struct FButtonData
{
	FString Name;
	FString JsonPath;
	FString ImagePath;
};

USTRUCT()
struct FMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector Scale;
};

USTRUCT()
struct FSceneData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMeshData> Meshes;
};

class FModuleComposerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

	FReply OnAddButton();
	FReply OnImportButtons();
	FReply OnAddConfirm();
	FReply OnDeleteButton();
	FReply OnUpdateButton();
	FReply OnUpdateConfirm();
	FReply OnMoveUpButton();
	FReply OnMoveDownButton();
	FReply OnSelectJsonButton();
	FReply OnExecutionButton();
	FReply OnResetButton();

	FReply OnSelectImageButton();

private:

	void RegisterMenus();
	void SaveButtons();
	void LoadButtons();

	void OnSelectionChanged(
		TSharedPtr<FButtonData> Item,
		ESelectInfo::Type SelectInfo);

	FString UpdateImagePath;

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<FButtonData> SelectedItem;
	TSharedPtr<SWindow> UpdateWindow;
	TSharedPtr<SWindow> ExportWindow;

	FString ExportJsonPath;
	FString ExportImagePath;

	TSharedPtr<SEditableTextBox> ExportNameBox;
	TSharedPtr<SEditableTextBox> ExportJsonPathBox;
	TSharedPtr<SEditableTextBox> ExportImagePathBox;

	TSharedPtr<SImage> ExportPreviewImage;
	TSharedPtr<FSlateDynamicImageBrush> ExportPreviewBrush;
	TArray<TSharedPtr<FButtonData>> ButtonItems;
	TArray<TSharedPtr<FButtonData>> FilteredButtonItems;
	TSharedPtr<SListView<TSharedPtr<FButtonData>>> ListView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SEditableTextBox> NameBox;
	TSharedPtr<SEditableTextBox> JsonPathBox;
	TSharedPtr<SEditableTextBox> ImagePathBox;
	TSharedPtr<class SSpinBox<float>> PosXBox;
	TSharedPtr<class SSpinBox<float>> PosYBox;
	TSharedPtr<class SSpinBox<float>> PosZBox;

	TSharedPtr<class SSpinBox<float>> RotPitchBox;
	TSharedPtr<class SSpinBox<float>> RotYawBox;
	TSharedPtr<class SSpinBox<float>> RotRollBox;

	TSharedPtr<class SSpinBox<int32>> ArrayCountBox;

	TSharedPtr<class SSpinBox<float>> ArrayPosXBox;
	TSharedPtr<class SSpinBox<float>> ArrayPosYBox;
	TSharedPtr<class SSpinBox<float>> ArrayPosZBox;

	TSharedPtr<class SSpinBox<float>> ArrayRotPitchBox;
	TSharedPtr<class SSpinBox<float>> ArrayRotYawBox;
	TSharedPtr<class SSpinBox<float>> ArrayRotRollBox;

	TSharedPtr<SSpinBox<float>> ArrayScaleXBox;
	TSharedPtr<SSpinBox<float>> ArrayScaleYBox;
	TSharedPtr<SSpinBox<float>> ArrayScaleZBox;

	int32 ArrayCount = 1;

	FVector ArrayPositionOffset = FVector::ZeroVector;

	FRotator ArrayRotationOffset = FRotator::ZeroRotator;

	FVector ArrayScaleOffset = FVector::ZeroVector;

	TSharedPtr<SImage> PreviewImage;
	TSharedPtr<FSlateDynamicImageBrush> PreviewBrush;

	TSharedPtr<SEditableTextBox> SpawnGroupBox;
	FReply OnDeleteSpawnGroupButton();
	FReply OnExportJsonButton();
	void ExportSelectedActorsToJson();

	void CaptureScreenshot();

	void OpenExportRegisterWindow();

	FReply OnExportRegisterConfirm();

	enum class EPreviewMode
	{
		Off,
		BoundingBox,
		Wireframe,
		Transparent,
		Preview
	};

	ECheckBoxState IsPreviewOff() const;
	ECheckBoxState IsBoundingBoxPreview() const;
	ECheckBoxState IsWireframePreview() const;
	ECheckBoxState IsTransparentPreview() const;
	ECheckBoxState IsPreview() const;
	void OnPreviewModeChanged(ECheckBoxState NewState, EPreviewMode Mode);
	EPreviewMode PreviewMode = EPreviewMode::Off;

	TArray<TWeakObjectPtr<AStaticMeshActor>> PreviewActors;
	FSceneData PreviewScene;
	FVector PreviewOffset = FVector::ZeroVector;
	FRotator PreviewRotation = FRotator::ZeroRotator;
	UMaterialInterface* PreviewMaterial = nullptr;

	UMaterialInterface* TransparentMaterial = nullptr;
	void SpawnPreviewActors();
	void SpawnBoundingBoxPreview();
	void DestroyPreviewActors();
	void OnPositionChanged(float);
	void OnArrayCountChanged(int32 Value);

	void OnArrayPositionChanged(float Value);

	void OnArrayRotationChanged(float Value);
	void OnArrayScaleChanged(float Value);
	void OnSearchChanged(const FText& Text);
	bool LoadPreviewScene();

	void ShowNotification(
		const FString& Message,
		SNotificationItem::ECompletionState State);

	bool CheckScreenshotFinished(float DeltaTime);

	void CalculateArrayTransform(
		const FMeshData& Mesh,
		int32 ArrayIndex,
		FVector& OutLocation,
		FRotator& OutRotation,
		FVector& OutScale
	);
};