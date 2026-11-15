#include "stdafx.h"
#include "Deserializer.h"
#include "Styler.h"

using namespace weasel;

Styler::Styler(weasel::ResponseParser* pTarget) : Deserializer(pTarget) {}

Styler::~Styler() {}

void Styler::Store(weasel::Deserializer::KeyType const& key,
                   std::wstring const& value) {
  if (!m_pTarget->p_style)
    return;

  UIStyle& sty = *m_pTarget->p_style;
  try {
    std::wstringstream ss(value);
    boost::archive::text_wiarchive ia(ss);
    // Deserialize into a temporary and publish it only on full success; a
    // half-updated UIStyle would otherwise leak into the candidate UI.
    UIStyle temp = sty;
    if (!TryDeserialize(ia, temp))
      return;
    sty = temp;
  } catch (...) {
    // Stream/archive construction threw: leave the style unchanged.
    return;
  }
}

weasel::Deserializer::Ptr Styler::Create(weasel::ResponseParser* pTarget) {
  return Deserializer::Ptr(new Styler(pTarget));
}
