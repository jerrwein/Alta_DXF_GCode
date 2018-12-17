#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "enumsBase.h"
#include "STL_VectorBase.h"
#include "STL_MapBase.h"
#include "GL/freeglut.h"
#include "GL/gl.h"
#include "OpenGL_DisplayPaths.h"

using namespace std;

#define OPENGL_VERBOSITY 6
#define SUB_WIND_WIDTH 800
#define SUB_WIND_HEIGHT 800

extern int INI_Verbosity_OpenGL;

char		chReport_OGL_DP[400];
char		chReport_OGL[400];

static int nMouseDownX = 0;
static int nMouseDownY = 0;
static float fMouseDownGlbX = 0.0;
static float fMouseDownGlbY = 0.0;

STL_OPENGL_CLASS *COpenGL::m_pSaSortedSegments;
STL_OPENGL_CLASS *COpenGL::m_pSaLoop1Segments;
STL_OPENGL_CLASS *COpenGL::m_pSaLoop2Segments;
STL_OPENGL_CLASS *COpenGL::m_pSaMaterialBlockSegments;
STL_OPENGL_CLASS *COpenGL::m_pSaGCodeSegments;

long  COpenGL::m_lCountX;
long  COpenGL::m_lCountY;
long  COpenGL::m_lCountZ;
float COpenGL::m_fRotationFactorX;
float COpenGL::m_fRotationFactorY;
float COpenGL::m_fRotationFactorZ;
float COpenGL::m_fXRotation;
float COpenGL::m_fYRotation;
float COpenGL::m_fZRotation;
//float COpenGL::m_fScaleFactor;

GLfloat COpenGL::m_fViewportOffsetX;
GLfloat COpenGL::m_fViewportOffsetY;
GLfloat COpenGL::m_fViewportWidth;
GLfloat COpenGL::m_fViewportHeight;
GLfloat COpenGL::m_fViewPortAspectRatio;
GLfloat COpenGL::m_fPartAspectRatio;

OpenGL_DisplayPaths::OpenGL_DisplayPaths()
{

}

