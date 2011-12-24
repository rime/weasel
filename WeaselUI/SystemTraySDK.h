// TrayIcon.h: interface for the CSystemTray class.
//
// Written by Chris Maunder (cmaunder@mail.com)
// Copyright (c) 1998.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. If 
// the source code in  this file is used in any commercial application 
// then acknowledgement must be made to the author of this file 
// (in whatever form you wish).
//
// This file is provided "as is" with no expressed or implied warranty.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRAYICON_H__43104212_F2C1_11D2_A9E9_8EA47C000000__INCLUDED_)
#define AFX_TRAYICON_H__43104212_F2C1_11D2_A9E9_8EA47C000000__INCLUDED_

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#include <ShellAPI.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// The debugger can't handle symbols more than 255 characters long.
// STL often creates symbols longer than that.
// When symbols are longer than 255 characters, the warning is disabled.
#pragma warning(disable:4786) 
#include <time.h>
#include <vector>
using namespace std;
typedef vector<HICON> ICONVECTOR;

#ifdef NOTIFYICONDATA_V1_SIZE   // If NOTIFYICONDATA_V1_SIZE, then we can use fun stuff
#define SYSTEMTRAY_USEW2K
#else
#define NIIF_NONE 0
#endif

class CSystemTray
{
// Construction/destruction
public:
    CSystemTray();
    CSystemTray(HINSTANCE hInst, HWND hParent, UINT uCallbackMessage, 
              LPCTSTR szTip, HICON icon, UINT uID, 
              BOOL bhidden = FALSE,
              LPCTSTR szBalloonTip = NULL, LPCTSTR szBalloonTitle = NULL, 
              DWORD dwBalloonIcon = NIIF_NONE, UINT uBalloonTimeout = 10);
    virtual ~CSystemTray();

// Operations
public:
    BOOL Enabled() { return m_bEnabled; }
    BOOL Visible() { return !m_bHidden; }

    // Create the tray icon
    BOOL Create(HINSTANCE hInst, HWND hParent, UINT uCallbackMessage, LPCTSTR szTip,
		   HICON icon, UINT uID, BOOL bHidden = FALSE,
           LPCTSTR szBalloonTip = NULL, LPCTSTR szBalloonTitle = NULL, 
           DWORD dwBalloonIcon = NIIF_NONE, UINT uBalloonTimeout = 10);

    // Change or retrieve the Tooltip text
    BOOL   SetTooltipText(LPCTSTR pszTooltipText);
    BOOL   SetTooltipText(UINT nID);
    LPTSTR GetTooltipText() const;

    // Change or retrieve the icon displayed
    BOOL  SetIcon(HICON hIcon);
    BOOL  SetIcon(LPCTSTR lpszIconName);
    BOOL  SetIcon(UINT nIDResource);
    BOOL  SetStandardIcon(LPCTSTR lpIconName);
    BOOL  SetStandardIcon(UINT nIDResource);
    HICON GetIcon() const;

    void  SetFocus();
    BOOL  HideIcon();
    BOOL  ShowIcon();
    BOOL  AddIcon();
    BOOL  RemoveIcon();
    BOOL  MoveToRight();

    BOOL ShowBalloon(LPCTSTR szText, LPCTSTR szTitle = NULL,
                     DWORD dwIcon = NIIF_NONE, UINT uTimeout = 10);

    // For icon animation
    BOOL  SetIconList(UINT uFirstIconID, UINT uLastIconID); 
    BOOL  SetIconList(HICON* pHIconList, UINT nNumIcons); 
    BOOL  Animate(UINT nDelayMilliSeconds, int nNumSeconds = -1);
    BOOL  StepAnimation();
    BOOL  StopAnimation();

    // Change menu default item
    void  GetMenuDefaultItem(UINT& uItem, BOOL& bByPos);
    BOOL  SetMenuDefaultItem(UINT uItem, BOOL bByPos);

    // Change or retrieve the window to send icon notification messages to
    BOOL  SetNotificationWnd(HWND hNotifyWnd);
    HWND  GetNotificationWnd() const;

    // Change or retrieve the window to send menu commands to
    BOOL  SetTargetWnd(HWND hTargetWnd);
    HWND  GetTargetWnd() const;

    // Change or retrieve  notification messages sent to the window
    BOOL  SetCallbackMessage(UINT uCallbackMessage);
    UINT  GetCallbackMessage() const;

    HWND  GetSafeHwnd() const  { return (this)? m_hWnd : NULL; }
    UINT_PTR GetTimerID() const   { return m_nTimerID; }

	// Static functions
public:
    static void MinimiseToTray(HWND hWnd);
    static void MaximiseFromTray(HWND hWnd);

public:
    // Default handler for tray notification message
    virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSystemTray)
	//}}AFX_VIRTUAL

// Static callback functions and data
public:
    static LRESULT PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static CSystemTray* m_pThis;

// Implementation
protected:
    void Initialise();
    void InstallIconPending();
    ATOM RegisterClass(HINSTANCE hInstance);

    virtual void CustomizeMenu(HMENU) {}

// Implementation
protected:
    NOTIFYICONDATA  m_tnd;
    HINSTANCE       m_hInstance;
    HWND            m_hWnd;
	HWND            m_hTargetWnd;       // Window that menu commands are sent

    BOOL            m_bEnabled;         // does O/S support tray icon?
    BOOL            m_bHidden;          // Has the icon been hidden?
    BOOL            m_bRemoved;         // Has the icon been removed?
    BOOL            m_bShowIconPending; // Show the icon once tha taskbar has been created
    BOOL            m_bWin2K;           // Use new W2K features?

    ICONVECTOR      m_IconList; 
    UINT_PTR        m_uIDTimer;
    int				m_nCurrentIcon;
    time_t			m_StartTime;
    int				m_nAnimationPeriod;
    HICON			m_hSavedIcon;
    UINT			m_DefaultMenuItemID;
    BOOL			m_DefaultMenuItemByPos;
	UINT			m_uCreationFlags;

// Static data
protected:
    static BOOL RemoveTaskbarIcon(HWND hWnd);

    static const UINT_PTR m_nTimerID;
    static UINT  m_nMaxTooltipLength;
    static const UINT m_nTaskbarCreatedMsg;
    static HWND  m_hWndInvisible;

    static BOOL GetW2K();
#ifndef _WIN32_WCE
    static void GetTrayWndRect(LPRECT lprect);
    static BOOL GetDoWndAnimation();
#endif

// message map functions
public:
	LRESULT OnTimer(UINT nIDEvent);
    LRESULT OnTaskbarCreated(WPARAM wParam, LPARAM lParam);
#ifndef _WIN32_WCE
	LRESULT OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
#endif
};


#endif // !defined(AFX_TRAYICON_H__43104212_F2C1_11D2_A9E9_8EA47C000000__INCLUDED_)
