// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_
#define SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_

#include "base/macros.h"
#include "chrome/browser/accessibility/accessibility_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

// Controls the accessibility web UI page.
class ElectronAccessibilityUI : public content::WebUIController {
 public:
  explicit ElectronAccessibilityUI(content::WebUI* web_ui);
  ~ElectronAccessibilityUI() override;
};

// Manages messages sent from accessibility.js via json.
class ElectronAccessibilityUIMessageHandler
    : public AccessibilityUIMessageHandler {
 public:
  ElectronAccessibilityUIMessageHandler();

  void RegisterMessages() final;

 private:
  void RequestNativeUITree(const base::ListValue* args);

  DISALLOW_COPY_AND_ASSIGN(ElectronAccessibilityUIMessageHandler);
};

#endif  // SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_