void OpenGL_DisplayPaths::DisplayAllPaths(STL_MAP_CLASS *pInsideFeaturesMap, STL_MAP_CLASS *pOutsideFeaturesMap, STL_MAP_CLASS *pOnFeaturesMap)
{
//	char chReport[200];

	// View all cut paths
	STL_CONNECTED_SEGS_CLASS *pCenterlineSegmentClass;
	STL_CONNECTED_SEGS_CLASS *pPathAlineSegmentClass;
	STL_CONNECTED_SEGS_CLASS *pPathBlineSegmentClass;
// JMW ddff 08/27/2014
//	STL_CONNECTED_SEGS_CLASS *pFinalPathSegmentClass;

	// Create a segment class of all OpenGL feature segments
	STL_OPENGL_CLASS *pCenterlineDisplaySegmentClass = new STL_OPENGL_CLASS;
	STL_OPENGL_CLASS *pPathADisplaySegmentClass = new STL_OPENGL_CLASS;
	STL_OPENGL_CLASS *pPathBDisplaySegmentClass = new STL_OPENGL_CLASS;
// JMW ddff 08/27/2014
//	STL_OPENGL_CLASS *pFinalPathDisplaySegmentClass = new STL_OPENGL_CLASS;

	CONNECTED_ELEMENT tDxfSegment;
	OPENGL_ELEMENT tOpenGlSegment;

	FEATURE_ELEMENT tFeature;
	int nFeatureCount = pInsideFeaturesMap->SMC_GetSize();

	sprintf (chReport_OGL_DP, "Inside Feature map, feature count: %ld", pInsideFeaturesMap->SMC_GetSize());
	ReportBasedOnVerbosity (4, chReport_OGL_DP);

	// Seed initial display boundary conditions
	double dMinX = 1.0E25, dMinY = 1.0E25;
	double dMaxX = -1.0E25, dMaxY = -1.0E25;

	int i;
	for (i=0; i<nFeatureCount; i++)
	{
		pInsideFeaturesMap->SMC_GetFeature(i, &tFeature);
		if (0 <= OPENGL_VERBOSITY)
		{
			sprintf (chReport_OGL_DP, "  Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity (2, chReport_OGL_DP);
			sprintf (chReport_OGL_DP, "    Centroid: X:%lf, Y:%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity (2, chReport_OGL_DP);
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

		pCenterlineSegmentClass = tFeature.m_pSaConnectedDxfSegments;
		pPathAlineSegmentClass = tFeature.m_pSaToolPathA;
		pPathBlineSegmentClass = tFeature.m_pSaToolPathB;

		int nMaxIndex = pCenterlineSegmentClass->SVC_Size() - 1;

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pCenterlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pCenterlineDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);

			tDxfSegment = pPathAlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pPathADisplaySegmentClass->SVC_AddElement(tOpenGlSegment);

			tDxfSegment = pPathBlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pPathBDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
			//	}
		}
	}

	nFeatureCount = pOutsideFeaturesMap->SMC_GetSize();
//	cout << "Outside Feature map, count: " << pOutsideFeaturesMap->SMC_GetSize() << std::endl;
	for (i=0; i<nFeatureCount; i++)
	{
		pOutsideFeaturesMap->SMC_GetFeature(i, &tFeature);
		if (0 <= OPENGL_VERBOSITY)
		{
			sprintf (chReport_OGL_DP, "  Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
			sprintf (chReport_OGL_DP, "    Centroid: X:%lf, Y:%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
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

		pCenterlineSegmentClass = tFeature.m_pSaConnectedDxfSegments;
		pPathAlineSegmentClass = tFeature.m_pSaToolPathA;
		pPathBlineSegmentClass = tFeature.m_pSaToolPathB;

		int nMaxIndex = pCenterlineSegmentClass->SVC_Size() - 1;

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pCenterlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pCenterlineDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);

			tDxfSegment = pPathAlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pPathADisplaySegmentClass->SVC_AddElement(tOpenGlSegment);

			tDxfSegment = pPathBlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pPathBDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
		}
	}

	nFeatureCount = pOnFeaturesMap->SMC_GetSize();
//	cout << "ON Feature map, count: " << pOnFeaturesMap->SMC_GetSize() << std::endl;

	for (i=0; i<nFeatureCount; i++)
	{
		pOnFeaturesMap->SMC_GetFeature(i, &tFeature);
		if (0 <= OPENGL_VERBOSITY)
		{
			sprintf (chReport_OGL_DP, "  Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
			sprintf (chReport_OGL_DP, "    Centroid: X:%lf, Y:%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
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

		pCenterlineSegmentClass = tFeature.m_pSaConnectedDxfSegments;
// JMW ddff 08/27/2014
//		pFinalPathSegmentClass = tFeature.m_pSaToolPathFinishCut;

//		int nMaxIndex = pSaCenterlineSegments->GetUsedUpperBound();
//		int nJunk = pSaFinalPathSegments->GetUsedUpperBound();
		int nMaxIndex = pCenterlineSegmentClass->SVC_Size() - 1;
//		int nJunk = pFinalPathSegmentClass->SVC_Size() - 1;

		// Get all feature segments and insert them into the display SArray
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pCenterlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pCenterlineDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
//			pFinalPathDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
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

	m_fPartWidth = (float)(1.1 * (dMaxX - dMinX));
	m_fPartHeight = (float)(1.1 * (dMaxY - dMinY));
	m_fOffsetX = (float)(dMinX - (m_fPartWidth / 20.0));
	m_fOffsetY = (float)(dMinY - (m_fPartHeight / 20.0));

	sprintf (chReport_OGL_DP, "Part width: %f, height: %f", m_fPartWidth, m_fPartHeight);
	ReportBasedOnVerbosity (2, chReport_OGL_DP);

	// Test
	//	UpdateData(TRUE);

	//	m_fPartWidth /= m_fZoomScale;
	//	m_fPartHeight /= m_fZoomScale;

#if 0
	void renderFunction()
	{
	    glClearColor(0.0, 0.0, 0.0, 0.0);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glColor3f(1.0, 1.0, 1.0);
	    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	    glBegin(GL_POLYGON);
	        glVertex2f(-0.5, -0.5);
	        glVertex2f(-0.5, 0.5);
	        glVertex2f(0.5, 0.5);
	        glVertex2f(0.5, -0.5);
	    glEnd();
	    glFlush();
	}

	/* Main method - main entry point of application
	the freeglut library does the window creation work for us,
	regardless of the platform. */
	int main(int argc, char** argv)
	{
	    glutInit(&argc, argv);
	    glutInitDisplayMode(GLUT_SINGLE);
	    glutInitWindowSize(500,500);
	    glutInitWindowPosition(100,100);
	    glutCreateWindow("OpenGL - First window demo");
	    glutDisplayFunc(renderFunction);
	    glutMainLoop();
	    return 0;
	}
#endif

	char chTitle[] = {"Centerline & Inner/Outer Tool Paths"};	/* JMW DDFF 08/26/2014 */
	// Create the OpenGL Window
	COpenGL *pOpenGlWindow = new COpenGL (pCenterlineDisplaySegmentClass, pPathADisplaySegmentClass, pPathBDisplaySegmentClass, m_fPartWidth, m_fPartHeight, m_fOffsetX, m_fOffsetY, 1.0f);
//	if (!pOpenGlWindow->CreateGlutGLWindow_2("Centerline & Inner/Outer Tool Paths", SUB_WIND_WIDTH, SUB_WIND_HEIGHT, m_fPartWidth, m_fPartHeight, 16,false))
	if (!pOpenGlWindow->CreateGlutGLWindow_2(chTitle, SUB_WIND_WIDTH, SUB_WIND_HEIGHT, m_fPartWidth, m_fPartHeight, 16,false))
	{
		assert(false);
		return;
			// Quit If Window Was Not Created
	}

	delete pOpenGlWindow;

	delete pCenterlineDisplaySegmentClass;
	delete pPathADisplaySegmentClass;
	delete pPathBDisplaySegmentClass;
}

void OpenGL_DisplayPaths::DisplayCutPaths(STL_MAP_CLASS *pInsideFeaturesMap, STL_MAP_CLASS *pOutsideFeaturesMap, STL_MAP_CLASS *pOnFeaturesMap)
{
//	char chReport[200];
	// View final cut paths
	STL_CONNECTED_SEGS_CLASS *pCenterlineSegmentClass;
	STL_CONNECTED_SEGS_CLASS *pFinalPathSegmentClass;

	// Create a segment class of all OpenGL feature segments
	STL_OPENGL_CLASS *pCenterlineDisplaySegmentClass = new STL_OPENGL_CLASS;
	STL_OPENGL_CLASS *pFinalPathDisplaySegmentClass = new STL_OPENGL_CLASS;

	CONNECTED_ELEMENT tDxfSegment;
	OPENGL_ELEMENT tOpenGlSegment;

	FEATURE_ELEMENT tFeature;
	int nFeatureCount = pInsideFeaturesMap->SMC_GetSize();
//	cout << "Inside Feature map, feature count: " << pInsideFeaturesMap->SMC_GetSize() << std::endl;

	// Seed initial display boundary conditions
	double dMinX = 1.0E25, dMinY = 1.0E25;
	double dMaxX = -1.0E25, dMaxY = -1.0E25;

	int i;
	for (i=0; i<nFeatureCount; i++)
	{
		pInsideFeaturesMap->SMC_GetFeature(i, &tFeature);
		if (0 <= OPENGL_VERBOSITY)
		{
			sprintf (chReport_OGL_DP, "  Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
			sprintf (chReport_OGL_DP, "    Centroid: X:%lf, Y:%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
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

		pCenterlineSegmentClass = tFeature.m_pSaConnectedDxfSegments;
		pFinalPathSegmentClass = tFeature.m_pSaToolPathFinishCut;

		// Get all feature segments and insert them into the display SArray
		int nMaxIndex = pCenterlineSegmentClass->SVC_Size() - 1;
//		int nJunk = pFinalPathSegmentClass->SVC_Size() - 1;
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pCenterlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pCenterlineDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);

			tDxfSegment = pFinalPathSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pFinalPathDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
		}
	}

	nFeatureCount = pOutsideFeaturesMap->SMC_GetSize();
//	cout << "Outside Feature map, count: " << pOutsideFeaturesMap->SMC_GetSize() << std::endl;
	for (i=0; i<nFeatureCount; i++)
	{
		pOutsideFeaturesMap->SMC_GetFeature(i, &tFeature);
		if (0 <= OPENGL_VERBOSITY)
		{
			sprintf (chReport_OGL_DP, "  Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
			sprintf (chReport_OGL_DP, "    Centroid: X:%lf, Y:%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
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

		pCenterlineSegmentClass = tFeature.m_pSaConnectedDxfSegments;
		pFinalPathSegmentClass = tFeature.m_pSaToolPathFinishCut;

		// Get all feature segments and insert them into the display SArray
		int nMaxIndex = pCenterlineSegmentClass->SVC_Size() - 1;
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pCenterlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pCenterlineDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);

			tDxfSegment = pFinalPathSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pFinalPathDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
		}
	}

	nFeatureCount = pOnFeaturesMap->SMC_GetSize();
//	cout << "ON Feature map, count: " << pOnFeaturesMap->SMC_GetSize() << std::endl;

	for (i=0; i<nFeatureCount; i++)
	{
		pOnFeaturesMap->SMC_GetFeature(i, &tFeature);
		if (0 <= OPENGL_VERBOSITY)
		{
			sprintf (chReport_OGL_DP, "  Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
			sprintf (chReport_OGL_DP, "    Centroid: X:%lf, Y:%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity (4, chReport_OGL_DP);
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

		pCenterlineSegmentClass = tFeature.m_pSaConnectedDxfSegments;
		pFinalPathSegmentClass = tFeature.m_pSaToolPathFinishCut;

		// Get all feature segments and insert them into the display SArray
		int nMaxIndex = pCenterlineSegmentClass->SVC_Size() - 1;
//		int nJunk = pFinalPathSegmentClass->SVC_Size() - 1;
		for (int j=0; j<=nMaxIndex; j++)
		{
			tDxfSegment = pCenterlineSegmentClass->SVC_GetElement(j);
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
			tOpenGlSegment.fRadius = (float)tDxfSegment.dRadius;
			tOpenGlSegment.fStartAngle = (float)tDxfSegment.dStartAngle;
			tOpenGlSegment.fEndAngle = (float)tDxfSegment.dEndAngle;
			pCenterlineDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
//			pFinalPathDisplaySegmentClass->SVC_AddElement(tOpenGlSegment);
		}
	}

	// How many display segments ?
	// nArraySize = pCenterlineDisplaySegmentClass->SVC_Size();
	// nArraySize = pFinalPathDisplaySegmentClass->SVC_Size();

//	m_fPartWidth = (float)(6.25);
//	m_fPartHeight = (float)(6.25);
//	m_fOffsetX = (float)1.8;
//	m_fOffsetY = (float)1.6;

	m_fPartWidth = (float)(1.1 * (dMaxX - dMinX));
	m_fPartHeight = (float)(1.1 * (dMaxY - dMinY));
	m_fOffsetX = (float)(dMinX - (m_fPartWidth / 20.0));
	m_fOffsetY = (float)(dMinY - (m_fPartHeight / 20.0));

	sprintf (chReport_OGL_DP, "Part width: %f, height: %f", m_fPartWidth, m_fPartHeight);
	ReportBasedOnVerbosity (2, chReport_OGL_DP);

	// Create the GLUT OpenGL Window
	char chTitle[] = {"Centerline & Final Tool Path"};	/* JMW DDFF 08/26/2014 */
	COpenGL *pOpenGlWindow = new COpenGL (pCenterlineDisplaySegmentClass, pFinalPathDisplaySegmentClass, NULL, m_fPartWidth, m_fPartHeight, m_fOffsetX, m_fOffsetY, 1.0f);
//	if (!pOpenGlWindow->CreateGlutGLWindow_2("Centerline & Final Tool Path", SUB_WIND_WIDTH, SUB_WIND_HEIGHT, m_fPartWidth, m_fPartHeight, 16,false))
	if (!pOpenGlWindow->CreateGlutGLWindow_2(chTitle, SUB_WIND_WIDTH, SUB_WIND_HEIGHT, m_fPartWidth, m_fPartHeight, 16,false))
	{
		assert(false);
		return;
		// Quit If Window Was Not Created
	}

	delete pOpenGlWindow;
	delete pCenterlineDisplaySegmentClass;
	delete pFinalPathDisplaySegmentClass;
}

bool OpenGL_DisplayPaths::PostReport(const char *chReport)
{
	cout << chReport << std::endl;
	return true;
}

bool OpenGL_DisplayPaths::ReportBasedOnVerbosity(int nVerbosity, const char *chReport)
{
	// INI values range between 0 and 5
	// 1 <= nVerbosity <= 5
	if (nVerbosity <= INI_Verbosity_OpenGL)
	{
		cout << chReport << std::endl;
		return true;
	}
	else
		return false;
}

// *******************************************************************************************

COpenGL::COpenGL()
{
	m_bActive = true;
//	m_hDC = NULL;
// ddd	m_hRC = NULL;
//	m_hWnd = NULL;
	m_bFullscreen = false;
}
COpenGL::COpenGL(STL_OPENGL_CLASS *pSaMaterialBlockSegments, STL_OPENGL_CLASS *pSaGCodeSegments, float fWidth, float fHeight, float fOffsetX, float fOffsetY, float fScale)
{
	m_bActive = true;
//	m_hDC = NULL;
// ddd	m_hRC = NULL;
//	m_hWnd = NULL;
	m_bFullscreen = false;

	m_pSaSortedSegments = NULL;
	m_pSaLoop1Segments = NULL;
	m_pSaLoop2Segments = NULL;

 	m_pSaGCodeSegments = pSaGCodeSegments;
	m_pSaMaterialBlockSegments = pSaMaterialBlockSegments;

	m_fViewportWidth = fWidth;
	m_fViewportHeight = fHeight;
	m_fViewportOffsetX = fOffsetX;
	m_fViewportOffsetY = fOffsetY;

	m_fScaleFactor = fScale;

	m_fXRotation = 0.0;
	m_fYRotation = 0.0;
	m_fZRotation = 0.0;

	m_fRotationFactorX = 0.0;
	m_fRotationFactorY = 0.0;
	m_fRotationFactorZ = 0.0;

	m_lCountX = 0;
	m_lCountY = 0;
	m_lCountZ = 0;
}

COpenGL::COpenGL(STL_OPENGL_CLASS *pSaPart, STL_OPENGL_CLASS *pSaLoop1, STL_OPENGL_CLASS *pSaLoop2, float fWidth, float fHeight, float fOffsetX, float fOffsetY, float fScale)
{
	m_bActive = true;
//	m_hDC = NULL;
// ddd	m_hRC = NULL;
//	m_hWnd = NULL;
	m_bFullscreen = false;

 	m_pSaSortedSegments = pSaPart;
	m_pSaLoop1Segments = pSaLoop1;
	m_pSaLoop2Segments = pSaLoop2;

	m_pSaMaterialBlockSegments = NULL;
	m_pSaGCodeSegments = NULL;

	m_fViewportWidth = fWidth;
	m_fViewportHeight = fHeight;
	m_fPartAspectRatio = fWidth / fHeight;
	m_fViewportOffsetX = fOffsetX;
	m_fViewportOffsetY = fOffsetY;

	m_fScaleFactor = fScale;

	m_fXRotation = 0.0;
	m_fYRotation = 0.0;
	m_fZRotation = 0.0;

	m_fRotationFactorX = 0.0;
	m_fRotationFactorY = 0.0;
	m_fRotationFactorZ = 0.0;

	m_lCountX = 0;
	m_lCountY = 0;
	m_lCountZ = 0;
}

COpenGL::~COpenGL()
{

}

int COpenGL::CreateGlutGLWindow_2(char *pTitle, int width, int height, float fPartWidth, float fPartHeight, int bits, bool fullscreenflag)
{
	char chArg1[] = {"arg1"};	/* JMW DDFF 08/26/2014 */
	char chArg2[] = {"arg2"};	/* JMW DDFF 08/26/2014 */

	int argc = 1;
	char *argv[] = {pTitle, chArg1, chArg2};
	/* JMW DDFF 08/26/2014 */
	
	m_bFullscreen = fullscreenflag;				// Set The Global Fullscreen Flag

#if 0
//	glutInit(&argc, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize (500, 500);

	int window1 = glutCreateWindow("First Window - Perspective");
	glutDisplayFunc(&COpenGL::Glut_DrawScene1);
	glutReshapeFunc(&COpenGL::GLUT_reshape1);
	glutMouseFunc(&COpenGL::GLUT_mouse1);
	glutKeyboardFunc(&COpenGL::Glut_keyboard1);
	glutSpecialFunc(&COpenGL::Glut_special1);

	int window1 = glutCreateWindow("Second Window - Perspective");
	glutPositionWindow(520,20);
	glutDisplayFunc(&COpenGL::Glut_DrawScene2);
	glutReshapeFunc(&COpenGL::GLUT_reshape2);
	glutMouseFunc(&COpenGL::GLUT_mouse2);
	glutKeyboardFunc(&COpenGL::Glut_keyboard2);
	glutSpecialFunc(&COpenGL::Glut_special2);
	glutIdleFunc(spinCube);

	glutMainLoop();
#endif

#if 1
	glutInit(&argc, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize (width, height);
	glutInitWindowPosition (100, 100);
	glutCreateWindow (argv[0]);
	//	init ();
	  glClearColor (0.0, 0.0, 0.0, 0.0);
	  glShadeModel(GL_FLAT);
	  glEnable (GL_LINE_STIPPLE);
	  glMatrixMode(GL_PROJECTION);
 //   glMatrixMode(GL_MODELVIEW);
 //   glMatrixMode(GL_TEXTURE);
	  glLoadIdentity();
//	  glOrtho(-(VIEWPORT_MARGIN*m_fViewportWidth), (VIEWPORT_MARGIN*m_fViewportWidth), -(VIEWPORT_MARGIN*m_fViewportHeight), (VIEWPORT_MARGIN*m_fViewportHeight), -12.0, 12.0);
//	  glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	  glOrtho(m_fViewportOffsetX, m_fViewportWidth, m_fViewportOffsetY, m_fViewportHeight, -12.0, 12.0);

	  //	  glOrtho(-250.0f, 250.0f, -250.0f, 250.0f, -250.0f, 250.0f);
	  // glShadeModel (GL_FLAT);
	//	init ();
	glutDisplayFunc(&COpenGL::Glut_DrawScene);
	glutReshapeFunc(&COpenGL::GLUT_reshape);
	glutMouseFunc(&COpenGL::GLUT_mouse);
	glutMotionFunc(&COpenGL::GLUT_MouseMovement);
//	glutMouseWheelFunc(&COpenGL::GLUT_MouseWheel);
	glutKeyboardFunc(&COpenGL::Glut_keyboard);
	glutSpecialFunc(&COpenGL::Glut_special);
	glutMainLoop();
#endif

	return 1;   /* ANSI C requires main to return int. */
	//return TRUE;
}


void COpenGL::Glut_DrawScene(void)									// Here's Where We Do All The Drawing
{
// JMW ddff 08/27/2014
//	static long lCount = 0;

 	glClear (GL_COLOR_BUFFER_BIT);
//	glColor3f (1.0, 0.0, 1.0);

//	Glut_bitmap_output(1.0f, 4.9f, "TIMES_ROMAN_24 bitmap font.", GLUT_BITMAP_TIMES_ROMAN_24);
//	Glut_bitmap_output(1.0f, 0.7f, "More bitmap text is a fixed 9 by 15 font.", GLUT_BITMAP_9_BY_15);
//	Glut_bitmap_output(1.0f, -4.5f, "Helvetica_18 is yet another bitmap font.", GLUT_BITMAP_HELVETICA_18);

//mmm	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
//mmm	glLoadIdentity();									// Reset The Current Modelview Matrix

//	if (m_fViewportHeight < m_fViewportWidth)
//		glTranslatef(0.0f, 0.0f, -(1.1f*m_fViewportWidth));	// Scale by width
//	else
//		glTranslatef(0.0f, 0.0f, -(1.1f*m_fViewportHeight));	// Scale by height
// 2/13
//	glTranslatef(0.0f, 0.0f, -(25.0f));	// Scale by width

 	// Axis rotations
	if (0 < m_lCountX)
	{
		m_fRotationFactorX += 1.0f;
//		glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);
		m_lCountX--;
	}
	else if (m_lCountX < 0)
	{
		m_fRotationFactorX -= 1.0f;
//		glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);
		m_lCountX++;
	}
//mmm	glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);

	if (0 < m_lCountY)
	{
		m_fRotationFactorY += 1.0f;
//		glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);
		m_lCountY--;
	}
	else if (m_lCountY < 0)
	{
		m_fRotationFactorY -= 1.0f;
//		glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);
		m_lCountY++;
	}
//mmm	glRotatef (m_fRotationFactorY, 0.0f, 1.0f, 0.0f);

	if (0 < m_lCountZ)
	{
		m_fRotationFactorZ += 1.0f;
//		glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);
		m_lCountZ--;
	}
	else if (m_lCountZ < 0)
	{
		m_fRotationFactorZ -= 1.0f;
//		glRotatef (m_fRotationFactorX, 1.0f, 0.0f, 0.0f);
		m_lCountZ++;
	}
//mmm	glRotatef (m_fRotationFactorZ, 0.0f, 0.0f, 1.0f);

#if 0
	// Draw part rectangle
	glColor3f (1.0f, 1.0f, 1.0f);
	glLineWidth(2);
	glPushAttrib(GL_LINE_BIT);
	glLineStipple (1, 0xaaaa);
	glBegin(GL_LINE_STRIP);									// Drawing Using Lines
		glVertex3f(-m_fViewportWidth/2.0f, m_fViewportHeight/2.0f, 0.0f);		// Line #1
		glVertex3f(m_fViewportWidth/2.0f, m_fViewportHeight/2.0f, 0.0f);
		glVertex3f(m_fViewportWidth/2.0f, -m_fViewportHeight/2.0f, 0.0f);
		glVertex3f(-m_fViewportWidth/2.0f, -m_fViewportHeight/2.0f, 0.0f);
		glVertex3f(-m_fViewportWidth/2.0f, m_fViewportHeight/2.0f, 0.0f);
	glEnd();
	glPopAttrib();
#endif

#if 0
	// Draw center line cursor
	glColor3f (1.0f, 0.0f, 1.0f);
	glLineWidth(2);
	glLineStipple (2, 0xaaaa);
	glBegin(GL_LINE_STRIP);									// Drawing Using Lines
		glVertex3f(m_fViewportOffsetX + m_fViewportWidth/2.0f - 2.0f, m_fViewportOffsetY + m_fViewportHeight/2.0f, 0.0f);		// Line #1
		glVertex3f(m_fViewportOffsetX + m_fViewportWidth/2.0f + 2.0f, m_fViewportOffsetY + m_fViewportHeight/2.0f, 0.0f);		// Line #1
	glEnd();

	glBegin(GL_LINE_STRIP);									// Drawing Using Lines
		glVertex3f(m_fViewportOffsetX + m_fViewportWidth/2.0f, m_fViewportOffsetY + m_fViewportHeight/2.0f - 2.0f, 0.0f);		// Line #1
		glVertex3f(m_fViewportOffsetX + m_fViewportWidth/2.0f, m_fViewportOffsetY + m_fViewportHeight/2.0f + 2.0f, 0.0f);		// Line #1
	glEnd();
#endif


	// Shift over by the offset of the part
//	glTranslatef(-(m_fViewportOffsetX+m_fViewportWidth/2.0f), -(m_fViewportOffsetY+m_fViewportHeight/2.0f), 0.0f);		// Move Left 1/2 withd, down 1/2 height
//mmm	glTranslatef(-(m_fViewportWidth/2.0f) + m_fViewportOffsetX, -(m_fViewportHeight/2.0f) + m_fViewportOffsetY, 0.0f);		// Move Left 1/2 withd, down 1/2 height
//mmm	glScalef(m_fScaleFactor, m_fScaleFactor, m_fScaleFactor);

 //	CONNECTED_SEGMENT	tSegment;
	OBJ_ARC				Arc;
	float				fXa, fYa, fZa, fXb, fYb, fZb;
//	int					nCurrentSortedSegmentCount;

 	if (true && m_pSaSortedSegments)
	{
		glColor3f (1.0f, 0.0f, 0.0f);
		glLineWidth(1);

		OPENGL_ELEMENT tToolPathSegment;

		int nMaxIndex = m_pSaSortedSegments->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tToolPathSegment = m_pSaSortedSegments->SVC_GetElement(n);

		//	glPushAttrib(GL_LINE_BIT);
			if (tToolPathSegment.eLinePattern == eShortDash)
			{
				glEnable (GL_LINE_STIPPLE);
				glLineStipple (1, 0xaaaa);
			}
			else if (tToolPathSegment.eLinePattern == eLongDash)
			{
				glEnable (GL_LINE_STIPPLE);
				glLineStipple (1, 0xcccc);
			}
			else
				glDisable (GL_LINE_STIPPLE);

			if (tToolPathSegment.eSegmentType == eLine)
			{
				fXa = tToolPathSegment.fAx;
				fYa = tToolPathSegment.fAy;
				fZa = tToolPathSegment.fAz;
				fXb = tToolPathSegment.fBx;
				fYb = tToolPathSegment.fBy;
				fZb = tToolPathSegment.fBz;
				glBegin(GL_LINES);									// Drawing Using Lines
					glVertex3f(fXa, fYa, fZa);
					glVertex3f(fXb, fYb, fZb);
 				glEnd();
			}
			else if (tToolPathSegment.eSegmentType == eCircle)
			{
 				Arc.fXc = tToolPathSegment.fRx;
				Arc.fYc = tToolPathSegment.fRy;
				Arc.fRadius = tToolPathSegment.fRadius;
				Arc.fStartAngle = 0.0;
				Arc.fEndAngle = 360.0;
			//	DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 40, 1.0f, 0.0f, 0.0f);
				DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 0.0f, 0.0f);
			}
			else if (tToolPathSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tToolPathSegment.fRx;
				Arc.fYc = tToolPathSegment.fRy;
				Arc.fRadius = tToolPathSegment.fRadius;
				Arc.fStartAngle = tToolPathSegment.fStartAngle;
				Arc.fEndAngle = tToolPathSegment.fEndAngle;
		//		DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 20, 1.0f, 0.0f, 0.0f);
				DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 0.0f, 0.0f);
			}
	//		glPopAttrib();
		}
	}

	if (true && m_pSaLoop1Segments)
	{
		glColor3f (1.0f, 1.0f, 1.0f);
		glLineWidth(1);

		OPENGL_ELEMENT tToolPathSegment;

		int nMaxIndex = m_pSaLoop1Segments->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tToolPathSegment = m_pSaLoop1Segments->SVC_GetElement(n);
	//		glPushAttrib(GL_LINE_BIT);
			if (tToolPathSegment.eLinePattern == eShortDash)
			{
				glEnable (GL_LINE_STIPPLE);
				glLineStipple (1, 0xaaaa);
			}
			else if (tToolPathSegment.eLinePattern == eLongDash)
			{
				glEnable (GL_LINE_STIPPLE);
				glLineStipple (1, 0xcccc);
			}
			else
				glDisable (GL_LINE_STIPPLE);

			if (tToolPathSegment.eSegmentType == eLine)
			{
				fXa = tToolPathSegment.fAx;
				fYa = tToolPathSegment.fAy;
				fZa = tToolPathSegment.fAz;
				fXb = tToolPathSegment.fBx;
				fYb = tToolPathSegment.fBy;
				fZb = tToolPathSegment.fBz;
				glBegin(GL_LINES);									// Drawing Using Lines
					glVertex3f(fXa, fYa, fZa);
					glVertex3f(fXb, fYb, fZb);
 				glEnd();

			}
			else if (tToolPathSegment.eSegmentType == eCircle)
			{
 				Arc.fXc = tToolPathSegment.fRx;
				Arc.fYc = tToolPathSegment.fRy;
				Arc.fRadius = tToolPathSegment.fRadius;
				Arc.fStartAngle = 0.0;
				Arc.fEndAngle = 360.0;
			//	DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 40, 1.0f, 1.0f, 1.0f);
				DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 1.0f, 1.0f);
			}
			else if (tToolPathSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tToolPathSegment.fRx;
				Arc.fYc = tToolPathSegment.fRy;
				Arc.fRadius = tToolPathSegment.fRadius;
				Arc.fStartAngle = tToolPathSegment.fStartAngle;
				Arc.fEndAngle = tToolPathSegment.fEndAngle;
			//	DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 10, 1.0f, 1.0f, 1.0f);
				DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 1.0f, 1.0f);
			}
	//		glPopAttrib();
		}
	}

	if (true && m_pSaLoop2Segments)
	{
		glColor3f (1.0f, 1.0f, 1.0f);
		glLineWidth(1);

		OPENGL_ELEMENT tToolPathSegment;

		int nMaxIndex = m_pSaLoop2Segments->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tToolPathSegment = m_pSaLoop2Segments->SVC_GetElement(n);

			if (tToolPathSegment.eLinePattern == eShortDash)
			{
				glEnable (GL_LINE_STIPPLE);
				glLineStipple (1, 0xaaaa);
			}
			else if (tToolPathSegment.eLinePattern == eLongDash)
			{
				glEnable (GL_LINE_STIPPLE);
				glLineStipple (1, 0xcccc);
			}
			else
				glDisable (GL_LINE_STIPPLE);

			if (tToolPathSegment.eSegmentType == eLine)
			{
				fXa = tToolPathSegment.fAx;
				fYa = tToolPathSegment.fAy;
				fZa = tToolPathSegment.fAz;
				fXb = tToolPathSegment.fBx;
				fYb = tToolPathSegment.fBy;
				fZb = tToolPathSegment.fBz;
				glBegin(GL_LINES);									// Drawing Using Lines
					glVertex3f(fXa, fYa, fZa);
					glVertex3f(fXb, fYb, fZb);
 				glEnd();

			}
			else if (tToolPathSegment.eSegmentType == eCircle)
			{
				// CIRCLE_MODS
 				Arc.fXc = tToolPathSegment.fRx;
				Arc.fYc = tToolPathSegment.fRy;
				Arc.fRadius = tToolPathSegment.fRadius;
				Arc.fStartAngle = 0.0;
				Arc.fEndAngle = 360.0;
		//		DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 40, 1.0f, 1.0f, 1.0f);
				DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 1.0f, 1.0f);
			}
			else if (tToolPathSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tToolPathSegment.fRx;
				Arc.fYc = tToolPathSegment.fRy;
				Arc.fRadius = tToolPathSegment.fRadius;
				Arc.fStartAngle = tToolPathSegment.fStartAngle;
				Arc.fEndAngle = tToolPathSegment.fEndAngle;
			//	DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 10, 1.0f, 1.0f, 1.0f);
				DrawArc (0.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 1.0f, 1.0f);
			}
		}
	}

	glFlush();
	return;

	// Draw block of material if available
	if (m_pSaMaterialBlockSegments)
	{
//		glColor3f (0.0f, 1.0f, 0.0f);
//		glLineWidth(1);
//		glBegin(GL_QUADS);									// Drawing Using Lines
//			glVertex3f(0.0, 0.0, -1.0);
//			glVertex3f(5.0, 0.0, -1.0);
//			glVertex3f(5.0, 5.0, -1.0);
//			glVertex3f(0.0, 5.0, -1.0);
//		glEnd();

		glColor3f (1.0f, 0.0f, 0.0f);
		glLineWidth(4);

		OPENGL_ELEMENT tBlockSegment;

		int nMaxIndex = m_pSaMaterialBlockSegments->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tBlockSegment = m_pSaMaterialBlockSegments->SVC_GetElement(n);
			if (tBlockSegment.eSegmentType == eLine)
			{
				glPushAttrib(GL_LINE_BIT);
				if (tBlockSegment.eLinePattern == eShortDash)
					glLineStipple (2, 0xaaaa);
				else if (tBlockSegment.eLinePattern == eLongDash)
					glLineStipple (2, 0xcccc);

				fXa = tBlockSegment.fAx;
				fYa = tBlockSegment.fAy;
				fZa = tBlockSegment.fAz;
				fXb = tBlockSegment.fBx;
				fYb = tBlockSegment.fBy;
				fZb = tBlockSegment.fBz;
				glBegin(GL_LINES);									// Drawing Using Lines
					glVertex3f(fXa, fYa, fZa);
					glVertex3f(fXb, fYb, fZb);
 				glEnd();
				glPopAttrib();
			}
			else if (tBlockSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tBlockSegment.fRx;
				Arc.fYc = tBlockSegment.fRy;
				Arc.fRadius = tBlockSegment.fRadius;
				Arc.fStartAngle = tBlockSegment.fStartAngle;
				Arc.fEndAngle = tBlockSegment.fEndAngle;
			//	DrawArc (12.0, &Arc, Arc.fXc, Arc.fYc, 10, 1.0f, 1.0f, 0.0f);
				DrawArc (12.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 1.0f, 0.0f);
			}
		}
	}

	if (m_pSaMaterialBlockSegments)
	{
//		glColor3f (0.0f, 1.0f, 0.0f);
//		glLineWidth(1);
//		glBegin(GL_QUADS);									// Drawing Using Lines
//			glVertex3f(0.0, 0.0, -1.0);
//			glVertex3f(5.0, 0.0, -1.0);
//			glVertex3f(5.0, 5.0, -1.0);
//			glVertex3f(0.0, 5.0, -1.0);
//		glEnd();

		glColor3f (1.0f, 0.0f, 0.0f);
		glLineWidth(4);

		OPENGL_ELEMENT tBlockSegment;

		int nMaxIndex = m_pSaMaterialBlockSegments->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tBlockSegment = m_pSaMaterialBlockSegments->SVC_GetElement(n);
			if (tBlockSegment.eSegmentType == eLine)
			{
				glPushAttrib(GL_LINE_BIT);
				if (tBlockSegment.eLinePattern == eShortDash)
					glLineStipple (2, 0xaaaa);
				else if (tBlockSegment.eLinePattern == eLongDash)
					glLineStipple (2, 0xcccc);

				fXa = tBlockSegment.fAx;
				fYa = tBlockSegment.fAy;
				fZa = tBlockSegment.fAz;
				fXb = tBlockSegment.fBx;
				fYb = tBlockSegment.fBy;
				fZb = tBlockSegment.fBz;
				glBegin(GL_LINES);									// Drawing Using Lines
					glVertex3f(fXa, fYa, fZa);
					glVertex3f(fXb, fYb, fZb);
 				glEnd();
				glPopAttrib();
			}
			else if (tBlockSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tBlockSegment.fRx;
				Arc.fYc = tBlockSegment.fRy;
				Arc.fRadius = tBlockSegment.fRadius;
				Arc.fStartAngle = tBlockSegment.fStartAngle;
				Arc.fEndAngle = tBlockSegment.fEndAngle;
		//		DrawArc (12.0, &Arc, Arc.fXc, Arc.fYc, 10, 1.0f, 1.0f, 0.0f);
				DrawArc (12.0, &Arc, Arc.fXc, Arc.fYc, 1.0f, 1.0f, 0.0f);
			}
		}
	}

	if (m_pSaGCodeSegments)
	{
		glColor3f (0.0f, 1.0f, 1.0f);

		glLineWidth(2);
	//	glPushAttrib(GL_LINE_BIT);
	//	glLineStipple (2, 0xaaaa);

		OPENGL_ELEMENT tGCodeSegment;

		int nMaxIndex = m_pSaGCodeSegments->SVC_Size();
		for (int n=0; n<=nMaxIndex; n++)
		{
			tGCodeSegment = m_pSaGCodeSegments->SVC_GetElement(n);
			if (tGCodeSegment.eSegmentType == eLine)
			{
				glPushAttrib(GL_LINE_BIT);
				if (tGCodeSegment.eLinePattern == eShortDash)
					glLineStipple (2, 0xaaaa);
				else if (tGCodeSegment.eLinePattern == eLongDash)
					glLineStipple (2, 0xcccc);

				fXa = tGCodeSegment.fAx;
				fYa = tGCodeSegment.fAy;
				fZa = tGCodeSegment.fAz;
				fXb = tGCodeSegment.fBx;
				fYb = tGCodeSegment.fBy;
				fZb = tGCodeSegment.fBz;

			//	if (tGCodeSegment.eLinePattern == eSolid)
			//	{
					glLineWidth(tGCodeSegment.fLineWidth);
					glBegin(GL_LINES);									// Drawing Using Lines
						glVertex3f(fXa, fYa, fZa);
						glVertex3f(fXb, fYb, fZb);
 					glEnd();
			//	}
				glPopAttrib();
			}
			else if (tGCodeSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tGCodeSegment.fRx;
				Arc.fYc = tGCodeSegment.fRy;
				Arc.fRadius = tGCodeSegment.fRadius;
				Arc.fStartAngle = tGCodeSegment.fStartAngle;
				Arc.fEndAngle = tGCodeSegment.fEndAngle;
				Arc.fZ = tGCodeSegment.fBz;
 				if (tGCodeSegment.eLinePattern == eShortDash)
					glLineStipple (2, 0xaaaa);
				else if (tGCodeSegment.eLinePattern == eLongDash)
					glLineStipple (2, 0xcccc);

				DrawArc2D (&Arc, Arc.fXc, Arc.fYc, Arc.fZ, 16, 0.0f, 1.0f, 1.0f);
//				DrawArc (12.0, &Arc, Arc.fXc, Arc.fYc, 10, 0.0f, 1.0f, 1.0f);
			}
		}
//		glPopAttrib();
	}


	if (m_pSaGCodeSegments)
	{
		glColor3f (0.0f, 1.0f, 1.0f);

		glLineWidth(2);
	//	glPushAttrib(GL_LINE_BIT);
	//	glLineStipple (2, 0xaaaa);

		OPENGL_ELEMENT tGCodeSegment;

		int nMaxIndex = m_pSaGCodeSegments->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tGCodeSegment = m_pSaGCodeSegments->SVC_GetElement(n);
			if (tGCodeSegment.eSegmentType == eLine)
			{
				glPushAttrib(GL_LINE_BIT);
				if (tGCodeSegment.eLinePattern == eShortDash)
					glLineStipple (2, 0xaaaa);
				else if (tGCodeSegment.eLinePattern == eLongDash)
					glLineStipple (2, 0xcccc);

				fXa = tGCodeSegment.fAx;
				fYa = tGCodeSegment.fAy;
				fZa = tGCodeSegment.fAz;
				fXb = tGCodeSegment.fBx;
				fYb = tGCodeSegment.fBy;
				fZb = tGCodeSegment.fBz;

			//	if (tGCodeSegment.eLinePattern == eSolid)
			//	{
					glLineWidth(tGCodeSegment.fLineWidth);
					glBegin(GL_LINES);									// Drawing Using Lines
						glVertex3f(fXa, fYa, fZa);
						glVertex3f(fXb, fYb, fZb);
 					glEnd();
			//	}
				glPopAttrib();
			}
			else if (tGCodeSegment.eSegmentType == eArc)
			{
 				Arc.fXc = tGCodeSegment.fRx;
				Arc.fYc = tGCodeSegment.fRy;
				Arc.fRadius = tGCodeSegment.fRadius;
				Arc.fStartAngle = tGCodeSegment.fStartAngle;
				Arc.fEndAngle = tGCodeSegment.fEndAngle;
				Arc.fZ = tGCodeSegment.fBz;
 				if (tGCodeSegment.eLinePattern == eShortDash)
					glLineStipple (2, 0xaaaa);
				else if (tGCodeSegment.eLinePattern == eLongDash)
					glLineStipple (2, 0xcccc);

				DrawArc2D (&Arc, Arc.fXc, Arc.fYc, Arc.fZ, 16, 0.0f, 1.0f, 1.0f);
//				DrawArc (12.0, &Arc, Arc.fXc, Arc.fYc, 10, 0.0f, 1.0f, 1.0f);
			}
		}
//		glPopAttrib();
	}

//	glRasterPos2f (5.0f, 5.0f);


// TEXT HERE	OpenGlPrintOutline(5.0f, 5.0f, -16.0f, "Line #1 @ 5,5");	// Print GL ] To The Screen

	//	glRasterPos2f (1.0f, 1.0f);
//	OpenGlPrint2("Line #1 @ 1,1");	// Print GL Text To The Screen
//	glRasterPos2f (2.0f, 2.0f);
 //	OpenGlPrint2("Line #3 @ 2,2");	// Print GL Text To The Screen
//	glRasterPos2f (3.0f, 3.0f);
 //	OpenGlPrint2("Line #4 @ 3,3");	// Print GL Text To The Screen
//	glRasterPos2f (4.0f, 4.0f);
 //	OpenGlPrint2("Line #5 @ 4,4");	// Print GL Text To The Screen
//	glRasterPos2f (5.0f, 5.0f);
 //	OpenGlPrint2("Line #6 @ 5,5");	// Print GL Text To The Screen

 	return;										// Keep Going
}

void COpenGL::GLUT_reshape(int w, int h)
{
// JMW ddff 08/27/2014
//	static bool bInitialReshape = true;
//	double dAspectRatio;
	double dLeft, dRight, dTop, dBottom;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
//   glOrtho(-250.0, 250.0, -250.0, 250.0, -250.0, 250.0);
//   glOrtho(-(VIEWPORT_MARGIN*m_fViewportWidth), (VIEWPORT_MARGIN*m_fViewportWidth), -(VIEWPORT_MARGIN*m_fViewportHeight), (VIEWPORT_MARGIN*m_fViewportHeight), -12.0, 12.0);
 // glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);

	m_fViewPortAspectRatio = (GLfloat)h / (GLfloat)w;
//	if (bInitialReshape)
//	{
		if (1.0 < m_fPartAspectRatio) 	// Wider than high
		{
			m_fViewportWidth = m_fViewportWidth;
			m_fViewportHeight = m_fViewportWidth * m_fViewPortAspectRatio;
		}
		else
		{
			m_fViewportHeight = m_fViewportHeight;
			m_fViewportWidth = m_fViewportHeight / m_fViewPortAspectRatio;
		}
// JMW ddff 08/27/2014
//		bInitialReshape = false;
//	}
//	else
//	{
//		m_fViewportWidth = m_fViewportWidth;
//		m_fViewportHeight = m_fViewportWidth * m_fViewPortAspectRatio;
//	}

	dLeft = m_fViewportOffsetX;
	dRight = m_fViewportOffsetX + m_fViewportWidth;
	dTop = m_fViewportOffsetY;
	dBottom = m_fViewportOffsetY + m_fViewportHeight;
//	gluOrtho2D(dLeft, dRight, dTop, dBottom);
	glOrtho(dLeft, dRight, dTop, dBottom, -12.0, 12.0);

//	glOrtho(m_fViewportOffsetX, m_fViewportWidth, m_fViewportOffsetY, m_fViewportHeight, -12.0, 12.0);

// zzz	glMatrixMode(GL_MODELVIEW);
// zzz	glLoadIdentity();
}
void COpenGL::GLUT_MouseMovement(int x , int y)
{
//   cout << "GLUT_MouseMovement() - X = " <<  x << "  Y = " << y << std::endl;

	float fPercentMouseX = (float)x / (float)SUB_WIND_WIDTH;
	float fPercentMouseY = 1.0f - ((float)y / (float)SUB_WIND_HEIGHT);
	float fMouseUpGlbX = m_fViewportOffsetX + fPercentMouseX*m_fViewportWidth;
	float fMouseUpGlbY = m_fViewportOffsetY + fPercentMouseY*m_fViewportHeight;

	float fDeltaX = fMouseUpGlbX - fMouseDownGlbX;
  	float fDeltaY = -(fMouseUpGlbY - fMouseDownGlbY);

 //	 cout << "GLUT_Mouse(UP1) - button = " << button  << " state = " << state << " <<  " delta Y: " << nDeltaY << std::endl;
 //  cout << "GLUT_Mouse(UP2) - button = " << button  << " state = " << state << " delta fX: " << fDeltaX <<  " delta fY: " << fDeltaY << std::endl;

  	GLUT_RePosition (fDeltaX, fDeltaY);
}

void COpenGL::GLUT_mouse(int button, int state, int x, int y)
{
	float fPercentMouseX;
	float fPercentMouseY;

//	cout << "GLUT_Mouse() - button = " << button  << " state = " << state << " X: " << x << std::endl;
	switch (button)
	{
      case GLUT_LEFT_BUTTON:
    	  if (state == 0)
    	  {
    		  fPercentMouseX = (float)x / (float)SUB_WIND_WIDTH;
    		  fPercentMouseY = 1.0f - ((float)y / (float)SUB_WIND_HEIGHT);
    		  nMouseDownX = x;
    		  nMouseDownY = y;
    		  fMouseDownGlbX = m_fViewportOffsetX + fPercentMouseX*m_fViewportWidth;
    		  fMouseDownGlbY = m_fViewportOffsetY + fPercentMouseY*m_fViewportHeight;
  //  		  cout << "GLUT_Mouse(DN) - GlbX: " <<  fMouseDownGlbX << " GlbY: " << fMouseDownGlbY << std::endl;
     	  }
    	  else if (state ==1)
    	  {
       		  fPercentMouseX = (float)x / (float)SUB_WIND_WIDTH;
    		  fPercentMouseY = 1.0f - ((float)y / (float)SUB_WIND_HEIGHT);
    		  float fMouseUpGlbX = m_fViewportOffsetX + fPercentMouseX*m_fViewportWidth;
    		  float fMouseUpGlbY = m_fViewportOffsetY + fPercentMouseY*m_fViewportHeight;

    		  float fDeltaX = fMouseUpGlbX - fMouseDownGlbX;
    		  float fDeltaY = -(fMouseUpGlbY - fMouseDownGlbY);

    	//	  cout << "GLUT_Mouse(UP1) - button = " << button  << " state = " << state << " <<  " delta Y: " << nDeltaY << std::endl;
   //    	cout << "GLUT_Mouse(UP2) - button = " << button  << " state = " << state << " delta fX: " << fDeltaX <<  " delta fY: " << fDeltaY << std::endl;

       		GLUT_RePosition (fDeltaX, fDeltaY);

    	  }
  //       if (state == GLUT_DOWN)
  //          glutIdleFunc(GLUT_spinDisplay);
         break;
      case GLUT_MIDDLE_BUTTON:
      	  cout << "WH U/D"  << std::endl;
          if (state == GLUT_DOWN)
          {
        	  cout << "WH Down"  << std::endl;
          }
          break;
      case GLUT_RIGHT_BUTTON:
    	  cout << "RB down"  << std::endl;
  //       if (state == GLUT_DOWN)
  //          glutIdleFunc(NULL);
         break;
      case 3:		// Wheel scroll up
    	  if (state == 1)
    	  {
    		  GLUT_ZoomIn(x, y);
     	  }
          break;
      case 4:		// Wheel scroll down
    	  if (state == 1)
    	  {
    		  GLUT_ZoomOut(x, y);
    	  }
          break;
      default:
         break;
   }
}
#if 0
void COpenGL::GLUT_MouseWheel(int wheel, int dir, int x, int y)
{
	cout << "GLUT_MouseWheel() - W = " << wheel  << std::endl;
	switch (wheel)
	{
		case GLUT_LEFT_BUTTON:
			cout << "LB down"  << std::endl;
	  //       if (state == GLUT_DOWN)
	  //          glutIdleFunc(GLUT_spinDisplay);
			break;
		case GLUT_MIDDLE_BUTTON:
			cout << "WH U/D"  << std::endl;
			if (dir == GLUT_DOWN)
			{
				cout << "WH Down"  << std::endl;
			}
			break;
		case GLUT_RIGHT_BUTTON:
			cout << "RB down"  << std::endl;
	  //       if (state == GLUT_DOWN)
	  //          glutIdleFunc(NULL);
			break;
		default:
			break;
	}
}
#endif
void COpenGL::Glut_keyboard(unsigned char key, int x, int y)
{
//	static int bRunMode;
// JMW ddff 08/27/2014
//	static long lCount;
//	char	chReport[200];
	GLfloat	dMidViewPortX, dMidViewPortY;
//	double dLeft, dRight, dTop, dBottom;

	bool bRedrawNeeded = true;

	switch (key)
	{
    	case 'l':
			m_lCountX++;
			m_fViewportOffsetX += 0.05f;
//			TRACE ("RIGHT key hit detected (C: %ld, A:%f)\n", m_lCountY, m_fRotationFactorY);
			sprintf (chReport_OGL, "l-key hit detected (C: %ld, A:%f)", m_lCountY, m_fViewportOffsetX);
			ReportBasedOnVerbosity (2, chReport_OGL);
			bRedrawNeeded = true;
			break;
		case 'r':
			m_lCountX--;
			m_fViewportOffsetX -= 0.05f;
		//	TRACE ("r-key hit detected (C: %ld, A:%f)\n", m_lCountY, m_fViewportOffsetX);
			sprintf (chReport_OGL, "r-key hit detected (C: %ld, A:%f)", m_lCountY, m_fViewportOffsetX);
			ReportBasedOnVerbosity (2, chReport_OGL);
			bRedrawNeeded = true;
			break;
		case 'u':
			m_lCountY--;
			m_fViewportOffsetY -= 0.05f;
			sprintf (chReport_OGL, "u-key hit detected (C: %ld, A:%f)", m_lCountY, m_fViewportOffsetX);
			ReportBasedOnVerbosity (2, chReport_OGL);
	//		TRACE ("u-key hit detected (C: %ld, A:%f)\n", m_lCountY, m_fViewportOffsetX);
			bRedrawNeeded = true;
			break;
		case 'd':
			m_lCountY++;
			m_fViewportOffsetY += 0.05f;
//			TRACE ("d-key hit detected (C: %ld, A:%f)\n", m_lCountY, m_fViewportOffsetX);
			sprintf (chReport_OGL, "d-key hit detected (C: %ld, A:%f)", m_lCountY, m_fViewportOffsetX);
			ReportBasedOnVerbosity (2, chReport_OGL);
			bRedrawNeeded = true;
			break;
   		case '+':
			dMidViewPortX = m_fViewportOffsetX + m_fViewportWidth / 2.0f;
			dMidViewPortY = m_fViewportOffsetY + m_fViewportHeight / 2.0f;

			m_fViewportWidth /= 2.0;
			m_fViewportHeight = m_fViewportWidth * m_fViewPortAspectRatio;

			m_fViewportOffsetX = dMidViewPortX - m_fViewportWidth / 2.0f;
			m_fViewportOffsetY = dMidViewPortY - m_fViewportHeight / 2.0f;

	//	 	dLeft = m_fViewportOffsetX;
	//		dRight = m_fViewportOffsetX + m_fViewportWidth;
	//		dTop = m_fViewportOffsetY;
	//		dBottom = m_fViewportOffsetY + m_fViewportHeight;
	//		glOrtho(dLeft, dRight, dTop, dBottom, -12.0, 12.0);

//			m_fViewportWidth /= 2.0f;
//			m_fViewportHeight /= 2.0f;
	//		TRACE ("+-key hit detected - Center(%f,%f)\n", dMidViewPortX, dMidViewPortY);
			sprintf (chReport_OGL, "+-key hit detected - Center(%f,%f)", dMidViewPortX, dMidViewPortY);
			ReportBasedOnVerbosity (2, chReport_OGL);
			bRedrawNeeded = true;
			break;

		case '-':
			dMidViewPortX = m_fViewportOffsetX + m_fViewportWidth / 2.0f;
			dMidViewPortY = m_fViewportOffsetY + m_fViewportHeight / 2.0f;

			m_fViewportWidth *= 2.0;
			m_fViewportHeight = m_fViewportWidth * m_fViewPortAspectRatio;

			m_fViewportOffsetX = dMidViewPortX - m_fViewportWidth / 2.0f;
			m_fViewportOffsetY = dMidViewPortY - m_fViewportHeight / 2.0f;

//			m_fViewportWidth *= 2.0f;
//			m_fViewportHeight *= 2.0f;
//			TRACE ("--key hit detected - Center(%f,%f)\n", dMidViewPortX, dMidViewPortY);
			sprintf (chReport_OGL, "--key hit detected - Center(%f,%f)", dMidViewPortX, dMidViewPortY);
			ReportBasedOnVerbosity (2, chReport_OGL);
			bRedrawNeeded = true;
			break;

		case 27:	// Escape key
			bRedrawNeeded = false;
	//		glutExit();
	//		glutLeaveMainLoop();
	//		exit(0);
			break;
	};

//	glutPostRedisplay();
	if (bRedrawNeeded)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
		glMatrixMode(GL_MODELVIEW);
	//	glLoadIdentity();

	    glutPostRedisplay();
	}
}

