// Windows Template Library - WTL version 10.0
// Copyright (C) Microsoft Corporation, WTL Team. All rights reserved.
//
// This file is a part of the Windows Template Library.
// The use and distribution terms for this software are covered by the
// Microsoft Public License (http://opensource.org/licenses/MS-PL)
// which can be found in the file MS-PL.txt at the root folder.

#ifndef __ATLMISC_H__
#define __ATLMISC_H__

#pragma once

#ifndef __ATLAPP_H__
	#error atlmisc.h requires atlapp.h to be included first
#endif

#ifndef _WTL_NO_COMPATIBILITY_INCLUDES
  #include <atlstr.h>
  #include <atltypes.h>
#endif // _WTL_NO_COMPATIBILITY_INCLUDES


///////////////////////////////////////////////////////////////////////////////
// Classes in this file:
//
// CRecentDocumentListBase<T, t_cchItemLen, t_nFirstID, t_nLastID>
// CRecentDocumentList
// CFindFile
// CRegProperty
// CRegPropertyImpl<T>
//
// Global functions:
//   AtlGetStockPen()
//   AtlGetStockBrush()
//   AtlGetStockFont()
//   AtlGetStockPalette()
//
//   AtlCompactPath()


namespace WTL
{

///////////////////////////////////////////////////////////////////////////////
// CSize scalar operators 

#if !defined(_WTL_NO_SIZE_SCALAR) && defined(__ATLTYPES_H__)

template <class Num>
inline CSize operator *(SIZE s, Num n) 
{
	return CSize((int)(s.cx * n), (int)(s.cy * n));
};

template <class Num>
inline void operator *=(SIZE & s, Num n)
{
	s = s * n;
};	

template <class Num>
inline CSize operator /(SIZE s, Num n) 
{
	return CSize((int)(s.cx / n), (int)(s.cy / n));
};

template <class Num>
inline void operator /=(SIZE & s, Num n)
{
	s = s / n;
};	

#endif // !defined(_WTL_NO_SIZE_SCALAR) && defined(__ATLTYPES_H__)


///////////////////////////////////////////////////////////////////////////////
// CRecentDocumentList - MRU List Support

#ifndef _WTL_MRUEMPTY_TEXT
  #define _WTL_MRUEMPTY_TEXT	_T("(empty)")
#endif

// forward declaration
inline bool AtlCompactPath(LPTSTR lpstrOut, LPCTSTR lpstrIn, int cchLen);

template <class T, int t_cchItemLen = MAX_PATH, int t_nFirstID = ID_FILE_MRU_FIRST, int t_nLastID = ID_FILE_MRU_LAST>
class CRecentDocumentListBase
{
public:
// Declarations
	struct _DocEntry
	{
		TCHAR szDocName[t_cchItemLen];
		bool operator ==(const _DocEntry& de) const
		{ return (lstrcmpi(szDocName, de.szDocName) == 0); }
	};

	enum
	{
		m_nMaxEntries_Min = 2,
		m_nMaxEntries_Max = t_nLastID - t_nFirstID + 1,
		m_cchMaxItemLen_Min = 6,
		m_cchMaxItemLen_Max = t_cchItemLen,
		m_cchItemNameLen = 11
	};

// Data members
	ATL::CSimpleArray<_DocEntry> m_arrDocs;
	int m_nMaxEntries;   // default is 4
	HMENU m_hMenu;

	TCHAR m_szNoEntries[t_cchItemLen];

	int m_cchMaxItemLen;

// Constructor
	CRecentDocumentListBase() : m_nMaxEntries(4), m_hMenu(NULL), m_cchMaxItemLen(-1)
	{
		m_szNoEntries[0] = 0;

		// These ASSERTs verify values of the template arguments
		ATLASSERT(t_cchItemLen > m_cchMaxItemLen_Min);
		ATLASSERT(m_nMaxEntries_Max > m_nMaxEntries_Min);
	}

// Attributes
	HMENU GetMenuHandle() const
	{
		return m_hMenu;
	}

	void SetMenuHandle(HMENU hMenu)
	{
		ATLASSERT((hMenu == NULL) || ::IsMenu(hMenu));
		m_hMenu = hMenu;
		if((m_hMenu == NULL) || (::GetMenuString(m_hMenu, t_nFirstID, m_szNoEntries, t_cchItemLen, MF_BYCOMMAND) == 0))
		{
			T* pT = static_cast<T*>(this);
			(void)pT;   // avoid level 4 warning
			ATL::Checked::tcsncpy_s(m_szNoEntries, _countof(m_szNoEntries), pT->GetMRUEmptyText(), _TRUNCATE);
		}
	}

	int GetMaxEntries() const
	{
		return m_nMaxEntries;
	}

	void SetMaxEntries(int nMaxEntries)
	{
		ATLASSERT((nMaxEntries >= m_nMaxEntries_Min) && (nMaxEntries <= m_nMaxEntries_Max));
		if(nMaxEntries < m_nMaxEntries_Min)
			nMaxEntries = m_nMaxEntries_Min;
		else if(nMaxEntries > m_nMaxEntries_Max)
			nMaxEntries = m_nMaxEntries_Max;
		m_nMaxEntries = nMaxEntries;
	}

	int GetMaxItemLength() const
	{
		return m_cchMaxItemLen;
	}

