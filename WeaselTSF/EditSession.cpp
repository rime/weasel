#include "stdafx.h"
#include "WeaselTSF.h"
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

	if (ok)
	{
		if (!commit.empty())
		{
			// 顶字上屏（如五笔 4 码上屏时），
			// 第 5 码输入时会将首选项顶屏。
			// 此时由于第五码的输入，composition 应是开启的，同时也要在输入处插入顶字。
			// 这里先关闭上一个字的 composition，然后为后续输入开启一个新 composition。
			if (_IsComposing()) {
				_EndComposition(_pEditSessionContext);
			}
			_InsertText(_pEditSessionContext, commit);
		}
		if (status.composing && !_IsComposing())
		{
			if (!_fCUASWorkaroundTested)
			{
				/* Test if we need to apply the workaround */
				_UpdateCompositionWindow(_pEditSessionContext);
			}
			else if (!_fCUASWorkaroundEnabled || config.inline_preedit)
			{
				/* Workaround not applied, update candidate window position at this point. */
				_UpdateCompositionWindow(_pEditSessionContext);
			}
			_StartComposition(_pEditSessionContext, _fCUASWorkaroundEnabled && !config.inline_preedit);
		}
		else if (!status.composing && _IsComposing())
		{
			_EndComposition(_pEditSessionContext);
		}
		if (_IsComposing() && config.inline_preedit)
		{
			_ShowInlinePreedit(_pEditSessionContext, context);
		}
	}
	return TRUE;
}

