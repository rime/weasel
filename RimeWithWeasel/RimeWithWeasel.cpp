#include "stdafx.h"
#include <logging.h>
#include <RimeWithWeasel.h>
#include <StringAlgorithm.hpp>
#include <WeaselConstants.h>
#include <WeaselUtility.h>
#include <boost/algorithm/string.hpp>
#include <vector>

#include <filesystem>
#include <map>
#include <regex>
#include <rime_api.h>

#define TRANSPARENT_COLOR 0x00000000
#define ARGB2ABGR(value)                                 \
  ((value & 0xff000000) | ((value & 0x000000ff) << 16) | \
   (value & 0x0000ff00) | ((value & 0x00ff0000) >> 16))
#define RGBA2ABGR(value)                                   \
  (((value & 0xff) << 24) | ((value & 0xff000000) >> 24) | \
   ((value & 0x00ff0000) >> 8) | ((value & 0x0000ff00) << 8))
typedef enum { COLOR_ABGR = 0, COLOR_ARGB, COLOR_RGBA } ColorFormat;

#ifdef USE_SHARP_COLOR_CODE
#define HEX_REGEX std::regex("^(0x|#)[0-9a-f]+$", std::regex::icase)
#define TRIMHEAD_REGEX std::regex("0x|#", std::regex::icase)
#else
#define HEX_REGEX std::regex("^0x[0-9a-f]+$", std::regex::icase)
#define TRIMHEAD_REGEX std::regex("0x", std::regex::icase)
#endif
using namespace weasel;

static RimeApi* rime_api;
WeaselSessionId _GenerateNewWeaselSessionId(SessionStatusMap sm, DWORD pid) {
  if (sm.empty())
    return (WeaselSessionId)(pid + 1);
  return (WeaselSessionId)(sm.rbegin()->first + 1);
}

int expand_ibus_modifier(int m) {
  return (m & 0xff) | ((m & 0xff00) << 16);
}

static void CleanOldLogs() {
  char ymd[12] = {0};
  time_t now = time(NULL);
  strftime(ymd, sizeof(ymd), ".%Y%m%d", localtime(&now));
  std::string today(ymd);
  const std::string app_name = "rime.weasel";
  std::string dir = WeaselLogPath().string();
  if (!fs::exists(fs::path(dir)))
    return;
  std::vector<fs::path> files;
  try {
    // preparing files
    for (const auto& entry : fs::directory_iterator(dir)) {
      const std::string& file_name(entry.path().filename().string());
      if (entry.is_regular_file() && !entry.is_symlink() &&
          boost::starts_with(file_name, app_name) &&
          boost::ends_with(file_name, ".log") &&
          !boost::contains(file_name, today)) {
        files.push_back(entry.path());
      }
    }
  } catch (const fs::filesystem_error&) {
  }
  // remove files
  for (const auto& file : files) {
    try {
      // ensure write permission
      fs::permissions(file, fs::perms::owner_write);
      fs::remove(file);
    } catch (const fs::filesystem_error&) {
    }
  }
}

RimeWithWeaselHandler::RimeWithWeaselHandler(UI* ui)
    : m_ui(ui),
      m_active_session(0),
      m_disabled(true),
      m_current_dark_mode(false),
      m_global_ascii_mode(false),
      m_show_notifications_time(1200),
      _UpdateUICallback(NULL) {
  rime_api = rime_get_api();
  assert(rime);
  m_pid = GetCurrentProcessId();
  uint16_t msbit = 0;
  for (auto i = 31; i >= 0; i--) {
    if (m_pid & (1 << i)) {
      msbit = i;
      break;
    }
  }
  m_pid = (m_pid << (31 - msbit));
  _Setup();
}

RimeWithWeaselHandler::~RimeWithWeaselHandler() {
  m_show_notifications.clear();
  m_session_status_map.clear();
  m_app_options.clear();
}

bool add_session = false;
void _UpdateUIStyle(RimeConfig* config, UI* ui, bool initialize);
bool _UpdateUIStyleColor(RimeConfig* config,
                         UIStyle& style,
                         std::string color = "");
void _LoadAppOptions(RimeConfig* config, AppOptionsByAppName& app_options);

void _RefreshTrayIcon(const RimeSessionId session_id,
                      const std::function<void()> _UpdateUICallback) {
  // Dangerous, don't touch
  static char app_name[50];
  rime_api->get_property(session_id, "client_app", app_name,
                         sizeof(app_name) - 1);
  if (string_to_wstring(app_name, CP_UTF8) == std::wstring(L"explorer.exe"))
    boost::thread th([=]() {
      ::Sleep(100);
      if (_UpdateUICallback)
        _UpdateUICallback();
    });
  else if (_UpdateUICallback)
    _UpdateUICallback();
}

void RimeWithWeaselHandler::_Setup() {
  RIME_STRUCT(RimeTraits, weasel_traits);
  std::string shared_dir =
      wstring_to_string(WeaselSharedDataPath().wstring(), CP_UTF8);
  std::string user_dir =
      wstring_to_string(WeaselUserDataPath().wstring(), CP_UTF8);
  weasel_traits.shared_data_dir = shared_dir.c_str();
  weasel_traits.user_data_dir = user_dir.c_str();
  weasel_traits.prebuilt_data_dir = weasel_traits.shared_data_dir;
  std::string distribution_name =
      wstring_to_string(get_weasel_ime_name(), CP_UTF8);
  weasel_traits.distribution_name = distribution_name.c_str();
  weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
  weasel_traits.distribution_version = WEASEL_VERSION;
  weasel_traits.app_name = "rime.weasel";
  std::string log_dir = WeaselLogPath().u8string();
  weasel_traits.log_dir = log_dir.c_str();
  rime_api->setup(&weasel_traits);
  rime_api->set_notification_handler(&RimeWithWeaselHandler::OnNotify, this);
}

void RimeWithWeaselHandler::Initialize() {
  m_disabled = _IsDeployerRunning();
  if (m_disabled) {
    return;
  }

  LOG(INFO) << "Initializing la rime.";
  rime_api->initialize(NULL);
#if 0
  if (rime_api->start_maintenance(/*full_check = */ False)) {
    m_disabled = true;
  }
#endif
  CleanOldLogs();

  RimeConfig config = {NULL};
  if (rime_api->config_open("weasel", &config)) {
    if (m_ui) {
      _UpdateUIStyle(&config, m_ui, true);
      _UpdateShowNotifications(&config, true);
      m_current_dark_mode = IsUserDarkMode();
      if (m_current_dark_mode) {
        const int BUF_SIZE = 255;
        char buffer[BUF_SIZE + 1] = {0};
        if (rime_api->config_get_string(&config, "style/color_scheme_dark",
                                        buffer, BUF_SIZE)) {
          std::string color_name(buffer);
          _UpdateUIStyleColor(&config, m_ui->style(), color_name);
        }
      }
      m_base_style = m_ui->style();
    }
    Bool global_ascii = false;
    if (rime_api->config_get_bool(&config, "global_ascii", &global_ascii))
      m_global_ascii_mode = !!global_ascii;
    if (!rime_api->config_get_int(&config, "show_notifications_time",
                                  &m_show_notifications_time))
      m_show_notifications_time = 1200;
    _LoadAppOptions(&config, m_app_options);
    rime_api->config_close(&config);
  }
  m_last_schema_id.clear();
}

void RimeWithWeaselHandler::Finalize() {
  m_active_session = 0;
  m_disabled = true;
  m_session_status_map.clear();
  LOG(INFO) << "Finalizing la rime.";
  rime_api->finalize();
}

DWORD RimeWithWeaselHandler::FindSession(WeaselSessionId ipc_id) {
  if (m_disabled)
    return 0;
  Bool found = rime_api->find_session(to_session_id(ipc_id));
  DLOG(INFO) << "Find session: session_id = " << to_session_id(ipc_id)
             << ", found = " << found;
  return found ? (ipc_id) : 0;
}

