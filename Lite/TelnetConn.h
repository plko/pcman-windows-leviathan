#if !defined(AFX_MYSOCKET_H__1837C9F6_3C2A_11D5_A2FD_946114C10000__INCLUDED_)
#define AFX_MYSOCKET_H__1837C9F6_3C2A_11D5_A2FD_946114C10000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TelnetConn.h : header file
//

#include <memory>
#include <atomic>

#include <winsock2.h>
// #include <WS2TCPIP.h> // for future IPV6 support

#include "Conn.h"
#include "SiteSettings.h"
#include "KeyMap.h"	// Added by ClassView
#include "TriggerList.h"	// Added by ClassView
#include "ConnIO.h"
#include "TcpSocket.h"

/////////////////////////////////////////////////////////////////////////////

class CTermView;
class CDownloadArticleDlg;

struct CConnEvent
{
	enum EventType
	{
		EVENT_CONNECT = 0,
		EVENT_CLOSE = 1,
		EVENT_DATA = 2,
		EVENT_CONNECT_FAILED = 3,
	};
	uint64_t connection_id;
	EventType event_type;
	std::string data;

	CConnEvent(uint64_t connection_id, EventType event_type)
		: connection_id(connection_id)
		, event_type(event_type)
	{}

	static std::unique_ptr<CConnEvent> MakeUnique(
		uint64_t connection_id, EventType event_type) {
		return std::unique_ptr<CConnEvent>(new CConnEvent(
			connection_id, event_type));
	}
};

int find_sub_str(char* str, char* sub);

class CTelnetConn : public CConn
{
	friend class CDownloadArticleDlg;
// Attributes
public:
	enum
	{
		MAX_LINES_PER_PAGE = 64,
		MAX_COLS_PER_PAGE = 160,
		MIN_LINE_PER_PAGE = 24,
		MIN_COLS_PER_PAGE = 40,
		MAX_LINE_COUNT = 32767,
		MIN_LINE_COUNT = 24
	};

	CString cfg_path;	//站台進階設定檔相對路徑
	CKeyMap* key_map;	//鍵盤對映

//	Socket handle
	uint64_t connection_id;
	std::shared_ptr<CConnIO> conn_io;
	std::shared_ptr<CTcpSocket> tcp_socket;

//	Screen Data
	LPSTR *screen;	//screen buffer
	long first_line;	//first line index
	long last_line;		//last line index
	CPoint cursor_pos;	//cursor position
	int scr_top;	//screen scroll region
	int scr_bottom;	//screen scroll region

//	Mouse Selection
	CPoint sel_start;
	CPoint sel_end;
	bool sel_block;

//	Site Settings
	CSiteSettings site_settings;	// BBS Site Settings

//	Runtime Internal Data
	static BYTE buffer[4096];	//接收網路資料的buffer，所有socket共用
	BYTE *buf;
	BYTE *last_byte;	//End of buffer
	ULONG time;		//連線時間
	ULONG idle_time;	//閒置時間
	int scroll_pos;	//視窗捲軸位置

	char ansi_param[64];	//用來處理ANSI彩色字串的暫存buffer
	char* pansi_param;		//用來操作ansi_param資料的指標
	CPoint old_cursor_pos;	//saved position
	BYTE saved_attr;		//saved attributes
	BYTE cur_attr;			//目前的螢幕字元屬性
	BYTE attr_flags;	//螢幕字元屬性用的暫存flag，儲存是否反相等資訊

BYTE ansi_mode: 1;
BYTE insert_mode: 1;

BYTE is_getting_article : 1;
BYTE get_article_with_ansi : 1;
BYTE get_article_in_editor : 1;

//	Delay Send Data
	CList<CTelnetConnDelayedSend, CTelnetConnDelayedSend> delay_send;

	static CString downloaded_article;
    static int current_download_line;

	static std::atomic<uint64_t> connection_counter;

public:
	CTelnetConn();
	virtual ~CTelnetConn();

// Operations
public:
	void CopyArticleComplete(bool cancel = false);
	void CopyArticle(bool with_color, bool in_editor);
	CString GetLineWithAnsi(long line);
	int IsEndOfArticleReached();
	void SendNaws();
	void SendMacroString(CString str);
	int GetLineBufLen();
	int GetLineBufLen(int _cols_per_page);
	LPBYTE GetLineAttr(const char* line);
	LPBYTE GetLineAttr(const int line);
	LPBYTE GetLineAttr(const int line, const int len);
	void InitNewLine(char* line);
	void Home();
	void End();
	void Delete(int num = 1);
	void SetHyperLink(long i, BOOL haslink = TRUE);
	BOOL GetHyperLink(const char* line);
	BOOL GetHyperLink(long i);
	void SetUpdateWholeLine(long line);
	BOOL GetUpdateLine(long line);
	void RemoveUpdateLine(long line);
	void SetUpdateLine(long line, BYTE curx);
	void Back(int num = 1);
	void EditorLineBack(LPSTR newline, LPBYTE newlineatb, int l);
	void EditorCarriageRetiurn();
	void EditorLineFeed(LPSTR newline, LPBYTE newlineatb, int l);
	BOOL IsEmptyLine(LPSTR line, int len);
	char* AllocNewLine();
	char* ResizeLine(int line, int newl);
	void ReSizeBuffer(long new_line_count, int new_cols_per_page, int new_lines_per_page);
	void ProcessAnsiEscapeSequence();
	void SetCurrentAttributes(USHORT clr);
	void GoUp(int p);
	void GoDown(int p);
	void GoRight(int p);
	void GoLeft(int p);
	void LineFeed(int param);
	void ScrollUp();
	void ScrollDown();
	void SetScrollRange(int pt, int progress_bar);
	void GotoXY(int line, int col);
	void InsertLines(int num);
	void ClearScreen(int param);
	void ClearCurrentLine(int param);
	void SaveCursorPos();
	void RestoreCursorPos();
	void InsertChar(int n);
	void DeleteLines(int n);
	void DeleteChar(int n);
	void NextPages(int n);
	void PrevPages(int n);
	void BackTab(int n);
	void SetMode(int p);
	void ResetMode(int p);
	void DeviceStatusReport(int p);

