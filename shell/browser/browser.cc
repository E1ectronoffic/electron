// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/browser.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/chrome_paths.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/login_handler.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_helper/arguments.h"

namespace electron {

namespace {

// Call |quit| after Chromium is fully started.
//
// This is important for quitting immediately in the "ready" event, when
// certain initialization task may still be pending, and quitting at that time
// could end up with crash on exit.
void RunQuitClosure(base::OnceClosure quit) {
  // On Linux/Windows the "ready" event is emitted in "PreMainMessageLoopRun",
  // make sure we quit after message loop has run for once.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(quit));
}

}  // namespace

#if defined(OS_WIN)
Browser::LaunchItem::LaunchItem() = default;
Browser::LaunchItem::~LaunchItem() = default;
Browser::LaunchItem::LaunchItem(const LaunchItem& other) = default;
#endif

Browser::LoginItemSettings::LoginItemSettings() = default;
Browser::LoginItemSettings::~LoginItemSettings() = default;
Browser::LoginItemSettings::LoginItemSettings(const LoginItemSettings& other) =
    default;

Browser::Browser() {
  WindowList::AddObserver(this);
}

Browser::~Browser() {
  WindowList::RemoveObserver(this);
}

// static
Browser* Browser::Get() {
  return ElectronBrowserMainParts::Get()->browser();
}

void Browser::Quit() {
  if (is_quiting_)
    return;

  is_quiting_ = HandleBeforeQuit();
  if (!is_quiting_)
    return;

  if (electron::WindowList::IsEmpty())
    NotifyAndShutdown();
  else
    electron::WindowList::CloseAllWindows();
}

void Browser::Exit(gin::Arguments* args) {
  int code = 0;
  args->GetNext(&code);

  if (!ElectronBrowserMainParts::Get()->SetExitCode(code)) {
    // Message loop is not ready, quit directly.
    exit(code);
  } else {
    // Prepare to quit when all windows have been closed.
    is_quiting_ = true;

    // Remember this caller so that we don't emit unrelated events.
    is_exiting_ = true;

    // Must destroy windows before quitting, otherwise bad things can happen.
    if (electron::WindowList::IsEmpty()) {
      Shutdown();
    } else {
      // Unlike Quit(), we do not ask to close window, but destroy the window
      // without asking.
      electron::WindowList::DestroyAllWindows();
    }
  }
}

void Browser::Shutdown() {
  if (is_shutdown_)
    return;

  is_shutdown_ = true;
  is_quiting_ = true;

  for (BrowserObserver& observer : observers_)
    observer.OnQuit();

  if (quit_main_message_loop_) {
    RunQuitClosure(std::move(quit_main_message_loop_));
  } else {
    // There is no message loop available so we are in early stage, wait until
    // the quit_main_message_loop_ is available.
    // Exiting now would leave defunct processes behind.
  }
}

std::string Browser::GetVersion() const {
  std::string ret = GetOverriddenApplicationVersion();
  if (ret.empty())
    ret = GetExecutableFileVersion();
  return ret;
}

void Browser::SetVersion(const std::string& version) {
  OverrideApplicationVersion(version);
}

std::string Browser::GetName() const {
  std::string ret = GetOverriddenApplicationName();
  if (ret.empty())
    ret = GetExecutableFileProductName();
  return ret;
}

void Browser::SetName(const std::string& name) {
  OverrideApplicationName(name);

  // Some of the paths in `app.getPath()` are based on `name` and should
  // update when `name` is changed before `app.ready` is fired. So if app
  // isn't ready yet, invalidate the PathService cache to ensure a refresh.
  if (!is_ready()) {
    base::PathService::InvalidateCache();
  }
}

int Browser::GetBadgeCount() {
  return badge_count_;
}

bool Browser::OpenFile(const std::string& file_path) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnOpenFile(&prevent_default, file_path);

  return prevent_default;
}

void Browser::OpenURL(const std::string& url) {
  for (BrowserObserver& observer : observers_)
    observer.OnOpenURL(url);
}

void Browser::Activate(bool has_visible_windows) {
  for (BrowserObserver& observer : observers_)
    observer.OnActivate(has_visible_windows);
}

void Browser::WillFinishLaunching() {
  for (BrowserObserver& observer : observers_)
    observer.OnWillFinishLaunching();
}

void Browser::DidFinishLaunching(base::DictionaryValue launch_info) {
  // Make sure the userData directory is created.
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  // 'userData' (DIR_USER_DATA) and 'userCache' (DIR_USER_CACHE) can
  // be customized by the user up until the `ready` event is fired.
  // Now that we've reached that point, it's time to freeze their values.
  // FIXME: setPath() should reject changes to these paths after this point
  base::FilePath user_data;
  base::PathService::Get(DIR_USER_DATA, &user_data);
  base::PathService::Override(DIR_USER_DATA, user_data);
  base::PathService::Override(chrome::DIR_USER_DATA, user_data);
  base::CreateDirectoryAndGetError(user_data, nullptr);

  base::FilePath user_cache;
  base::PathService::Get(DIR_USER_CACHE, &user_cache);
  base::PathService::Override(DIR_USER_CACHE, user_cache);

  base::PathService::Override(
      chrome::DIR_APP_DICTIONARIES,
      user_data.Append(base::FilePath::FromUTF8Unsafe("Dictionaries")));

  is_ready_ = true;
  if (ready_promise_) {
    ready_promise_->Resolve();
  }
  for (BrowserObserver& observer : observers_)
    observer.OnFinishLaunching(launch_info);
}

v8::Local<v8::Value> Browser::WhenReady(v8::Isolate* isolate) {
  if (!ready_promise_) {
    ready_promise_ = std::make_unique<gin_helper::Promise<void>>(isolate);
    if (is_ready()) {
      ready_promise_->Resolve();
    }
  }
  return ready_promise_->GetHandle();
}

void Browser::OnAccessibilitySupportChanged() {
  for (BrowserObserver& observer : observers_)
    observer.OnAccessibilitySupportChanged();
}

void Browser::PreMainMessageLoopRun() {
  for (BrowserObserver& observer : observers_) {
    observer.OnPreMainMessageLoopRun();
  }
}

void Browser::PreCreateThreads() {
  for (BrowserObserver& observer : observers_) {
    observer.OnPreCreateThreads();
  }
}

void Browser::SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure) {
  if (is_shutdown_)
    RunQuitClosure(std::move(quit_closure));
  else
    quit_main_message_loop_ = std::move(quit_closure);
}

void Browser::NotifyAndShutdown() {
  if (is_shutdown_)
    return;

  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnWillQuit(&prevent_default);

  if (prevent_default) {
    is_quiting_ = false;
    return;
  }

  Shutdown();
}

bool Browser::HandleBeforeQuit() {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnBeforeQuit(&prevent_default);

  return !prevent_default;
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  if (is_quiting_)
    // Once a beforeunload handler has prevented the closing, we think the quit
    // is cancelled too.
    is_quiting_ = false;
}

void Browser::OnWindowAllClosed() {
  if (is_exiting_) {
    Shutdown();
  } else if (is_quiting_) {
    NotifyAndShutdown();
  } else {
    for (BrowserObserver& observer : observers_)
      observer.OnWindowAllClosed();
  }
}

#if defined(OS_MAC)
void Browser::NewWindowForTab() {
  for (BrowserObserver& observer : observers_)
    observer.OnNewWindowForTab();
}

void Browser::DidBecomeActive() {
  for (BrowserObserver& observer : observers_)
    observer.OnDidBecomeActive();
}
#endif

}  // namespace electron
