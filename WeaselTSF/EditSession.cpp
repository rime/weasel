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
			// ��������������� 4 ������ʱ����
			// �� 5 ������ʱ�Ὣ��ѡ�����
			// ��ʱ���ڵ���������룬composition Ӧ�ǿ����ģ�ͬʱҲҪ�����봦���붥�֡�
			// �����ȹر���һ���ֵ� composition��Ȼ��Ϊ�������뿪��һ���� composition��
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

