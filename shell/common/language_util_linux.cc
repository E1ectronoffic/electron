// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/language_util.h"

#include "base/i18n/rtl.h"

#if defined(USE_GLIB)
#include <glib.h>
#endif

namespace electron {

std::vector<std::string> GetPreferredLanguages() {
  std::vector<std::string> preferredLanguages;

#if defined(USE_GLIB)
  // From
  // https://source.chromium.org/chromium/chromium/src/+/refs/tags/108.0.5329.0:ui/base/l10n/l10n_util.cc;l=543-554
  // GLib implements correct environment variable parsing with
  // the precedence order: LANGUAGE, LC_ALL, LC_MESSAGES and LANG.
  const char* const* languages = g_get_language_names();
  DCHECK(languages);   // A valid pointer is guaranteed.
  DCHECK(*languages);  // At least one entry, "C", is guaranteed.

  for (; *languages; ++languages) {
    preferredLanguages.push_back(base::i18n::GetCanonicalLocale(*languages));
  }
#endif

  // If USE_GLIB isn't defined, an empty vector is returned.
  // You may be able to use GetApplicationLocale() of a browser process instead.
  return preferredLanguages;
}

}  // namespace electron
