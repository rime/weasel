#include "stdafx.h"
#include "ColorSchemePreview.h"

#include <gdiplus.h>
#include <algorithm>
#include <cwchar>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace weasel_deployer {

namespace {

using Gdiplus::Bitmap;
using Gdiplus::Color;
using Gdiplus::Font;
using Gdiplus::FontFamily;
using Gdiplus::Graphics;
using Gdiplus::GraphicsPath;
using Gdiplus::LinearGradientBrush;
using Gdiplus::Pen;
using Gdiplus::PointF;
using Gdiplus::RectF;
using Gdiplus::SolidBrush;
using Gdiplus::StringFormat;

Color GdipColor(uint32_t rgba) {
  return Color(static_cast<BYTE>((rgba >> 0) & 0xff),
               static_cast<BYTE>((rgba >> 24) & 0xff),
               static_cast<BYTE>((rgba >> 16) & 0xff),
               static_cast<BYTE>((rgba >> 8) & 0xff));
}

uint32_t ColorOr(const RgbaColorMap& colors,
                 const char* key,
                 uint32_t fallback) {
  auto it = colors.find(key);
  return it != colors.end() ? it->second : fallback;
}

bool IsVisible(uint32_t rgba) {
  return (rgba & 0xff) != 0;
}

void FillRoundedRect(Graphics& g,
                     const SolidBrush& brush,
                     const RectF& rect,
                     float radius) {
  if (radius <= 0.0f) {
    g.FillRectangle(&brush, rect);
    return;
  }
  GraphicsPath path;
  float d = radius * 2.0f;
  d = (std::min)(d, (std::min)(rect.Width, rect.Height));
  path.AddArc(rect.X, rect.Y, d, d, 180, 90);
  path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
  path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
  path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
  path.CloseFigure();
  g.FillPath(&brush, &path);
}

void DrawCheckerboard(Graphics& g, const RectF& area, float cell) {
  SolidBrush light(Color(255, 235, 235, 235));
  SolidBrush dark(Color(255, 205, 205, 205));
  g.FillRectangle(&light, area);
  for (float y = area.Y; y < area.GetBottom(); y += cell) {
    for (float x = area.X; x < area.GetRight(); x += cell) {
      int row = static_cast<int>((y - area.Y) / cell);
      int col = static_cast<int>((x - area.X) / cell);
      if ((row + col) % 2 == 0)
        continue;
      float w = (std::min)(cell, area.GetRight() - x);
      float h = (std::min)(cell, area.GetBottom() - y);
      g.FillRectangle(&dark, x, y, w, h);
    }
  }
}

struct TextChunk {
  const wchar_t* text;
  Color color;
};

// Draws chunks of text left to right; returns the x position after the last
// chunk.
float DrawChunks(Graphics& g,
                 const Font& font,
                 float x,
                 float y,
                 const std::vector<TextChunk>& chunks) {
  PointF origin(x, y);
  for (const auto& chunk : chunks) {
    SolidBrush brush(chunk.color);
    g.DrawString(chunk.text, -1, &font, origin,
                 StringFormat::GenericTypographic(), &brush);
    RectF box;
    g.MeasureString(chunk.text, -1, &font, origin,
                    StringFormat::GenericTypographic(), &box);
    origin.X += box.Width;
  }
  return origin.X;
}

void RenderMockCandidateWindow(Graphics& g,
                               float width,
                               float height,
                               const RgbaColorMap& colors,
                               float scale) {
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

  const uint32_t back = ColorOr(colors, "back_color", 0xffffffff);
  const uint32_t border = ColorOr(colors, "border_color", 0x000000ff);
  const uint32_t shadow = ColorOr(colors, "shadow_color", 0);
  const uint32_t text = ColorOr(colors, "text_color", 0x000000ff);
  const uint32_t hilited_text = ColorOr(colors, "hilited_text_color", text);
  const uint32_t hilited_back = ColorOr(colors, "hilited_back_color", back);
  const uint32_t hilited_mark = ColorOr(colors, "hilited_mark_color", 0);
  const uint32_t cand_text = ColorOr(colors, "candidate_text_color", text);
  const uint32_t cand_back = ColorOr(colors, "candidate_back_color", 0);
  const uint32_t cand_border = ColorOr(colors, "candidate_border_color", 0);
  const uint32_t label = ColorOr(colors, "label_color", cand_text);
  const uint32_t comment = ColorOr(colors, "comment_text_color", label);
  const uint32_t hi_cand_text =
      ColorOr(colors, "hilited_candidate_text_color", hilited_text);
  const uint32_t hi_cand_back =
      ColorOr(colors, "hilited_candidate_back_color", hilited_back);
  const uint32_t hi_cand_shadow =
      ColorOr(colors, "hilited_candidate_shadow_color", 0);
  const uint32_t hilited_shadow = ColorOr(colors, "hilited_shadow_color", 0);
  const uint32_t cand_shadow = ColorOr(colors, "candidate_shadow_color", 0);
  const uint32_t hi_cand_border =
      ColorOr(colors, "hilited_candidate_border_color", 0);
  const uint32_t hi_label =
      ColorOr(colors, "hilited_label_color", hi_cand_text);
  const uint32_t hi_comment =
      ColorOr(colors, "hilited_comment_text_color", hi_label);
  const uint32_t prev_page = ColorOr(colors, "prevpage_color", 0);
  const uint32_t next_page = ColorOr(colors, "nextpage_color", 0);

  RectF area(0, 0, width, height);
  DrawCheckerboard(g, area, 8.0f * scale);

  const float margin = 10.0f * scale;
  const float corner = 6.0f * scale;
  const float pad_x = 12.0f * scale;
  const float pad_y = 9.0f * scale;
  RectF window_rect(area.X + margin, area.Y + margin, area.Width - margin * 2,
                    area.Height - margin * 2);

  if (IsVisible(shadow)) {
    SolidBrush shadow_brush(GdipColor(shadow));
    RectF shadow_rect = window_rect;
    shadow_rect.Offset(3.0f * scale, 3.0f * scale);
    FillRoundedRect(g, shadow_brush, shadow_rect, corner);
  }
  {
    SolidBrush back_brush(GdipColor(back));
    FillRoundedRect(g, back_brush, window_rect, corner);
  }
  if (IsVisible(border)) {
    Pen border_pen(GdipColor(border), (std::max)(1.0f, 1.5f * scale));
    g.DrawRectangle(&border_pen, window_rect);
  }

  FontFamily yahei(L"Microsoft YaHei");
  const FontFamily* font_family =
      yahei.IsAvailable() ? &yahei : FontFamily::GenericSansSerif();
  Font preedit_font(font_family, 13.0f * scale, Gdiplus::FontStyleRegular,
                    Gdiplus::UnitPixel);
  Font cand_font(font_family, 15.0f * scale, Gdiplus::FontStyleRegular,
                 Gdiplus::UnitPixel);

  float content_x = window_rect.X + pad_x;
  float content_y = window_rect.Y + pad_y;

  // Preedit line, mirroring the official preview images: the converted
  // (selected) text highlighted, followed by the remaining pinyin and a
  // next-page hint. Wide literals are written as escapes because this file
  // is compiled without /utf-8.
  const float preedit_h = 20.0f * scale;
  const wchar_t* kConverted = L"\u4F60\u597D";
  RectF converted_box;
  g.MeasureString(kConverted, -1, &preedit_font, PointF(0, 0),
                  StringFormat::GenericTypographic(), &converted_box);
  const float hi_w = converted_box.Width + 4.0f * scale;
  RectF hi_rect(content_x - 2 * scale, content_y - 1 * scale, hi_w, preedit_h);
  if (IsVisible(hilited_shadow)) {
    SolidBrush s(GdipColor(hilited_shadow));
    RectF sr = hi_rect;
    sr.Offset(2.0f * scale, 2.0f * scale);
    FillRoundedRect(g, s, sr, 3.0f * scale);
  }
  if (IsVisible(hilited_back)) {
    SolidBrush hi_brush(GdipColor(hilited_back));
    FillRoundedRect(g, hi_brush, hi_rect, 3.0f * scale);
  }
  DrawChunks(
      g, preedit_font, content_x, content_y,
      {{kConverted, GdipColor(hilited_text)},
       {L" ni hao", GdipColor(text)},
       {L" \u203A", GdipColor(IsVisible(next_page) ? next_page : text)}});
  if (IsVisible(hilited_mark)) {
    Pen mark_pen(GdipColor(hilited_mark), (std::max)(1.0f, 2.0f * scale));
    g.DrawLine(&mark_pen, content_x, content_y + preedit_h,
               content_x + converted_box.Width, content_y + preedit_h);
  }

  // Candidate row.
  float row_y = content_y + preedit_h + 8.0f * scale;
  const float row_h = 26.0f * scale;
  const float spacing = 16.0f * scale;
  float x = content_x;

  // First (highlighted) candidate.
  const float first_w = 96.0f * scale;
  RectF first_rect(x - 4 * scale, row_y - 3 * scale, first_w + 8 * scale,
                   row_h);
  if (IsVisible(hi_cand_shadow)) {
    SolidBrush s(GdipColor(hi_cand_shadow));
    RectF sr = first_rect;
    sr.Offset(2.0f * scale, 2.0f * scale);
    FillRoundedRect(g, s, sr, 4.0f * scale);
  }
  if (IsVisible(hi_cand_back)) {
    SolidBrush b(GdipColor(hi_cand_back));
    FillRoundedRect(g, b, first_rect, 4.0f * scale);
  }
  if (IsVisible(hi_cand_border)) {
    Pen p(GdipColor(hi_cand_border), (std::max)(1.0f, 1.0f * scale));
    g.DrawRectangle(&p, first_rect);
  }
  x = DrawChunks(g, cand_font, x, row_y,
                 {{L"1. ", GdipColor(hi_label)},
                  {L"\u4F60\u597D", GdipColor(hi_cand_text)},
                  {L" hao", GdipColor(hi_comment)}});
  x += spacing;

  // Remaining candidates.
  struct Candidate {
    const wchar_t* label;
    const wchar_t* text;
    const wchar_t* comment;
  };
  const Candidate candidates[] = {
      {L"2. ", L"\u4F60", L" ni"},
      {L"3. ", L"\u5462", L" ne"},
      {L"4. ", L"\u59AE", L" ni"},
      {L"5. ", L"\u5C3C", L" ni"},
  };
  for (const auto& c : candidates) {
    float cand_w = 56.0f * scale;
    RectF cr(x - 3 * scale, row_y - 3 * scale, cand_w + 6 * scale, row_h);
    if (IsVisible(cand_shadow)) {
      SolidBrush s(GdipColor(cand_shadow));
      RectF sr = cr;
      sr.Offset(2.0f * scale, 2.0f * scale);
      FillRoundedRect(g, s, sr, 3.0f * scale);
    }
    if (IsVisible(cand_back)) {
      SolidBrush b(GdipColor(cand_back));
      FillRoundedRect(g, b, cr, 3.0f * scale);
    }
    if (IsVisible(cand_border)) {
      Pen p(GdipColor(cand_border), (std::max)(1.0f, 1.0f * scale));
      g.DrawRectangle(&p, cr);
    }
    x = DrawChunks(g, cand_font, x, row_y,
                   {{c.label, GdipColor(label)},
                    {c.text, GdipColor(cand_text)},
                    {c.comment, GdipColor(comment)}});
    x += spacing;
  }

  // Paging arrows, right aligned.
  if (IsVisible(prev_page) || IsVisible(next_page)) {
    float arrow_x = window_rect.GetRight() - pad_x - 28.0f * scale;
    DrawChunks(g, cand_font, arrow_x, row_y,
               {{L"< ", GdipColor(IsVisible(prev_page) ? prev_page : text)},
                {L">", GdipColor(IsVisible(next_page) ? next_page : text)}});
  }
}

}  // namespace