DWORD RimeWithWeaselHandler::AddSession(LPWSTR buffer, EatLine eat) {
  if (m_disabled) {
    DLOG(INFO) << "Trying to resume service.";
    EndMaintenance();
    if (m_disabled)
      return 0;
  }
  RimeSessionId session_id = (RimeSessionId)rime_api->create_session();
  if (m_global_ascii_mode) {
    for (const auto& pair : m_session_status_map) {
      if (pair.first) {
        rime_api->set_option(session_id, "ascii_mode",
                             !!pair.second.status.is_ascii_mode);
        break;
      }
    }
  }

  WeaselSessionId ipc_id =
      _GenerateNewWeaselSessionId(m_session_status_map, m_pid);
  DLOG(INFO) << "Add session: created session_id = " << session_id
             << ", ipc_id = " << ipc_id;
  SessionStatus& session_status = new_session_status(ipc_id);
  session_status.style = m_base_style;
  session_status.session_id = session_id;
  _ReadClientInfo(ipc_id, buffer);

  RIME_STRUCT(RimeStatus, status);
  if (rime_api->get_status(session_id, &status)) {
    std::string schema_id = status.schema_id;
    m_last_schema_id = schema_id;
    _LoadSchemaSpecificSettings(ipc_id, schema_id);
    _LoadAppInlinePreeditSet(ipc_id, true);
    _UpdateInlinePreeditStatus(ipc_id);
    _RefreshTrayIcon(session_id, _UpdateUICallback);
    session_status.status = status;
    session_status.__synced = false;
    rime_api->free_status(&status);
  }
  m_ui->style() = session_status.style;
  // show session's welcome message :-) if any
  if (eat) {
    _Respond(ipc_id, eat);
  }
  add_session = true;
  _UpdateUI(ipc_id);
  add_session = false;
  m_active_session = ipc_id;
  return ipc_id;
}

DWORD RimeWithWeaselHandler::RemoveSession(WeaselSessionId ipc_id) {
  if (m_ui)
    m_ui->Hide();
  if (m_disabled)
    return 0;
  DLOG(INFO) << "Remove session: session_id = " << to_session_id(ipc_id);
  // TODO: force committing? otherwise current composition would be lost
  rime_api->destroy_session(to_session_id(ipc_id));
  m_session_status_map.erase(ipc_id);
  m_active_session = 0;
  return 0;
}

namespace ibus {
enum Keycode {
  Escape = 0xFF1B,
  XK_bracketleft = 0x005b, /* U+005B LEFT SQUARE BRACKET */
  XK_c = 0x0063,           /* U+0063 LATIN SMALL LETTER C */
  XK_C = 0x0043,           /* U+0043 LATIN CAPITAL LETTER C */
};
}

void RimeWithWeaselHandler::UpdateColorTheme(BOOL darkMode) {
  RimeConfig config = {NULL};
  if (rime_api->config_open("weasel", &config)) {
    if (m_ui) {
      _UpdateUIStyle(&config, m_ui, true);
      m_current_dark_mode = darkMode;
      if (darkMode) {
        const int BUF_SIZE = 255;
        char buffer[BUF_SIZE + 1] = {0};
        if (rime_api->config_get_string(&config, "style/color_scheme_dark",
                                        buffer, BUF_SIZE)) {
          std::string color_name(buffer);
          _UpdateUIStyleColor(&config, m_ui->style(), color_name);
        }
      }
      m_base_style = m_ui->style();
    }
    rime_api->config_close(&config);
  }

  for (auto& pair : m_session_status_map) {
    RIME_STRUCT(RimeStatus, status);
    if (rime_api->get_status(to_session_id(pair.first), &status)) {
      _LoadSchemaSpecificSettings(pair.first, std::string(status.schema_id));
      _LoadAppInlinePreeditSet(pair.first, true);
      _UpdateInlinePreeditStatus(pair.first);
      pair.second.status = status;
      pair.second.__synced = false;
      rime_api->free_status(&status);
    }
  }
  m_ui->style() = get_session_status(m_active_session).style;
}

BOOL RimeWithWeaselHandler::ProcessKeyEvent(KeyEvent keyEvent,
                                            WeaselSessionId ipc_id,
                                            EatLine eat) {
  DLOG(INFO) << "Process key event: keycode = " << keyEvent.keycode
             << ", mask = " << keyEvent.mask << ", ipc_id = " << ipc_id;
  if (m_disabled)
    return FALSE;
  RimeSessionId session_id = to_session_id(ipc_id);
  Bool handled = rime_api->process_key(session_id, keyEvent.keycode,
                                       expand_ibus_modifier(keyEvent.mask));
  if (!handled) {
    bool isVimBackInCommandMode =
        (keyEvent.keycode == ibus::Keycode::Escape) ||
        ((keyEvent.mask & (1 << 2)) &&
         (keyEvent.keycode == ibus::Keycode::XK_c ||
          keyEvent.keycode == ibus::Keycode::XK_C ||
          keyEvent.keycode == ibus::Keycode::XK_bracketleft));
    if (isVimBackInCommandMode &&
        rime_api->get_option(session_id, "vim_mode") &&
        !rime_api->get_option(session_id, "ascii_mode")) {
      rime_api->set_option(session_id, "ascii_mode", True);
    }
  }
  _Respond(ipc_id, eat);
  _UpdateUI(ipc_id);
  m_active_session = ipc_id;
  return (BOOL)handled;
}

void RimeWithWeaselHandler::CommitComposition(WeaselSessionId ipc_id) {
  DLOG(INFO) << "Commit composition: ipc_id = " << ipc_id;
  if (m_disabled)
    return;
  rime_api->commit_composition(to_session_id(ipc_id));
  _UpdateUI(ipc_id);
  m_active_session = ipc_id;
}

void RimeWithWeaselHandler::ClearComposition(WeaselSessionId ipc_id) {
  DLOG(INFO) << "Clear composition: ipc_id = " << ipc_id;
  if (m_disabled)
    return;
  rime_api->clear_composition(to_session_id(ipc_id));
  _UpdateUI(ipc_id);
  m_active_session = ipc_id;
}

void RimeWithWeaselHandler::SelectCandidateOnCurrentPage(
    size_t index,
    WeaselSessionId ipc_id) {
  DLOG(INFO) << "select candidate on current page, ipc_id = " << ipc_id
             << ", index = " << index;
  if (m_disabled)
    return;
  rime_api->select_candidate_on_current_page(to_session_id(ipc_id), index);
}

bool RimeWithWeaselHandler::HighlightCandidateOnCurrentPage(
    size_t index,
    WeaselSessionId ipc_id,
    EatLine eat) {
  DLOG(INFO) << "highlight candidate on current page, ipc_id = " << ipc_id
             << ", index = " << index;
  bool res = rime_api->highlight_candidate_on_current_page(
      to_session_id(ipc_id), index);
  _Respond(ipc_id, eat);
  _UpdateUI(ipc_id);
  return res;
}

bool RimeWithWeaselHandler::ChangePage(bool backward,
                                       WeaselSessionId ipc_id,
                                       EatLine eat) {
  DLOG(INFO) << "change page, ipc_id = " << ipc_id
             << (backward ? "backward" : "foreward");
  bool res = rime_api->change_page(to_session_id(ipc_id), backward);
  _Respond(ipc_id, eat);
  _UpdateUI(ipc_id);
  return res;
}

void RimeWithWeaselHandler::FocusIn(DWORD client_caps, WeaselSessionId ipc_id) {
  DLOG(INFO) << "Focus in: ipc_id = " << ipc_id
             << ", client_caps = " << client_caps;
  if (m_disabled)
    return;
  _UpdateUI(ipc_id);
  m_active_session = ipc_id;
}

void RimeWithWeaselHandler::FocusOut(DWORD param, WeaselSessionId ipc_id) {
  DLOG(INFO) << "Focus out: ipc_id = " << ipc_id;
  if (m_ui)
    m_ui->Hide();
  m_active_session = 0;
}

void RimeWithWeaselHandler::UpdateInputPosition(RECT const& rc,
                                                WeaselSessionId ipc_id) {
  DLOG(INFO) << "Update input position: (" << rc.left << ", " << rc.top
             << "), ipc_id = " << ipc_id
             << ", m_active_session = " << m_active_session;
  if (m_ui)
    m_ui->UpdateInputPosition(rc);
  if (m_disabled)
    return;
  if (m_active_session != ipc_id) {
    _UpdateUI(ipc_id);
    m_active_session = ipc_id;
  }
}

