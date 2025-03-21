// PCMan4.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PCMan.h"

#include "MainFrm.h"

#include <wininet.h>
#include "WinUtils.h"

#include "SearchPlugin.h"
#include "OleImage.h"

#ifdef	_COMBO_
//	#include <..\src\occimpl.h>
#include "..\Combo\CustSite.h"
#include "..\Combo\Version.h"
#else
#include "..\Lite\Version.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CString AppPath;
CString ConfigPath;
CString DefaultConfigPath;

/////////////////////////////////////////////////////////////////////////////
// CApp

BEGIN_MESSAGE_MAP(CApp, CWinApp)
	//{{AFX_MSG_MAP(CApp)
	ON_COMMAND(ID_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CApp construction

CApp::CApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CApp object

CApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CApp initialization

BOOL CApp::InitInstance()
{
	DBG_INIT(1);

	// Find other existing instances
	HWND mainwnd = GetTopWindow(HWND_DESKTOP);
	while ((mainwnd =::FindWindowEx(HWND_DESKTOP, mainwnd, CMainFrame::mainfrm_class_name, NULL)))
	{
		if (!::SendMessage(mainwnd, WM_QUERY_APPCONFIG, AC_MULTIPCMAN, 0) &&
			!::SendMessage(mainwnd, WM_QUERY_APPCONFIG, AC_PCMANLOCKED, 0))
		{
			if (*m_lpCmdLine)
			{
				int l = strlen(m_lpCmdLine) + 1;
				COPYDATASTRUCT d;
				d.lpData = m_lpCmdLine;
				d.cbData = l;
				d.dwData = 0;
				::SendMessage(mainwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&d);
			}
			if (IsIconic(mainwnd))
				ShowWindow(mainwnd, SW_RESTORE);
			BringWindowToTop(mainwnd);
			SetForegroundWindow(mainwnd);
			return FALSE;
		}
	}

	// Initialize paths
	::GetModuleFileName(AfxGetInstanceHandle(), AppPath.GetBuffer(MAX_PATH), _MAX_PATH);
	AppPath.ReleaseBuffer();
	AppPath = AppPath.Left(AppPath.ReverseFind('\\') + 1);
	DefaultConfigPath = AppPath + CONFIG_DIR;

	/*
		// Test for PCMan 2004
		if( IsFileExist(DefaultConfigPath + "Config") )	// PCMan 2004
		{
			// Installed into the directory containing old config files.
			AfxMessageBox( IDS_PROMPT_IMPORT, MB_OK|MB_ICONINFORMATION );
			ShellExecute( NULL, "open", "http://pcman.openfoundry.org/faq.html", NULL, NULL, SW_SHOW );
		}
	*/
	// Supporting per-user settings under Windows NT/2000/xp/Vista
	if (IsWinNT() && !IsFileExist(AppPath + "Portable"))
	{
		BOOL ret = SHGetSpecialFolderPath(NULL, ConfigPath.GetBuffer(_MAX_PATH),
										  CSIDL_APPDATA, TRUE);
		ConfigPath.ReleaseBuffer();
		if (ret)
		{
#if defined (_COMBO_)
			ConfigPath += "\\PCMan Combo\\";
#else
			ConfigPath += "\\PCMan\\";
#endif
			if (!IsFileExist(ConfigPath))	// Copy default settings when necessary
			{
				CreateDirectory(ConfigPath, NULL);
				// AppConfig.BackupConfig( DefaultConfigPath, ConfigPath );
				CopyFile(DefaultConfigPath + CONFIG_FILENAME, ConfigPath + CONFIG_FILENAME, TRUE);
				CopyFile(DefaultConfigPath + UI_FILENAME, ConfigPath + UI_FILENAME, TRUE);
				CopyFile(DefaultConfigPath + FUS_FILENAME, ConfigPath + FUS_FILENAME, TRUE);
				CopyFile(DefaultConfigPath + BBS_FAVORITE_FILENAME, ConfigPath + BBS_FAVORITE_FILENAME, TRUE);

				CopyFile(DefaultConfigPath + TOOLBAR_BMP_FILENAME, ConfigPath + TOOLBAR_BMP_FILENAME, TRUE);
				CopyFile(DefaultConfigPath + ICON_BMP_FILENAME, ConfigPath + ICON_BMP_FILENAME, TRUE);
#if defined(_COMBO_)
				CopyFile(DefaultConfigPath + WEB_ICON_BMP_FILENAME, ConfigPath + WEB_ICON_BMP_FILENAME, TRUE);
#endif
			}
			if (!IsFileExist(ConfigPath + UI_FILENAME))
			{
				CopyFile(DefaultConfigPath + UI_FILENAME, ConfigPath + UI_FILENAME, TRUE);
			}
		}
		else
			ConfigPath.ReleaseBuffer();
	}
	else
	{
		ConfigPath = DefaultConfigPath;
	}

#if defined (_COMBO_)
	//IShellUIHandle
	CCustomOccManager *pMgr = new CCustomOccManager;

	// Create an IDispatch class for extending the Dynamic HTML Object Model
//	m_pDispOM = new CImpIDispatch;
	//Drop target
//	m_pDropTarget = new CImpIDropTarget;

	// Set our control containment up but using our control container
	// management class instead of MFC's default
	AfxEnableControlContainer(pMgr);
#endif

	AppConfig.Load(ConfigPath + CONFIG_FILENAME);

#if defined(_COMBO_)
	// Lite version calls this function before showing popup menu to reduce startup time.
	// Combo version loads all search plugins here for search bar.
	SearchPluginCollection.LoadAll();
#endif

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	if (! pFrame->LoadFrame(IDR_MAINFRAME,
							WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
							NULL))
	{
		return FALSE;
	}

	//Register Hotkey
	BOOL r = RegisterHotKey(pFrame->m_hWnd, 1, AppConfig.pcman_hotkey_mod, AppConfig.pcman_hotkey);

	pFrame->view.OnInitialUpdate();

	if (AppConfig.save_session)	// open saved session if any
		pFrame->OpenLastSession();

	if (*m_lpCmdLine) {	// open file or address passed in command line argument
		CCommandLineInfo info;
		ParseCommandLine(info);
		if (!info.m_strFileName.IsEmpty()) {
			pFrame->OnNewConnectionAds(info.m_strFileName);
		}
	}
	else	// otherwise, open homepage if no command line argument was passed
		pFrame->OpenHomepage();

	pFrame->SwitchToConn(0);

	//Restore Main Window Position
	AppConfig.mainwnd_state.Restore(pFrame->m_hWnd);
	pFrame->UpdateWindow();

//如果只允許執行一個 PCMan，則把User data設為1
	SetWindowLong(m_pMainWnd->m_hWnd, GWL_USERDATA, !AppConfig.multiple_instance);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CApp message handlers


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	afx_msg void OnWeb();
	afx_msg void OnHelp();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void OpenUrl(const TCHAR *url);
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_WEB, OnWeb)
	ON_BN_CLICKED(IDB_HELP, OnHelp)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CApp message handlers


void CAboutDlg::OnWeb()
{
	OpenUrl(TEXT("http://pcman.ptt.cc/"));
}

void CAboutDlg::OnHelp()
{
	OpenUrl(TEXT("http://pcman.ptt.cc/pcman_help.html"));
}

void CAboutDlg::OpenUrl(const TCHAR *url)
{
#ifdef	_COMBO_
	((CMainFrame*)AfxGetApp()->m_pMainWnd)->view.ConnectWeb(CAddress(url), TRUE);
#else
	ShellExecute(m_hWnd, "open", url, NULL, NULL, SW_SHOW);
#endif
}

int CApp::ExitInstance()
{
	WSACleanup();
	return CWinApp::ExitInstance();
}

bool ReadFileAppend(const CString& path, std::string* buf)
{
	CFile f;
	if (!f.Open(path, CFile::modeRead))
		return false;
	size_t orig_buf_size = buf->size();
	size_t size = f.GetLength();
	buf->resize(orig_buf_size + size);
	if (f.Read(&(*buf)[orig_buf_size], size) != size)
	{
		buf->resize(orig_buf_size);
		return false;
	}
	f.Close();
	return true;
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	std::string buffer;
	ReadFileAppend(AppPath + "story.txt", &buffer);
	buffer += _T("\r\n");
	ReadFileAppend(AppPath + "OpenSourceLicenses.txt", &buffer);
	buffer += _T("\r\n");
	GetDlgItem(IDC_EDIT)->SetWindowText(buffer.c_str());

	char buf1[0x100];

	wsprintf(buf1, "PCMan Leviathan 0.2.0 Based on %s - %d.%d.%d%s (Novus)",
			 Version_InternalNameA, Version_Major, Version_Minor, Version_PatchLevel, Version_Append);

	GetDlgItem(IDC_VERSION)->SetWindowText(buf1);
	return TRUE;
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC && IDC_EDIT == pWnd->GetDlgCtrlID())
	{
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		return HBRUSH(COLOR_WINDOW + 1);
	}
	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

