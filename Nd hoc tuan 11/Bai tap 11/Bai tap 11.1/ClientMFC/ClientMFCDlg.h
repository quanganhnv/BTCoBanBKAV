
// ClientMFCDlg.h : header file
//

#pragma once


// CClientMFCDlg dialog
class CClientMFCDlg : public CDialogEx
{
// Construction
public:
	CClientMFCDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENTMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
private:
	BOOL g_checkConnect = TRUE;
public:
	CListBox boxOnlineUser;
	CListBox boxMessages;
	afx_msg void OnBnClickedButton3();
};
