// ALTA_DXF_To_GCode_1Dlg.cpp : implementation file
//

#include "stdafx.h"
#include <math.h>

#include <freeglut.h>
#include "ObjectData.h"
#include "FileIO.h"
#include "ToolPaths.h"
#include "SmartArrays.h"
#include "OpenGL.h"

#include "ALTA_DXF_To_GCode_1.h"
#include "ALTA_DXF_To_GCode_1Dlg.h"

#define TRACE_VERBOSITY 9

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CALTA_DXF_To_GCode_1Dlg dialog




CALTA_DXF_To_GCode_1Dlg::CALTA_DXF_To_GCode_1Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CALTA_DXF_To_GCode_1Dlg::IDD, pParent)
	, DLG_cstrFileName(_T(""))
	, DLG_dEndpointCompliance(12.5)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	DLG_dToolDiameter = 0.250;
	DLG_dCutHeight = 1.750;
	DLG_dCutSpeed = 30.0;
	DLG_dTransSpeed = 90.0;
	DLG_dTransHeight = 3.250;
	DLG_dPlungeSpeed = 40.0;
	DLG_dRetractSpeed = 100.0;
}

void CALTA_DXF_To_GCode_1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, DLG_cstrFileName);
	DDX_Text(pDX, IDC_EDIT2, DLG_dEndpointCompliance);
	DDX_Control(pDX, IDC_CHECK2, DLG_EndpointCompEnabled);
	DDX_Text(pDX, IDC_TOOL_DIAM, DLG_dToolDiameter);
	DDV_MinMaxDouble(pDX, DLG_dToolDiameter, 0.001, 2.5);
	DDX_Text(pDX, IDC_EDIT3, DLG_dCutHeight);
	DDX_Text(pDX, IDC_EDIT4, DLG_dCutSpeed);
	//  DDX_Text(pDX, IDC_EDIT5, DLG_dTransHeighy);
	DDX_Text(pDX, IDC_EDIT6, DLG_dTransSpeed);
	DDX_Text(pDX, IDC_EDIT5, DLG_dTransHeight);
	DDX_Control(pDX, IDC_BUTTON2, DLG_FileCloseButton);
	DDX_Control(pDX, IDC_BUTTON1, DLG_FileOpenButton);
	DDX_Control(pDX, IDC_BUTTON3, DLG_CalcPathsButton);
	DDX_Control(pDX, IDC_BUTTON4, DLG_ViewAllPathsButton);
	DDX_Control(pDX, IDC_BUTTON5, DLG_ViewFinalPathButton);
	DDX_Control(pDX, IDC_BUTTON6, DLG_OutputGCodeButton);
	DDX_Text(pDX, IDC_EDIT7, DLG_dPlungeSpeed);
	DDX_Text(pDX, IDC_EDIT8, DLG_dRetractSpeed);
}

BEGIN_MESSAGE_MAP(CALTA_DXF_To_GCode_1Dlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON3, &CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_CHECK1, &CALTA_DXF_To_GCode_1Dlg::OnBnClickedCheck1)
END_MESSAGE_MAP()


// CALTA_DXF_To_GCode_1Dlg message handlers