void COpenGL::Glut_special(int key, int x, int y)
{
  	const char *name;	/* JMW DDFF 08/26/2014 */

  	switch (key) 
	{
 		case GLUT_KEY_F1:
    			name = "F1";
    			break;
  		case GLUT_KEY_F2:
    			name = "F2";
   			 break;
		case GLUT_KEY_F3:
		    	name = "F3";
		    	break;
		case GLUT_KEY_F4:
			name = "F4";
			break;
		case GLUT_KEY_F5:
			name = "F5";
			break;
		case GLUT_KEY_F6:
			name = "F6";
			break;
		case GLUT_KEY_F7:
			name = "F7";
			break;
		case GLUT_KEY_F8:
			name = "F8";
			break;
		case GLUT_KEY_F9:
			name = "F9";
			break;
		case GLUT_KEY_F10:
			name = "F11";
			break;
		case GLUT_KEY_F11:
			name = "F12";
			break;
		case GLUT_KEY_LEFT:
			name = "Left";
			GLUT_ScrollLeft(0, 0);
			break;
		case GLUT_KEY_UP:
			name = "Up";
			GLUT_ScrollDown(0, 0);
			break;
		case GLUT_KEY_RIGHT:
			name = "Right";
			GLUT_ScrollRight(0, 0);
			break;
		case GLUT_KEY_DOWN:
			name = "Down";
			GLUT_ScrollUp(0, 0);
			break;
		case GLUT_KEY_PAGE_UP:
			name = "Page up";
			break;
		case GLUT_KEY_PAGE_DOWN:
			name = "Page down";
			break;
		case GLUT_KEY_HOME:
			name = "Home";
			GLUT_ZoomIn(-1, -1);
			break;
		case GLUT_KEY_END:
			name = "End";
			GLUT_ZoomOut(-1, -1);
			break;
		case GLUT_KEY_INSERT:
			name = "Insert";
			break;
		default:
			name = "UNKNOWN";
			break;
	}
// JMW ddff 08/27/2014 - supress compiler warning
	name = name; 

		 // TRACE("special: %s %d,%d\n", name, x, y);
}

