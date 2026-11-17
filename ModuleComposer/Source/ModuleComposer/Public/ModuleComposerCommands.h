// Copyright (c) 2026 Lucky Machida. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ModuleComposerStyle.h"

class FModuleComposerCommands : public TCommands<FModuleComposerCommands>
{
public:

	FModuleComposerCommands()
		: TCommands<FModuleComposerCommands>(TEXT("ModuleComposer"), NSLOCTEXT("Contexts", "ModuleComposer", "ModuleComposer Plugin"), NAME_None, FModuleComposerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};