std::string RimeWithWeaselHandler::m_message_type;
std::string RimeWithWeaselHandler::m_message_value;
std::string RimeWithWeaselHandler::m_message_label;
std::string RimeWithWeaselHandler::m_option_name;

void RimeWithWeaselHandler::OnNotify(void* context_object,
                                     uintptr_t session_id,
                                     const char* message_type,
                                     const char* message_value) {
  // may be running in a thread when deploying rime
  RimeWithWeaselHandler* self =
      reinterpret_cast<RimeWithWeaselHandler*>(context_object);
  if (!self || !message_type || !message_value)
    return;
  m_message_type = message_type;
  m_message_value = message_value;
  if (RIME_API_AVAILABLE(rime_api, get_state_label) &&
      !strcmp(message_type, "option")) {
    Bool state = message_value[0] != '!';
    const char* option_name = message_value + !state;
    m_option_name = option_name;
    const char* state_label =
        rime_api->get_state_label(session_id, option_name, state);
    if (state_label) {
      m_message_label = std::string(state_label);
    }
  }
}

void RimeWithWeaselHandler::_ReadClientInfo(WeaselSessionId ipc_id,
                                            LPWSTR buffer) {
  std::string app_name;
  std::string client_type;
  // parse request text
  wbufferstream bs(buffer, WEASEL_IPC_BUFFER_LENGTH);
  std::wstring line;
  while (bs.good()) {
    std::getline(bs, line);
    if (!bs.good())
      break;
    // file ends
    if (line == L".")
      break;
    const std::wstring kClientAppKey = L"session.client_app=";
    if (starts_with(line, kClientAppKey)) {
      std::wstring lwr = line;
      to_lower(lwr);
      app_name = wstring_to_string(lwr.substr(kClientAppKey.length()).c_str(),
                                   CP_UTF8);
    }
    const std::wstring kClientTypeKey = L"session.client_type=";
    if (starts_with(line, kClientTypeKey)) {
      client_type = wstring_to_string(
          line.substr(kClientTypeKey.length()).c_str(), CP_UTF8);
    }
  }
  SessionStatus& session_status = get_session_status(ipc_id);
  RimeSessionId session_id = session_status.session_id;
  // set app specific options
  if (!app_name.empty()) {
    rime_api->set_property(session_id, "client_app", app_name.c_str());

    auto it = m_app_options.find(app_name);
    if (it != m_app_options.end()) {
      AppOptions& options(m_app_options[it->first]);
      for (const auto& pair : options) {
        DLOG(INFO) << "set app option: " << pair.first << " = " << pair.second;
        rime_api->set_option(session_id, pair.first.c_str(), Bool(pair.second));
      }
    }
  }
  // ime | tsf
  rime_api->set_property(session_id, "client_type", client_type.c_str());
  // inline preedit
  bool inline_preedit =
      session_status.style.inline_preedit && (client_type == "tsf");
  rime_api->set_option(session_id, "inline_preedit", Bool(inline_preedit));
  // show soft cursor on weasel panel but not inline
  rime_api->set_option(session_id, "soft_cursor", Bool(!inline_preedit));
}

void RimeWithWeaselHandler::_GetCandidateInfo(CandidateInfo& cinfo,
                                              RimeContext& ctx) {
  cinfo.candies.resize(ctx.menu.num_candidates);
  cinfo.comments.resize(ctx.menu.num_candidates);
  cinfo.labels.resize(ctx.menu.num_candidates);
  for (int i = 0; i < ctx.menu.num_candidates; ++i) {
    cinfo.candies[i].str =
        escape_string(string_to_wstring(ctx.menu.candidates[i].text, CP_UTF8));
    if (ctx.menu.candidates[i].comment) {
      cinfo.comments[i].str = escape_string(
          string_to_wstring(ctx.menu.candidates[i].comment, CP_UTF8));
    }
    if (RIME_STRUCT_HAS_MEMBER(ctx, ctx.select_labels) && ctx.select_labels) {
      cinfo.labels[i].str =
          escape_string(string_to_wstring(ctx.select_labels[i], CP_UTF8));
    } else if (ctx.menu.select_keys) {
      cinfo.labels[i].str =
          escape_string(std::wstring(1, ctx.menu.select_keys[i]));
    } else {
      cinfo.labels[i].str = std::to_wstring((i + 1) % 10);
    }
  }
  cinfo.highlighted = ctx.menu.highlighted_candidate_index;
  cinfo.currentPage = ctx.menu.page_no;
  cinfo.is_last_page = ctx.menu.is_last_page;
}

void RimeWithWeaselHandler::StartMaintenance() {
  m_session_status_map.clear();
  Finalize();
  _UpdateUI(0);
}

void RimeWithWeaselHandler::EndMaintenance() {
  if (m_disabled) {
    Initialize();
    _UpdateUI(0);
  }
  m_session_status_map.clear();
}

void RimeWithWeaselHandler::SetOption(WeaselSessionId ipc_id,
                                      const std::string& opt,
                                      bool val) {
  // from no-session client, not actual typing session
  if (!ipc_id) {
    if (m_global_ascii_mode && opt == "ascii_mode") {
      for (auto& pair : m_session_status_map)
        rime_api->set_option(to_session_id(pair.first), "ascii_mode", val);
    } else {
      rime_api->set_option(to_session_id(m_active_session), opt.c_str(), val);
    }
  } else {
    rime_api->set_option(to_session_id(ipc_id), opt.c_str(), val);
  }
}

void RimeWithWeaselHandler::OnUpdateUI(std::function<void()> const& cb) {
  _UpdateUICallback = cb;
}

bool RimeWithWeaselHandler::_IsDeployerRunning() {
  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
  bool deployer_detected = hMutex && GetLastError() == ERROR_ALREADY_EXISTS;
  if (hMutex) {
    CloseHandle(hMutex);
  }
  return deployer_detected;
}

void RimeWithWeaselHandler::_UpdateUI(WeaselSessionId ipc_id) {
  // if m_ui nullptr, _UpdateUI meaningless
  if (!m_ui)
    return;

  Status& weasel_status = m_ui->status();
  Context weasel_context;

  RimeSessionId session_id = to_session_id(ipc_id);
  bool is_tsf = _IsSessionTSF(session_id);

  if (ipc_id == 0)
    weasel_status.disabled = m_disabled;

  _GetStatus(weasel_status, ipc_id, weasel_context);

  if (!is_tsf) {
    _GetContext(weasel_context, session_id);
  }

  SessionStatus& session_status = get_session_status(ipc_id);
  if (rime_api->get_option(session_id, "inline_preedit"))
    session_status.style.client_caps |= INLINE_PREEDIT_CAPABLE;
  else
    session_status.style.client_caps &= ~INLINE_PREEDIT_CAPABLE;

  if (weasel_status.composing && !is_tsf) {
    m_ui->Update(weasel_context, weasel_status);
    m_ui->Show();
  } else if (!_ShowMessage(weasel_context, weasel_status) && !is_tsf) {
    m_ui->Hide();
    m_ui->Update(weasel_context, weasel_status);
  }

  _RefreshTrayIcon(session_id, _UpdateUICallback);

  m_message_type.clear();
  m_message_value.clear();
  m_message_label.clear();
  m_option_name.clear();
}

std::wstring _LoadIconSettingFromSchema(
    RimeConfig& config,
    const char* key1,
    const char* key2,
    const std::filesystem::path& user_dir,
    const std::filesystem::path& shared_dir) {
  const int BUF_SIZE = 255;
  char buffer[BUF_SIZE + 1] = {0};
  if (rime_api->config_get_string(&config, key1, buffer, BUF_SIZE) ||
      (key2 != NULL &&
       rime_api->config_get_string(&config, key2, buffer, BUF_SIZE))) {
    std::wstring resource = string_to_wstring(buffer, CP_UTF8);
    DWORD dwAttrib = GetFileAttributes((user_dir / resource).c_str());
    if (INVALID_FILE_ATTRIBUTES != dwAttrib &&
        0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
      return (user_dir / resource).wstring();
    }
    dwAttrib = GetFileAttributes((shared_dir / resource).c_str());
    if (INVALID_FILE_ATTRIBUTES != dwAttrib &&
        0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
      return (shared_dir / resource).wstring();
    }
  }
  return L"";
}

