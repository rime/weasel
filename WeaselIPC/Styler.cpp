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
  std::wstringstream ss(value);
  boost::archive::text_wiarchive ia(ss);

  TryDeserialize(ia, sty);
}

weasel::Deserializer::Ptr Styler::Create(weasel::ResponseParser* pTarget) {
  return Deserializer::Ptr(new Styler(pTarget));
}
