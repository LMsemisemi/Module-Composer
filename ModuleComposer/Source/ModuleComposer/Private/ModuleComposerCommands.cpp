// Copyright (c) 2026 Lucky Machida. All rights reserved.

#include "ModuleComposerCommands.h"

#define LOCTEXT_NAMESPACE "FModuleComposerModule"

void FModuleComposerCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ModuleComposer", "Bring up ModuleComposer window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