void RimeWithWeaselHandler::_LoadSchemaSpecificSettings(
    WeaselSessionId ipc_id,
    const std::string& schema_id) {
  if (!m_ui)
    return;
  RimeConfig config;
  if (!rime_api->schema_open(schema_id.c_str(), &config))
    return;
  _UpdateShowNotifications(&config);
  m_ui->style() = m_base_style;
  _UpdateUIStyle(&config, m_ui, false);
  SessionStatus& session_status = get_session_status(ipc_id);
  session_status.style = m_ui->style();
  // load schema color style config
  const int BUF_SIZE = 255;
  char buffer[BUF_SIZE + 1] = {0};
  if (!m_current_dark_mode &&
      rime_api->config_get_string(&config, "style/color_scheme", buffer,
                                  BUF_SIZE)) {
    std::string color_name(buffer);
    RimeConfigIterator preset = {0};
    if (rime_api->config_begin_map(
            &preset, &config, ("preset_color_schemes/" + color_name).c_str())) {
      _UpdateUIStyleColor(&config, session_status.style, color_name);
    } else {
      RimeConfig weaselconfig;
      if (rime_api->config_open("weasel", &weaselconfig)) {
        _UpdateUIStyleColor(&weaselconfig, session_status.style, color_name);
        rime_api->config_close(&weaselconfig);
      }
    }
  } else if (m_current_dark_mode &&
             rime_api->config_get_string(&config, "style/color_scheme_dark",
                                         buffer, BUF_SIZE)) {
    std::string color_name(buffer);
    RimeConfigIterator preset = {0};
    if (rime_api->config_begin_map(
            &preset, &config, ("preset_color_schemes/" + color_name).c_str())) {
      _UpdateUIStyleColor(&config, session_status.style, color_name);
    } else {
      RimeConfig weaselconfig;
      if (rime_api->config_open("weasel", &weaselconfig)) {
        _UpdateUIStyleColor(&weaselconfig, session_status.style, color_name);
        rime_api->config_close(&weaselconfig);
      }
    }
  }
  // load schema icon start
  {
    auto user_dir = WeaselUserDataPath();
    auto shared_dir = WeaselSharedDataPath();
    session_status.style.current_zhung_icon = _LoadIconSettingFromSchema(
        config, "schema/icon", "schema/zhung_icon", user_dir, shared_dir);
    session_status.style.current_ascii_icon = _LoadIconSettingFromSchema(
        config, "schema/ascii_icon", NULL, user_dir, shared_dir);
    session_status.style.current_full_icon = _LoadIconSettingFromSchema(
        config, "schema/full_icon", NULL, user_dir, shared_dir);
    session_status.style.current_half_icon = _LoadIconSettingFromSchema(
        config, "schema/half_icon", NULL, user_dir, shared_dir);
  }
  // load schema icon end
  rime_api->config_close(&config);
}

void RimeWithWeaselHandler::_LoadAppInlinePreeditSet(WeaselSessionId ipc_id,
                                                     bool ignore_app_name) {
  SessionStatus& session_status = get_session_status(ipc_id);
  RimeSessionId session_id = session_status.session_id;
  static char _app_name[50];
  rime_api->get_property(session_id, "client_app", _app_name,
                         sizeof(_app_name) - 1);
  std::string app_name(_app_name);
  if (!ignore_app_name && m_last_app_name == app_name)
    return;
  m_last_app_name = app_name;
  bool inline_preedit = session_status.style.inline_preedit;
  bool found = false;
  if (!app_name.empty()) {
    auto it = m_app_options.find(app_name);
    if (it != m_app_options.end()) {
      AppOptions& options(m_app_options[it->first]);
      for (const auto& pair : options) {
        if (pair.first == "inline_preedit") {
          rime_api->set_option(session_id, pair.first.c_str(),
                               Bool(pair.second));
          session_status.style.inline_preedit = Bool(pair.second);
          found = true;
          break;
        }
      }
    }
  }
  if (!found) {
    session_status.style.inline_preedit = m_base_style.inline_preedit;
    // load from schema.
    RIME_STRUCT(RimeStatus, status);
    if (rime_api->get_status(session_id, &status)) {
      std::string schema_id = status.schema_id;
      RimeConfig config;
      if (rime_api->schema_open(schema_id.c_str(), &config)) {
        Bool value = False;
        if (rime_api->config_get_bool(&config, "style/inline_preedit",
                                      &value)) {
          session_status.style.inline_preedit = value;
        }
        rime_api->config_close(&config);
      }
      rime_api->free_status(&status);
    }
  }
  if (session_status.style.inline_preedit != inline_preedit)
    _UpdateInlinePreeditStatus(ipc_id);
}