void COpenGL::DrawArc (float fScale, OBJ_ARC *pArc, int nSegments, float cRed, float cGreen, float cBlue)
{
	glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(0.0, 0.0, fScale);						// Center, 6 units out of screen
	glColor3f (cRed, cGreen, cBlue);

//	double dSegments = (double)nSegments;

	double dSweepAngleRads = pArc->fEndAngle - pArc->fStartAngle;
	if (dSweepAngleRads < 0.0)
		dSweepAngleRads += 360.0;
	dSweepAngleRads *= M_PI / 180.0;

	double dStartAngleRads = pArc->fStartAngle * M_PI / 180.0;
	double dCurAngleRads;
	float fFirstVerticeX = (float)(pArc->fRadius * cos (dStartAngleRads));
	float fFirstVerticeY = (float)(pArc->fRadius * sin (dStartAngleRads));
 	float fNextVerticeX;
	float fNextVerticeY;

	glBegin(GL_LINE_STRIP);		// Drawing Using Lines
		glVertex3f (fFirstVerticeX, fFirstVerticeY, 0.0f);
		for (int k=1; k<=nSegments; k++)
		{
			dCurAngleRads = dStartAngleRads + (dSweepAngleRads * ((double)k / (double)nSegments));
			fNextVerticeX = (float)(pArc->fRadius * cos (dCurAngleRads));
			fNextVerticeY = (float)(pArc->fRadius * sin (dCurAngleRads));
			glVertex3f (fNextVerticeX, fNextVerticeY, 0.0f);
//			fLastVerticeX = fNextVerticeX;
//			fLastVerticeY = fNextVerticeY;
		}
 	glEnd();
}

