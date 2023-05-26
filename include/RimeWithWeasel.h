#pragma once
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <map>
#include <string>

#include <rime_api.h>

typedef std::map<std::string, bool> AppOptions;
typedef std::map<std::string, AppOptions> AppOptionsByAppName;
class RimeWithWeaselHandler :
	public weasel::RequestHandler
{
public:
	RimeWithWeaselHandler(weasel::UI *ui);
	virtual ~RimeWithWeaselHandler();
	virtual void Initialize();
	virtual void Finalize();
	virtual UINT FindSession(UINT session_id);
	virtual UINT AddSession(LPWSTR buffer, EatLine eat = 0);
	virtual UINT RemoveSession(UINT session_id);
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, EatLine eat);
	virtual void CommitComposition(UINT session_id);
	virtual void ClearComposition(UINT session_id);
	virtual void FocusIn(DWORD param, UINT session_id);
	virtual void FocusOut(DWORD param, UINT session_id);
	virtual void UpdateInputPosition(RECT const& rc, UINT session_id);
	virtual void StartMaintenance();
	virtual void EndMaintenance();
	virtual void SetOption(UINT session_id, const std::string &opt, bool val);

	void OnUpdateUI(std::function<void()> const &cb);

private:
	void _Setup();
	bool _IsDeployerRunning();
	void _UpdateUI(UINT session_id);
	void _LoadSchemaSpecificSettings(const std::string& schema_id);
	bool _ShowMessage(weasel::Context& ctx, weasel::Status& status);
	bool _Respond(UINT session_id, EatLine eat);
	void _ReadClientInfo(UINT session_id, LPWSTR buffer);
	void _GetCandidateInfo(weasel::CandidateInfo &cinfo, RimeContext &ctx);
	void _GetStatus(weasel::Status &stat, UINT session_id);
	void _GetContext(weasel::Context &ctx, UINT session_id);

	bool _IsSessionTSF(UINT session_id);
	void _UpdateInlinePreeditStatus(UINT session_id);

	AppOptionsByAppName m_app_options;
	weasel::UI* m_ui;  // reference
	UINT m_active_session;
	bool m_disabled;
	std::string m_last_schema_id;
	weasel::UIStyle m_base_style;
#ifdef USE_THEME_DARK
	weasel::UIStyle m_base_style_dark;
#endif

	std::function<void()> _UpdateUICallback;

	static void OnNotify(void* context_object,
		                 uintptr_t session_id,
                         const char* message_type,
                         const char* message_value);
	static std::string m_message_type;
	static std::string m_message_value;
};