bool RimeWithWeaselHandler::_ShowMessage(Context& ctx, Status& status) {
  // show as auxiliary string
  std::wstring& tips(ctx.aux.str);
  bool show_icon = false;
  if (m_message_type == "deploy") {
    if (m_message_value == "start")
      if (GetThreadUILanguage() == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        tips = L"Deploying RIME";
      else
        tips = L"正在部署 RIME";
    else if (m_message_value == "success")
      if (GetThreadUILanguage() == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        tips = L"Deployed";
      else
        tips = L"部署完成";
    else if (m_message_value == "failure") {
      if (GetThreadUILanguage() ==
          MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
        tips = L"有錯誤，請查看日誌 %TEMP%\\rime.weasel\\rime.weasel.*.INFO";
      else if (GetThreadUILanguage() ==
               MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
        tips = L"有错误，请查看日志 %TEMP%\\rime.weasel\\rime.weasel.*.INFO";
      else
        tips =
            L"There is an error, please check the logs "
            L"%TEMP%\\rime.weasel\\rime.weasel.*.INFO";
    }
  } else if (m_message_type == "schema") {
    tips = /*L"【" + */ status.schema_name /* + L"】"*/;
  } else if (m_message_type == "option") {
    status.type = SCHEMA;
    if (m_message_value == "!ascii_mode") {
      show_icon = true;
    } else if (m_message_value == "ascii_mode") {
      show_icon = true;
    } else
      tips = string_to_wstring(m_message_label, CP_UTF8);

    if (m_message_value == "full_shape" || m_message_value == "!full_shape")
      status.type = FULL_SHAPE;
  }
  if (tips.empty() && !show_icon)
    return m_ui->IsCountingDown();
  auto foption = m_show_notifications.find(m_option_name);
  auto falways = m_show_notifications.find("always");
  if ((!add_session && (foption != m_show_notifications.end() ||
                        falways != m_show_notifications.end())) ||
      m_message_type == "deploy") {
    m_ui->Update(ctx, status);
    if (m_show_notifications_time)
      m_ui->ShowWithTimeout(m_show_notifications_time);
    return true;
  } else {
    return m_ui->IsCountingDown();
  }
}
inline std::string _GetLabelText(const std::vector<Text>& labels,
                                 int id,
                                 const wchar_t* format) {
  wchar_t buffer[128];
  swprintf_s<128>(buffer, format, labels.at(id).str.c_str());
  return wstring_to_string(std::wstring(buffer), CP_UTF8);
}

bool RimeWithWeaselHandler::_Respond(WeaselSessionId ipc_id, EatLine eat) {
  std::set<std::string> actions;
  std::list<std::string> messages;

  SessionStatus& session_status = get_session_status(ipc_id);
  RimeSessionId session_id = session_status.session_id;
  RIME_STRUCT(RimeCommit, commit);
  if (rime_api->get_commit(session_id, &commit)) {
    actions.insert("commit");

    std::string commit_text = escape_string<char>(commit.text);
    messages.push_back(std::string("commit=") + commit_text + '\n');
    rime_api->free_commit(&commit);
  }

  bool is_composing = false;
  RIME_STRUCT(RimeStatus, status);
  if (rime_api->get_status(session_id, &status)) {
    is_composing = !!status.is_composing;
    actions.insert("status");
    messages.push_back(std::string("status.ascii_mode=") +
                       std::to_string(status.is_ascii_mode) + '\n');
    messages.push_back(std::string("status.composing=") +
                       std::to_string(status.is_composing) + '\n');
    messages.push_back(std::string("status.disabled=") +
                       std::to_string(status.is_disabled) + '\n');
    messages.push_back(std::string("status.full_shape=") +
                       std::to_string(status.is_full_shape) + '\n');
    messages.push_back(std::string("status.schema_id=") +
                       std::string(status.schema_id) + '\n');
    if (m_global_ascii_mode &&
        (session_status.status.is_ascii_mode != status.is_ascii_mode)) {
      for (auto& pair : m_session_status_map) {
        if (pair.first != ipc_id)
          rime_api->set_option(to_session_id(pair.first), "ascii_mode",
                               !!status.is_ascii_mode);
      }
    }
    session_status.status = status;
    rime_api->free_status(&status);
  }

  RIME_STRUCT(RimeContext, ctx);
  if (rime_api->get_context(session_id, &ctx)) {
    if (is_composing) {
      actions.insert("ctx");
      switch (session_status.style.preedit_type) {
        case UIStyle::PREVIEW:
          if (ctx.commit_text_preview != NULL) {
            std::string first = ctx.commit_text_preview;
            messages.push_back(std::string("ctx.preedit=") +
                               escape_string<char>(first) + '\n');
            messages.push_back(
                std::string("ctx.preedit.cursor=") +
                std::to_string(utf8towcslen(first.c_str(), 0)) + ',' +
                std::to_string(utf8towcslen(first.c_str(), (int)first.size())) +
                ',' +
                std::to_string(utf8towcslen(first.c_str(), (int)first.size())) +
                '\n');
            break;
          }
          // no preview, fall back to composition
        case UIStyle::COMPOSITION:
          messages.push_back(std::string("ctx.preedit=") +
                             escape_string<char>(ctx.composition.preedit) +
                             '\n');
          if (ctx.composition.sel_start <= ctx.composition.sel_end) {
            messages.push_back(
                std::string("ctx.preedit.cursor=") +
                std::to_string(utf8towcslen(ctx.composition.preedit,
                                            ctx.composition.sel_start)) +
                ',' +
                std::to_string(utf8towcslen(ctx.composition.preedit,
                                            ctx.composition.sel_end)) +
                ',' +
                std::to_string(utf8towcslen(ctx.composition.preedit,
                                            ctx.composition.cursor_pos)) +
                '\n');
          }
          break;
        case UIStyle::PREVIEW_ALL:
          CandidateInfo cinfo;
          _GetCandidateInfo(cinfo, ctx);
          std::string topush = std::string("ctx.preedit=") +
                               escape_string<char>(ctx.composition.preedit) +
                               "  [";
          for (auto i = 0; i < ctx.menu.num_candidates; i++) {
            std::string label =
                session_status.style.label_font_point > 0
                    ? _GetLabelText(
                          cinfo.labels, i,
                          session_status.style.label_text_format.c_str())
                    : "";
            std::string comment =
                session_status.style.comment_font_point > 0
                    ? wstring_to_string(cinfo.comments.at(i).str, CP_UTF8)
                    : "";
            std::string mark_text =
                session_status.style.mark_text.empty()
                    ? "*"
                    : wstring_to_string(session_status.style.mark_text,
                                        CP_UTF8);
            std::string prefix =
                (i != ctx.menu.highlighted_candidate_index) ? "" : mark_text;
            topush += " " + prefix + escape_string(label) +
                      escape_string<char>(ctx.menu.candidates[i].text) + " " +
                      escape_string(comment);
          }
          messages.push_back(topush + " ]\n");
          if (ctx.composition.sel_start <= ctx.composition.sel_end) {
            messages.push_back(
                std::string("ctx.preedit.cursor=") +
                std::to_string(utf8towcslen(ctx.composition.preedit,
                                            ctx.composition.sel_start)) +
                ',' +
                std::to_string(utf8towcslen(ctx.composition.preedit,
                                            ctx.composition.sel_end)) +
                ',' +
                std::to_string(utf8towcslen(ctx.composition.preedit,
                                            ctx.composition.cursor_pos)) +
                '\n');
          }
          break;
      }
    }
    if (ctx.menu.num_candidates) {
      CandidateInfo cinfo;
      std::wstringstream ss;
      boost::archive::text_woarchive oa(ss);
      _GetCandidateInfo(cinfo, ctx);

      oa << cinfo;

      messages.push_back(std::string("ctx.cand=") +
                         wstring_to_string(ss.str().c_str(), CP_UTF8) + '\n');
    }
    rime_api->free_context(&ctx);
  }

  // configuration information
  actions.insert("config");
  messages.push_back(std::string("config.inline_preedit=") +
                     std::to_string((int)session_status.style.inline_preedit) +
                     '\n');

  // style
  if (!session_status.__synced) {
    std::wstringstream ss;
    boost::archive::text_woarchive oa(ss);
    oa << session_status.style;

    actions.insert("style");
    messages.push_back(std::string("style=") +
                       wstring_to_string(ss.str().c_str(), CP_UTF8) + '\n');
    session_status.__synced = true;
  }

  // summarize

  if (actions.empty()) {
    messages.insert(messages.begin(), std::string("action=noop\n"));
  } else {
    std::string actionList(join(actions, ","));
    messages.insert(messages.begin(),
                    std::string("action=") + actionList + '\n');
  }

  messages.push_back(std::string(".\n"));

  return std::all_of(
      messages.begin(), messages.end(), [&eat](std::string& msg) {
        return eat(std::wstring(string_to_wstring(msg.c_str(), CP_UTF8)));
      });
}

static inline COLORREF blend_colors(COLORREF fcolor, COLORREF bcolor) {
  return RGB((GetRValue(fcolor) * 2 + GetRValue(bcolor)) / 3,
             (GetGValue(fcolor) * 2 + GetGValue(bcolor)) / 3,
             (GetBValue(fcolor) * 2 + GetBValue(bcolor)) / 3) |
         ((((fcolor >> 24) + (bcolor >> 24) / 2) << 24));
}
// convertions from color format to COLOR_ABGR
static inline int ConvertColorToAbgr(int color, ColorFormat fmt = COLOR_ABGR) {
  if (fmt == COLOR_ABGR)
    return color;
  else if (fmt == COLOR_ARGB)
    return ARGB2ABGR(color);
  else
    return RGBA2ABGR(color);
}
// parse color value, with fallback value
static Bool _RimeConfigGetColor32bWithFallback(RimeConfig* config,
                                               const std::string key,
                                               int& value,
                                               const ColorFormat& fmt,
                                               const int& fallback) {
  char color[256] = {0};
  if (!rime_api->config_get_string(config, key.c_str(), color, 256)) {
    value = fallback;
    return False;
  }
  std::string color_str = std::string(color);
  // color code hex
  if (std::regex_match(color_str, HEX_REGEX)) {
    std::string tmp = std::regex_replace(color_str, TRIMHEAD_REGEX, "");
    // limit first 8 code
    tmp = tmp.substr(0, 8);
    if (tmp.length() == 6)  // color code without alpha, xxyyzz add alpha ff
    {
      value = std::stoi(tmp, 0, 16);
      if (fmt != COLOR_RGBA)
        value |= 0xff000000;
      else
        value = (value << 8) | 0x000000ff;
    } else if (tmp.length() == 3)  // color hex code xyz => xxyyzz and alpha ff
    {
      tmp = tmp.substr(0, 1) + tmp.substr(0, 1) + tmp.substr(1, 1) +
            tmp.substr(1, 1) + tmp.substr(2, 1) + tmp.substr(2, 1);

      value = std::stoi(tmp, 0, 16);
      if (fmt != COLOR_RGBA)
        value |= 0xff000000;
      else
        value = (value << 8) | 0x000000ff;
    } else if (tmp.length() == 4)  // color hex code vxyz => vvxxyyzz
    {
      tmp = tmp.substr(0, 1) + tmp.substr(0, 1) + tmp.substr(1, 1) +
            tmp.substr(1, 1) + tmp.substr(2, 1) + tmp.substr(2, 1) +
            tmp.substr(3, 1) + tmp.substr(3, 1);

      std::string tmp1 = tmp.substr(0, 6);
      int value1 = std::stoi(tmp1, 0, 16);
      tmp1 = tmp.substr(6);
      int value2 = std::stoi(tmp1, 0, 16);
      value = (value1 << (tmp1.length() * 4)) | value2;
    } else if (tmp.length() > 6 &&
               tmp.length() <= 8) /* color code with alpha */
    {
      // stoi limitation, split to handle
      std::string tmp1 = tmp.substr(0, 6);
      int value1 = std::stoi(tmp1, 0, 16);
      tmp1 = tmp.substr(6);
      int value2 = std::stoi(tmp1, 0, 16);
      value = (value1 << (tmp1.length() * 4)) | value2;
    } else  // reject other code, length less then 3 or length == 5
    {
      value = fallback;
      return False;
    }
    value = ConvertColorToAbgr(value, fmt);
    value = (value & 0xffffffff);
    return True;
  }
  // regular number or other stuff, if user use pure dec number, they should
  // take care themselves
  else {
    int tmp = 0;
    if (!rime_api->config_get_int(config, key.c_str(), &tmp)) {
      value = fallback;
      return False;
    }
    if (fmt != COLOR_RGBA)
      value = (tmp | 0xff000000) & 0xffffffff;
    else
      value = ((tmp << 8) | 0x000000ff) & 0xffffffff;
    value = ConvertColorToAbgr(value, fmt);
    return True;
  }
}
// for remove useless spaces around seperators, begining and ending
static inline void _RemoveSpaceAroundSep(std::wstring& str) {
  str = std::regex_replace(str, std::wregex(L"\\s*(,|:|^|$)\\s*"), L"$1");
}
// parset bool type configuration to T type value trueValue / falseValue
template <class T>
static void _RimeGetBool(RimeConfig* config,
                         char* key,
                         bool cond,
                         T& value,
                         const T& trueValue,
                         const T& falseValue) {
  Bool tempb = False;
  if (rime_api->config_get_bool(config, key, &tempb) || cond)
    value = (!!tempb) ? trueValue : falseValue;
}
//	parse string option to T type value, with fallback
template <typename T>
void _RimeParseStringOptWithFallback(RimeConfig* config,
                                     const std::string key,
                                     T& value,
                                     const std::map<std::string, T> amap,
                                     const T& fallback) {
  char str_buff[256] = {0};
  if (rime_api->config_get_string(config, key.c_str(), str_buff,
                                  sizeof(str_buff) - 1)) {
    auto it = amap.find(std::string(str_buff));
    value = (it != amap.end()) ? it->second : fallback;
  } else
    value = fallback;
}
static inline void _abs(int* value) {
  *value = abs(*value);
}  // turn *value to be non-negative
// get int type value with fallback key fb_key, and func to execute after
// reading
static void _RimeGetIntWithFallback(RimeConfig* config,
                                    const char* key,
                                    int* value,
                                    const char* fb_key = NULL,
                                    std::function<void(int*)> func = NULL) {
  if (!rime_api->config_get_int(config, key, value) && fb_key != NULL) {
    rime_api->config_get_int(config, fb_key, value);
  }
  if (func)
    func(value);
}
// get string value, with fallback value *fallback, and func to execute after
// reading
static void _RimeGetStringWithFunc(
    RimeConfig* config,
    const char* key,
    std::wstring& value,
    const std::wstring* fallback = NULL,
    const std::function<void(std::wstring&)> func = NULL) {
  const int BUF_SIZE = 2047;
  char buffer[BUF_SIZE + 1] = {0};
  if (rime_api->config_get_string(config, key, buffer, BUF_SIZE)) {
    std::wstring tmp = string_to_wstring(buffer, CP_UTF8);
    if (func)
      func(tmp);
    value = tmp;
  } else if (fallback)
    value = *fallback;
}

void RimeWithWeaselHandler::_UpdateShowNotifications(RimeConfig* config,
                                                     bool initialize) {
  Bool show_notifications = true;
  RimeConfigIterator iter;
  if (initialize)
    m_show_notifications_base.clear();
  m_show_notifications.clear();

  if (rime_api->config_get_bool(config, "show_notifications",
                                &show_notifications)) {
    // config read as bool, for gloal all on or off
    if (show_notifications)
      m_show_notifications["always"] = true;
    if (initialize)
      m_show_notifications_base = m_show_notifications;
  } else if (rime_api->config_begin_list(&iter, config, "show_notifications")) {
    // config read as list, list item should be option name in schema
    // or key word 'schema' for schema switching tip
    while (rime_api->config_next(&iter)) {
      char buffer[256] = {0};
      if (rime_api->config_get_string(config, iter.path, buffer, 256))
        m_show_notifications[std::string(buffer)] = true;
    }
    if (initialize)
      m_show_notifications_base = m_show_notifications;
    rime_api->config_end(&iter);
  } else {
    // not configured, or incorrect type
    if (initialize)
      m_show_notifications_base["always"] = true;
    m_show_notifications = m_show_notifications_base;
  }
}

// update ui's style parameters, ui has been check before referenced
static void _UpdateUIStyle(RimeConfig* config, UI* ui, bool initialize) {
  UIStyle& style(ui->style());
  // get font faces
  _RimeGetStringWithFunc(config, "style/font_face", style.font_face, NULL,
                         _RemoveSpaceAroundSep);
  std::wstring* const pFallbackFontFace = initialize ? &style.font_face : NULL;
  _RimeGetStringWithFunc(config, "style/label_font_face", style.label_font_face,
                         pFallbackFontFace, _RemoveSpaceAroundSep);
  _RimeGetStringWithFunc(config, "style/comment_font_face",
                         style.comment_font_face, pFallbackFontFace,
                         _RemoveSpaceAroundSep);
  // able to set label font/comment font empty, force fallback to font face.
  if (style.label_font_face.empty())
    style.label_font_face = style.font_face;
  if (style.comment_font_face.empty())
    style.comment_font_face = style.font_face;
  // get font points
  _RimeGetIntWithFallback(config, "style/font_point", &style.font_point);
  if (style.font_point <= 0)
    style.font_point = 12;
  _RimeGetIntWithFallback(config, "style/label_font_point",
                          &style.label_font_point, "style/font_point", _abs);
  _RimeGetIntWithFallback(config, "style/comment_font_point",
                          &style.comment_font_point, "style/font_point", _abs);
  _RimeGetIntWithFallback(config, "style/candidate_abbreviate_length",
                          &style.candidate_abbreviate_length, NULL, _abs);
  _RimeGetBool(config, "style/inline_preedit", initialize, style.inline_preedit,
               true, false);
  _RimeGetBool(config, "style/vertical_auto_reverse", initialize,
               style.vertical_auto_reverse, true, false);
  const std::map<std::string, UIStyle::PreeditType> _preeditMap = {
      {std::string("composition"), UIStyle::COMPOSITION},
      {std::string("preview"), UIStyle::PREVIEW},
      {std::string("preview_all"), UIStyle::PREVIEW_ALL}};
  _RimeParseStringOptWithFallback(config, "style/preedit_type",
                                  style.preedit_type, _preeditMap,
                                  style.preedit_type);
  const std::map<std::string, UIStyle::AntiAliasMode> _aliasModeMap = {
      {std::string("force_dword"), UIStyle::FORCE_DWORD},
      {std::string("cleartype"), UIStyle::CLEARTYPE},
      {std::string("grayscale"), UIStyle::GRAYSCALE},
      {std::string("aliased"), UIStyle::ALIASED},
      {std::string("default"), UIStyle::DEFAULT}};
  _RimeParseStringOptWithFallback(config, "style/antialias_mode",
                                  style.antialias_mode, _aliasModeMap,
                                  style.antialias_mode);
  const std::map<std::string, UIStyle::HoverType> _hoverTypeMap = {
      {std::string("none"), UIStyle::HoverType::NONE},
      {std::string("semi_hilite"), UIStyle::HoverType::SEMI_HILITE},
      {std::string("hilite"), UIStyle::HoverType::HILITE}};
  _RimeParseStringOptWithFallback(config, "style/hover_type", style.hover_type,
                                  _hoverTypeMap, style.hover_type);
  const std::map<std::string, UIStyle::LayoutAlignType> _alignType = {
      {std::string("top"), UIStyle::ALIGN_TOP},
      {std::string("center"), UIStyle::ALIGN_CENTER},
      {std::string("bottom"), UIStyle::ALIGN_BOTTOM}};
  _RimeParseStringOptWithFallback(config, "style/layout/align_type",
                                  style.align_type, _alignType,
                                  style.align_type);
  _RimeGetBool(config, "style/display_tray_icon", initialize,
               style.display_tray_icon, true, false);
  _RimeGetBool(config, "style/ascii_tip_follow_cursor", initialize,
               style.ascii_tip_follow_cursor, true, false);
  _RimeGetBool(config, "style/horizontal", initialize, style.layout_type,
               UIStyle::LAYOUT_HORIZONTAL, UIStyle::LAYOUT_VERTICAL);
  _RimeGetBool(config, "style/paging_on_scroll", initialize,
               style.paging_on_scroll, true, false);
  _RimeGetBool(config, "style/click_to_capture", initialize,
               style.click_to_capture, true, false);
  _RimeGetBool(config, "style/fullscreen", false, style.layout_type,
               ((style.layout_type == UIStyle::LAYOUT_HORIZONTAL)
                    ? UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN
                    : UIStyle::LAYOUT_VERTICAL_FULLSCREEN),
               style.layout_type);
  _RimeGetBool(config, "style/vertical_text", false, style.layout_type,
               UIStyle::LAYOUT_VERTICAL_TEXT, style.layout_type);
  _RimeGetBool(config, "style/vertical_text_left_to_right", false,
               style.vertical_text_left_to_right, true, false);
  _RimeGetBool(config, "style/vertical_text_with_wrap", false,
               style.vertical_text_with_wrap, true, false);
  const std::map<std::string, bool> _text_orientation = {
      {std::string("horizontal"), false}, {std::string("vertical"), true}};
  bool _text_orientation_bool = false;
  _RimeParseStringOptWithFallback(config, "style/text_orientation",
                                  _text_orientation_bool, _text_orientation,
                                  _text_orientation_bool);
  if (_text_orientation_bool)
    style.layout_type = UIStyle::LAYOUT_VERTICAL_TEXT;
  _RimeGetStringWithFunc(config, "style/label_format", style.label_text_format);
  _RimeGetStringWithFunc(config, "style/mark_text", style.mark_text);
  _RimeGetIntWithFallback(config, "style/layout/baseline", &style.baseline,
                          NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/linespacing",
                          &style.linespacing, NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/min_width", &style.min_width,
                          NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/max_width", &style.max_width,
                          NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/min_height", &style.min_height,
                          NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/max_height", &style.max_height,
                          NULL, _abs);
  // layout (alternative to style/horizontal)
  const std::map<std::string, UIStyle::LayoutType> _layoutMap = {
      {std::string("vertical"), UIStyle::LAYOUT_VERTICAL},
      {std::string("horizontal"), UIStyle::LAYOUT_HORIZONTAL},
      {std::string("vertical_text"), UIStyle::LAYOUT_VERTICAL_TEXT},
      {std::string("vertical+fullscreen"), UIStyle::LAYOUT_VERTICAL_FULLSCREEN},
      {std::string("horizontal+fullscreen"),
       UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN}};
  _RimeParseStringOptWithFallback(config, "style/layout/type",
                                  style.layout_type, _layoutMap,
                                  style.layout_type);
  // disable max_width when full screen
  if (style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN ||
      style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN) {
    style.max_width = 0;
    style.inline_preedit = false;
  }
  _RimeGetIntWithFallback(config, "style/layout/border", &style.border,
                          "style/layout/border_width", _abs);
  _RimeGetIntWithFallback(config, "style/layout/margin_x", &style.margin_x);
  _RimeGetIntWithFallback(config, "style/layout/margin_y", &style.margin_y);
  _RimeGetIntWithFallback(config, "style/layout/spacing", &style.spacing, NULL,
                          _abs);
  _RimeGetIntWithFallback(config, "style/layout/candidate_spacing",
                          &style.candidate_spacing, NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/hilite_spacing",
                          &style.hilite_spacing, NULL, _abs);
  _RimeGetIntWithFallback(config, "style/layout/hilite_padding_x",
                          &style.hilite_padding_x,
                          "style/layout/hilite_padding", _abs);
  _RimeGetIntWithFallback(config, "style/layout/hilite_padding_y",
                          &style.hilite_padding_y,
                          "style/layout/hilite_padding", _abs);
  _RimeGetIntWithFallback(config, "style/layout/shadow_radius",
                          &style.shadow_radius, NULL, _abs);
  // disable shadow for fullscreen layout
  style.shadow_radius *=
      (!(style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN ||
         style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN));
  _RimeGetIntWithFallback(config, "style/layout/shadow_offset_x",
                          &style.shadow_offset_x);
  _RimeGetIntWithFallback(config, "style/layout/shadow_offset_y",
                          &style.shadow_offset_y);
  // round_corner as alias of hilited_corner_radius
  _RimeGetIntWithFallback(config, "style/layout/hilited_corner_radius",
                          &style.round_corner, "style/layout/round_corner",
                          _abs);
  // corner_radius not set, fallback to round_corner
  _RimeGetIntWithFallback(config, "style/layout/corner_radius",
                          &style.round_corner_ex, "style/layout/round_corner",
                          _abs);
  // fix padding and spacing settings
  if (style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) {
    // hilite_padding vs spacing
    // if hilite_padding over spacing, increase spacing
    style.spacing = max(style.spacing, style.hilite_padding_y * 2);
    // hilite_padding vs candidate_spacing
    if (style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN ||
        style.layout_type == UIStyle::LAYOUT_VERTICAL) {
      // vertical, if hilite_padding_y over candidate spacing,
      // increase candidate spacing
      style.candidate_spacing =
          max(style.candidate_spacing, style.hilite_padding_y * 2);
    } else {
      // horizontal, if hilite_padding_x over candidate
      // spacing, increase candidate spacing
      style.candidate_spacing =
          max(style.candidate_spacing, style.hilite_padding_x * 2);
    }
    // hilite_padding_x vs hilite_spacing
    if (!style.inline_preedit)
      style.hilite_spacing = max(style.hilite_spacing, style.hilite_padding_x);
  } else  // LAYOUT_VERTICAL_TEXT
  {
    // hilite_padding_x vs spacing
    // if hilite_padding over spacing, increase spacing
    style.spacing = max(style.spacing, style.hilite_padding_x * 2);
    // hilite_padding vs candidate_spacing
    // if hilite_padding_x over candidate
    // spacing, increase candidate spacing
    style.candidate_spacing =
        max(style.candidate_spacing, style.hilite_padding_x * 2);
    // vertical_text_with_wrap and hilite_padding_y over candidate_spacing
    if (style.vertical_text_with_wrap)
      style.candidate_spacing =
          max(style.candidate_spacing, style.hilite_padding_y * 2);
    // hilite_padding_y vs hilite_spacing
    if (!style.inline_preedit)
      style.hilite_spacing = max(style.hilite_spacing, style.hilite_padding_y);
  }
  // fix padding and margin settings
  int scale = style.margin_x < 0 ? -1 : 1;
  style.margin_x = scale * max(style.hilite_padding_x, abs(style.margin_x));
  scale = style.margin_y < 0 ? -1 : 1;
  style.margin_y = scale * max(style.hilite_padding_y, abs(style.margin_y));
  // get enhanced_position
  _RimeGetBool(config, "style/enhanced_position", initialize,
               style.enhanced_position, true, false);
  // get color scheme
  const int BUF_SIZE = 255;
  char buffer[BUF_SIZE + 1] = {0};
  if (initialize && rime_api->config_get_string(config, "style/color_scheme",
                                                buffer, BUF_SIZE))
    _UpdateUIStyleColor(config, style);
}
// load color configs to style, by "style/color_scheme" or specific scheme name
// "color" which is default empty
static bool _UpdateUIStyleColor(RimeConfig* config,
                                UIStyle& style,
                                std::string color) {
  const int BUF_SIZE = 255;
  char buffer[BUF_SIZE + 1] = {0};
  std::string color_mark = "style/color_scheme";
  // color scheme
  if (rime_api->config_get_string(config, color_mark.c_str(), buffer,
                                  BUF_SIZE) ||
      !color.empty()) {
    std::string prefix("preset_color_schemes/");
    prefix += (color.empty()) ? buffer : color;
    // define color format, default abgr if not set
    ColorFormat fmt = COLOR_ABGR;
    const std::map<std::string, ColorFormat> _colorFmt = {
        {std::string("argb"), COLOR_ARGB},
        {std::string("rgba"), COLOR_RGBA},
        {std::string("abgr"), COLOR_ABGR}};
    _RimeParseStringOptWithFallback(config, (prefix + "/color_format"), fmt,
                                    _colorFmt, COLOR_ABGR);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/back_color"),
                                       style.back_color, fmt, 0xffffffff);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/shadow_color"),
                                       style.shadow_color, fmt,
                                       TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/prevpage_color"),
                                       style.prevpage_color, fmt,
                                       TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/nextpage_color"),
                                       style.nextpage_color, fmt,
                                       TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/text_color"),
                                       style.text_color, fmt, 0xff000000);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/candidate_text_color"), style.candidate_text_color,
        fmt, style.text_color);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/candidate_back_color"), style.candidate_back_color,
        fmt, TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/border_color"),
                                       style.border_color, fmt,
                                       style.text_color);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/hilited_text_color"),
                                       style.hilited_text_color, fmt,
                                       style.text_color);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/hilited_back_color"),
                                       style.hilited_back_color, fmt,
                                       style.back_color);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_candidate_text_color"),
        style.hilited_candidate_text_color, fmt, style.hilited_text_color);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_candidate_back_color"),
        style.hilited_candidate_back_color, fmt, style.hilited_back_color);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_candidate_shadow_color"),
        style.hilited_candidate_shadow_color, fmt, TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_shadow_color"), style.hilited_shadow_color,
        fmt, TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/candidate_shadow_color"),
        style.candidate_shadow_color, fmt, TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/candidate_border_color"),
        style.candidate_border_color, fmt, TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_candidate_border_color"),
        style.hilited_candidate_border_color, fmt, TRANSPARENT_COLOR);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/label_color"), style.label_text_color, fmt,
        blend_colors(style.candidate_text_color, style.candidate_back_color));
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_label_color"),
        style.hilited_label_text_color, fmt,
        blend_colors(style.hilited_candidate_text_color,
                     style.hilited_candidate_back_color));
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/comment_text_color"),
                                       style.comment_text_color, fmt,
                                       style.label_text_color);
    _RimeConfigGetColor32bWithFallback(
        config, (prefix + "/hilited_comment_text_color"),
        style.hilited_comment_text_color, fmt, style.hilited_label_text_color);
    _RimeConfigGetColor32bWithFallback(config, (prefix + "/hilited_mark_color"),
                                       style.hilited_mark_color, fmt,
                                       TRANSPARENT_COLOR);
    return true;
  }
  return false;
}