void COpenGL::DrawArc (float fScale, OBJ_ARC *pArc, float fOffsetUnitsX, float fOffsetUnitsY, float cRed, float cGreen, float cBlue)
{
//	glLoadIdentity();									// Reset The Current Modelview Matrix
//	glTranslatef(0.0, 0.0, fScale);						// Center, 6 units out of screen
	glColor3f (cRed, cGreen, cBlue);

	double dSweepAngleDegs = pArc->fEndAngle - pArc->fStartAngle;
	if (dSweepAngleDegs < 0.0)
		dSweepAngleDegs += 360.0;

	int nSegments = (int)(dSweepAngleDegs / 4.5);

	double dSweepAngleRads = dSweepAngleDegs * M_PI / 180.0;

	double dStartAngleRads = pArc->fStartAngle * M_PI / 180.0;
	double dCurAngleRads;
	float fFirstVerticeX = (float)(pArc->fRadius * cos (dStartAngleRads));
	float fFirstVerticeY = (float)(pArc->fRadius * sin (dStartAngleRads));
 	float fNextVerticeX;
	float fNextVerticeY;

	glBegin(GL_LINE_STRIP);		// Drawing Using Lines
	glVertex3f (fOffsetUnitsX+fFirstVerticeX, fOffsetUnitsY+fFirstVerticeY, 0.0f);
	for (int k=1; k<=nSegments; k++)
	{
		dCurAngleRads = dStartAngleRads + (dSweepAngleRads * ((double)k / (double)nSegments));
		fNextVerticeX = (float)(pArc->fRadius * cos (dCurAngleRads));
		fNextVerticeY = (float)(pArc->fRadius * sin (dCurAngleRads));
		glVertex3f (fOffsetUnitsX+fNextVerticeX, fNextVerticeY+fOffsetUnitsY, 0.0f);
	}
	glEnd();
 }

