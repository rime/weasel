#include "stdafx.h"
#include <logging.h>
#include <RimeWithWeasel.h>
#include <StringAlgorithm.hpp>
#include <WeaselUtility.h>
#include <WeaselVersion.h>
#include <algorithm>
#include <list>
#include <set>
#include <string>

#pragma warning(disable: 4005)
#include <rime_api.h>
#pragma warning(default: 4005)

int expand_ibus_modifier(int m)
{
	return (m & 0xff) | ((m & 0xff00) << 16);
}

RimeWithWeaselHandler::RimeWithWeaselHandler(weasel::UI *ui)
	: m_ui(ui), m_active_session(0), m_disabled(true)
{
	_Setup();
}

RimeWithWeaselHandler::~RimeWithWeaselHandler()
{
}

void _UpdateUIStyle(RimeConfig* config, weasel::UI* ui, bool initialize);
void _LoadAppOptions(RimeConfig* config, AppOptionsByAppName& app_options);

void RimeWithWeaselHandler::_Setup()
{
	RIME_STRUCT(RimeTraits, weasel_traits);
	weasel_traits.shared_data_dir = weasel_shared_data_dir();
	weasel_traits.user_data_dir = weasel_user_data_dir();
	const int len = 20;
	char utf8_str[len];
	memset(utf8_str, 0, sizeof(utf8_str));
	WideCharToMultiByte(CP_UTF8, 0, WEASEL_IME_NAME, -1, utf8_str, len - 1, NULL, NULL);
	weasel_traits.distribution_name = utf8_str;
	weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
	weasel_traits.distribution_version = WEASEL_VERSION;
	weasel_traits.app_name = "rime.weasel";
	RimeSetup(&weasel_traits);
	RimeSetNotificationHandler(&RimeWithWeaselHandler::OnNotify, this);
}

void RimeWithWeaselHandler::Initialize()
{
	m_disabled = _IsDeployerRunning();
	if (m_disabled)
	{
		 return;
	}

	LOG(INFO) << "Initializing la rime.";
	RimeInitialize(NULL);
	if (RimeStartMaintenance(/*full_check = */False))
	{
		m_disabled = true;
	}

	RimeConfig config = { NULL };
	if (RimeConfigOpen("weasel", &config))
	{
		if (m_ui)
		{
			_UpdateUIStyle(&config, m_ui, true);
			m_base_style = m_ui->style();
		}
		_LoadAppOptions(&config, m_app_options);
		RimeConfigClose(&config);
	}
	m_last_schema_id.clear();
}

void RimeWithWeaselHandler::Finalize()
{
	m_active_session = 0;
	m_disabled = true;
	LOG(INFO) << "Finalizing la rime.";
	RimeFinalize();
}

UINT RimeWithWeaselHandler::FindSession(UINT session_id)
{
	if (m_disabled) return 0;
	Bool found = RimeFindSession(session_id);
	DLOG(INFO) << "Find session: session_id = " << session_id << ", found = " << found;
	return found ? session_id : 0;
}

UINT RimeWithWeaselHandler::AddSession(LPWSTR buffer)
{
	if (m_disabled)
	{
		DLOG(INFO) << "Trying to resume service.";
		EndMaintenance();
		if (m_disabled) return 0;
	}
	UINT session_id = RimeCreateSession();
	DLOG(INFO) << "Add session: created session_id = " << session_id;
	_ReadClientInfo(session_id, buffer);
	// show session's welcome message :-) if any
	_UpdateUI(session_id);
	m_active_session = session_id;
	return session_id;
}

UINT RimeWithWeaselHandler::RemoveSession(UINT session_id)
{
	if (m_ui) m_ui->Hide();
	if (m_disabled) return 0;
	DLOG(INFO) << "Remove session: session_id = " << session_id;
	// TODO: force committing? otherwise current composition would be lost
	RimeDestroySession(session_id);
	m_active_session = 0;
	return 0;
}

namespace ibus
{
	enum Keycode
	{
		Escape = 0xFF1B,
	};
}

