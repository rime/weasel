#include "stdafx.h"

#include <boost/detail/lightweight_test.hpp>

#include "../../WeaselServer/CredentialStore.h"

void test_credential_store_save_load_remove() {
  CredentialStore store;
  std::wstring target = L"assistant/test/openai";
  std::wstring secret = L"sk-test-123";

  BOOST_TEST(store.Save(target, secret));

  auto loaded = store.Load(target);
  BOOST_TEST(static_cast<bool>(loaded));
  if (loaded) {
    BOOST_TEST(*loaded == secret);
  }

  BOOST_TEST(store.Remove(target));

  auto missing = store.Load(target);
  BOOST_TEST(!missing.has_value());
}

void run_credential_store_tests() {
  test_credential_store_save_load_remove();
}
