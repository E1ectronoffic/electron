// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_window.h"

#include "base/values.h"
#include "browser/native_window.h"
#include "common/v8_conversions.h"
#include "common/v8_value_converter_impl.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_process_host.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

using content::V8ValueConverter;
using content::NavigationController;
using node::ObjectWrap;

#define UNWRAP_WINDOW_AND_CHECK \
  Window* self = ObjectWrap::Unwrap<Window>(args.This()); \
  if (self == NULL) \
    return node::ThrowError("Window is already destroyed")

namespace atom {

namespace api {

Window::Window(v8::Handle<v8::Object> wrapper, base::DictionaryValue* options)
    : EventEmitter(wrapper),
      window_(NativeWindow::Create(options)) {
  window_->InitFromOptions(options);
  window_->AddObserver(this);
}

Window::~Window() {
  Emit("destroyed");

  window_->RemoveObserver(this);
}

void Window::OnPageTitleUpdated(bool* prevent_default,
                                const std::string& title) {
  base::ListValue args;
  args.AppendString(title);
  *prevent_default = Emit("page-title-updated", &args);
}

void Window::OnLoadingStateChanged(bool is_loading) {
  base::ListValue args;
  args.AppendBoolean(is_loading);
  Emit("loading-state-changed", &args);
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::OnWindowClosed() {
  Emit("closed");

  // Free memory immediately when window is closed.
  delete this;
}

void Window::OnWindowBlur() {
  Emit("blur");
}

void Window::OnRendererUnresponsive() {
  Emit("unresponsive");
}

void Window::OnRendererResponsive() {
  Emit("responsive");
}

void Window::OnRendererCrashed() {
  Emit("crashed");
}

// static
v8::Handle<v8::Value> Window::New(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  if (!args[0]->IsObject())
    return node::ThrowTypeError("Need options creating Window");

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverterImpl());
  scoped_ptr<base::Value> options(
      converter->FromV8Value(args[0], v8::Context::GetCurrent()));

  if (!options || !options->IsType(base::Value::TYPE_DICTIONARY))
    return node::ThrowTypeError("Invalid options");

  new Window(args.This(), static_cast<base::DictionaryValue*>(options.get()));

  return args.This();
}

// static
v8::Handle<v8::Value> Window::Destroy(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  delete self;

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Close(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Close();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Focus(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Focus(args[0]->IsBoolean() ? args[0]->BooleanValue(): true);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsFocused(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;
  return ToV8Value(self->window_->IsFocused());
}

// static
v8::Handle<v8::Value> Window::Show(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Show();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Hide(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Hide();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Maximize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Maximize();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Unmaximize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Unmaximize();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Minimize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Minimize();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Restore(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Restore();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetFullscreen(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool fs;
  if (!FromV8Arguments(args, &fs))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetFullscreen(fs);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsFullscreen(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->IsFullscreen());
}

// static
v8::Handle<v8::Value> Window::SetSize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  int width, height;
  if (!FromV8Arguments(args, &width, &height))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetSize(gfx::Size(width, height));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetSize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Size size = self->window_->GetSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(size.width()));
  ret->Set(1, ToV8Value(size.height()));

  return ret;
}

// static
v8::Handle<v8::Value> Window::SetMinimumSize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  int width, height;
  if (!FromV8Arguments(args, &width, &height))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetMinimumSize(gfx::Size(width, height));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetMinimumSize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Size size = self->window_->GetMinimumSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(size.width()));
  ret->Set(1, ToV8Value(size.height()));

  return ret;
}

// static
v8::Handle<v8::Value> Window::SetMaximumSize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;


  int width, height;
  if (!FromV8Arguments(args, &width, &height))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetMaximumSize(gfx::Size(width, height));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetMaximumSize(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Size size = self->window_->GetMaximumSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(size.width()));
  ret->Set(1, ToV8Value(size.height()));

  return ret;
}

// static
v8::Handle<v8::Value> Window::SetResizable(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool resizable;
  if (!FromV8Arguments(args, &resizable))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetResizable(resizable);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsResizable(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->IsResizable());
}

// static
v8::Handle<v8::Value> Window::SetAlwaysOnTop(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool top;
  if (!FromV8Arguments(args, &top))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetAlwaysOnTop(top);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsAlwaysOnTop(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->IsAlwaysOnTop());
}

// static
v8::Handle<v8::Value> Window::Center(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->Center();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetPosition(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  int x, y;
  if (!FromV8Arguments(args, &x, &y))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetPosition(gfx::Point(x, y));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetPosition(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Point pos = self->window_->GetPosition();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(pos.x()));
  ret->Set(1, ToV8Value(pos.y()));

  return ret;
}

// static
v8::Handle<v8::Value> Window::SetTitle(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  std::string title;
  if (!FromV8Arguments(args, &title))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetTitle(title);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetTitle(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;
  return ToV8Value(self->window_->GetTitle());
}

// static
v8::Handle<v8::Value> Window::FlashFrame(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->FlashFrame(
      args[0]->IsBoolean() ? args[0]->BooleanValue(): true);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetKiosk(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool kiosk;
  if (!FromV8Arguments(args, &kiosk))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetKiosk(kiosk);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsKiosk(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->IsKiosk());
}

// static
v8::Handle<v8::Value> Window::OpenDevTools(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->OpenDevTools();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::CloseDevTools(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->CloseDevTools();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::InspectElement(const v8::Arguments& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int x, y;
  if (!FromV8Arguments(args, &x, &y))
    return node::ThrowTypeError("Bad argument");

  self->window_->InspectElement(x, y);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::FocusOnWebView(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->FocusOnWebView();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::BlurWebView(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->BlurWebView();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsWebViewFocused(const v8::Arguments& args) {
  UNWRAP_WINDOW_AND_CHECK;
  return ToV8Value(self->window_->IsWebViewFocused());
}

// static
v8::Handle<v8::Value> Window::RestartHangMonitorTimeout(
    const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->RestartHangMonitorTimeout();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetPageTitle(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->GetWebContents()->GetTitle());
}

// static
v8::Handle<v8::Value> Window::IsLoading(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->GetWebContents()->IsLoading());
}

// static
v8::Handle<v8::Value> Window::IsWaitingForResponse(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->GetWebContents()->IsWaitingForResponse());
}

// static
v8::Handle<v8::Value> Window::Stop(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  self->window_->GetWebContents()->Stop();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetRoutingID(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->GetWebContents()->GetRoutingID());
}

// static
v8::Handle<v8::Value> Window::GetProcessID(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(
      self->window_->GetWebContents()->GetRenderProcessHost()->GetID());
}

// static
v8::Handle<v8::Value> Window::IsCrashed(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  return ToV8Value(self->window_->GetWebContents()->IsCrashed());
}

// static
v8::Handle<v8::Value> Window::LoadURL(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  std::string url;
  if (!FromV8Arguments(args, &url))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.LoadURL(GURL(url),
                     content::Referrer(),
                     content::PAGE_TRANSITION_AUTO_TOPLEVEL,
                     std::string());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetURL(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  std::string url;
  if (controller.GetActiveEntry())
    url = controller.GetActiveEntry()->GetVirtualURL().spec();

  return ToV8Value(url);
}

// static
v8::Handle<v8::Value> Window::CanGoBack(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  return ToV8Value(controller.CanGoBack());
}

// static
v8::Handle<v8::Value> Window::CanGoForward(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  return ToV8Value(controller.CanGoForward());
}

// static
v8::Handle<v8::Value> Window::CanGoToOffset(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  int offset;
  if (!FromV8Arguments(args, &offset))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  return ToV8Value(controller.CanGoToOffset(offset));
}

// static
v8::Handle<v8::Value> Window::GoBack(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoBack();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GoForward(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoForward();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GoToIndex(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  int index;
  if (!FromV8Arguments(args, &index))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoToIndex(index);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GoToOffset(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  int offset;
  if (!FromV8Arguments(args, &offset))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoToOffset(offset);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Reload(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.Reload(args[0]->IsBoolean() ? args[0]->BooleanValue(): false);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::ReloadIgnoringCache(const v8::Arguments &args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.ReloadIgnoringCache(
      args[0]->IsBoolean() ? args[0]->BooleanValue(): false);

  return v8::Undefined();
}

// static
void Window::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(Window::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("BrowserWindow"));

  NODE_SET_PROTOTYPE_METHOD(t, "destroy", Destroy);
  NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
  NODE_SET_PROTOTYPE_METHOD(t, "focus", Focus);
  NODE_SET_PROTOTYPE_METHOD(t, "isFocused", IsFocused);
  NODE_SET_PROTOTYPE_METHOD(t, "show", Show);
  NODE_SET_PROTOTYPE_METHOD(t, "hide", Hide);
  NODE_SET_PROTOTYPE_METHOD(t, "maximize", Maximize);
  NODE_SET_PROTOTYPE_METHOD(t, "unmaximize", Unmaximize);
  NODE_SET_PROTOTYPE_METHOD(t, "minimize", Minimize);
  NODE_SET_PROTOTYPE_METHOD(t, "restore", Restore);
  NODE_SET_PROTOTYPE_METHOD(t, "setFullScreen", SetFullscreen);
  NODE_SET_PROTOTYPE_METHOD(t, "isFullScreen", IsFullscreen);
  NODE_SET_PROTOTYPE_METHOD(t, "setSize", SetSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getSize", GetSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setMinimumSize", SetMinimumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getMinimumSize", GetMinimumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaximumSize", SetMaximumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getMaximumSize", GetMaximumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setResizable", SetResizable);
  NODE_SET_PROTOTYPE_METHOD(t, "isResizable", IsResizable);
  NODE_SET_PROTOTYPE_METHOD(t, "setAlwaysOnTop", SetAlwaysOnTop);
  NODE_SET_PROTOTYPE_METHOD(t, "isAlwaysOnTop", IsAlwaysOnTop);
  NODE_SET_PROTOTYPE_METHOD(t, "center", Center);
  NODE_SET_PROTOTYPE_METHOD(t, "setPosition", SetPosition);
  NODE_SET_PROTOTYPE_METHOD(t, "getPosition", GetPosition);
  NODE_SET_PROTOTYPE_METHOD(t, "setTitle", SetTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "getTitle", GetTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "flashFrame", FlashFrame);
  NODE_SET_PROTOTYPE_METHOD(t, "setKiosk", SetKiosk);
  NODE_SET_PROTOTYPE_METHOD(t, "isKiosk", IsKiosk);
  NODE_SET_PROTOTYPE_METHOD(t, "openDevTools", OpenDevTools);
  NODE_SET_PROTOTYPE_METHOD(t, "closeDevTools", CloseDevTools);
  NODE_SET_PROTOTYPE_METHOD(t, "inspectElement", InspectElement);
  NODE_SET_PROTOTYPE_METHOD(t, "focusOnWebView", FocusOnWebView);
  NODE_SET_PROTOTYPE_METHOD(t, "blurWebView", BlurWebView);
  NODE_SET_PROTOTYPE_METHOD(t, "isWebViewFocused", IsWebViewFocused);
  NODE_SET_PROTOTYPE_METHOD(t,
                            "restartHangMonitorTimeout",
                            RestartHangMonitorTimeout);

  NODE_SET_PROTOTYPE_METHOD(t, "getPageTitle", GetPageTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "isLoading", IsLoading);
  NODE_SET_PROTOTYPE_METHOD(t, "isWaitingForResponse", IsWaitingForResponse);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", Stop);
  NODE_SET_PROTOTYPE_METHOD(t, "getRoutingId", GetRoutingID);
  NODE_SET_PROTOTYPE_METHOD(t, "getProcessId", GetProcessID);
  NODE_SET_PROTOTYPE_METHOD(t, "isCrashed", IsCrashed);

  NODE_SET_PROTOTYPE_METHOD(t, "loadUrl", LoadURL);
  NODE_SET_PROTOTYPE_METHOD(t, "getUrl", GetURL);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoBack", CanGoBack);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoForward", CanGoForward);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoToOffset", CanGoToOffset);
  NODE_SET_PROTOTYPE_METHOD(t, "goBack", GoBack);
  NODE_SET_PROTOTYPE_METHOD(t, "goForward", GoForward);
  NODE_SET_PROTOTYPE_METHOD(t, "goToIndex", GoToIndex);
  NODE_SET_PROTOTYPE_METHOD(t, "goToOffset", GoToOffset);
  NODE_SET_PROTOTYPE_METHOD(t, "reload", Reload);
  NODE_SET_PROTOTYPE_METHOD(t, "reloadIgnoringCache", ReloadIgnoringCache);

  target->Set(v8::String::NewSymbol("BrowserWindow"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_window, atom::api::Window::Initialize)