BOOL RimeWithWeaselHandler::ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, EatLine eat)
{
	DLOG(INFO) << "Process key event: keycode = " << keyEvent.keycode << ", mask = " << keyEvent.mask
		 << ", session_id = " << session_id;
	if (m_disabled) return FALSE;
	Bool handled = RimeProcessKey(session_id, keyEvent.keycode, expand_ibus_modifier(keyEvent.mask));
	_Respond(session_id, eat);
	_UpdateUI(session_id);
	m_active_session = session_id;
	return (BOOL)handled;
}

void RimeWithWeaselHandler::CommitComposition(UINT session_id)
{
	DLOG(INFO) << "Commit composition: session_id = " << session_id;
	if (m_disabled) return;
	RimeCommitComposition(session_id);
	_UpdateUI(session_id);
	m_active_session = session_id;
}

void RimeWithWeaselHandler::ClearComposition(UINT session_id)
{
	DLOG(INFO) << "Clear composition: session_id = " << session_id;
	if (m_disabled) return;
	RimeClearComposition(session_id);
	_UpdateUI(session_id);
	m_active_session = session_id;
}

void RimeWithWeaselHandler::FocusIn(DWORD client_caps, UINT session_id)
{
	DLOG(INFO) << "Focus in: session_id = " << session_id << ", client_caps = " << client_caps;
	if (m_disabled) return;
	_UpdateUI(session_id);
	m_active_session = session_id;
}

void RimeWithWeaselHandler::FocusOut(DWORD param, UINT session_id)
{
	DLOG(INFO) << "Focus out: session_id = " << session_id;
	if (m_ui) m_ui->Hide();
	m_active_session = 0;
}

void RimeWithWeaselHandler::UpdateInputPosition(RECT const& rc, UINT session_id)
{
	DLOG(INFO) << "Update input position: (" << rc.left << ", " << rc.top
		<< "), session_id = " << session_id << ", m_active_session = " << m_active_session;
	if (m_ui) m_ui->UpdateInputPosition(rc);
	if (m_disabled) return;
	if (m_active_session != session_id)
	{
		_UpdateUI(session_id);
		m_active_session = session_id;
	}
}

std::string RimeWithWeaselHandler::m_message_type;
std::string RimeWithWeaselHandler::m_message_value;

void RimeWithWeaselHandler::OnNotify(void* context_object,
	                                 uintptr_t session_id,
                                     const char* message_type,
                                     const char* message_value)
{
	// may be running in a thread when deploying rime
	RimeWithWeaselHandler* self = reinterpret_cast<RimeWithWeaselHandler*>(context_object);
	if (!self || !message_type || !message_value) return;
	m_message_type = message_type;
	m_message_value = message_value;
}

void RimeWithWeaselHandler::_ReadClientInfo(UINT session_id, LPWSTR buffer)
{
	std::string app_name;
	std::string client_type;
	// parse request text
	wbufferstream bs(buffer, WEASEL_IPC_BUFFER_LENGTH);
	std::wstring line;
	while (bs.good())
	{
		std::getline(bs, line);
		if (!bs.good())
			break;
		// file ends
		if (line == L".")
			break;
		const std::wstring kClientAppKey = L"session.client_app=";
		if (starts_with(line, kClientAppKey))
		{
			std::wstring lwr = line;
			to_lower(lwr);
			app_name = wcstoutf8(lwr.substr(kClientAppKey.length()).c_str());
		}
		const std::wstring kClientTypeKey = L"session.client_type=";
		if (starts_with(line, kClientTypeKey))
		{
			client_type = wcstoutf8(line.substr(kClientTypeKey.length()).c_str());
		}
	}
    // set app specific options
	if (!app_name.empty())
	{
		RimeSetProperty(session_id, "client_app", app_name.c_str());

		if (m_app_options.find(app_name) != m_app_options.end())
		{
			AppOptions& options(m_app_options[app_name]);
			std::for_each(options.begin(), options.end(), [session_id](std::pair<const std::string, bool> &pair)
			{
				DLOG(INFO) << "set app option: " << pair.first << " = " << pair.second;
				RimeSetOption(session_id, pair.first.c_str(), Bool(pair.second));
			});
		}
	}
	// ime | tsf
	RimeSetProperty(session_id, "client_type", client_type.c_str());
	// inline preedit
	bool inline_preedit = m_ui->style().inline_preedit && (client_type == "tsf");	
	RimeSetOption(session_id, "inline_preedit", Bool(inline_preedit));
	// show soft cursor on weasel panel but not inline
	RimeSetOption(session_id, "soft_cursor", Bool(!inline_preedit));
}