void COpenGL::DrawArc (float fScale, OBJ_ARC *pArc, float fOffsetUnitsX, float fOffsetUnitsY, int nSegments, float cRed, float cGreen, float cBlue)
{
//	glLoadIdentity();									// Reset The Current Modelview Matrix
//	glTranslatef(0.0, 0.0, fScale);						// Center, 6 units out of screen
	glColor3f (cRed, cGreen, cBlue);

	double dSweepAngleRads = pArc->fEndAngle - pArc->fStartAngle;
	if (dSweepAngleRads < 0.0)
		dSweepAngleRads += 360.0;
	dSweepAngleRads *= M_PI / 180.0;

	double dStartAngleRads = pArc->fStartAngle * M_PI / 180.0;
	double dCurAngleRads;
	float fFirstVerticeX = (float)(pArc->fRadius * cos (dStartAngleRads));
	float fFirstVerticeY = (float)(pArc->fRadius * sin (dStartAngleRads));
 	float fNextVerticeX;
	float fNextVerticeY;

	glBegin(GL_LINE_STRIP);		// Drawing Using Lines
	glVertex3f (fOffsetUnitsX+fFirstVerticeX, fOffsetUnitsY+fFirstVerticeY, 0.0f);
	for (int k=1; k<=nSegments; k++)
	{
		dCurAngleRads = dStartAngleRads + (dSweepAngleRads * ((double)k / (double)nSegments));
		fNextVerticeX = (float)(pArc->fRadius * cos (dCurAngleRads));
		fNextVerticeY = (float)(pArc->fRadius * sin (dCurAngleRads));
		glVertex3f (fOffsetUnitsX+fNextVerticeX, fNextVerticeY+fOffsetUnitsY, 0.0f);
	}
	glEnd();
 }

