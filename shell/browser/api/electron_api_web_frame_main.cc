// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_frame_main.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/browser/renderer_host/frame_tree_node.h"  // nogncheck
#include "content/public/browser/render_frame_host.h"
#include "gin/object_template_builder.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

typedef std::unordered_map<content::RenderFrameHost*, WebFrame*> RenderFrameMap;
base::LazyInstance<RenderFrameMap>::DestructorAtExit g_render_frame_map =
    LAZY_INSTANCE_INITIALIZER;

WebFrame* FromRenderFrameHost(content::RenderFrameHost* rfh) {
  auto frame_map = g_render_frame_map.Get();
  auto iter = frame_map.find(rfh);
  auto* web_frame = iter == frame_map.end() ? nullptr : iter->second;
  return web_frame;
}

gin::WrapperInfo WebFrame::kWrapperInfo = {gin::kEmbedderNativeGin};

WebFrame::WebFrame(content::RenderFrameHost* rfh) : render_frame_(rfh) {
  g_render_frame_map.Get().emplace(rfh, this);
}

WebFrame::~WebFrame() {
  MarkRenderFrameDisposed();
}

void WebFrame::MarkRenderFrameDisposed() {
  g_render_frame_map.Get().erase(render_frame_);
  render_frame_disposed_ = true;
}

bool WebFrame::CheckRenderFrame() const {
  if (render_frame_disposed_) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Render frame was disposed before WebFrame could be accessed");
    return false;
  }
  return true;
}

v8::Local<v8::Promise> WebFrame::ExecuteJavaScript(gin::Arguments* args,
                                                   const base::string16& code) {
  gin_helper::Promise<base::Value> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Optional userGesture parameter
  bool user_gesture;
  if (!args->PeekNext().IsEmpty()) {
    if (args->PeekNext()->IsBoolean()) {
      args->GetNext(&user_gesture);
    } else {
      args->ThrowTypeError("userGesture must be a boolean");
      return handle;
    }
  } else {
    user_gesture = false;
  }

  if (render_frame_disposed_) {
    promise.RejectWithErrorMessage(
        "Render frame was disposed before WebFrame could be accessed");
    return handle;
  }

  if (user_gesture) {
    auto* ftn = content::FrameTreeNode::From(render_frame_);
    ftn->UpdateUserActivationState(
        blink::mojom::UserActivationUpdateType::kNotifyActivation,
        blink::mojom::UserActivationNotificationType::kTest);
  }

  render_frame_->ExecuteJavaScriptForTests(
      code, base::BindOnce([](gin_helper::Promise<base::Value> promise,
                              base::Value value) { promise.Resolve(value); },
                           std::move(promise)));

  return handle;
}

bool WebFrame::Reload(v8::Isolate* isolate) {
  if (!CheckRenderFrame())
    return false;
  return render_frame_->Reload();
}

int WebFrame::FrameTreeNodeID(v8::Isolate* isolate) const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_->GetFrameTreeNodeId();
}

int WebFrame::ProcessID(v8::Isolate* isolate) const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_->GetProcess()->GetID();
}

int WebFrame::RoutingID(v8::Isolate* isolate) const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_->GetRoutingID();
}

GURL WebFrame::URL(v8::Isolate* isolate) const {
  if (!CheckRenderFrame())
    return GURL::EmptyGURL();
  return render_frame_->GetLastCommittedURL();
}

content::RenderFrameHost* WebFrame::Top(v8::Isolate* isolate) const {
  if (!CheckRenderFrame())
    return nullptr;
  return render_frame_->GetMainFrame();
}

content::RenderFrameHost* WebFrame::Parent(v8::Isolate* isolate) const {
  if (!CheckRenderFrame())
    return nullptr;
  return render_frame_->GetParent();
}

std::vector<content::RenderFrameHost*> WebFrame::Frames(
    v8::Isolate* isolate) const {
  std::vector<content::RenderFrameHost*> frame_hosts;
  if (!CheckRenderFrame())
    return frame_hosts;

  for (auto* rfh : render_frame_->GetFramesInSubtree()) {
    if (rfh->GetParent() == render_frame_)
      frame_hosts.push_back(rfh);
  }

  return frame_hosts;
}

std::vector<content::RenderFrameHost*> WebFrame::FramesInSubtree(
    v8::Isolate* isolate) const {
  std::vector<content::RenderFrameHost*> frame_hosts;
  if (!CheckRenderFrame())
    return frame_hosts;

  for (auto* rfh : render_frame_->GetFramesInSubtree()) {
    frame_hosts.push_back(rfh);
  }

  return frame_hosts;
}

// static
gin::Handle<WebFrame> WebFrame::From(v8::Isolate* isolate,
                                     content::RenderFrameHost* rfh) {
  if (rfh == nullptr)
    return gin::Handle<WebFrame>();
  auto* web_frame = FromRenderFrameHost(rfh);
  auto handle = gin::CreateHandle(
      isolate, web_frame == nullptr ? new WebFrame(rfh) : web_frame);
  return handle;
}

// static
gin::Handle<WebFrame> WebFrame::FromID(v8::Isolate* isolate,
                                       int render_process_id,
                                       int render_frame_id) {
  auto* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  return From(isolate, rfh);
}

// static
void WebFrame::RenderFrameDeleted(content::RenderFrameHost* rfh) {
  auto* web_frame = FromRenderFrameHost(rfh);
  if (web_frame)
    web_frame->MarkRenderFrameDisposed();
}

gin::ObjectTemplateBuilder WebFrame::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebFrame>::GetObjectTemplateBuilder(isolate)
      .SetMethod("executeJavaScript", &WebFrame::ExecuteJavaScript)
      .SetMethod("reload", &WebFrame::Reload)
      .SetProperty("frameTreeNodeId", &WebFrame::FrameTreeNodeID)
      .SetProperty("processId", &WebFrame::ProcessID)
      .SetProperty("routingId", &WebFrame::RoutingID)
      .SetProperty("url", &WebFrame::URL)
      .SetProperty("top", &WebFrame::Top)
      .SetProperty("parent", &WebFrame::Parent)
      .SetProperty("frames", &WebFrame::Frames)
      .SetProperty("framesInSubtree", &WebFrame::FramesInSubtree);
}

const char* WebFrame::GetTypeName() {
  return "WebFrame";
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::WebFrame;

v8::Local<v8::Value> FromID(gin_helper::ErrorThrower thrower,
                            int render_process_id,
                            int render_frame_id) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrame is available only after app ready");
    return v8::Null(thrower.isolate());
  }

  return WebFrame::FromID(thrower.isolate(), render_process_id, render_frame_id)
      .ToV8();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("fromId", &FromID);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_frame_main, Initialize)