void RimeWithWeaselHandler::StartMaintenance()
{
	Finalize();
	_UpdateUI(0);
}

void RimeWithWeaselHandler::EndMaintenance()
{
	if (m_disabled)
	{
		Initialize();
		_UpdateUI(0);
	}
}

bool RimeWithWeaselHandler::_IsDeployerRunning()
{
	HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
	bool deployer_detected = hMutex && GetLastError() == ERROR_ALREADY_EXISTS;
	if (hMutex)
	{
		CloseHandle(hMutex);
	}
	return deployer_detected;
}


void RimeWithWeaselHandler::_UpdateUI(UINT session_id)
{
	weasel::Status weasel_status;
	weasel::Context weasel_context;

	if (session_id == 0)
		weasel_status.disabled = m_disabled;

	RIME_STRUCT(RimeStatus, status);
	if (RimeGetStatus(session_id, &status))
	{
		std::string schema_id = status.schema_id;
		if (schema_id != m_last_schema_id)
		{
			m_last_schema_id = schema_id;
			_LoadSchemaSpecificSettings(schema_id);
		}
		weasel_status.schema_name = utf8towcs(status.schema_name);
		weasel_status.ascii_mode = !!status.is_ascii_mode;
		weasel_status.composing = !!status.is_composing;
		weasel_status.disabled = !!status.is_disabled;
		RimeFreeStatus(&status);
	}

	RIME_STRUCT(RimeContext, ctx);
	if (RimeGetContext(session_id, &ctx))
	{
		if (ctx.composition.length > 0)
		{
			weasel_context.preedit.str = utf8towcs(ctx.composition.preedit);
			if (ctx.composition.sel_start < ctx.composition.sel_end)
			{
				weasel::TextAttribute attr;
				attr.type = weasel::HIGHLIGHTED;
				attr.range.start = utf8towcslen(ctx.composition.preedit, ctx.composition.sel_start);
				attr.range.end = utf8towcslen(ctx.composition.preedit, ctx.composition.sel_end);
			
				weasel_context.preedit.attributes.push_back(attr);
			}
		}
		if (ctx.menu.num_candidates)
		{
			weasel::CandidateInfo &cinfo(weasel_context.cinfo);
			cinfo.candies.resize(ctx.menu.num_candidates);
			cinfo.comments.resize(ctx.menu.num_candidates);
			cinfo.labels.resize(ctx.menu.num_candidates);
			for (int i = 0; i < ctx.menu.num_candidates; ++i)
			{
				cinfo.candies[i].str = utf8towcs(ctx.menu.candidates[i].text);
				if (ctx.menu.candidates[i].comment)
				{
					cinfo.comments[i].str = utf8towcs(ctx.menu.candidates[i].comment);
				}
				if (RIME_STRUCT_HAS_MEMBER(ctx, ctx.select_labels) && ctx.select_labels)
				{
					cinfo.labels[i].str = utf8towcs(ctx.select_labels[i]);
				}
				else if (ctx.menu.select_keys)
				{
					cinfo.labels[i].str = std::wstring(1, ctx.menu.select_keys[i]);
				}
				else
				{
					cinfo.labels[i].str = std::to_wstring((i + 1) % 10);
				}
			}
			cinfo.highlighted = ctx.menu.highlighted_candidate_index;
			cinfo.currentPage = ctx.menu.page_no;
		}
		RimeFreeContext(&ctx);
	}

	if (!m_ui) return;
	if (RimeGetOption(session_id, "inline_preedit"))
		m_ui->style().client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
	else
		m_ui->style().client_caps &= ~weasel::INLINE_PREEDIT_CAPABLE;
	if (weasel_status.composing)
	{
		m_ui->Update(weasel_context, weasel_status);
		m_ui->Show();
	}
	else if (!_ShowMessage(weasel_context, weasel_status))
	{
		m_ui->Hide();
		m_ui->Update(weasel_context, weasel_status);
	}

	m_message_type.clear();
	m_message_value.clear();
}

