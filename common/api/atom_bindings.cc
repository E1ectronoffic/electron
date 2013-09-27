// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_bindings.h"

#include "base/debug/debugger.h"
#include "base/logging.h"
#include "common/atom_version.h"
#include "common/v8_conversions.h"
#include "vendor/node/src/node.h"

namespace atom {

namespace {

static int kMaxCallStackSize = 200;  // Same with WebKit.

static uv_async_t dummy_uv_handle;

void UvNoOp(uv_async_t* handle, int status) {
}

v8::Handle<v8::Object> DumpStackFrame(v8::Handle<v8::StackFrame> stack_frame) {
  v8::Local<v8::Object> result = v8::Object::New();
  result->Set(ToV8Value("line"), ToV8Value(stack_frame->GetLineNumber()));
  result->Set(ToV8Value("column"), ToV8Value(stack_frame->GetColumn()));

  v8::Handle<v8::String> script = stack_frame->GetScriptName();
  if (!script.IsEmpty())
    result->Set(ToV8Value("script"), script);

  v8::Handle<v8::String> function = stack_frame->GetScriptNameOrSourceURL();
  if (!function.IsEmpty())
    result->Set(ToV8Value("function"), function);
  return result;
}

}  // namespace

// Defined in atom_extensions.cc.
node::node_module_struct* GetBuiltinModule(const char *name, bool is_browser);

AtomBindings::AtomBindings() {
  uv_async_init(uv_default_loop(), &dummy_uv_handle, UvNoOp);
}

AtomBindings::~AtomBindings() {
}

void AtomBindings::BindTo(v8::Handle<v8::Object> process) {
  v8::HandleScope scope;

  node::SetMethod(process, "atomBinding", Binding);
  node::SetMethod(process, "crash", Crash);
  node::SetMethod(process, "activateUvLoop", ActivateUVLoop);
  node::SetMethod(process, "log", Log);
  node::SetMethod(process, "getCurrentStackTrace", GetCurrentStackTrace);

  process->Get(v8::String::New("versions"))->ToObject()->
    Set(v8::String::New("atom-shell"), v8::String::New(ATOM_VERSION_STRING));
}

// static
v8::Handle<v8::Value> AtomBindings::Binding(const v8::Arguments& args) {
  v8::HandleScope scope;

  v8::Local<v8::String> module = args[0]->ToString();
  v8::String::Utf8Value module_v(module);
  node::node_module_struct* modp;

  v8::Local<v8::Object> process = v8::Context::GetCurrent()->Global()->
      Get(v8::String::New("process"))->ToObject();
  DCHECK(!process.IsEmpty());

  // is_browser = process.__atom_type == 'browser'.
  bool is_browser = std::string("browser") == *v8::String::Utf8Value(
      process->Get(v8::String::New("__atom_type")));

  // Cached in process.__atom_binding_cache.
  v8::Local<v8::Object> binding_cache;
  v8::Local<v8::String> bc_name = v8::String::New("__atomBindingCache");
  if (process->Has(bc_name)) {
    binding_cache = process->Get(bc_name)->ToObject();
    DCHECK(!binding_cache.IsEmpty());
  } else {
    binding_cache = v8::Object::New();
    process->Set(bc_name, binding_cache);
  }

  v8::Local<v8::Object> exports;

  if (binding_cache->Has(module)) {
    exports = binding_cache->Get(module)->ToObject();
    return scope.Close(exports);
  }

  if ((modp = GetBuiltinModule(*module_v, is_browser)) != NULL) {
    exports = v8::Object::New();
    // Internal bindings don't have a "module" object,
    // only exports.
    modp->register_func(exports, v8::Undefined());
    binding_cache->Set(module, exports);
    return scope.Close(exports);
  }

  return v8::ThrowException(v8::Exception::Error(
      v8::String::New("No such module")));
}

// static
v8::Handle<v8::Value> AtomBindings::Crash(const v8::Arguments& args) {
  base::debug::BreakDebugger();
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AtomBindings::ActivateUVLoop(const v8::Arguments& args) {
  uv_async_send(&dummy_uv_handle);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AtomBindings::Log(const v8::Arguments& args) {
  std::string message;
  for (int i = 0; i < args.Length(); ++i)
    message += *v8::String::Utf8Value(args[i]);

  logging::LogMessage("CONSOLE", 0, 0).stream() << message;
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AtomBindings::GetCurrentStackTrace(
    const v8::Arguments& args) {
  v8::HandleScope scope;

  int stack_limit = kMaxCallStackSize;
  FromV8Arguments(args, &stack_limit);

  v8::Local<v8::StackTrace> stack_trace = v8::StackTrace::CurrentStackTrace(
      stack_limit, v8::StackTrace::kDetailed);

  int frame_count = stack_trace->GetFrameCount();
  v8::Local<v8::Array> result = v8::Array::New(frame_count);
  for (int i = 0; i < frame_count; ++i)
    result->Set(i, DumpStackFrame(stack_trace->GetFrame(i)));

  return scope.Close(result);
}

}  // namespace atom
