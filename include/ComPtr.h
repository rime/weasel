#ifndef __COMPTR_H__
#define __COMPTR_H__

#ifndef ASSERT
#include <cassert>
#define ASSERT assert
#endif // ASSERT

template <typename Interface>
class ComPtr
{
	template <typename Interface>
	class RemoveAddRefRelease : public Interface
	{
		ULONG __stdcall AddRef();
		ULONG __stdcall Release();
	};
	template <typename T>
	friend class ComPtr;

public:
	ComPtr() noexcept = default;
	ComPtr(ComPtr const & other) noexcept
		: m_ptr(other.m_ptr)
	{
		InternalAddRef();
	}

	template <typename T>
	ComPtr(ComPtr<T> const & other) noexcept
		: m_ptr(other.m_ptr)
	{
		InternalAddRef();
	}

	ComPtr(ComPtr && other) noexcept
		: m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}

	template <typename T>
	ComPtr(ComPtr<T> && other) noexcept
		: m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}

	~ComPtr() noexcept
	{
		InternalRelease();
	}

	explicit operator bool() const noexcept
	{
		return nullptr != m_ptr;
	}

	RemoveAddRefRelease<Interface> * operator->() const noexcept
	{
		return static_cast<RemoveAddRefRelease<Interface> *>(m_ptr);
	}

	ComPtr & operator=(ComPtr const & other) noexcept
	{
		InternalCopy(other.m_ptr);
		return *this;
	}

	bool operator==(std::nullptr_t null) noexcept
	{
		return m_ptr == null;
	}

	bool operator!=(std::nullptr_t null) noexcept
	{
		return m_ptr != null;
	}

	template <typename T>
	ComPtr & operator=(ComPtr<T> const & other) noexcept
	{
		InternalCopy(other.m_ptr);
		return *this;
	}

	template <typename T>
	ComPtr & operator=(ComPtr<T> && other) noexcept
	{
		InternalMove(other);
		return *this;
	}

	void Swap(ComPtr & other) noexcept
	{
		Interface * temp = m_ptr;
		m_ptr = other.m_ptr;
		other.m_ptr = temp;
	}

	void Reset() noexcept
	{
		InternalRelease();
	}

	Interface * Get() const noexcept
	{
		return m_ptr;
	}

	Interface * Detach() noexcept
	{
		Interface * temp = m_ptr;
		m_ptr = nullptr;
		return temp;
	}

	void Copy(Interface * other) noexcept
	{
		InternalCopy(other);
	}

	void Attach(Interface * other) noexcept
	{
		InternalRelease();
		m_ptr = other;
	}

	Interface ** GetAddressOf() noexcept
	{
		ASSERT(m_ptr == nullptr);
		return &m_ptr;
	}

	void CopyTo(Interface ** other) const noexcept
	{
		InternalAddRef();
		*other = m_ptr;
	}

	template <typename T>
	ComPtr<T> As() const noexcept
	{
		ComPtr<T> temp;
		m_ptr->QueryInterface(temp.GetAddressOf());
		return temp;
	}

	HRESULT CoCreate(const CLSID &clsid, IUnknown *outer, DWORD context)
	{
		InternalRelease();
		HRESULT hr;
		IID iid = __uuidof(Interface);
		if (context & (CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)) {
			IUnknown *unknown = NULL;
			hr = CoCreateInstance(clsid, outer, context, IID_IUnknown,
				reinterpret_cast<void**>(&unknown));
			if (SUCCEEDED(hr) && unknown != NULL) {
				hr = OleRun(unknown);
				if (SUCCEEDED(hr))
					hr = unknown->QueryInterface(iid, reinterpret_cast<void**>(&m_ptr));
				unknown->Release();
			}
		}
		else {
			hr = CoCreateInstance(clsid, outer, context, iid,
				reinterpret_cast<void**>(&m_ptr));
		}
		return hr;
	}

private:
	void InternalAddRef() const noexcept
	{
		if (m_ptr)
		{
			m_ptr->AddRef();
		}
	}

	void InternalRelease() noexcept
	{
		Interface * temp = m_ptr;
		if (temp)
		{
			m_ptr = nullptr;
			temp->Release();
		}
	}

	void InternalCopy(Interface * other) noexcept
	{
		if (m_ptr != other)
		{
			InternalRelease();
			m_ptr = other;
			InternalAddRef();
		}
	}

	template <typename T>
	void InternalMove(ComPtr<T> & other) noexcept
	{
		if (m_ptr != other.m_ptr)
		{
			InternalRelease();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
	}

	Interface * m_ptr = nullptr;
};

template <typename Interface>
void swap(ComPtr<Interface> & left,
	ComPtr<Interface> & right) noexcept
{
	left.Swap(right);
}

#endif // __COMPTR_H__