void RimeWithWeaselHandler::_LoadSchemaSpecificSettings(const std::string& schema_id)
{
	if (!m_ui) return;
	RimeConfig config;
	if (!RimeSchemaOpen(schema_id.c_str(), &config))
		return;
	m_ui->style() = m_base_style;
	_UpdateUIStyle(&config, m_ui, false);
	RimeConfigClose(&config);
}

bool RimeWithWeaselHandler::_ShowMessage(weasel::Context& ctx, weasel::Status& status) {
	// show as auxiliary string
	std::wstring& tips(ctx.aux.str);
	bool show_icon = false;
	if (m_message_type == "deploy") {
		if (m_message_type == "start")
			tips = L"正在部署 RIME";
		else if (m_message_value == "success")
			tips = L"部署完成";
		else if (m_message_value == "failure")
			tips = L"有錯誤，請查看日誌 %TEMP%\rime.weasel.*.INFO";
	}
	else if (m_message_type == "schema") {
		tips = /*L"【" + */status.schema_name/* + L"】"*/;
	}
	else if (m_message_type == "option") {
		if (m_message_value == "!ascii_mode")
			show_icon = true;  //tips = L"中文";
		else if (m_message_value == "ascii_mode")
			show_icon = true;  //tips = L"西文";
		else if (m_message_value == "!full_shape")
			tips = L"半角";
		else if (m_message_value == "full_shape")
			tips = L"全角";
		else if (m_message_value == "!ascii_punct")
			tips = L"，。";
		else if (m_message_value == "ascii_punct")
			tips = L"，．";
		else if (m_message_value == "!simplification")
			tips = L"漢字";
		else if (m_message_value == "simplification")
			tips = L"汉字";
	}
	if (tips.empty() && !show_icon)
		return m_ui->IsCountingDown();

	m_ui->Update(ctx, status);
	m_ui->ShowWithTimeout(1200 + 200 * tips.length());
	return true;
}

bool RimeWithWeaselHandler::_Respond(UINT session_id, EatLine eat)
{
	std::set<std::string> actions;
	std::list<std::string> messages;

	// extract information

	RIME_STRUCT(RimeCommit, commit);
	if (RimeGetCommit(session_id, &commit))
	{
		actions.insert("commit");
		messages.push_back(std::string("commit=") + commit.text + '\n');
		RimeFreeCommit(&commit);
	}
	
	bool is_composing;
	RIME_STRUCT(RimeStatus, status);
	if (RimeGetStatus(session_id, &status))
	{
		is_composing = !!status.is_composing;
		actions.insert("status");
		messages.push_back(std::string("status.ascii_mode=") + std::to_string(status.is_ascii_mode) + '\n');
		messages.push_back(std::string("status.composing=") + std::to_string(status.is_composing) + '\n');
		messages.push_back(std::string("status.disabled=") + std::to_string(status.is_disabled) + '\n');
		RimeFreeStatus(&status);
	}
	
	RIME_STRUCT(RimeContext, ctx);
	if (RimeGetContext(session_id, &ctx))
	{
		if (is_composing)
		{
			actions.insert("ctx");
			switch (m_ui->style().preedit_type)
			{
			case weasel::PREVIEW:
				if (ctx.menu.num_candidates > 0)
				{
					std::string first = ctx.commit_text_preview;
					messages.push_back(std::string("ctx.preedit=") + first + '\n');
					messages.push_back(std::string("ctx.preedit.cursor=") +
						std::to_string(utf8towcslen(first.c_str(), 0)) + ',' +
						std::to_string(utf8towcslen(first.c_str(), first.size())) + '\n');
					break;
				}
				// no candidates, fall back to composition
			case weasel::COMPOSITION:
				messages.push_back(std::string("ctx.preedit=") + ctx.composition.preedit + '\n');
				if (ctx.composition.sel_start <= ctx.composition.sel_end)
				{
					messages.push_back(std::string("ctx.preedit.cursor=") +
						std::to_string(utf8towcslen(ctx.composition.preedit, ctx.composition.sel_start)) + ',' +
						std::to_string(utf8towcslen(ctx.composition.preedit, ctx.composition.sel_end)) + '\n');
				}
				break;
			}
		}
		RimeFreeContext(&ctx);
	}

	// configuration information
	actions.insert("config");
	messages.push_back(std::string("config.inline_preedit=") + std::to_string((int)m_ui->style().inline_preedit) + '\n');

	// summarize

	if (actions.empty())
	{
		messages.insert(messages.begin(), std::string("action=noop\n"));
	}
	else
	{
		std::string actionList(join(actions, ","));
		messages.insert(messages.begin(), std::string("action=") + actionList + '\n');
	}

	messages.push_back(std::string(".\n"));

	return std::all_of(messages.begin(), messages.end(), [&eat](std::string &msg)
	{
		return eat(std::wstring(utf8towcs(msg.c_str())));
	});
}