static void _LoadAppOptions(RimeConfig* config,
                            AppOptionsByAppName& app_options) {
  app_options.clear();
  RimeConfigIterator app_iter;
  RimeConfigIterator option_iter;
  rime_api->config_begin_map(&app_iter, config, "app_options");
  while (rime_api->config_next(&app_iter)) {
    AppOptions& options(app_options[app_iter.key]);
    rime_api->config_begin_map(&option_iter, config, app_iter.path);
    while (rime_api->config_next(&option_iter)) {
      Bool value = False;
      if (rime_api->config_get_bool(config, option_iter.path, &value)) {
        options[option_iter.key] = !!value;
      }
    }
    rime_api->config_end(&option_iter);
  }
  rime_api->config_end(&app_iter);
}

void RimeWithWeaselHandler::_GetStatus(Status& stat,
                                       WeaselSessionId ipc_id,
                                       Context& ctx) {
  SessionStatus& session_status = get_session_status(ipc_id);
  RimeSessionId session_id = session_status.session_id;
  RIME_STRUCT(RimeStatus, status);
  if (rime_api->get_status(session_id, &status)) {
    std::string schema_id = "";
    if (status.schema_id)
      schema_id = status.schema_id;
    stat.schema_name = string_to_wstring(status.schema_name, CP_UTF8);
    stat.schema_id = string_to_wstring(status.schema_id, CP_UTF8);
    stat.ascii_mode = !!status.is_ascii_mode;
    stat.composing = !!status.is_composing;
    stat.disabled = !!status.is_disabled;
    stat.full_shape = !!status.is_full_shape;
    if (schema_id != m_last_schema_id) {
      session_status.__synced = false;
      m_last_schema_id = schema_id;
      if (schema_id != ".default") {  // don't load for schema select menu
        bool inline_preedit = session_status.style.inline_preedit;
        _LoadSchemaSpecificSettings(ipc_id, schema_id);
        _LoadAppInlinePreeditSet(ipc_id, true);
        if (session_status.style.inline_preedit != inline_preedit)
          // in case of inline_preedit set in schema
          _UpdateInlinePreeditStatus(ipc_id);
        // refresh icon after schema changed
        _RefreshTrayIcon(session_id, _UpdateUICallback);
        m_ui->style() = session_status.style;
        if (m_show_notifications.find("schema") != m_show_notifications.end() &&
            m_show_notifications_time > 0) {
          ctx.aux.str = stat.schema_name;
          m_ui->Update(ctx, stat);
          m_ui->ShowWithTimeout(m_show_notifications_time);
        }
      }
    }
    rime_api->free_status(&status);
  }
}

