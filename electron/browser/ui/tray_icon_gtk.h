// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_UI_TRAY_ICON_GTK_H_
#define ELECTRON_BROWSER_UI_TRAY_ICON_GTK_H_

#include <string>

#include "electron/browser/ui/tray_icon.h"
#include "ui/views/linux_ui/status_icon_linux.h"

namespace views {
class StatusIconLinux;
}

namespace atom {

class TrayIconGtk : public TrayIcon,
                    public views::StatusIconLinux::Delegate {
 public:
  TrayIconGtk();
  virtual ~TrayIconGtk();

  // TrayIcon:
  void SetImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetContextMenu(ui::SimpleMenuModel* menu_model) override;

 private:
  // views::StatusIconLinux::Delegate:
  void OnClick() override;
  bool HasClickAction() override;

  scoped_ptr<views::StatusIconLinux> icon_;

  DISALLOW_COPY_AND_ASSIGN(TrayIconGtk);
};

}  // namespace atom

#endif  // ELECTRON_BROWSER_UI_TRAY_ICON_GTK_H_