	uint64_t GetConnectionID() const { return connection_id; }
	bool HasSocket() const { return tcp_socket.operator bool(); }
	SOCKET GetSocket() const { return tcp_socket->GetSocket(); }
	bool IsSecureConn() const { return conn_io && conn_io->IsSecure(); }
	void OnSocket(WPARAM wparam, LPARAM lparam);
	int Close();
	int Shutdown();
	void HandleConnEvent(std::unique_ptr<CConnEvent> event);
	void Connect(sockaddr* addr, int len);
	void ConnectWebsocket();
	BOOL Create();
	int SendString(LPCTSTR str);
	void LocalEcho(void* str, int len);
	void ProcessData(int len);
	void CreateBuffer();
	void CheckStrTrigger();
	int Send(const void* lpBuf, int nBufLen);
	void CheckHyperLinks();
	static void SetFgColor(BYTE &attr, BYTE fg);
	static void SetBgColor(BYTE& attr, BYTE bk);
	void LineFeed();
	void UpdateLine(int line);
	void OnClose();
	void UpdateCursorPos();
	void OnText();
	void OnIAC();

	void ClearAllFlags()
	{
		CConn::ClearAllFlags();
		is_telnet = true;

		scr_top = 0;
		scr_bottom = site_settings.lines_per_page - 1;
	}

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTelnetConn)
public:
	virtual void OnReceive(int len);
	virtual void OnConnect(bool success);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CTelnetConn)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
};

inline void CTelnetConn::SetBgColor(BYTE &attr, BYTE bk)
{
	attr &= 143;		//1000,1111b=143d;
	attr |= (bk << 4);
}

inline void CTelnetConn::SetFgColor(BYTE &attr, BYTE fg)
{
	attr &= 248;		//1111,1000b=248d
	attr |= fg;
}

inline BOOL CTelnetConn::GetHyperLink(long i)
{	return GetLineAttr(i)[site_settings.cols_per_page+1];	}

inline BOOL CTelnetConn::GetHyperLink(const char* line)
{	return GetLineAttr(line)[site_settings.cols_per_page+1];	}

inline void CTelnetConn::SetHyperLink(long i, BOOL haslink /*=TRUE*/)
{	GetLineAttr(i)[site_settings.cols_per_page+1] = (BYTE)haslink;	}


inline void CTelnetConn::Home()
{
	cursor_pos.x = 0;
	UpdateCursorPos();
}

// FIXME: There isn't enough space to fit UTF8 chinese string.
inline LPBYTE CTelnetConn::GetLineAttr(const char *line)
{
	return LPBYTE(line + site_settings.cols_per_page + 5);
}

inline LPBYTE CTelnetConn::GetLineAttr(const int line)
{
	return LPBYTE(screen[line] + site_settings.cols_per_page + 5);
}

inline LPBYTE CTelnetConn::GetLineAttr(const int line, const int len)
{
	return LPBYTE(screen[line] + len + 5);
}

inline void CTelnetConn::InitNewLine(char *line)
{
	LPBYTE attr = GetLineAttr(line);
	memset(line, ' ', site_settings.cols_per_page);
	*(DWORD*)(line + site_settings.cols_per_page) = 0;
	memset(attr, 7, site_settings.cols_per_page);
	*(DWORD*)(attr + site_settings.cols_per_page) = 0;
	*(DWORD*)(attr - 4) = 0;
}

inline char* CTelnetConn::AllocNewLine()
{
	char* str = new char[GetLineBufLen()];
	InitNewLine(str);
	return str;
}


inline int CTelnetConn::GetLineBufLen()
{	return site_settings.cols_per_page*2 + 10;	}

inline int CTelnetConn::GetLineBufLen(int _cols_per_page)
{	return _cols_per_page*2 + 10;	}

inline BYTE GetAttrBkColor(BYTE attr)
{
	BYTE bk = (attr & 112) >> 4;		//0111,0000b=112d;
	return bk;
}

