#include "stdafx.h"
#include "WeaselClientImpl.h"
#include <StringAlgorithm.hpp>

using namespace weasel;
using namespace weasel::ipc;

Client::Client() 
	: client_(new client())
{}

Client::~Client()
{
  if (client_)
  {
    delete client_;
    client_ = nullptr;
  }
}

bool Client::Connect(ServerLauncher launcher)
{
  return true;
}

void Client::Disconnect()
{
}

void Client::ShutdownServer()
{
  client_->shutdown_server();
}

bool Client::ProcessKeyEvent(KeyEvent const& keyEvent)
{
  return client_->process_key_event(keyEvent);
}

bool Client::CommitComposition()
{
  return client_->commit_composition();
}

bool Client::ClearComposition()
{
  return client_->clear_composition();
}

void Client::UpdateInputPosition(RECT const& rc)
{
  return client_->update_input_position(rc);
}

void Client::FocusIn()
{
  client_->focus_in();
}

void Client::FocusOut()
{
  client_->focus_out();
}

void Client::StartSession()
{
  client_->start_session();
}

void Client::EndSession()
{
  client_->end_session();
}

void Client::StartMaintenance()
{
  client_->start_maintenance();
}

void Client::EndMaintenance()
{
  client_->end_maintenance();
}

void Client::TrayCommand(UINT menuId)
{
  client_->tray_command(menuId);
}

bool Client::Echo()
{
  return client_->echo();
}

bool Client::GetResponseData(ResponseHandler handler)
{
  return client_->get_response_data(handler);
}