	void SetMaxItemLength(int cchMaxLen)
	{
		ATLASSERT(((cchMaxLen >= m_cchMaxItemLen_Min) && (cchMaxLen <= m_cchMaxItemLen_Max)) || (cchMaxLen == -1));
		if(cchMaxLen != -1)
		{
			if(cchMaxLen < m_cchMaxItemLen_Min)
				cchMaxLen = m_cchMaxItemLen_Min;
			else if(cchMaxLen > m_cchMaxItemLen_Max)
				cchMaxLen = m_cchMaxItemLen_Max;
		}
		m_cchMaxItemLen = cchMaxLen;
		T* pT = static_cast<T*>(this);
		pT->UpdateMenu();
	}

// Operations
	BOOL AddToList(LPCTSTR lpstrDocName)
	{
		_DocEntry de;
		errno_t nRet = ATL::Checked::tcsncpy_s(de.szDocName, _countof(de.szDocName), lpstrDocName, _TRUNCATE);
		if((nRet != 0) && (nRet != STRUNCATE))
			return FALSE;

		for(int i = 0; i < m_arrDocs.GetSize(); i++)
		{
			if(lstrcmpi(m_arrDocs[i].szDocName, lpstrDocName) == 0)
			{
				m_arrDocs.RemoveAt(i);
				break;
			}
		}

		if(m_arrDocs.GetSize() == m_nMaxEntries)
			m_arrDocs.RemoveAt(0);

		BOOL bRet = m_arrDocs.Add(de);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			bRet = pT->UpdateMenu();
		}
		return bRet;
	}

	// This function is deprecated because it is not safe. 
	// Use the version below that accepts the buffer length.
	__declspec(deprecated)
	BOOL GetFromList(int /*nItemID*/, LPTSTR /*lpstrDocName*/)
	{
		ATLASSERT(FALSE);
		return FALSE;
	}

	BOOL GetFromList(int nItemID, LPTSTR lpstrDocName, int cchLength)
	{
		int nIndex = m_arrDocs.GetSize() - (nItemID - t_nFirstID) - 1;
		if((nIndex < 0) || (nIndex >= m_arrDocs.GetSize()))
			return FALSE;
		if(lstrlen(m_arrDocs[nIndex].szDocName) >= cchLength)
			return FALSE;
		ATL::Checked::tcscpy_s(lpstrDocName, cchLength, m_arrDocs[nIndex].szDocName);

		return TRUE;
	}

#ifdef __ATLSTR_H__
	BOOL GetFromList(int nItemID, ATL::CString& strDocName)
	{
		int nIndex = m_arrDocs.GetSize() - (nItemID - t_nFirstID) - 1;
		if((nIndex < 0) || (nIndex >= m_arrDocs.GetSize()))
			return FALSE;
		strDocName = m_arrDocs[nIndex].szDocName;
		return TRUE;
	}
#endif // __ATLSTR_H__

	BOOL RemoveFromList(int nItemID)
	{
		int nIndex = m_arrDocs.GetSize() - (nItemID - t_nFirstID) - 1;
		BOOL bRet = m_arrDocs.RemoveAt(nIndex);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			bRet = pT->UpdateMenu();
		}
		return bRet;
	}

	BOOL MoveToTop(int nItemID)
	{
		int nIndex = m_arrDocs.GetSize() - (nItemID - t_nFirstID) - 1;
		if((nIndex < 0) || (nIndex >= m_arrDocs.GetSize()))
			return FALSE;
		_DocEntry de;
		de = m_arrDocs[nIndex];
		m_arrDocs.RemoveAt(nIndex);
		BOOL bRet = m_arrDocs.Add(de);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			bRet = pT->UpdateMenu();
		}
		return bRet;
	}

	BOOL ReadFromRegistry(LPCTSTR lpstrRegKey)
	{
		T* pT = static_cast<T*>(this);
		ATL::CRegKey rkParent;
		ATL::CRegKey rk;

		LONG lRet = rkParent.Open(HKEY_CURRENT_USER, lpstrRegKey);
		if(lRet != ERROR_SUCCESS)
			return FALSE;
		lRet = rk.Open(rkParent, pT->GetRegKeyName());
		if(lRet != ERROR_SUCCESS)
			return FALSE;

		DWORD dwRet = 0;
		lRet = rk.QueryDWORDValue(pT->GetRegCountName(), dwRet);
		if(lRet != ERROR_SUCCESS)
			return FALSE;
		SetMaxEntries(dwRet);

		m_arrDocs.RemoveAll();

		TCHAR szRetString[t_cchItemLen] = {};
		_DocEntry de;

		for(int nItem = m_nMaxEntries; nItem > 0; nItem--)
		{
			TCHAR szBuff[m_cchItemNameLen] = {};
			_stprintf_s(szBuff, m_cchItemNameLen, pT->GetRegItemName(), nItem);
			ULONG ulCount = t_cchItemLen;
			lRet = rk.QueryStringValue(szBuff, szRetString, &ulCount);
			if(lRet == ERROR_SUCCESS)
			{
				ATL::Checked::tcscpy_s(de.szDocName, _countof(de.szDocName), szRetString);
				m_arrDocs.Add(de);
			}
		}

		rk.Close();
		rkParent.Close();

		return pT->UpdateMenu();
	}

	BOOL WriteToRegistry(LPCTSTR lpstrRegKey)
	{
		T* pT = static_cast<T*>(this);
		(void)pT;   // avoid level 4 warning
		ATL::CRegKey rkParent;
		ATL::CRegKey rk;

		LONG lRet = rkParent.Create(HKEY_CURRENT_USER, lpstrRegKey);
		if(lRet != ERROR_SUCCESS)
			return FALSE;
		lRet = rk.Create(rkParent, pT->GetRegKeyName());
		if(lRet != ERROR_SUCCESS)
			return FALSE;

		lRet = rk.SetDWORDValue(pT->GetRegCountName(), m_nMaxEntries);
		ATLASSERT(lRet == ERROR_SUCCESS);

		// set new values
		int nItem;
		for(nItem = m_arrDocs.GetSize(); nItem > 0; nItem--)
		{
			TCHAR szBuff[m_cchItemNameLen] = {};
			_stprintf_s(szBuff, m_cchItemNameLen, pT->GetRegItemName(), nItem);
			TCHAR szDocName[t_cchItemLen] = {};
			GetFromList(t_nFirstID + nItem - 1, szDocName, t_cchItemLen);
			lRet = rk.SetStringValue(szBuff, szDocName);
			ATLASSERT(lRet == ERROR_SUCCESS);
		}

		// delete unused keys
		for(nItem = m_arrDocs.GetSize() + 1; nItem <= m_nMaxEntries_Max; nItem++)
		{
			TCHAR szBuff[m_cchItemNameLen] = {};
			_stprintf_s(szBuff, m_cchItemNameLen, pT->GetRegItemName(), nItem);
			rk.DeleteValue(szBuff);
		}

		rk.Close();
		rkParent.Close();

		return TRUE;
	}

