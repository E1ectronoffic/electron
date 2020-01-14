// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_browser_api_provider.h"

#include "extensions/browser/extension_function_registry.h"
#include "shell/browser/extensions/api/tabs/tabs_api.h"

namespace extensions {

ElectronExtensionsBrowserAPIProvider::ElectronExtensionsBrowserAPIProvider() =
    default;
ElectronExtensionsBrowserAPIProvider::~ElectronExtensionsBrowserAPIProvider() =
    default;

void ElectronExtensionsBrowserAPIProvider::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) {
  registry->RegisterFunction<TabsExecuteScriptFunction>();
  /*
  // Preferences.
  registry->RegisterFunction<GetPreferenceFunction>();
  registry->RegisterFunction<SetPreferenceFunction>();
  registry->RegisterFunction<ClearPreferenceFunction>();

  // Generated APIs from Electron.
  api::ElectronGeneratedFunctionRegistry::RegisterAll(registry);
  */
}

}  // namespace extensions
