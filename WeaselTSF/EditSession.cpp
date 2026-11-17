#include "stdafx.h"
#include "WeaselTSF.h"
#include "CandidateList.h"
#include "ResponseParser.h"

STDMETHODIMP WeaselTSF::DoEditSession(TfEditCookie ec) {
  try {
    // get commit string from server
    std::wstring commit;
    weasel::Config config;
    auto context = std::make_shared<weasel::Context>();
    weasel::ResponseParser parser(&commit, context.get(), &_status, &config,
                                  &_cand->style());

    bool ok = m_client.GetResponseData(std::ref(parser));

    _UpdateLanguageBar(_status);

    bool compositionEnded = false;
    if (ok && _status.composing) {
      _pendingUIContext = context;
      _pendingUIStatus = _status;
      _hasPendingUI = true;
    }
    if (ok) {
      compositionEnded = false;
      if (!commit.empty()) {
        // For auto-selecting, commit and preedit can both exist.
        // Commit the old TSF composition. If Rime immediately has a new
        // preedit (top-word input), _EndComposition() drops the local pointer
        // synchronously, so the following state check starts a new TSF
        // composition instead of observing the old one.
        if (!_IsComposing()) {
          _StartComposition(_pEditSessionContext,
                            _fCUASWorkaroundEnabled && !config.inline_preedit);
        }
        _InsertText(_pEditSessionContext, commit);
        // Keep the candidate UI alive while the replacement composition is
        // being created; otherwise the key-down path destroys the old window
        // and the new one cannot be positioned until key-up.
        _EndComposition(_pEditSessionContext, false, !_status.composing);
        compositionEnded = true;
        _committed = TRUE;
      } else {
        _committed = FALSE;
      }
      if (_status.composing && (compositionEnded || !_IsComposing())) {
        _StartComposition(_pEditSessionContext,
                          _fCUASWorkaroundEnabled && !config.inline_preedit);
      } else if (!_status.composing && _IsComposing()) {
        _EndComposition(_pEditSessionContext, true);
      }
      if (_IsComposing() && config.inline_preedit) {
        _ShowInlinePreedit(_pEditSessionContext, context);
      }
    }

    // For a normal update, position the panel before publishing new candidates.
    // After a commit, the replacement composition's extent session flushes the
    // pending UI from _SetCompositionPosition().
    if (ok && _IsComposing() && !compositionEnded)
      _UpdateCompositionWindow(_pEditSessionContext);

    // Wait for the current composition's real text extent before painting new
    // candidates. This avoids one frame at the previous caret position.
    if (!(_status.composing && _IsComposing())) {
      _UpdateUI(*context, _status);
    }

    return TRUE;
  } catch (...) {
    // An exception out of an edit-session callback is delivered to the TSF
    // manager (msctf) and crashes the host process with 0xc000041d. Swallow
    // anything here so a malformed IPC response never takes down the app.
    // Keep the trace non-modal and non-throwing (issue #1949).
    // This is deliberately not a rollback: the TSF side effects already
    // performed in the try block (composition started or ended, text inserted,
    // language bar and candidate window updated) are left as they are, so the
    // local IME state can be inconsistent until the next edit session
    // resynchronises it. Host liveness is preferred over that consistency;
    // recovering at each stage is follow-up work and is not covered by this
    // barrier.
    OutputDebugStringW(L"weasel: DoEditSession swallowed an exception\n");
    return TRUE;
  }
}