// Implementation
	BOOL UpdateMenu()
	{
		if(m_hMenu == NULL)
			return FALSE;
		ATLASSERT(::IsMenu(m_hMenu));

		int nItems = ::GetMenuItemCount(m_hMenu);
		int nInsertPoint = 0;
		for(int i = 0; i < nItems; i++)
		{
			CMenuItemInfo mi;
			mi.fMask = MIIM_ID;
			::GetMenuItemInfo(m_hMenu, i, TRUE, &mi);
			if (mi.wID == t_nFirstID)
			{
				nInsertPoint = i;
				break;
			}
		}

		ATLASSERT((nInsertPoint < nItems) && "You need a menu item with an ID = t_nFirstID");

		for(int j = t_nFirstID; j < (t_nFirstID + m_nMaxEntries); j++)
		{
			// keep the first one as an insertion point
			if (j != t_nFirstID)
				::DeleteMenu(m_hMenu, j, MF_BYCOMMAND);
		}

		TCHAR szItemText[t_cchItemLen + 6] = {};   // add space for &, 2 digits, and a space
		int nSize = m_arrDocs.GetSize();
		int nItem = 0;
		if(nSize > 0)
		{
			for(nItem = 0; nItem < nSize; nItem++)
			{
				if(m_cchMaxItemLen == -1)
				{
					_stprintf_s(szItemText, t_cchItemLen + 6, _T("&%i %s"), nItem + 1, m_arrDocs[nSize - 1 - nItem].szDocName);
				}
				else
				{
					TCHAR szBuff[t_cchItemLen] = {};
					T* pT = static_cast<T*>(this);
					(void)pT;   // avoid level 4 warning
					bool bRet = pT->CompactDocumentName(szBuff, m_arrDocs[nSize - 1 - nItem].szDocName, m_cchMaxItemLen);
					(void)bRet;   // avoid level 4 warning
					ATLASSERT(bRet);
					_stprintf_s(szItemText, t_cchItemLen + 6, _T("&%i %s"), nItem + 1, szBuff);
				}

				::InsertMenu(m_hMenu, nInsertPoint + nItem, MF_BYPOSITION | MF_STRING, t_nFirstID + nItem, szItemText);
			}
		}
		else	// empty
		{
			::InsertMenu(m_hMenu, nInsertPoint, MF_BYPOSITION | MF_STRING, t_nFirstID, m_szNoEntries);
			::EnableMenuItem(m_hMenu, t_nFirstID, MF_GRAYED);
			nItem++;
		}
		::DeleteMenu(m_hMenu, nInsertPoint + nItem, MF_BYPOSITION);

		return TRUE;
	}

// Overrideables
	// override to provide a different method of compacting document names
	static bool CompactDocumentName(LPTSTR lpstrOut, LPCTSTR lpstrIn, int cchLen)
	{
		return AtlCompactPath(lpstrOut, lpstrIn, cchLen);
	}

	static LPCTSTR GetRegKeyName()
	{
		return _T("Recent Document List");
	}

	static LPCTSTR GetRegCountName()
	{
		return _T("DocumentCount");
	}

	static LPCTSTR GetRegItemName()
	{
		// Note: This string is a format string used with wsprintf().
		// Resulting formatted string must be m_cchItemNameLen or less 
		// characters long, including the terminating null character.
		return _T("Document%i");
	}

	static LPCTSTR GetMRUEmptyText()
	{
		return _WTL_MRUEMPTY_TEXT;
	}
};

class CRecentDocumentList : public CRecentDocumentListBase<CRecentDocumentList>
{
public:
// nothing here
};


///////////////////////////////////////////////////////////////////////////////
// CFindFile - file search helper class

class CFindFile
{
public:
// Data members
	HANDLE m_hFind;
	WIN32_FIND_DATA m_fd;
	LPTSTR m_lpszRoot;
	const TCHAR m_chDirSeparator;
	BOOL m_bFound;

// Constructor/destructor
	CFindFile() : m_hFind(NULL), m_lpszRoot(NULL), m_chDirSeparator(_T('\\')), m_bFound(FALSE)
	{
		memset(&m_fd, 0, sizeof(m_fd));
	}