void COpenGL::DrawArc2D (OBJ_ARC *pArc, float fOffsetUnitsX, float fOffsetUnitsY, float fZ, int nSegments, float cRed, float cGreen, float cBlue)
{
 	glColor3f (cRed, cGreen, cBlue);

	double dSweepAngleRads = pArc->fEndAngle - pArc->fStartAngle;
	if (dSweepAngleRads < 0.0)
		dSweepAngleRads += 360.0;
	dSweepAngleRads *= M_PI / 180.0;

	double dStartAngleRads = pArc->fStartAngle * M_PI / 180.0;
	double dCurAngleRads;
	float fFirstVerticeX = (float)(pArc->fRadius * cos (dStartAngleRads));
	float fFirstVerticeY = (float)(pArc->fRadius * sin (dStartAngleRads));
 	float fNextVerticeX;
	float fNextVerticeY;

	glBegin(GL_LINE_STRIP);		// Drawing Using Lines
		glVertex3f (fOffsetUnitsX+fFirstVerticeX, fOffsetUnitsY+fFirstVerticeY, fZ);
		for (int k=1; k<=nSegments; k++)
		{
			dCurAngleRads = dStartAngleRads + (dSweepAngleRads * ((double)k / (double)nSegments));
			fNextVerticeX = (float)(pArc->fRadius * cos (dCurAngleRads));
			fNextVerticeY = (float)(pArc->fRadius * sin (dCurAngleRads));
			glVertex3f (fOffsetUnitsX+fNextVerticeX, fNextVerticeY+fOffsetUnitsY, fZ);
		}
 	glEnd();
}