inline BYTE GetAttrFgColor(BYTE attr)
{
	BYTE fg = attr & 7;		//0000,0111b=7;
	if (attr&8)
		fg += 8;		//0000,1000b=8d;
	return fg;
}

inline bool IsAttrBlink(BYTE attr)
{
	return !!(attr&128);	//1000,0000b=128d
}

CString AttrToStr(BYTE prevatb, BYTE attr);

// This time write this in TelnetConn, because most function related with it.
// TODO: Move to another file.
class CPluginBBL
{
private:
    // const
    static const int BUF_SIZE = 24;
    enum
    {
        STATE_NONE = 0,
        STATE_BOARD = 1,
        STATE_READ = 2,
    };

    //"編號   日 期  作  者       文  章  標  題"
    //"編號    日 期 作  者       文  章  標  題"
    //"(PgUp)(PgDn)(0)($)移動 (/n)搜尋 (C)暫存 ←(q)結束"
    //"文章選讀  (g)寫得好!"
    static constexpr const char* inBoardChk1 = "\xBD\x73\xB8\xB9\x20\x20\x20\xA4\xE9\x20\xB4\xC1\x20\x20\xA7\x40\x20\x20\xAA\xCC\x20\x20\x20\x20\x20\x20\x20\xA4\xE5\x20\x20\xB3\xB9\x20\x20\xBC\xD0\x20\x20\xC3\x44";
    static constexpr const char* inBoardChk2 = "\xBD\x73\xB8\xB9\x20\x20\x20\x20\xA4\xE9\x20\xB4\xC1\x20\xA7\x40\x20\x20\xAA\xCC\x20\x20\x20\x20\x20\x20\x20\xA4\xE5\x20\x20\xB3\xB9\x20\x20\xBC\xD0\x20\x20\xC3\x44";
    static constexpr const char* inReadingChk1 = "\x28\x50\x67\x55\x70\x29\x28\x50\x67\x44\x6E\x29\x28\x30\x29\x28\x24\x29\xB2\xBE\xB0\xCA\x20\x28\x2F\x6E\x29\xB7\x6A\xB4\x4D\x20\x28\x43\x29\xBC\xC8\xA6\x73\x20\xA1\xF6\x28\x71\x29\xB5\xB2\xA7\xF4";
    static constexpr const char* inReadingChk2 = "\xA4\xE5\xB3\xB9\xBF\xEF\xC5\xAA\x20\x20\x28\x67\x29\xBC\x67\xB1\x6F\xA6\x6E\x21";
    //" 作者 "
    //" 標題 "
    //" 時間 "
    static constexpr const char* newPostChk1 = "\x20\xA7\x40\xAA\xCC\x20";
    static constexpr const char* newPostChk2 = "\x20\xBC\xD0\xC3\x44\x20";
    static constexpr const char* newPostChk3 = "\x20\xAE\xC9\xB6\xA1\x20";
    //"※ "
    //" 作者 "
    //"※ 引述《"
    //"《"
    static constexpr const char* postCheckStr1 = "\xA1\xB0\x20";
    static constexpr const char* postCheckStr2 = "\x20\xA7\x40\xAA\xCC\x20";
    static constexpr const char* postCheckStr3 = "\xA1\xB0\x20\xA4\xDE\xAD\x7A\xA1\x6D";
    static constexpr const char* postCheckStr4 = "\xA1\x6D";

public:
    enum
    {
        BLACKSTYLE_DEFAULT = 0,
        BLACKSTYLE_MOSAIC = 1,
        BLACKSTYLE_MAPLE = 2,
    };

    // Ctor
    CPluginBBL();

    // member.
    bool isHiding[BUF_SIZE];            // if line already hided. prevent re-draw.
    std::string lineTag[BUF_SIZE];      // tag author.
    std::string rawLine[BUF_SIZE];      // 
    std::string postAuthor;             // Author id.
    std::string quoAuthor[BUF_SIZE];    // quote content author.
    bool kwHiding[BUF_SIZE];            // for keyword
    bool hidingAllPost;                 // for keyword black
    std::vector<std::string> idList;    // black id list.
    std::vector<std::string> kwList;    // black keyword list.
    int state;                          // bahamut in which state.
    std::vector<std::string> validServerList;

    // method
    void resetAll();
    void loadBLIDList();
    void loadBLKWList();
    //void update(LPSTR *screen,long first,long last);
    void update(CTelnetConn* tconn);
    void render(CDC &dc, CTermView* view, int line, int drawy);
    bool getInBlackList(std::string id);
    bool getInBlackList(int line);
    bool chkServerValid(std::string ser);

    /*static inline int styleDefault() { return BLACKSTYLE_DEFAULT; }
    static inline int styleMosaic() { return BLACKSTYLE_MOSAIC; }
    static inline int styleMaple() { return BLACKSTYLE_MAPLE; }*/
};
extern CPluginBBL PluginBBL;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYSOCKET_H__1837C9F6_3C2A_11D5_A2FD_946114C10000__INCLUDED_)