	~CFindFile()
	{
		Close();
	}

// Attributes
	ULONGLONG GetFileSize() const
	{
		ATLASSERT(m_hFind != NULL);

		ULARGE_INTEGER nFileSize = {};
		if(m_bFound)
		{
			nFileSize.LowPart = m_fd.nFileSizeLow;
			nFileSize.HighPart = m_fd.nFileSizeHigh;
		}
		else
		{
			nFileSize.QuadPart = 0;
		}

		return nFileSize.QuadPart;
	}

	BOOL GetFileName(LPTSTR lpstrFileName, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);
		if(lstrlen(m_fd.cFileName) >= cchLength)
			return FALSE;

		if(m_bFound)
			ATL::Checked::tcscpy_s(lpstrFileName, cchLength, m_fd.cFileName);

		return m_bFound;
	}

	BOOL GetFilePath(LPTSTR lpstrFilePath, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);

		int nLen = lstrlen(m_lpszRoot);
		ATLASSERT(nLen > 0);
		if(nLen == 0)
			return FALSE;

		bool bAddSep = (m_lpszRoot[nLen - 1] != m_chDirSeparator);

		if((lstrlen(m_lpszRoot) + (bAddSep ?  1 : 0)) >= cchLength)
			return FALSE;

		ATL::Checked::tcscpy_s(lpstrFilePath, cchLength, m_lpszRoot);

		if(bAddSep)
		{
			TCHAR szSeparator[2] = { m_chDirSeparator, 0 };
			ATL::Checked::tcscat_s(lpstrFilePath, cchLength, szSeparator);
		}

		ATL::Checked::tcscat_s(lpstrFilePath, cchLength, m_fd.cFileName);

		return TRUE;
	}

	BOOL GetFileTitle(LPTSTR lpstrFileTitle, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);

		TCHAR szBuff[MAX_PATH] = {};
		if(!GetFileName(szBuff, MAX_PATH))
			return FALSE;

		if(lstrlen(szBuff) >= cchLength)
			return FALSE;

		// find the last dot
		LPTSTR pstrDot  = _tcsrchr(szBuff, _T('.'));
		if(pstrDot != NULL)
			*pstrDot = 0;

		ATL::Checked::tcscpy_s(lpstrFileTitle, cchLength, szBuff);

		return TRUE;
	}

	BOOL GetFileURL(LPTSTR lpstrFileURL, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);

		LPCTSTR lpstrFileURLPrefix = _T("file://");
		const int cchPrefix = lstrlen(lpstrFileURLPrefix);
		if(cchPrefix >= cchLength)
			return FALSE;

		ATL::Checked::tcscpy_s(lpstrFileURL, cchLength, lpstrFileURLPrefix);

		return GetFilePath(&lpstrFileURL[cchPrefix], cchLength - cchPrefix);
	}

	BOOL GetRoot(LPTSTR lpstrRoot, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);
		if(lstrlen(m_lpszRoot) >= cchLength)
			return FALSE;

		ATL::Checked::tcscpy_s(lpstrRoot, cchLength, m_lpszRoot);

		return TRUE;
	}

#ifdef __ATLSTR_H__
	ATL::CString GetFileName() const
	{
		ATLASSERT(m_hFind != NULL);

		ATL::CString ret;

		if(m_bFound)
			ret = m_fd.cFileName;
		return ret;
	}

	ATL::CString GetFilePath() const
	{
		ATLASSERT(m_hFind != NULL);

		ATL::CString strResult = m_lpszRoot;
		int nLen = strResult.GetLength();
		ATLASSERT(nLen > 0);
		if(nLen == 0)
			return strResult;

		if(strResult[nLen - 1] != m_chDirSeparator)
			strResult += m_chDirSeparator;
		strResult += GetFileName();
		return strResult;
	}

	ATL::CString GetFileTitle() const
	{
		ATLASSERT(m_hFind != NULL);

		ATL::CString strResult;
		GetFileTitle(strResult.GetBuffer(MAX_PATH), MAX_PATH);
		strResult.ReleaseBuffer();

		return strResult;
	}

	ATL::CString GetFileURL() const
	{
		ATLASSERT(m_hFind != NULL);

		ATL::CString strResult("file://");
		strResult += GetFilePath();
		return strResult;
	}

	ATL::CString GetRoot() const
	{
		ATLASSERT(m_hFind != NULL);

		ATL::CString str = m_lpszRoot;
		return str;
	}
