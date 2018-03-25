#include "stdafx.h"
#include "WeaselTSF.h"
#include "CandidateList.h"
#include "ResponseParser.h"

STDAPI WeaselTSF::DoEditSession(TfEditCookie ec)
{
	// get commit string from server
	std::wstring commit;
	weasel::Status status;
	weasel::Config config;

	auto context = std::make_shared<weasel::Context>();

	weasel::ResponseParser parser(&commit, context.get(), &status, &config);

	bool ok = m_client.GetResponseData(std::ref(parser));
	_cand->UpdateUI(*context, status);

	if (ok)
	{
		if (!commit.empty())
		{
			// For auto-selecting, commit and preedit can both exist.
			// Commit and close the original composition first.
			if (!_IsComposing()) {
				_StartComposition(_pEditSessionContext, _fCUASWorkaroundEnabled && !config.inline_preedit);
			}
			_InsertText(_pEditSessionContext, commit);
			_EndComposition(_pEditSessionContext, false);
		}
		if (status.composing && !_IsComposing())
		{
			_StartComposition(_pEditSessionContext, _fCUASWorkaroundEnabled && !config.inline_preedit);
		}
		else if (!status.composing && _IsComposing())
		{
			_EndComposition(_pEditSessionContext, true);
		}
		if (_IsComposing() && config.inline_preedit)
		{
			_ShowInlinePreedit(_pEditSessionContext, context);
		}
	}
	return TRUE;
}

