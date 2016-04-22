// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/mac/atom_application_delegate.h"

#import "atom/browser/mac/atom_application.h"
#include "atom/browser/browser.h"
#include "base/strings/sys_string_conversions.h"

@implementation AtomApplicationDelegate

- (id)init {
  self = [super init];
  menu_controller_.reset([[AtomMenuController alloc] init]);
  return self;
}

- (void)setApplicationDockMenu:(ui::MenuModel*)model {
  [menu_controller_ populateWithModel:model];
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];

  // Add observer to monitor the system's Dark Mode theme.
  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(platformThemeChanged:)
                                                          name:@"AppleInterfaceThemeChangedNotification"
                                                        object:nil];

  // Add observer to monitor the system's Graphite Appearance.
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(platformAppearanceChanged:)
                                               name:NSControlTintDidChangeNotification
                                             object:nil];

  atom::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  atom::Browser::Get()->DidFinishLaunching();
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  return [menu_controller_ menu];
}

- (BOOL)application:(NSApplication*)sender
           openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return atom::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
  atom::Browser* browser = atom::Browser::Get();
  if (browser->is_quiting()) {
    return NSTerminateNow;
  } else {
    // System started termination.
    atom::Browser::Get()->Quit();
    return NSTerminateCancel;
  }
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  atom::Browser* browser = atom::Browser::Get();
  browser->Activate(static_cast<bool>(flag));
  return flag;
}

- (void)platformThemeChanged:(NSNotification *)notify {
  atom::Browser::Get()->PlatformThemeChanged();
}

- (void)platformAppearanceChanged:(NSNotification *)notify {
  atom::Browser::Get()->PlatformAppearanceChanged();
}

@end