#endif // __ATLSTR_H__

	BOOL GetLastWriteTime(FILETIME* pTimeStamp) const
	{
		ATLASSERT(m_hFind != NULL);
		ATLASSERT(pTimeStamp != NULL);

		if(m_bFound && (pTimeStamp != NULL))
		{
			*pTimeStamp = m_fd.ftLastWriteTime;
			return TRUE;
		}

		return FALSE;
	}

	BOOL GetLastAccessTime(FILETIME* pTimeStamp) const
	{
		ATLASSERT(m_hFind != NULL);
		ATLASSERT(pTimeStamp != NULL);

		if(m_bFound && (pTimeStamp != NULL))
		{
			*pTimeStamp = m_fd.ftLastAccessTime;
			return TRUE;
		}

		return FALSE;
	}

	BOOL GetCreationTime(FILETIME* pTimeStamp) const
	{
		ATLASSERT(m_hFind != NULL);

		if(m_bFound && (pTimeStamp != NULL))
		{
			*pTimeStamp = m_fd.ftCreationTime;
			return TRUE;
		}

		return FALSE;
	}

	BOOL MatchesMask(DWORD dwMask) const
	{
		ATLASSERT(m_hFind != NULL);

		if(m_bFound)
			return ((m_fd.dwFileAttributes & dwMask) != 0);

		return FALSE;
	}

	BOOL IsDots() const
	{
		ATLASSERT(m_hFind != NULL);

		// return TRUE if the file name is "." or ".." and
		// the file is a directory

		BOOL bResult = FALSE;
		if(m_bFound && IsDirectory())
		{
			if((m_fd.cFileName[0] == _T('.')) && ((m_fd.cFileName[1] == _T('\0')) || ((m_fd.cFileName[1] == _T('.')) && (m_fd.cFileName[2] == _T('\0')))))
				bResult = TRUE;
		}

		return bResult;
	}

	BOOL IsReadOnly() const
	{
		return MatchesMask(FILE_ATTRIBUTE_READONLY);
	}

	BOOL IsDirectory() const
	{
		return MatchesMask(FILE_ATTRIBUTE_DIRECTORY);
	}

	BOOL IsCompressed() const
	{
		return MatchesMask(FILE_ATTRIBUTE_COMPRESSED);
	}

	BOOL IsSystem() const
	{
		return MatchesMask(FILE_ATTRIBUTE_SYSTEM);
	}

	BOOL IsHidden() const
	{
		return MatchesMask(FILE_ATTRIBUTE_HIDDEN);
	}

	BOOL IsTemporary() const
	{
		return MatchesMask(FILE_ATTRIBUTE_TEMPORARY);
	}

	BOOL IsNormal() const
	{
		return MatchesMask(FILE_ATTRIBUTE_NORMAL);
	}

	BOOL IsArchived() const
	{
		return MatchesMask(FILE_ATTRIBUTE_ARCHIVE);
	}

// Operations
	BOOL FindFile(LPCTSTR pstrName = NULL, bool bAutoLongPath = false)
	{
		Close();

		if(pstrName == NULL)
			pstrName = _T("*.*");

		if(bAutoLongPath && (lstrlen(pstrName) >= MAX_PATH))
		{
			LPCTSTR lpstrPrefix = _T("\\\\?\\");
			int cchLongPath = lstrlen(lpstrPrefix) + lstrlen(pstrName) + 1;
			ATL::CTempBuffer<TCHAR, _WTL_STACK_ALLOC_THRESHOLD> buff;
			LPTSTR lpstrLongPath = buff.Allocate(cchLongPath);
			if(lpstrLongPath != NULL)
			{
				ATL::Checked::tcscpy_s(lpstrLongPath, cchLongPath, lpstrPrefix);
				ATL::Checked::tcscat_s(lpstrLongPath, cchLongPath, pstrName);
				m_hFind = ::FindFirstFile(lpstrLongPath, &m_fd);
			}
		}
		else
		{
			m_hFind = ::FindFirstFile(pstrName, &m_fd);
		}

		if(m_hFind == INVALID_HANDLE_VALUE)
			return FALSE;

		int cchRoot = ::GetFullPathName(pstrName, 0, NULL, NULL);
		if(cchRoot > 0)
			ATLTRY(m_lpszRoot = new TCHAR[cchRoot]);
		if(m_lpszRoot == NULL)
			return FALSE;

		bool bFullPath = (::GetFullPathName(pstrName, cchRoot, m_lpszRoot, NULL) != 0);

		// passed name isn't a valid path but was found by the API
		ATLASSERT(bFullPath);
		if(!bFullPath)
		{
			Close();
			::SetLastError(ERROR_INVALID_NAME);
			return FALSE;
		}
		else
		{
			// find the last separator
			LPTSTR pstrSep  = _tcsrchr(m_lpszRoot, m_chDirSeparator);
			if(pstrSep != NULL)
				*pstrSep = _T('\0');
		}

		m_bFound = TRUE;

		return TRUE;
	}

	BOOL FindNextFile()
	{
		ATLASSERT(m_hFind != NULL);

		if(m_hFind == NULL)
			return FALSE;

		if(!m_bFound)
			return FALSE;

		m_bFound = ::FindNextFile(m_hFind, &m_fd);

		return m_bFound;
	}

	void Close()
	{
		m_bFound = FALSE;

		delete [] m_lpszRoot;
		m_lpszRoot = NULL;

		if((m_hFind != NULL) && (m_hFind != INVALID_HANDLE_VALUE))
		{
			::FindClose(m_hFind);
			m_hFind = NULL;
		}
	}
};


///////////////////////////////////////////////////////////////////////////////
// CRegProperty and CRegPropertyImpl<> - properties stored in registry

// How to use: Derive a class from CRegPropertyImpl, add data members 
// for properties, and add REGPROP map to map properties to registry value names.
// You can then call Read() and Write() methods to read and write properties to/from registry.
// You can also use CRegProperty class directly, for one time read/write, or for custom stuff.

#define REGPROP_CURRENTUSER   0x0000
#define REGPROP_LOCALMACHINE  0x0001
#define REGPROP_READONLY      0x0002
#define REGPROP_WRITEONLY     0x0004

class CRegProperty
{
public:
// Type declarations
	struct BinaryProp
	{
		void* pBinary;
		ULONG uSize;   // buffer size in bytes, used size after read

