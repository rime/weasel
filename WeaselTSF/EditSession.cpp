#include "stdafx.h"
#include "WeaselTSF.h"
#include "CandidateList.h"
#include "ResponseParser.h"

STDAPI WeaselTSF::DoEditSession(TfEditCookie ec)
{
	// get commit string from server
	std::wstring commit;
	auto context = std::make_shared<weasel::Context>();
	weasel::ResponseParser parser(&commit, context.get(), &_status, &_config, &_cand->style());

	bool ok = m_client.GetResponseData(std::ref(parser));


	if (ok)
	{
		_SyncAsciiMode(_status.ascii_mode);

		if (!commit.empty())
		{
			// For auto-selecting, commit and preedit can both exist.
			// Commit and close the original composition first.
			if (!_IsComposing()) {
				_StartComposition(_pEditSessionContext, _fCUASWorkaroundEnabled && !_config.inline_preedit);
			}
			_InsertText(_pEditSessionContext, commit);
			_EndComposition(_pEditSessionContext, false);
		}
		if (_status.composing && !_IsComposing())
		{
			_StartComposition(_pEditSessionContext, _fCUASWorkaroundEnabled && !_config.inline_preedit);
		}
		else if (!_status.composing && _IsComposing())
		{
			_EndComposition(_pEditSessionContext, true);
		}
		_UpdateCompositionWindow(_pEditSessionContext);
		if (_IsComposing() && _config.inline_preedit)
		{
			_ShowInlinePreedit(_pEditSessionContext, context);
		}
	}

	_UpdateUI(*context, _status);

	return TRUE;
}

