#include "stdafx.h"
#include "Configurator.h"
#include "SwitcherSettingsDialog.h"
#include <rime/deployer.h>
#include <rime/service.h>
#include <rime/expl/switcher_settings.h>

Configurator::Configurator()
{
}

bool Configurator::Run()
{
        rime::Deployer& deployer(rime::Service::instance().deployer());
		bool saved = false;
		{
                rime::SwitcherSettings settings;
                if (!settings.Load(&deployer))
                        return false;
                SwitcherSettingsDialog dialog(&settings);
                if (dialog.DoModal() == IDOK) {
                        if (settings.Save(&deployer))
							saved = true;
                }
        }
        return saved;
}