		BinaryProp() : pBinary(NULL), uSize(0U)
		{ }
	};

	struct CharArrayProp
	{
		LPTSTR lpstrText;
		ULONG uSize;   // buffer size in chars

		CharArrayProp() : lpstrText(NULL), uSize(0U)
		{ }
	};

// Data members
	ATL::CRegKey m_regkey;
	WORD m_wFlags;

// Constructor
	CRegProperty() : m_wFlags(REGPROP_CURRENTUSER)
	{ }

// Registry key methods
	LSTATUS OpenRegKey(LPCTSTR lpstrRegKey, bool bWrite)
	{
		ATLASSERT(m_regkey.m_hKey == NULL);

		HKEY hKey = ((m_wFlags & REGPROP_LOCALMACHINE) != 0) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
		REGSAM sam = KEY_READ | KEY_WRITE;
		LSTATUS lRet = -1;
		if(bWrite)
			lRet = m_regkey.Create(hKey, lpstrRegKey, NULL, 0, ((m_wFlags & REGPROP_WRITEONLY) != 0) ? KEY_WRITE : sam);
		else
			lRet = m_regkey.Open(hKey, lpstrRegKey, ((m_wFlags & REGPROP_READONLY) != 0) ? KEY_READ : sam);

		return lRet;
	}

	void CloseRegKey()
	{
		LSTATUS lRet = m_regkey.Close();
		(void)lRet;   // avoid level 4 warning
		ATLASSERT(lRet == ERROR_SUCCESS);
	}

// Flag methods
	WORD GetFlags() const
	{
		return m_wFlags;
	}

	WORD SetFlags(WORD wFlags, WORD wMask = 0)
	{
		WORD wPrevFlags = m_wFlags;
		if(wMask == 0)
			m_wFlags = wFlags;
		else
			m_wFlags = (m_wFlags & ~wMask) | (wFlags & wMask);

		return wPrevFlags;
	}

// Generic read/write methods
	template <class TProp>
	LSTATUS ReadProp(LPCTSTR lpstrRegValue, TProp& prop)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		DWORD dwRet = 0;
		LSTATUS lRet = m_regkey.QueryDWORDValue(lpstrRegValue, dwRet);
		if(lRet == ERROR_SUCCESS)
			prop = static_cast<TProp>(dwRet);

		return lRet;
	}

	template <class TProp>
	LSTATUS WriteProp(LPCTSTR lpstrRegValue, TProp& prop)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		return m_regkey.SetDWORDValue(lpstrRegValue, (DWORD)prop);
	}

// Specialization for bool
	template <>
	LSTATUS ReadProp(LPCTSTR lpstrRegValue, bool& bProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		DWORD dwRet = 0;
		LSTATUS lRet = m_regkey.QueryDWORDValue(lpstrRegValue, dwRet);
		if(lRet == ERROR_SUCCESS)
			bProp = (dwRet != 0);

		return lRet;
	}

	template <>
	LSTATUS WriteProp(LPCTSTR lpstrRegValue, bool& bProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		return m_regkey.SetDWORDValue(lpstrRegValue, bProp ? 1 : 0);
	}

// Specialization for HFONT
	template <>
	LSTATUS ReadProp(LPCTSTR lpstrRegValue, HFONT& hFont)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		LOGFONT lf = {};
		ULONG uSize = sizeof(lf);
		LSTATUS lRet = m_regkey.QueryBinaryValue(lpstrRegValue, &lf, &uSize);
		if(lRet == ERROR_SUCCESS)
		{
			if(hFont != NULL)
				::DeleteObject(hFont);

			hFont = ::CreateFontIndirect(&lf);
			if(hFont == NULL)
				lRet = ERROR_INVALID_DATA;
		}

		return lRet;
	}

	template <>
	LSTATUS WriteProp(LPCTSTR lpstrRegValue, HFONT& hFont)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		CLogFont lf(hFont);
		return m_regkey.SetBinaryValue(lpstrRegValue, &lf, sizeof(lf));
	}

// Specialization for BinaryProp
	template <>
	LSTATUS ReadProp(LPCTSTR lpstrRegValue, BinaryProp& binProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		ULONG uSize = 0U;
		LSTATUS lRet = m_regkey.QueryBinaryValue(lpstrRegValue, NULL, &uSize);
		if(lRet == ERROR_SUCCESS)
		{
			if(uSize <= binProp.uSize)
				lRet = m_regkey.QueryBinaryValue(lpstrRegValue, binProp.pBinary, &binProp.uSize);
			else
				lRet = ERROR_OUTOFMEMORY;
		}

		return lRet;
	}

	template <>
	LSTATUS WriteProp(LPCTSTR lpstrRegValue, BinaryProp& binProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		return m_regkey.SetBinaryValue(lpstrRegValue, binProp.pBinary, binProp.uSize);
	}

// Specialization for CharArrayProp
	template <>
	LSTATUS ReadProp(LPCTSTR lpstrRegValue, CharArrayProp& caProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		ULONG uSize = 0U;
		LSTATUS lRet = m_regkey.QueryStringValue(lpstrRegValue, NULL, &uSize);
		if(lRet == ERROR_SUCCESS)
		{
			if(uSize <= caProp.uSize)
				lRet = m_regkey.QueryStringValue(lpstrRegValue, caProp.lpstrText, &caProp.uSize);
			else
				lRet = ERROR_OUTOFMEMORY;
		}

		return lRet;
	}

	template <>
	LSTATUS WriteProp(LPCTSTR lpstrRegValue, CharArrayProp& caProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		return m_regkey.SetStringValue(lpstrRegValue, caProp.lpstrText);
	}