BOOL CALTA_DXF_To_GCode_1Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_pFileIO = new CFileIO;

	// Raw DXF Unconnected OUT/INSIDE segments
	m_pSaUnconnectedCutInsideDxfSegments = new CSmartArrayUnconnectedSegments (25, 10);
	m_pSaUnconnectedCutOutsideDxfSegments = new CSmartArrayUnconnectedSegments (25, 10);
	m_pSaUnconnectedCutOnDxfSegments = new CSmartArrayUnconnectedSegments (25, 10);

	m_pToolPaths = new CToolPaths;

	DLG_EndpointCompEnabled.SetCheck(1);
	DLG_FileCloseButton.EnableWindow(0);
	DLG_CalcPathsButton.EnableWindow(0);
	DLG_ViewAllPathsButton.EnableWindow(0);
	DLG_ViewFinalPathButton.EnableWindow(0);
	DLG_OutputGCodeButton.EnableWindow(0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CALTA_DXF_To_GCode_1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CALTA_DXF_To_GCode_1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CALTA_DXF_To_GCode_1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton1()
{
	TCHAR tcString[25];

	m_pFileIO->OpenDxfFile();

	DLG_cstrFileName = m_pFileIO->get_FileName();

	// Load all DXF segments
	m_pFileIO->LoadDxfSegments(m_pFileIO->get_FilePtr(), &m_cmCutInsideFeatures, &m_cmCutOutsideFeatures, &m_cmCutOnFeatures,
							   m_pSaUnconnectedCutInsideDxfSegments, m_pSaUnconnectedCutOutsideDxfSegments, m_pSaUnconnectedCutOnDxfSegments);

	int nInsideFeatures = m_pFileIO->get_DxfSegmets_INSIDE();
	int nOutsideFeatures = m_pFileIO->get_DxfSegmets_OUTSIDE();
	int nOnFeatures = m_pFileIO->get_DxfSegmets_ON();
	
//	int nOnArcSegments = m_pFileIO->get_DxfArcSegments_ON();
//	int nOnLineSegments = m_pFileIO->get_DxfLineSegments_ON();
//	int nOnCircleSegments = m_pFileIO->get_DxfCircleSegments_ON();
 
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfArcSegments_ON());
	GetDlgItem(ID_STATIC_ON_ARCS)->SetWindowTextW (tcString);
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfLineSegments_ON());
	GetDlgItem(ID_STATIC_ON_LINES)->SetWindowTextW (tcString);
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfCircleSegments_ON());
	GetDlgItem(ID_STATIC_ON_CIRCLES)->SetWindowTextW (tcString);

	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfArcSegments_INSIDE());
	GetDlgItem(ID_STATIC_IN_ARCS)->SetWindowTextW (tcString);
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfLineSegments_INSIDE());
	GetDlgItem(ID_STATIC_IN_LINES)->SetWindowTextW (tcString);
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfCircleSegments_INSIDE());
	GetDlgItem(ID_STATIC_IN_CIRCLES)->SetWindowTextW (tcString);

	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfArcSegments_OUTSIDE());
	GetDlgItem(ID_STATIC_OUT_ARCS)->SetWindowTextW (tcString);
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfLineSegments_OUTSIDE());
	GetDlgItem(ID_STATIC_OUT_LINES)->SetWindowTextW (tcString);
	swprintf(tcString, 25, _T("%d"), m_pFileIO->get_DxfCircleSegments_OUTSIDE());
	GetDlgItem(ID_STATIC_OUT_CIRCLES)->SetWindowTextW (tcString);

	DXF_VERSION eVendor = m_pFileIO->get_VendorInfo();
	if (ACAD_14 == eVendor)
	{
		swprintf(tcString, 25, _T("ACAD"));
		GetDlgItem(IDC_STATIC_VENDOR_ID)->SetWindowTextW (tcString);
		swprintf(tcString, 25, _T("V-14"));
		GetDlgItem(IDC_STATIC_VENDOR_VER)->SetWindowTextW (tcString);
	}
	else
	{
		swprintf(tcString, 25, _T("UnKnown"));
		GetDlgItem(IDC_STATIC_VENDOR_ID)->SetWindowTextW (tcString);
		swprintf(tcString, 25, _T("???"));
		GetDlgItem(IDC_STATIC_VENDOR_VER)->SetWindowTextW (tcString);
	}

 	UpdateData (FALSE);
	
	DLG_FileOpenButton.EnableWindow(0);
	DLG_FileCloseButton.EnableWindow(1);
	DLG_CalcPathsButton.EnableWindow(1);
}

void CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton3()
{
	UpdateData (TRUE);
	double dToolDiam = DLG_dToolDiameter;

	// Calculate cut paths
	::SetCursor(LoadCursor(NULL, IDC_WAIT));

	// Process both inside & outside features
	int nInsideFeatures = m_pToolPaths->ExtractDxfFeatures (eCutInsideFeature, &m_cmCutInsideFeatures, m_pSaUnconnectedCutInsideDxfSegments, dToolDiam);
	int nOutsideFeatures = m_pToolPaths->ExtractDxfFeatures (eCutOutsideFeature, &m_cmCutOutsideFeatures, m_pSaUnconnectedCutOutsideDxfSegments, dToolDiam);
	int nOnFeatures = m_pToolPaths->ExtractDxfFeatures (eCutOnFeature, &m_cmCutOnFeatures, m_pSaUnconnectedCutOnDxfSegments, dToolDiam);

	DLG_CalcPathsButton.EnableWindow(0);
	DLG_ViewAllPathsButton.EnableWindow(1);
	DLG_ViewFinalPathButton.EnableWindow(1);
	DLG_OutputGCodeButton.EnableWindow(1);
}

void CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton4()
{
	// View all cut paths
	CSmartArrayConnectedSegments *pSaCenterlineSegments;
	CSmartArrayConnectedSegments *pSaPathASegments;
	CSmartArrayConnectedSegments *pSaPathBSegments;
	CSmartArrayConnectedSegments *pSaFinalPathSegments;

	// Create a segment SArray of all feature segments
	CSmartArrayOpenGlSegments *pSaCenterlineDisplaySegments = new CSmartArrayOpenGlSegments (50, 10);
	CSmartArrayOpenGlSegments *pSaPathADisplaySegments = new CSmartArrayOpenGlSegments (50, 10);
	CSmartArrayOpenGlSegments *pSaPathBDisplaySegments = new CSmartArrayOpenGlSegments (50, 10);
	CSmartArrayOpenGlSegments *pSaFinalPathDisplaySegments = new CSmartArrayOpenGlSegments (50, 10);
 
	CONNECTED_SEGMENT tDxfSegment;
	OPENGL_SEGMENT tOpenGlSegment;

	FEATURE tFeature;
//	int nArraySize;
	int nFeatureCount = m_cmCutInsideFeatures.GetCount();
	TRACE("OnDisplayFeatureToolPaths()..Inside cut feature map count: %d\n", nFeatureCount);
	
	// Seed initial display boundary conditions
	double dMinX = 1.0E25, dMinY = 1.0E25;
	double dMaxX = -1.0E25, dMaxY = -1.0E25;

	int i;
	for (i=1; i<=nFeatureCount; i++)
	{
		m_cmCutInsideFeatures.Lookup (i, tFeature);
		if (0 <= TRACE_VERBOSITY)
		{
			TRACE ("Feature: %d (%s), Segments: %d\n", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			TRACE ("    Centroid: X:%lf, Y:%lf\n", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
		}
		
		// Update display boundary regions
		if (tFeature.m_BoundingRect.dLeft < dMinX)
			dMinX = tFeature.m_BoundingRect.dLeft;
		if (tFeature.m_BoundingRect.dBottom < dMinY)
			dMinY = tFeature.m_BoundingRect.dBottom;
		if (dMaxX < tFeature.m_BoundingRect.dRight)
			dMaxX = tFeature.m_BoundingRect.dRight;
		if (dMaxY < tFeature.m_BoundingRect.dTop)
			dMaxY = tFeature.m_BoundingRect.dTop;
	
		pSaCenterlineSegments = tFeature.m_pSaConnectedDxfSegments;
		pSaPathASegments = tFeature.m_pSaToolPathA;
		pSaPathBSegments = tFeature.m_pSaToolPathB;

		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pSaCenterlineSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eSolid;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaCenterlineDisplaySegments->AppendElement (tOpenGlSegment);

		//	if (tFeature.m_dToolPathLengthA < tFeature.m_dToolPathLengthB)
		//	{
				tDxfSegment = pSaPathASegments->GetAt(j);
				tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
				tOpenGlSegment.eLinePattern = eShortDash;
				tOpenGlSegment.fLineWidth = 2.0;
				tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
				tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
				tOpenGlSegment.fAz = 0.0f;
				tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
				tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
				tOpenGlSegment.fBz = 0.0f;
				tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
				tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
				tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
				tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
				tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
				pSaPathADisplaySegments->AppendElement (tOpenGlSegment);
		//	}
		//	else
		//	{
				tDxfSegment = pSaPathBSegments->GetAt(j);
				tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
				tOpenGlSegment.eLinePattern = eShortDash;
				tOpenGlSegment.fLineWidth = 2.0;
				tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
				tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
				tOpenGlSegment.fAz = 0.0f;
				tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
				tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
				tOpenGlSegment.fBz = 0.0f;
				tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
				tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
				tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
				tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
				tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
				pSaPathBDisplaySegments->AppendElement (tOpenGlSegment);
		//	}
		}
	}

	nFeatureCount = m_cmCutOutsideFeatures.GetCount();
	TRACE("OnDisplayFeatureToolPaths()..Outside cut feature map count: %d\n", nFeatureCount);
	
	for (i=1; i<=nFeatureCount; i++)
	{
		m_cmCutOutsideFeatures.Lookup (i, tFeature);
		if (0 <= TRACE_VERBOSITY)
		{
			TRACE ("Feature: %d (%s), Segments: %d\n", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			TRACE ("    Centroid: X:%lf, Y:%lf\n", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
		}

		// Update display boundary regions
		if (tFeature.m_BoundingRect.dLeft < dMinX)
			dMinX = tFeature.m_BoundingRect.dLeft;
		if (tFeature.m_BoundingRect.dBottom < dMinY)
			dMinY = tFeature.m_BoundingRect.dBottom;
		if (dMaxX < tFeature.m_BoundingRect.dRight)
			dMaxX = tFeature.m_BoundingRect.dRight;
		if (dMaxY < tFeature.m_BoundingRect.dTop)
			dMaxY = tFeature.m_BoundingRect.dTop;
	
		pSaCenterlineSegments = tFeature.m_pSaConnectedDxfSegments;
		pSaPathASegments = tFeature.m_pSaToolPathA;
		pSaPathBSegments = tFeature.m_pSaToolPathB;

		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pSaCenterlineSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eSolid;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaCenterlineDisplaySegments->AppendElement (tOpenGlSegment);

			tDxfSegment = pSaPathASegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eShortDash;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaPathADisplaySegments->AppendElement (tOpenGlSegment);

			tDxfSegment = pSaPathBSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eShortDash;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaPathBDisplaySegments->AppendElement (tOpenGlSegment);
		}
	}

	int nCutOnFeatureCount = m_cmCutOnFeatures.GetCount();
	TRACE("OnDisplayFeatureToolPaths()..ON cut feature map count: %d\n", nCutOnFeatureCount);
	
	for (i=1; i<=nCutOnFeatureCount; i++)
	{
		m_cmCutOnFeatures.Lookup (i, tFeature);
		if (0 <= TRACE_VERBOSITY)
		{
			TRACE ("ON Feature: %d (%s), Segments: %d\n", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			TRACE ("    Centroid: X:%lf, Y:%lf\n", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
		}

		// Update display boundary regions
		if (tFeature.m_BoundingRect.dLeft < dMinX)
			dMinX = tFeature.m_BoundingRect.dLeft;
		if (tFeature.m_BoundingRect.dBottom < dMinY)
			dMinY = tFeature.m_BoundingRect.dBottom;
		if (dMaxX < tFeature.m_BoundingRect.dRight)
			dMaxX = tFeature.m_BoundingRect.dRight;
		if (dMaxY < tFeature.m_BoundingRect.dTop)
			dMaxY = tFeature.m_BoundingRect.dTop;
	
		pSaCenterlineSegments = tFeature.m_pSaConnectedDxfSegments;
		pSaFinalPathSegments = tFeature.m_pSaToolPathFinishCut;

		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();
		int nJunk = pSaFinalPathSegments->GetUsedUpperBound();

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pSaCenterlineSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eSolid;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaCenterlineDisplaySegments->AppendElement (tOpenGlSegment);
			pSaFinalPathDisplaySegments->AppendElement (tOpenGlSegment);	
		}
	}

	// How many display segments ?
//	nArraySize = 1 + pSaCenterlineDisplaySegments->GetUsedUpperBound();
//	nArraySize = 1 + pSaPathADisplaySegments->GetUsedUpperBound();
//	nArraySize = 1 + pSaPathBDisplaySegments->GetUsedUpperBound();
 
//	m_fPartWidth = (float)(6.25);
//	m_fPartHeight = (float)(6.25);
//	m_fOffsetX = (float)1.8;
//	m_fOffsetY = (float)1.6;

	m_fPartWidth = (float)(1.0 * (dMaxX - dMinX));
	m_fPartHeight = (float)(1.0 * (dMaxY - dMinY));
	m_fOffsetX = (float)(dMinX / 1.0);
	m_fOffsetY = (float)(dMinY / 1.0);

	TRACE ("Part width: %f, height: %f\n", m_fPartWidth, m_fPartHeight);

	// Test
	UpdateData(TRUE);

//	m_fPartWidth /= m_fZoomScale;
//	m_fPartHeight /= m_fZoomScale;
 	COpenGL *pOpenGlWindow = new COpenGL (pSaCenterlineDisplaySegments, pSaPathADisplaySegments, pSaPathBDisplaySegments, m_fPartWidth, m_fPartHeight, m_fOffsetX, m_fOffsetY, 1.0f);
 
	// Create the OpenGL Window
//	if (!pOpenGlWindow->CreateGLWindow(_T("Centerline & Inner/Outer Tool Paths"),640,480,16,false))
//	if (!pOpenGlWindow->CreateGlutGLWindow_1(_T("Centerline & Inner/Outer Tool Paths"),640,480,16,false))
	if (!pOpenGlWindow->CreateGlutGLWindow_2("Centerline & Inner/Outer Tool Paths", 400, 400, m_fPartWidth, m_fPartHeight, 16,false))
	{
		ASSERT(FALSE);
		return;
		// Quit If Window Was Not Created
	}

	delete pOpenGlWindow;

	delete pSaCenterlineDisplaySegments;
	delete pSaPathADisplaySegments;
	delete pSaPathBDisplaySegments;
}

void CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton5()
{
 	CSmartArrayConnectedSegments *pSaCenterlineSegments;
 	CSmartArrayConnectedSegments *pSaFinalPathSegments;

	// Create a segment SArray of all feature segments
	CSmartArrayOpenGlSegments *pSaCenterlineDisplaySegments = new CSmartArrayOpenGlSegments (50, 10);
 	CSmartArrayOpenGlSegments *pSaFinalPathDisplaySegments = new CSmartArrayOpenGlSegments (50, 10);

	CONNECTED_SEGMENT tDxfSegment;
	OPENGL_SEGMENT tOpenGlSegment;

	FEATURE tFeature;
	int nArraySize;
	int nFeatureCount = m_cmCutInsideFeatures.GetCount();
	TRACE("OnDisplayFeatureToolPaths()..Inside cut feature map count: %d\n", nFeatureCount);
	
	// Seed initial display boundary conditions
	double dMinX = 1.0E25, dMinY = 1.0E25;
	double dMaxX = -1.0E25, dMaxY = -1.0E25;

	int i;
	for (i=1; i<=nFeatureCount; i++)
	{
		m_cmCutInsideFeatures.Lookup (i, tFeature);
		if (0 <= TRACE_VERBOSITY)
		{
			TRACE ("Feature: %d (%s), Segments: %d\n", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			TRACE ("    Centroid: X:%lf, Y:%lf\n", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
		}
		
		// Update display boundary regions
		if (tFeature.m_BoundingRect.dLeft < dMinX)
			dMinX = tFeature.m_BoundingRect.dLeft;
		if (tFeature.m_BoundingRect.dBottom < dMinY)
			dMinY = tFeature.m_BoundingRect.dBottom;
		if (dMaxX < tFeature.m_BoundingRect.dRight)
			dMaxX = tFeature.m_BoundingRect.dRight;
		if (dMaxY < tFeature.m_BoundingRect.dTop)
			dMaxY = tFeature.m_BoundingRect.dTop;
	
 		pSaCenterlineSegments = tFeature.m_pSaConnectedDxfSegments;
 		pSaFinalPathSegments = tFeature.m_pSaToolPathFinishCut;

		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();
 		int nJunk = pSaFinalPathSegments->GetUsedUpperBound();

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pSaCenterlineSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eSolid;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaCenterlineDisplaySegments->AppendElement (tOpenGlSegment);

//			if (tFeature.m_dToolPathLengthA < tFeature.m_dToolPathLengthB)
//			{
//				tDxfSegment = pSaPathASegments->GetAt(j);
//				tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
//				tOpenGlSegment.eLinePattern = eShortDash;
//				tOpenGlSegment.fLineWidth = 2.0;
//				tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
//				tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
//				tOpenGlSegment.fAz = 0.0f;
//				tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
//				tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
//				tOpenGlSegment.fBz = 0.0f;
//				tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
//				tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
//				tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
//				tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
//				tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
//				pSaPathADisplaySegments->InsertElement (tOpenGlSegment);
//			}
//			else
//			{
//				tDxfSegment = pSaPathBSegments->GetAt(j);
//				tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
//				tOpenGlSegment.eLinePattern = eShortDash;
//				tOpenGlSegment.fLineWidth = 2.0;
//				tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
//				tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
//				tOpenGlSegment.fAz = 0.0f;
//				tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
//				tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
//				tOpenGlSegment.fBz = 0.0f;
//				tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
//				tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
//				tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
//				tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
//				tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
//				pSaPathBDisplaySegments->InsertElement (tOpenGlSegment);
//			}

			tDxfSegment = pSaFinalPathSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eShortDash;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaFinalPathDisplaySegments->AppendElement (tOpenGlSegment);
		}
	}

	nFeatureCount = m_cmCutOutsideFeatures.GetCount();
	TRACE("OnDisplayFeatureToolPaths()..Outside cut feature map count: %d\n", nFeatureCount);
	
	for (i=1; i<=nFeatureCount; i++)
	{
		m_cmCutOutsideFeatures.Lookup (i, tFeature);
		if (0 <= TRACE_VERBOSITY)
		{
			TRACE ("Feature: %d (%s), Segments: %d\n", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			TRACE ("    Centroid: X:%lf, Y:%lf\n", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
		}

		// Update display boundary regions
		if (tFeature.m_BoundingRect.dLeft < dMinX)
			dMinX = tFeature.m_BoundingRect.dLeft;
		if (tFeature.m_BoundingRect.dBottom < dMinY)
			dMinY = tFeature.m_BoundingRect.dBottom;
		if (dMaxX < tFeature.m_BoundingRect.dRight)
			dMaxX = tFeature.m_BoundingRect.dRight;
		if (dMaxY < tFeature.m_BoundingRect.dTop)
			dMaxY = tFeature.m_BoundingRect.dTop;
	
		pSaCenterlineSegments = tFeature.m_pSaConnectedDxfSegments;
//		pSaPathASegments = tFeature.m_pSaToolPathA;
//		pSaPathBSegments = tFeature.m_pSaToolPathB;
		pSaFinalPathSegments = tFeature.m_pSaToolPathFinishCut;

		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();
//		int nJunk = pSaPathASegments->GetUsedUpperBound();
//		nJunk = pSaPathBSegments->GetUsedUpperBound();
		int nJunk = pSaFinalPathSegments->GetUsedUpperBound();

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pSaCenterlineSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eSolid;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaCenterlineDisplaySegments->AppendElement (tOpenGlSegment);

	//		if (tFeature.m_dToolPathLengthB < tFeature.m_dToolPathLengthA)
	//		{
// 				tDxfSegment = pSaPathASegments->GetAt(j);
//				tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
//				tOpenGlSegment.eLinePattern = eShortDash;
//				tOpenGlSegment.fLineWidth = 2.0;
//				tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
//				tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
//				tOpenGlSegment.fAz = 0.0f;
//				tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
//				tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
//				tOpenGlSegment.fBz = 0.0f;
//				tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
//				tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
//				tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
//				tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
//				tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
//				pSaPathADisplaySegments->InsertElement (tOpenGlSegment);
	//		}
	//		else
	//		{
//				tDxfSegment = pSaPathBSegments->GetAt(j);
//				tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
//				tOpenGlSegment.eLinePattern = eShortDash;
//				tOpenGlSegment.fLineWidth = 2.0;
//				tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
//				tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
//				tOpenGlSegment.fAz = 0.0f;
//				tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
//				tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
//				tOpenGlSegment.fBz = 0.0f;
//				tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
//				tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
//				tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
//				tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
//				tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
//				pSaPathBDisplaySegments->InsertElement (tOpenGlSegment);
	//		}
		
			tDxfSegment = pSaFinalPathSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eShortDash;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaFinalPathDisplaySegments->AppendElement (tOpenGlSegment);
		}

	}

	nFeatureCount = m_cmCutOnFeatures.GetCount();
	TRACE("OnDisplayFeatureToolPaths()..ON cut feature map count: %d\n", nFeatureCount);
	
	for (i=1; i<=nFeatureCount; i++)
	{
		m_cmCutOnFeatures.Lookup (i, tFeature);
		if (0 <= TRACE_VERBOSITY)
		{
			TRACE ("ON Feature: %d (%s), Segments: %d\n", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			TRACE ("    Centroid: X:%lf, Y:%lf\n", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
		}

		// Update display boundary regions
		if (tFeature.m_BoundingRect.dLeft < dMinX)
			dMinX = tFeature.m_BoundingRect.dLeft;
		if (tFeature.m_BoundingRect.dBottom < dMinY)
			dMinY = tFeature.m_BoundingRect.dBottom;
		if (dMaxX < tFeature.m_BoundingRect.dRight)
			dMaxX = tFeature.m_BoundingRect.dRight;
		if (dMaxY < tFeature.m_BoundingRect.dTop)
			dMaxY = tFeature.m_BoundingRect.dTop;
	
		pSaCenterlineSegments = tFeature.m_pSaConnectedDxfSegments;
//		pSaPathASegments = tFeature.m_pSaToolPathA;
//		pSaPathBSegments = tFeature.m_pSaToolPathB;
		pSaFinalPathSegments = tFeature.m_pSaToolPathFinishCut;

		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();
//		int nJunk = pSaPathASegments->GetUsedUpperBound();
//		nJunk = pSaPathBSegments->GetUsedUpperBound();
		int nJunk = pSaFinalPathSegments->GetUsedUpperBound();

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
#if 1
			tDxfSegment = pSaCenterlineSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eSolid;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaCenterlineDisplaySegments->AppendElement (tOpenGlSegment);
#endif 	
#if 0
			tDxfSegment = pSaFinalPathSegments->GetAt(j);
			tOpenGlSegment.eSegmentType = tDxfSegment.eSegmentType;
			tOpenGlSegment.eLinePattern = eShortDash;
			tOpenGlSegment.fLineWidth = 2.0;
			tOpenGlSegment.fAx = (float)tDxfSegment.dAx;
			tOpenGlSegment.fAy = (float)tDxfSegment.dAy;
			tOpenGlSegment.fAz = 0.0f;
			tOpenGlSegment.fBx = (float)tDxfSegment.dBx;
			tOpenGlSegment.fBy = (float)tDxfSegment.dBy;
			tOpenGlSegment.fBz = 0.0f;
			tOpenGlSegment.fRx = (float)tDxfSegment.dRx;
			tOpenGlSegment.fRy = (float)tDxfSegment.dRy;
			tOpenGlSegment.fRad = (float)tDxfSegment.dRad;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pSaFinalPathDisplaySegments->AppendElement (tOpenGlSegment);
#endif 	
		}
	}

	// How many display segments ?
	nArraySize = 1 + pSaCenterlineDisplaySegments->GetUsedUpperBound();
	nArraySize = 1 + pSaFinalPathDisplaySegments->GetUsedUpperBound();
//	nArraySize = 1 + pSaPathBDisplaySegments->GetUsedUpperBound();

//	GLB_fPartWidth = (float)(LoopRect.dRight - LoopRect.dLeft);
//	GLB_fPartHeight = (float)(LoopRect.dTop - LoopRect.dBottom);
//	GLB_fOffsetX = (float)LoopRect.dLeft;
//	GLB_fOffsetY = (float)LoopRect.dBottom;

	m_fPartWidth = (float)(6.25);
	m_fPartHeight = (float)(6.25);
	m_fOffsetX = (float)1.8;
	m_fOffsetY = (float)1.6;

	m_fPartWidth = (float)(1.0 * (dMaxX - dMinX));
	m_fPartHeight = (float)(1.0 * (dMaxY - dMinY));
	m_fOffsetX = (float)(dMinX / 1.0);
	m_fOffsetY = (float)(dMinY / 1.0);

	TRACE ("Part width: %f, height: %f\n", m_fPartWidth, m_fPartHeight);

	// Create the GLUT OpenGL Window
	COpenGL *pOpenGlWindow = new COpenGL (pSaCenterlineDisplaySegments, pSaFinalPathDisplaySegments, NULL, m_fPartWidth, m_fPartHeight, m_fOffsetX, m_fOffsetY, 1.0f);
	if (!pOpenGlWindow->CreateGlutGLWindow_2("Centerline & Final Tool Path", 400, 400, m_fPartWidth, m_fPartHeight, 16,false))
	{
		ASSERT(FALSE);
		return;
		// Quit If Window Was Not Created
	}

	delete pOpenGlWindow;
  
	delete pSaCenterlineDisplaySegments;
	delete pSaFinalPathDisplaySegments;
}

void CALTA_DXF_To_GCode_1Dlg::OnBnClickedButton6()
{
	// Output GCode
	TCHAR tcString[25];

	UpdateData (TRUE);

	double dCosCompliance = cos (DLG_dEndpointCompliance * 3.141592653589 / 180.0); 

	int nCheck = DLG_EndpointCompEnabled.GetCheck();
	BOOL bChainingOn = (BST_CHECKED == DLG_EndpointCompEnabled.GetCheck());
	int nLinesOutput = m_pFileIO->OutputGCodeFile(&m_cmCutInsideFeatures, &m_cmCutOutsideFeatures, &m_cmCutOnFeatures, bChainingOn, dCosCompliance, DLG_dCutHeight, DLG_dCutSpeed, DLG_dTransHeight, DLG_dTransSpeed, DLG_dPlungeSpeed, DLG_dRetractSpeed);
 
	swprintf(tcString, 25, _T("%d"), nLinesOutput);
	GetDlgItem(ID_STATIC_LINES_OUTPUT)->SetWindowTextW (tcString);
}


void CALTA_DXF_To_GCode_1Dlg::OnBnClickedCheck1()
{
	// TODO: Add your control notification handler code here
}
