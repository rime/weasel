#pragma once
#include <string>
#include <memory>
#include <windows.h>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/thread.hpp>

namespace weasel {

	class PipeChannelBase {
	public:
		using Stream = boost::interprocess::wbufferstream;

		PipeChannelBase(std::wstring &&pn_cmd, size_t bs, SECURITY_ATTRIBUTES *s);
		PipeChannelBase(PipeChannelBase &&r);
		~PipeChannelBase();

	protected:

		/* To ensure connection before operation */
		bool _Ensure();
		/* Connect pipe as client */
		HANDLE _Connect(const wchar_t *name);
		/* To reconnect message pipe */
		void _Reconnect();
		/* Try to connect for one time */
		HANDLE _TryConnect();
		size_t _WritePipe(HANDLE p, size_t s, char *b);
		void _FinalizePipe(HANDLE &p);
		void _Receive(HANDLE pipe, LPVOID msg, size_t rec_len);
		/* Try to get a connection from client */
		HANDLE _ConnectServerPipe(std::wstring &pn);
		inline bool _Invalid(HANDLE p) const { return p == INVALID_HANDLE_VALUE; }

	protected:
		std::wstring pname;
		HANDLE hpipe;

		bool has_body;
		const size_t buff_size;
		std::unique_ptr<char[]> buffer;
		std::unique_ptr<Stream> write_stream;

	private:
		/* Security attributes */
		SECURITY_ATTRIBUTES *sa;
	};


	/* Pipe based IPC channel */
	template<
		typename _TyMsg,
		typename _TyRes = DWORD,
		size_t _MsgSize = sizeof(_TyMsg),
		size_t _ResSize = sizeof(_TyRes)>
	class PipeChannel : public PipeChannelBase
	{
	public:

		/* Type definitions */

		using Ptr = std::shared_ptr<PipeChannel>;
		using UPtr = std::unique_ptr<PipeChannel>;
		using Msg = _TyMsg;
		using Res = _TyRes;

		enum class ChannalCommand {
			NEW_MSG_PIPE,
			REFRESH
		};

	public:
		PipeChannel(std::wstring &&pn_cmd, SECURITY_ATTRIBUTES *s = NULL, size_t bs = 64 * 1024)
			: PipeChannelBase(std::move(pn_cmd), bs, s)
		{}

	public:
		/* Common pipe operations */

		bool Connect() { return _Ensure(); }
		bool Connected() const { return !_Invalid(hpipe); }
		void Disconnect() { _FinalizePipe(hpipe); }

		/* Write data to buffer */

		template<typename _TyWrite>
		void Write(_TyWrite cnt)
		{
			has_body = true;
			_BufferWriteStream() << cnt;
		}

		/* Write data to buffer */
		template<typename _TyWrite>
		PipeChannel& operator<<(_TyWrite cnt)
		{
			Write(cnt);
			return *this;
		}

		_TyRes Transact(Msg &msg)
		{
			_Ensure();
			_Send(hpipe, msg);
			return _ReceiveResponse();
		}


		void ClearBufferStream()
		{
			has_body = false;
			if (write_stream != nullptr) {
				delete write_stream.release();
				write_stream.reset(nullptr);
			}
		}

		char *SendBuffer() const { return buffer.get() + _MsgSize; }

		char *ReceiveBuffer() const { return buffer.get() + _ResSize; }

		template<typename _TyHandler>
		bool HandleResponseData(_TyHandler const &handler)
		{
			if (!handler) {
				return false;
			}

			// Use whole buffer to receive data in client
			return handler((LPWSTR)buffer.get(), (UINT)(buff_size * sizeof(char) / sizeof(wchar_t)));
		}


	protected:

		void _Send(HANDLE pipe, Msg &msg)
		{
			char *pbuff = buffer.get();
			DWORD lwritten = 0;

			*reinterpret_cast<Msg *>(pbuff) = msg;
			size_t data_sz = has_body ? buff_size : _MsgSize;

			try {
				_WritePipe(pipe, data_sz, pbuff);
			}
			catch (...) {
				_Reconnect();
				_WritePipe(pipe, data_sz, pbuff);
			}
			ClearBufferStream();
		}

		_TyRes _ReceiveResponse()
		{
			_TyRes result;
			_Receive(hpipe, &result, sizeof(result));
			return result;
		}

		Stream& _BufferWriteStream()
		{
			if (write_stream == nullptr) {
				char *pbuff = (char *)buffer.get() + _MsgSize;
				memset(pbuff, 0, buff_size - _MsgSize);
				write_stream = std::make_unique<Stream>((wchar_t *)pbuff, _SendBufferSizeW());
			}
			return *write_stream;
		}

	private:

		inline size_t _SendBufferSizeW() const
		{
			return (buff_size - _MsgSize) * sizeof(char) / sizeof(wchar_t);
		}

		inline size_t _ReceiveBufferSizeW() const
		{
			return (buff_size - _ResSize) * sizeof(char) / sizeof(wchar_t);
		}

	};
};