void RimeWithWeaselHandler::_GetContext(Context& weasel_context,
                                        RimeSessionId session_id) {
  RIME_STRUCT(RimeContext, ctx);
  if (rime_api->get_context(session_id, &ctx)) {
    if (ctx.composition.length > 0) {
      weasel_context.preedit.str =
          string_to_wstring(ctx.composition.preedit, CP_UTF8);
      if (ctx.composition.sel_start < ctx.composition.sel_end) {
        TextAttribute attr;
        attr.type = HIGHLIGHTED;
        attr.range.start =
            utf8towcslen(ctx.composition.preedit, ctx.composition.sel_start);
        attr.range.end =
            utf8towcslen(ctx.composition.preedit, ctx.composition.sel_end);

        weasel_context.preedit.attributes.push_back(attr);
      }
    }
    if (ctx.menu.num_candidates) {
      CandidateInfo& cinfo(weasel_context.cinfo);
      _GetCandidateInfo(cinfo, ctx);
    }
    rime_api->free_context(&ctx);
  }
}

bool RimeWithWeaselHandler::_IsSessionTSF(RimeSessionId session_id) {
  static char client_type[20] = {0};
  rime_api->get_property(session_id, "client_type", client_type,
                         sizeof(client_type) - 1);
  return std::string(client_type) == "tsf";
}

void RimeWithWeaselHandler::_UpdateInlinePreeditStatus(WeaselSessionId ipc_id) {
  if (!m_ui)
    return;
  SessionStatus& session_status = get_session_status(ipc_id);
  RimeSessionId session_id = session_status.session_id;
  // set inline_preedit option
  bool inline_preedit =
      session_status.style.inline_preedit && _IsSessionTSF(session_id);
  rime_api->set_option(session_id, "inline_preedit", Bool(inline_preedit));
  // show soft cursor on weasel panel but not inline
  rime_api->set_option(session_id, "soft_cursor", Bool(!inline_preedit));
}