// Specialization for CString
#ifdef __ATLSTR_H__
	template <>
	LSTATUS ReadProp(LPCTSTR lpstrRegValue, ATL::CString& strProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		ULONG uSize = 0U;
		LSTATUS lRet = m_regkey.QueryStringValue(lpstrRegValue, NULL, &uSize);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = m_regkey.QueryStringValue(lpstrRegValue, strProp.GetBufferSetLength(uSize), &uSize);
			strProp.ReleaseBuffer();
		}

		return lRet;
	}

	template <>
	LSTATUS WriteProp(LPCTSTR lpstrRegValue, ATL::CString& strProp)
	{
		ATLASSERT(m_regkey.m_hKey != NULL);

		return m_regkey.SetStringValue(lpstrRegValue, (LPCTSTR)strProp);
	}
#endif // __ATLSTR_H__

// Static methods for one time read/write
	template <class TProp>
	static bool ReadOne(LPCTSTR lpstrRegKey, LPCTSTR lpstrRegValue, TProp& prop, WORD wFlags = REGPROP_CURRENTUSER)
	{
		CRegProperty rp;
		rp.SetFlags(wFlags);
		LSTATUS lRet = rp.OpenRegKey(lpstrRegKey, false);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = rp.ReadProp(lpstrRegValue, prop);
			rp.CloseRegKey();
		}

		return (lRet == ERROR_SUCCESS) || (lRet == ERROR_FILE_NOT_FOUND);
	}

	template <class TProp>
	static bool WriteOne(LPCTSTR lpstrRegKey, LPCTSTR lpstrRegValue, TProp& prop, WORD wFlags = REGPROP_CURRENTUSER)
	{
		CRegProperty rp;
		rp.SetFlags(wFlags);
		LSTATUS lRet = rp.OpenRegKey(lpstrRegKey, true);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = rp.WriteProp(lpstrRegValue, prop);
			rp.CloseRegKey();
		}

		return (lRet == ERROR_SUCCESS);
	}
};


#define BEGIN_REGPROP_MAP(class) \
	void ReadWriteAll(bool bWrite) \
	{

#define REG_PROPERTY(name, prop) \
		this->ReadWriteProp(name, prop, bWrite);

#define END_REGPROP_MAP() \
	}

template <class T>
class CRegPropertyImpl : public CRegProperty
{
public:
// Methods
	void Read(LPCTSTR lpstrRegKey)
	{
		T* pT = static_cast<T*>(this);
		LSTATUS lRet = pT->OpenRegKey(lpstrRegKey, false);
		if(lRet == ERROR_SUCCESS)
		{
			pT->ReadWriteAll(false);
			pT->OnRead(lpstrRegKey);

			pT->CloseRegKey();
		}
		else if(lRet != ERROR_FILE_NOT_FOUND)
		{
			pT->OnReadError(NULL, lRet);
		}
	}

	void Write(LPCTSTR lpstrRegKey)
	{
		T* pT = static_cast<T*>(this);
		LSTATUS lRet = pT->OpenRegKey(lpstrRegKey, true);
		if(lRet == ERROR_SUCCESS)
		{
			pT->ReadWriteAll(true);
			pT->OnWrite(lpstrRegKey);

			pT->CloseRegKey();
		}
		else
		{
			pT->OnWriteError(NULL, lRet);
		}
	}

// Implementation
	template <class TProp>
	void ReadWriteProp(LPCTSTR lpstrRegValue, TProp& prop, bool bWrite)
	{
		T* pT = static_cast<T*>(this);
		if(bWrite)
		{
			LSTATUS lRet = pT->WriteProp(lpstrRegValue, prop);
			if(lRet != ERROR_SUCCESS)
				pT->OnWriteError(lpstrRegValue, lRet);
		}
		else
		{
			LSTATUS lRet = pT->ReadProp(lpstrRegValue, prop);
			if((lRet != ERROR_SUCCESS) && (lRet != ERROR_FILE_NOT_FOUND))
				pT->OnReadError(lpstrRegValue, lRet);
		}
	}

// Overrideable handlers
	void OnRead(LPCTSTR /*lpstrRegKey*/)
	{ }

	void OnWrite(LPCTSTR /*lpstrRegKey*/)
	{ }

	void OnReadError(LPCTSTR /*lpstrRegValue*/, LSTATUS /*lError*/)
	{
		ATLASSERT(FALSE);
	}

	void OnWriteError(LPCTSTR /*lpstrRegValue*/, LSTATUS /*lError*/)
	{
		ATLASSERT(FALSE);
	}
};


///////////////////////////////////////////////////////////////////////////////
// Global functions for stock GDI objects

inline HPEN AtlGetStockPen(int nPen)
{
	ATLASSERT((nPen == WHITE_PEN) || (nPen == BLACK_PEN) || (nPen == NULL_PEN) || (nPen == DC_PEN));
	return (HPEN)::GetStockObject(nPen);
}

inline HBRUSH AtlGetStockBrush(int nBrush)
{
	ATLASSERT(((nBrush >= WHITE_BRUSH) && (nBrush <= HOLLOW_BRUSH)) || (nBrush == DC_BRUSH));
	return (HBRUSH)::GetStockObject(nBrush);
}