static inline COLORREF blend_colors(COLORREF fcolor, COLORREF bcolor)
{
	return RGB(
		(GetRValue(fcolor) * 2 + GetRValue(bcolor)) / 3,
		(GetGValue(fcolor) * 2 + GetGValue(bcolor)) / 3,
		(GetBValue(fcolor) * 2 + GetBValue(bcolor)) / 3
		);
}

static void _UpdateUIStyle(RimeConfig* config, weasel::UI* ui, bool initialize)
{
	if (!ui) return;
	weasel::UIStyle &style(ui->style());

	const int BUF_SIZE = 99;
	char buffer[BUF_SIZE + 1];
	memset(buffer, '\0', sizeof(buffer));
	if (RimeConfigGetString(config, "style/font_face", buffer, BUF_SIZE))
	{
		style.font_face = utf8towcs(buffer);
	}
	RimeConfigGetInt(config, "style/font_point", &style.font_point);
	Bool inline_preedit = False;
	if (RimeConfigGetBool(config, "style/inline_preedit", &inline_preedit) || initialize)
	{
		style.inline_preedit = !!inline_preedit;
	}
	char preedit_type[20] = { 0 };
	if (RimeConfigGetString(config, "style/preedit_type", preedit_type, sizeof(preedit_type) - 1))
	{
		if (!std::strcmp(preedit_type, "composition"))
			style.preedit_type = weasel::COMPOSITION;
		else if (!std::strcmp(preedit_type, "preview"))
			style.preedit_type = weasel::PREVIEW;
	}
	Bool display_tray_icon = False;
	if (RimeConfigGetBool(config, "style/display_tray_icon", &display_tray_icon) || initialize)
	{
		style.display_tray_icon = !!display_tray_icon;
	}
	Bool horizontal = False;
	if (RimeConfigGetBool(config, "style/horizontal", &horizontal) || initialize)
	{
		style.layout_type = horizontal ? weasel::LAYOUT_HORIZONTAL : weasel::LAYOUT_VERTICAL;
	}
	Bool fullscreen = False;
	if (RimeConfigGetBool(config, "style/fullscreen", &fullscreen) && fullscreen)
	{
		style.layout_type = (style.layout_type == weasel::LAYOUT_HORIZONTAL)
			 ? weasel::LAYOUT_HORIZONTAL_FULLSCREEN : weasel::LAYOUT_VERTICAL_FULLSCREEN;
	}
	// layout (alternative to style/horizontal)
	char layout_type[256] = {0};
	if (RimeConfigGetString(config, "style/layout/type", layout_type, sizeof(layout_type) - 1))
	{
		if (!std::strcmp(layout_type, "vertical"))
			style.layout_type = weasel::LAYOUT_VERTICAL;
		else if (!std::strcmp(layout_type, "horizontal"))
			style.layout_type = weasel::LAYOUT_HORIZONTAL;
		if (!std::strcmp(layout_type, "vertical+fullscreen"))
			style.layout_type = weasel::LAYOUT_VERTICAL_FULLSCREEN;
		else if (!std::strcmp(layout_type, "horizontal+fullscreen"))
			style.layout_type = weasel::LAYOUT_HORIZONTAL_FULLSCREEN;
		else
			LOG(WARNING) << "Invalid style type: " << layout_type;
	}
	RimeConfigGetInt(config, "style/layout/min_width", &style.min_width);
	RimeConfigGetInt(config, "style/layout/min_height", &style.min_height);
	if (!RimeConfigGetInt(config, "style/layout/border", &style.border)) {
		RimeConfigGetInt(config, "style/layout/border_width", &style.border);
	}
	RimeConfigGetInt(config, "style/layout/margin_x", &style.margin_x);
	RimeConfigGetInt(config, "style/layout/margin_y", &style.margin_y);
	RimeConfigGetInt(config, "style/layout/spacing", &style.spacing);
	RimeConfigGetInt(config, "style/layout/candidate_spacing", &style.candidate_spacing);
	RimeConfigGetInt(config, "style/layout/hilite_spacing", &style.hilite_spacing);
	RimeConfigGetInt(config, "style/layout/hilite_padding", &style.hilite_padding);
	RimeConfigGetInt(config, "style/layout/round_corner", &style.round_corner);
	// color scheme
	if (initialize && RimeConfigGetString(config, "style/color_scheme", buffer, BUF_SIZE))
	{
		std::string prefix("preset_color_schemes/");
		prefix += buffer;
		RimeConfigGetInt(config, (prefix + "/text_color").c_str(), &style.text_color);
		if (!RimeConfigGetInt(config, (prefix + "/candidate_text_color").c_str(), &style.candidate_text_color))
		{
			style.candidate_text_color = style.text_color;
		}
		RimeConfigGetInt(config, (prefix + "/back_color").c_str(), &style.back_color);
		if (!RimeConfigGetInt(config, (prefix + "/border_color").c_str(), &style.border_color))
		{
			style.border_color = style.text_color;
		}
		if (!RimeConfigGetInt(config, (prefix + "/hilited_text_color").c_str(), &style.hilited_text_color))
		{
			style.hilited_text_color = style.text_color;
		}
		if (!RimeConfigGetInt(config, (prefix + "/hilited_back_color").c_str(), &style.hilited_back_color))
		{
			style.hilited_back_color = style.back_color;
		}
		if (!RimeConfigGetInt(config, (prefix + "/hilited_candidate_text_color").c_str(), &style.hilited_candidate_text_color))
		{
			style.hilited_candidate_text_color = style.hilited_text_color;
		}
		if (!RimeConfigGetInt(config, (prefix + "/hilited_candidate_back_color").c_str(), &style.hilited_candidate_back_color))
		{
			style.hilited_candidate_back_color = style.hilited_back_color;
		}
		if (!RimeConfigGetInt(config, (prefix + "/label_color").c_str(), &style.label_text_color))
		{
			style.label_text_color = blend_colors(style.candidate_text_color, style.back_color);
		}
		if (!RimeConfigGetInt(config, (prefix + "/hilited_label_color").c_str(), &style.hilited_label_text_color))
		{
			style.hilited_label_text_color = blend_colors(style.hilited_candidate_text_color, style.hilited_candidate_back_color);
		}
		style.comment_text_color = style.label_text_color;
		style.hilited_comment_text_color = style.hilited_label_text_color;
		if (RimeConfigGetInt(config, (prefix + "/comment_text_color").c_str(), &style.comment_text_color))
		{
			style.hilited_comment_text_color = style.comment_text_color;
		}
		RimeConfigGetInt(config, (prefix + "/hilited_comment_text_color").c_str(), &style.hilited_comment_text_color);
	}
}

static void _LoadAppOptions(RimeConfig* config, AppOptionsByAppName& app_options)
{
	app_options.clear();
	RimeConfigIterator app_iter;
	RimeConfigIterator option_iter;
	RimeConfigBeginMap(&app_iter, config, "app_options");
	while (RimeConfigNext(&app_iter)) {
		AppOptions &options(app_options[app_iter.key]);
		RimeConfigBeginMap(&option_iter, config, app_iter.path);
		while (RimeConfigNext(&option_iter)) {
			Bool value = False;
			if (RimeConfigGetBool(config, option_iter.path, &value)) {
				options[option_iter.key] = !!value;
			}
		}
		RimeConfigEnd(&option_iter);
	}
	RimeConfigEnd(&app_iter);
}