void COpenGL::GLUT_ScrollLeft(int x, int y)
{
	m_fViewportOffsetX += 0.05f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

void COpenGL::GLUT_ScrollRight(int x, int y)
{
	m_fViewportOffsetX -= 0.05f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

void COpenGL::GLUT_ScrollUp(int x, int y)
{
	m_fViewportOffsetY += 0.05f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

void COpenGL::GLUT_ScrollDown(int x, int y)
{
	m_fViewportOffsetY -= 0.05f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

void COpenGL::GLUT_RePosition(float fDeltaX, float fDeltaY)
{
	GLfloat	dMidViewPortX, dMidViewPortY;

	dMidViewPortX = (m_fViewportOffsetX - fDeltaX) + m_fViewportWidth / 2.0f;
	dMidViewPortY = (m_fViewportOffsetY + fDeltaY) + m_fViewportHeight / 2.0f;

//	m_fViewportWidth *= 1.1f;
//	m_fViewportHeight = m_fViewportWidth * m_fAspectRatio;

	m_fViewportOffsetX = dMidViewPortX - m_fViewportWidth / 2.0f;
	m_fViewportOffsetY = dMidViewPortY - m_fViewportHeight / 2.0f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

void COpenGL::GLUT_ZoomIn(int x, int y)
{
	GLfloat	dMidViewPortX, dMidViewPortY;

//	if ((-1 == x) || (-1 == y))
//	{
		dMidViewPortX = m_fViewportOffsetX + m_fViewportWidth / 2.0f;
		dMidViewPortY = m_fViewportOffsetY + m_fViewportHeight / 2.0f;
//	}
//	else
//	{
//		float fPercentMouseX = (float)x / (float)SUB_WIND_WIDTH;
//		float fPercentMouseY = 1.0f - ((float)y / (float)SUB_WIND_HEIGHT);
//		dMidViewPortX = m_fViewportOffsetX + (fPercentMouseX * m_fViewportWidth);
//		dMidViewPortY = m_fViewportOffsetY + (fPercentMouseY * m_fViewportHeight);
//	}

//	cout << "ZI() - X: " << x << " Y: " << y << std::endl;
//	cout << "ZI() - X%: " << fPercentMouseX << " Y%: " << fPercentMouseY << std::endl;

	m_fViewportWidth /= 1.1f;
	m_fViewportHeight = m_fViewportWidth * m_fViewPortAspectRatio;

	m_fViewportOffsetX = dMidViewPortX - m_fViewportWidth / 2.0f;
	m_fViewportOffsetY = dMidViewPortY - m_fViewportHeight / 2.0f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

void COpenGL::GLUT_ZoomOut(int x, int y)
{
	GLfloat	dMidViewPortX, dMidViewPortY;

	dMidViewPortX = m_fViewportOffsetX + m_fViewportWidth / 2.0f;
	dMidViewPortY = m_fViewportOffsetY + m_fViewportHeight / 2.0f;

	m_fViewportWidth *= 1.1f;
	m_fViewportHeight = m_fViewportWidth * m_fViewPortAspectRatio;

	m_fViewportOffsetX = dMidViewPortX - m_fViewportWidth / 2.0f;
	m_fViewportOffsetY = dMidViewPortY - m_fViewportHeight / 2.0f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_fViewportOffsetX, m_fViewportOffsetX+m_fViewportWidth, m_fViewportOffsetY, m_fViewportOffsetY+m_fViewportHeight, -12.0, 12.0);
	glMatrixMode(GL_MODELVIEW);

	glutPostRedisplay();
}

bool COpenGL::ReportBasedOnVerbosity(int nVerbosity, const char *chReport)
{
	// INI values range between 0 and 5
	// 1 <= nVerbosity <= 5
	if (nVerbosity <= INI_Verbosity_OpenGL)
	{
		cout << chReport << std::endl;
		return true;
	}
	else
		return false;
}

