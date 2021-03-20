// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_HARFBUZZ_FONT_SKIA_H_
#define UI_GFX_HARFBUZZ_FONT_SKIA_H_

#include "skheader.h"
#include <hb.h>

class SkTypeface;

namespace gfx {

hb_font_t* CreateHarfBuzzFont(sk_sp<SkTypeface> skia_face,
                              SkScalar text_size,
                              const FontRenderParams& params,
                              bool subpixel_rendering_suppressed);

}  // namespace gfx

#endif  // UI_GFX_HARFBUZZ_FONT_SKIA_H_