inline HFONT AtlGetStockFont(int nFont)
{
	ATLASSERT(((nFont >= OEM_FIXED_FONT) && (nFont <= SYSTEM_FIXED_FONT)) || (nFont == DEFAULT_GUI_FONT));
	return (HFONT)::GetStockObject(nFont);
}

inline HPALETTE AtlGetStockPalette(int nPalette)
{
	ATLASSERT(nPalette == DEFAULT_PALETTE); // the only one supported
	return (HPALETTE)::GetStockObject(nPalette);
}


///////////////////////////////////////////////////////////////////////////////
// Global function for compacting a path by replacing parts with ellipsis

// helper for multi-byte character sets
inline bool _IsDBCSTrailByte(LPCTSTR lpstr, int nChar)
{
#ifndef _UNICODE
	int i = nChar;
	for( ; i > 0; i--)
	{
		if(!::IsDBCSLeadByte(lpstr[i - 1]))
			break;
	}
	return ((nChar > 0) && (((nChar - i) & 1) != 0));
#else // _UNICODE
	(void)lpstr;   // avoid level 4 warning
	(void)nChar;   // avoid level 4 warning
	return false;
#endif // _UNICODE
}

inline bool AtlCompactPath(LPTSTR lpstrOut, LPCTSTR lpstrIn, int cchLen)
{
	ATLASSERT(lpstrOut != NULL);
	ATLASSERT(lpstrIn != NULL);
	ATLASSERT(cchLen > 0);

	LPCTSTR szEllipsis = _T("...");
	const int cchEndEllipsis = 3;
	const int cchMidEllipsis = 4;

	if(lstrlen(lpstrIn) < cchLen)
	{
		ATL::Checked::tcscpy_s(lpstrOut, cchLen, lpstrIn);
		return true;
	}

	lpstrOut[0] = 0;

	// check if the separator is a slash or a backslash
	TCHAR chSlash = _T('\\');
	for(LPTSTR lpstr = (LPTSTR)lpstrIn; *lpstr != 0; lpstr = ::CharNext(lpstr))
	{
		if((*lpstr == _T('/')) || (*lpstr == _T('\\')))
			chSlash = *lpstr;
	}

	// find the filename portion of the path
	LPCTSTR lpstrFileName = lpstrIn;
	for(LPCTSTR pPath = lpstrIn; *pPath; pPath = ::CharNext(pPath))
	{
		if(((pPath[0] == _T('\\')) || (pPath[0] == _T(':')) || (pPath[0] == _T('/')))
				&& pPath[1] && (pPath[1] != _T('\\')) && (pPath[1] != _T('/')))
			lpstrFileName = pPath + 1;
	}
	int cchFileName = lstrlen(lpstrFileName);

	// handle just the filename without a path
	if((lpstrFileName == lpstrIn) && (cchLen > cchEndEllipsis))
	{
		bool bRet = (ATL::Checked::tcsncpy_s(lpstrOut, cchLen, lpstrIn, cchLen - cchEndEllipsis - 1) == 0);
		if(bRet)
		{
#ifndef _UNICODE
			if(_IsDBCSTrailByte(lpstrIn, cchLen - cchEndEllipsis))
				lpstrOut[cchLen - cchEndEllipsis - 1] = 0;
#endif // _UNICODE
			ATL::Checked::tcscat_s(lpstrOut, cchLen, szEllipsis);
		}
		return bRet;
	}

	// handle just ellipsis
	if((cchLen < (cchMidEllipsis + cchEndEllipsis)))
	{
		for(int i = 0; i < cchLen - 1; i++)
			lpstrOut[i] = ((i + 1) == cchMidEllipsis) ? chSlash : _T('.');
		lpstrOut[cchLen - 1] = 0;
		return true;
	}

	// calc how much we have to copy
	int cchToCopy = cchLen - (cchMidEllipsis + cchFileName) - 1;

	if(cchToCopy < 0)
		cchToCopy = 0;

#ifndef _UNICODE
	if((cchToCopy > 0) && _IsDBCSTrailByte(lpstrIn, cchToCopy))
		cchToCopy--;
#endif // _UNICODE

	bool bRet = (ATL::Checked::tcsncpy_s(lpstrOut, cchLen, lpstrIn, cchToCopy) == 0);
	if(!bRet)
		return false;

	// add ellipsis
	ATL::Checked::tcscat_s(lpstrOut, cchLen, szEllipsis);
	TCHAR szSlash[2] = { chSlash, 0 };
	ATL::Checked::tcscat_s(lpstrOut, cchLen, szSlash);

	// add filename (and ellipsis, if needed)
	if(cchLen > (cchMidEllipsis + cchFileName))
	{
		ATL::Checked::tcscat_s(lpstrOut, cchLen, lpstrFileName);
	}
	else
	{
		cchToCopy = cchLen - cchMidEllipsis - cchEndEllipsis - 1;
#ifndef _UNICODE
		if((cchToCopy > 0) && _IsDBCSTrailByte(lpstrFileName, cchToCopy))
			cchToCopy--;
#endif // _UNICODE
		bRet = (ATL::Checked::tcsncpy_s(&lpstrOut[cchMidEllipsis], cchLen - cchMidEllipsis, lpstrFileName, cchToCopy) == 0);
		if(bRet)
			ATL::Checked::tcscat_s(lpstrOut, cchLen, szEllipsis);
	}

	return bRet;
}

} // namespace WTL

#endif // __ATLMISC_H__
