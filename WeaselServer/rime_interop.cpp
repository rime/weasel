#include "stdafx.h"
#include "rime_interop.h"
#include <rime_api.h>
#include <weasel/util.h>
#include <weasel/version.h>
#include <weasel/log.h>

namespace weasel::server
{

namespace
{

template <typename T>
T new_rime_struct()
{
  RIME_STRUCT(T, x);
  return x;
}

void rime_config_close(RimeConfig *c)
{
  RimeConfigClose(c);
  delete c;
}
using unique_rime_config_t = std::unique_ptr<RimeConfig, decltype(&rime_config_close)>;
unique_rime_config_t open_rime_config(const char* config_id) noexcept
{
  auto *c = new RimeConfig;
  unique_rime_config_t x(nullptr, &rime_config_close);
  if (RimeConfigOpen(config_id, c))
  {
    x.reset(c);
  } else
  {
    delete c;
  }
  return x;
}

void on_rime_notify(void* context_object, uintptr_t session_id, const char* message_type,
  const char* message_value)
{
  if (auto *m = static_cast<rime*>(context_object))
  {
    m->set_notify_msg(message_type, message_value);
  }
}

bool is_deployer_running()
{
  HANDLE h = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
  bool r = h && GetLastError() == ERROR_ALREADY_EXISTS;
  if (h) CloseHandle(h);
  return r;
}

void load_app_options(RimeConfig* config, std::unordered_map<std::string, rime::app_options_t> &map)
{
  /*
  app_options:
    cmd.exe:
      ascii_mode: true
    conhost.exe:
      ascii_mode: true
  */
  
  map.clear();
  RimeConfigIterator it_app, it_opt;

  RimeConfigBeginMap(&it_app, config, "app_options");
  while (RimeConfigNext(&it_app))
  {
    rime::app_options_t& opts(map[it_app.key]);

    RimeConfigBeginMap(&it_opt, config, it_app.path);
    while (RimeConfigNext(&it_opt))
    {
      Bool value = false;
      if (RimeConfigGetBool(config, it_opt.path, &value))
      {
        opts[it_opt.key] = !!value;
      }
    }
    RimeConfigEnd(&it_opt);
  }
  RimeConfigEnd(&it_app);
}

}

rime::rime()
  : disabled_(true)
{
  // setup rime
  auto traits = new_rime_struct<RimeTraits>();
  traits.shared_data_dir = utils::ws_to_u8s(utils::shared_data_dir()).c_str();
  traits.user_data_dir = utils::ws_to_u8s(utils::user_data_dir()).c_str();
  traits.prebuilt_data_dir = traits.shared_data_dir;
  traits.distribution_name = WEASEL_IME_NAME_U8;
  traits.distribution_code_name = WEASEL_CODE_NAME;
  traits.distribution_version = WEASEL_VERSION_STRING;
  traits.app_name = "rime.weasel";
  RimeSetup(&traits);
  // install notification handler
  RimeSetNotificationHandler(&on_rime_notify, this);
}

void rime::init()
{
  disabled_ = is_deployer_running();
  if (disabled_) return;

  LOG(info, "Initializing la rime");
  RimeInitialize(NULL);
  if (RimeStartMaintenance(false)) disabled_ = true;

  auto config = open_rime_config("weasel");
  if (config)
  {
		/*
		if (m_ui)
		{
			_UpdateUIStyle(&config, m_ui, true);
			m_base_style = m_ui->style();
		}
		*/
    load_app_options(config.get(), app_options_);
  }
  last_schema_id_.clear();
}

void rime::finalize()
{
  active_session_ = 0;
  disabled_ = true;
  LOG(info, "Finalizing la rime");
  RimeFinalize();
}

session_id_t rime::find_session(session_id_t session_id) const
{
  if (disabled_) return 0;
  const bool found = RimeFindSession(session_id);
  return found ? session_id : 0;
}

session_id_t rime::add_session(const ipc::buffer& in, ipc::buffer& out)
{
  if (disabled_)
  {
    // trying to resume service
    end_maintenance();
    if (disabled_) return 0;
  }

  session_id_t id = RimeCreateSession();
  // read_client_info(id, in);

  auto status = new_rime_struct<RimeStatus>();
  if (RimeGetStatus(id, &status))
  {
    std::string schema_id = status.schema_id;
    last_schema_id_ = schema_id;
    // LoadSchemaSpecificSettings
    // UpdateInlinePreeditStatus
    // RefreshTrayIcon
  }
  // Respond
  // UpdateUI
  active_session_ = id;
  return id;
}

session_id_t rime::remove_session(session_id_t session_id)
{
  // TODO
  return 0;
}

void rime::end_maintenance()
{
}

void rime::set_notify_msg(const char* message_type, const char* message_value)
{
  if (!message_type || !message_value) return;
  notify_msg_type_ = message_type;
  notify_msg_value_ = message_value;
}

}
