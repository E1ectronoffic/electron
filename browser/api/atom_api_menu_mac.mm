// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "browser/api/atom_api_menu_mac.h"

#include "base/message_loop.h"
#include "base/mac/scoped_sending_event.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/native_window.h"
#include "common/v8_conversions.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"

namespace atom {

namespace api {

MenuMac::MenuMac(v8::Handle<v8::Object> wrapper)
    : Menu(wrapper) {
}

MenuMac::~MenuMac() {
}

void MenuMac::Popup(NativeWindow* native_window) {
  scoped_nsobject<AtomMenuController> menu_controller(
      [[AtomMenuController alloc] initWithModel:model_.get()]);

  NSWindow* window = native_window->GetNativeWindow();
  content::WebContents* web_contents = native_window->GetWebContents();

  // Fake out a context menu event.
  NSEvent* currentEvent = [NSApp currentEvent];
  NSPoint position = [window mouseLocationOutsideOfEventStream];
  NSTimeInterval eventTime = [currentEvent timestamp];
  NSEvent* clickEvent = [NSEvent mouseEventWithType:NSRightMouseDown
                                           location:position
                                      modifierFlags:NSRightMouseDownMask
                                          timestamp:eventTime
                                       windowNumber:[window windowNumber]
                                            context:nil
                                        eventNumber:0
                                         clickCount:1
                                           pressure:1.0];

  {
    // Make sure events can be pumped while the menu is up.
    MessageLoop::ScopedNestableTaskAllower allow(MessageLoop::current());

    // One of the events that could be pumped is |window.close()|.
    // User-initiated event-tracking loops protect against this by
    // setting flags in -[CrApplication sendEvent:], but since
    // web-content menus are initiated by IPC message the setup has to
    // be done manually.
    base::mac::ScopedSendingEvent sendingEventScoper;

    // Show the menu.
    [NSMenu popUpContextMenu:[menu_controller menu]
                   withEvent:clickEvent
                     forView:web_contents->GetView()->GetContentNativeView()];
  }
}

// static
void MenuMac::SendActionToFirstResponder(const std::string& action) {
  SEL selector = NSSelectorFromString(base::SysUTF8ToNSString(action));
  [[NSApplication sharedApplication] sendAction:selector
                                             to:nil
                                           from:[NSApp mainMenu]];
}

// static
v8::Handle<v8::Value> Menu::SetApplicationMenu(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsObject())
    return node::ThrowTypeError("Bad argument");

  MenuMac* menu = ObjectWrap::Unwrap<MenuMac>(args[0]->ToObject());
  if (!menu)
    return node::ThrowError("Menu is destroyed");

  scoped_nsobject<AtomMenuController> menu_controller(
      [[AtomMenuController alloc] initWithModel:menu->model_.get()]);
  [NSApp setMainMenu:[menu_controller menu]];

  // Ensure the menu_controller_ is destroyed after main menu is set.
  menu_controller.swap(menu->menu_controller_);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::SendActionToFirstResponder(
    const v8::Arguments &args) {
  v8::HandleScope scope;

  std::string action;
  if (!FromV8Arguments(args, &action))
    return node::ThrowTypeError("Bad argument");

  MenuMac::SendActionToFirstResponder(action);

  return v8::Undefined();
}

// static
Menu* Menu::Create(v8::Handle<v8::Object> wrapper) {
  return new MenuMac(wrapper);
}

}  // namespace api

}  // namespace atom
