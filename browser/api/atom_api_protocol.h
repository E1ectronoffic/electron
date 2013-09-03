// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_

#include <string>
#include <map>

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class Protocol {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

 private:
  static v8::Handle<v8::Value> RegisterProtocol(const v8::Arguments& args);
  static v8::Handle<v8::Value> UnregisterProtocol(const v8::Arguments& args);
  static v8::Handle<v8::Value> IsHandledProtocol(const v8::Arguments& args);

  static v8::Handle<v8::Value> InterceptProtocol(const v8::Arguments& args);
  static v8::Handle<v8::Value> UninterceptProtocol(const v8::Arguments& args);

  static void RegisterProtocolInIO(const std::string& scheme);
  static void UnregisterProtocolInIO(const std::string& scheme);

  static void InterceptProtocolInIO(const std::string& scheme);
  static void UninterceptProtocolInIO(const std::string& scheme);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Protocol);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