void PaintMockCandidateWindow(HDC hdc,
                              const RECT& target,
                              const RgbaColorMap& colors) {
  const int width = target.right - target.left;
  const int height = target.bottom - target.top;
  if (width <= 0 || height <= 0)
    return;

  HDC mem_dc = CreateCompatibleDC(hdc);
  HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, width, height);
  HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(mem_dc, mem_bmp));
  {
    Graphics g(mem_dc);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    float dpi_scale = dpi > 0 ? static_cast<float>(dpi) / 96.0f : 1.0f;
    // The mock window is ~420x100 at scale 1; shrink it to fit the target
    // box in both dimensions so the candidate row is not clipped.
    float fit = (std::min)(static_cast<float>(width) / 420.0f,
                           static_cast<float>(height) / 100.0f);
    float scale = dpi_scale * (std::min)(1.0f, fit);
    RenderMockCandidateWindow(g, static_cast<float>(width),
                              static_cast<float>(height), colors, scale);
  }
  BitBlt(hdc, target.left, target.top, width, height, mem_dc, 0, 0, SRCCOPY);
  SelectObject(mem_dc, old_bmp);
  DeleteObject(mem_bmp);
  DeleteDC(mem_dc);
}

bool SavePreviewPng(const std::wstring& png_path,
                    int width,
                    int height,
                    const RgbaColorMap& colors) {
  Bitmap bitmap(width, height, PixelFormat32bppARGB);
  {
    Graphics g(&bitmap);
    // The mock window is ~420x100 at scale 1; shrink to fit the image in
    // both dimensions so candidates are not clipped.
    float fit = (std::min)(static_cast<float>(width) / 420.0f,
                           static_cast<float>(height) / 100.0f);
    float scale = (std::min)(1.0f, fit);
    RenderMockCandidateWindow(g, static_cast<float>(width),
                              static_cast<float>(height), colors, scale);
  }
  CLSID png_clsid;
  {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
      return false;
    std::vector<BYTE> buffer(size);
    auto* info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    Gdiplus::GetImageEncoders(num, size, info);
    bool found = false;
    for (UINT i = 0; i < num; ++i) {
      if (wcscmp(info[i].MimeType, L"image/png") == 0) {
        png_clsid = info[i].Clsid;
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }
  return bitmap.Save(png_path.c_str(), &png_clsid, nullptr) == Gdiplus::Ok;
}

}  // namespace weasel_deployer
