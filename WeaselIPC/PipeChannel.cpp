#include "stdafx.h"

#include <PipeChannel.h>

using namespace weasel;
using namespace std;
using namespace boost;

#define _ThrowLastError throw GetLastError()


PipeChannelBase::PipeChannelBase(std::wstring &pn_cmd, size_t bs = 4 * 1024, SECURITY_ATTRIBUTES *s = NULL)
	: cmd_name(pn_cmd),
	msg_name(pn_cmd), // TBD: request msg pipe from cmd pipe
	write_stream(nullptr),
	buff_size(bs),
	buffer(std::make_unique<char[]>(bs)),
	msg_pipe(INVALID_HANDLE_VALUE),
	cmd_pipe(INVALID_HANDLE_VALUE),
	has_body(false),
	sa(s) {};

PipeChannelBase::PipeChannelBase(PipeChannelBase &&r)
	: cmd_name(r.cmd_name),
	write_stream(std::move(r.write_stream)),
	buff_size(r.buff_size),
	buffer(std::move(r.buffer)),
	msg_pipe(r.msg_pipe),
	cmd_pipe(r.cmd_pipe),
	has_body(r.has_body),
	sa(r.sa) {};


PipeChannelBase::~PipeChannelBase()
{
	_FinalizePipe(msg_pipe);
	_FinalizePipe(cmd_pipe);
}

bool PipeChannelBase::_Ensure()
{
	try {
		/* TBD: Request message pipe from command pipe */
		if (_Invalid(msg_pipe)) {
			//msg_pipe = _Connect(msg_name.c_str());
			msg_pipe = _Connect(cmd_name.c_str());
			return !_Invalid(msg_pipe);
		}
	}
	catch (...) {
		return false;
	}

	return true;
}

HANDLE PipeChannelBase::_Connect(const wchar_t *name)
{
	HANDLE pipe = INVALID_HANDLE_VALUE;
	while (_Invalid(pipe = _TryConnect(name)))
		::WaitNamedPipe(name, 500);
	DWORD mode = PIPE_READMODE_MESSAGE;
	if (!SetNamedPipeHandleState(pipe, &mode, NULL, NULL)) {
		_ThrowLastError;
	}
	return pipe;
}

void PipeChannelBase::_Reconnect(HANDLE pipe)
{
	_FinalizePipe(pipe);
	_Ensure();
}

HANDLE PipeChannelBase::_TryConnect(const wchar_t *name)
{
	DWORD connectErr;
	auto pipe = ::CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (!_Invalid(pipe)) {
		// connected to the pipe
		return pipe;
	}
	// being busy is not really an error since we just need to wait.
	if ((connectErr = ::GetLastError()) != ERROR_PIPE_BUSY) {

		throw connectErr; // otherwise, pipe creation fails
	}
	// All pipe instances are busy
	return INVALID_HANDLE_VALUE;
}

size_t PipeChannelBase::_WritePipe(HANDLE p, size_t s, char *b)
{
	DWORD lwritten;
	if (!::WriteFile(p, b, s, &lwritten, NULL) || lwritten <= 0) {
		_ThrowLastError;
	}
	::FlushFileBuffers(p);
	return lwritten;
}

void PipeChannelBase::_FinalizePipe(HANDLE pipe)
{
	if (!_Invalid(pipe)) {
		DisconnectNamedPipe(pipe);
		CloseHandle(pipe);
		pipe = INVALID_HANDLE_VALUE;
	}

}

void PipeChannelBase::_Receive(HANDLE pipe, LPVOID msg, size_t rec_len)
{
	DWORD lread;
	BOOL success = ::ReadFile(pipe, msg, rec_len, &lread, NULL);
	if (!success) {
		if (GetLastError() != ERROR_MORE_DATA) {
			_ThrowLastError;
		}
		memset(buffer.get(), 0, buff_size);
		success = ::ReadFile(pipe, buffer.get(), buff_size, &lread, NULL);
		if (!success) {
			_ThrowLastError;
		}
	}
	has_body = false;
}

HANDLE PipeChannelBase::_ConnectServerPipe(std::wstring &pn)
{
	HANDLE pipe = CreateNamedPipe(
		pn.c_str(),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		buff_size,
		buff_size,
		0,
		sa);
	if (pipe == INVALID_HANDLE_VALUE || !::ConnectNamedPipe(pipe, NULL)) {
		_ThrowLastError;
	}
	return pipe;
}

