#include "stdafx.h"
#include <StringAlgorithm.hpp>
#include "Deserializer.h"
#include "ActionLoader.h"
#include <algorithm>

using namespace weasel;

Deserializer::Ptr ActionLoader::Create(ResponseParser* pTarget) {
  return Deserializer::Ptr(new ActionLoader(pTarget));
}

ActionLoader::ActionLoader(ResponseParser* pTarget) : Deserializer(pTarget) {}

ActionLoader::~ActionLoader() {}

void ActionLoader::Store(Deserializer::KeyType const& key,
                         std::wstring const& value) {
  if (key.size() == 1)  // no extention parts
  {
    // split value by L","
    std::vector<std::wstring> vecAction;
    split(vecAction, value, L",");

    // require specified action deserializers
    std::for_each(vecAction.begin(), vecAction.end(),
                  [this](std::wstring& action) {
                    Deserializer::Require(action, m_pTarget);
                  });
  }
}
