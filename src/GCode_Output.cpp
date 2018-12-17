#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "enumsBase.h"
#include "STL_VectorBase.h"
#include "STL_MapBase.h"
#include "ToolPaths.h"
#include "GCode_Output.h"

using namespace std;

#define ARC_CONCENTRICITY 5.0E-4
#define CIRCLE_PROXIMITY 5.0E-7
#define CONNECT_EPSILON 4.0E-2
#define EQUIVALENCY_EPSILON 1.0E-7
#define GCODE_VERBOSITY 6

extern int INI_Verbosity_GCode_Output;
extern bool	INI_Tool_Path_SortOnCentroids;

char	chReport_GC[400];

GCode_Output::GCode_Output()
{
	m_nGCodeLinesOutput = 0;
	m_fPartWidth = 0.0f;
	m_fPartHeight = 0.0f;
}

bool GCode_Output::Set_PathFileName(std::string strPathFile)
{
	m_strPathFileName = strPathFile;
	return true;
}

bool GCode_Output::Get_PathFileName(std::string *pStrBaseFile)
{
	*pStrBaseFile = m_strPathFileName;
	return true;
}

bool GCode_Output::MarkAllAsAvailableFeatures(STL_MAP_CLASS *pFeatures)
{
	FEATURE_ELEMENT tFeature;

	int nFeatureCount = pFeatures->SMC_GetSize();
	for (int i=0; i<nFeatureCount; i++)
	{
		// Get the requested feature
		pFeatures->SMC_GetFeature(i, &tFeature);
		tFeature.m_bAvailable = true;
		// Update the feature
		if (!pFeatures->SMC_UpdateFeature(i, tFeature))
		{
			ReportBasedOnVerbosity(-1, "MarkAllAsAvailableFeatures(), failure to update feature");
			return false;
		}
	}
	return true;
}

int GCode_Output::FindClosestAvailableFeature(STL_MAP_CLASS *pFeatures, double dX, double dY, double *pNextX, double *pNextY)
{
	FEATURE_ELEMENT tFeature;
	int nIndexClosestSoFar = -1;
	double dDist, dMinDist=1.0E49;
	double dClosestX, dClosestY;

	int nFeatureCount = pFeatures->SMC_GetSize();
	for (int i=0; i<nFeatureCount; i++)
	{
		// Get the requested feature
		pFeatures->SMC_GetFeature(i, &tFeature);
		if (tFeature.m_bAvailable)
		{
			dDist = sqrt ((tFeature.m_dCentroidX-dX)*(tFeature.m_dCentroidX-dX) + (tFeature.m_dCentroidY-dY)*(tFeature.m_dCentroidY-dY));
			if (dDist < dMinDist)
			{
				dClosestX = tFeature.m_dCentroidX;
				dClosestY = tFeature.m_dCentroidY;
				dMinDist = dDist;
				nIndexClosestSoFar = i;
			}
		}
	}
	if (-1 != nIndexClosestSoFar)
	{
		*pNextX = dClosestX;
		*pNextY = dClosestY;
		return nIndexClosestSoFar;
	}
	else
		return -1;
}

int GCode_Output::OutputGCodeFile (STL_MAP_CLASS *pInsideFeatures, STL_MAP_CLASS *pOutsideFeatures, STL_MAP_CLASS *pOnFeatures, bool bInsideCutDirCCW, bool bOutsideCutDirCW, bool bChainMoves, double dMinNodeCompliance, double dCutHeight, double dCutSpeed, double dRapidMoveHeight, double dRapidMoveSpeed, double dPlungeSpeed, double dRetractSpeed)
{
	// Open the output file
	FILE *pFile;
//	string strDxfFileNameAndExt(m_strBaseFileName);
//	string strNcFileNameAndExt(m_strBaseFileName);
//	strDxfFileNameAndExt = strDxfFileNameAndExt + ".dxf";
//	strNcFileNameAndExt = strNcFileNameAndExt + ".ngc";
//	char chReport[150];

//	CString cstrLowerCaseDxf(cstrDxfFileNameAndExt);
//	cstrLowerCaseDxf.MakeLower();

//	int nNumCharsInBaseName = cstrLowerCaseDxf.Find(_T(".dxf"));
//	CString cstrBaseName(_T("UnknownName"));
//	if (-1 != nNumCharsInBaseName)
//		cstrBaseName = cstrDxfFileNameAndExt.Left(nNumCharsInBaseName);

//	CString cstrDefExt = _T("nc");
//	LPTSTR lpszDefExt = new TCHAR[cstrDefExt.GetLength() + 1];
//	_tcscpy_s (lpszDefExt, cstrDefExt.GetLength()+1, cstrDefExt);

//	CString cstrFileFilter = _T("Hazelden Files (*.nc)|*.nc|Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");
//  	LPTSTR lpszFileFilter = new TCHAR[cstrFileFilter.GetLength() + 1];
//	_tcscpy_s (lpszFileFilter, cstrFileFilter.GetLength()+1, cstrFileFilter);

//	CFileDialog OpenFileDlg (TRUE, lpszDefExt, NULL, OFN_HIDEREADONLY, lpszFileFilter, NULL);
//	CString cstrPathAndName;
//	if (OpenFileDlg.DoModal())
//	{
//		cstrPathAndName = OpenFileDlg.GetPathName();
//		if (!cstrPathAndName.IsEmpty())
//		{
//			errno_t err = _wfopen_s (&pFile, cstrPathAndName, _T("w"));
//			ASSERT (pFile && (0 == err));
//		}
//	}
//	else
//		return - 1;

	pFile = fopen(m_strPathFileName.data(), "w");
	if (!pFile)
	{
		sprintf (chReport_GC, "\nOutputGCodeFile(), Failure to open file: %s", m_strPathFileName.data());
		ReportBasedOnVerbosity(-1, chReport_GC);
		return -1;
	}


//	CString cstrNgcFileNameAndExt;
//	cstrNgcFileNameAndExt = OpenFileDlg.GetFileName();
//	CStringA ansiNgcFileNameAndExt (cstrNgcFileNameAndExt);

	// Setup GCode params & home all axis
	m_nGCodeLinesOutput = 1;
	char LineToOutput[200];

	time_t rawTime = time(0);
	struct tm *pTime;
	pTime = localtime(&rawTime);

	sprintf (LineToOutput, "%%\n");
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "#101 = %3.2f (Plunge FeedRate)\n", dPlungeSpeed);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "#102 = %3.2f (Transport FeedRate)\n", dRapidMoveSpeed);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "#103 = %3.3f (Cut FeedRate)\n", dCutSpeed);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d (Filename: %s)\n", m_nGCodeLinesOutput++, m_strPathFileName.data());
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d (Generated by: DxfToGCode Ver:L1.1)\n", m_nGCodeLinesOutput++);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d (Date-Time: %d/%d/%d - %d:%d:%d)\n", m_nGCodeLinesOutput, pTime->tm_mon+1,  pTime->tm_mday,  pTime->tm_year+1900, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d G20 (Units: Inches)\n", m_nGCodeLinesOutput++);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d G64 (Blend without tolerance)\n", m_nGCodeLinesOutput++);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "(G61 - Exact Path Mode)\n");
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "(G61.1 - Exact Stop Mode)\n");
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "(G64 P#.# Q#.# - Blend with tolerance, Q optionally specifies straight line deviation)\n");
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d G54 (1st. Coordinate Workpiece System)\n", m_nGCodeLinesOutput++);
	fputs (LineToOutput, pFile);
	sprintf (LineToOutput, "N%d G28 (Go HOME)\n", m_nGCodeLinesOutput++);
	fputs (LineToOutput, pFile);

//	sprintf_s (LineToOutput, "O1\n");
//	fputs (LineToOutput, pFile);
//	sprintf_s (LineToOutput, "N%d G28\n", m_nGCodeLinesOutput++);
//	fputs (LineToOutput, pFile);
//	sprintf_s (LineToOutput, "N%d G91 Z0\n", m_nGCodeLinesOutput++);
//	fputs (LineToOutput, pFile);
//	sprintf_s (LineToOutput, "N%d G54 G90 G98 G20\n", m_nGCodeLinesOutput++);
//	fputs (LineToOutput, pFile);
//	sprintf_s (LineToOutput, "N%d T1 M06 (EM05R2)\n", m_nGCodeLinesOutput++);
//	fputs (LineToOutput, pFile);
//	sprintf_s (LineToOutput, "N%d S6300 M03\n", m_nGCodeLinesOutput++);
//	fputs (LineToOutput, pFile);
//	sprintf_s (LineToOutput, "N%d G43 H1 M08\n", m_nGCodeLinesOutput++);
//	fputs (LineToOutput, pFile);

 	FEATURE_ELEMENT tFeature;
	char chComment[100];
	int i;

	// If optimizing based on centroids, mark all features available
	if (INI_Tool_Path_SortOnCentroids)
	{
		if (!MarkAllAsAvailableFeatures(pInsideFeatures))
			return -1;
		if (!MarkAllAsAvailableFeatures(pOutsideFeatures))
			return -1;
		if (!MarkAllAsAvailableFeatures(pOnFeatures))
			return -1;
	}

	double dNextX, dNextY, dFeatureX = 0.0, dFeatureY = 0.0;
	int nClosestIndex;

	// Cut INSIDE Features
	int nFeatureCount = pInsideFeatures->SMC_GetSize();
	for (i=0; i<nFeatureCount; i++)
	{
		if (INI_Tool_Path_SortOnCentroids)
		{
			// Sort based on proximity to last centroid
			nClosestIndex = FindClosestAvailableFeature (pInsideFeatures, dFeatureX, dFeatureY, &dNextX, &dNextY);
			if (-1 != nClosestIndex)
			{
				sprintf (chComment, "(Milling INSIDE Feature %d of %d)", nClosestIndex+1, nFeatureCount);
				if (!GenerateFeatureGCode (pFile, chComment, NULL, eCutInsideFeature, pInsideFeatures, bInsideCutDirCCW, bOutsideCutDirCW, bChainMoves, dMinNodeCompliance, nClosestIndex, dCutHeight, dCutSpeed, dRapidMoveHeight, dRapidMoveSpeed, dPlungeSpeed, dRetractSpeed))
				{
					sprintf (chReport_GC, "GenerateFeatureGCode(INSIDE.%d) failed!\n", nClosestIndex);
					PostReport(chReport_GC);
					return -1;
				}
				pInsideFeatures->SMC_GetFeature(nClosestIndex, &tFeature);
				tFeature.m_bAvailable = false;
				// Update the feature
				if (!pInsideFeatures->SMC_UpdateFeature(nClosestIndex, tFeature))
				{
					ReportBasedOnVerbosity(-1, "SMC_UpdateFeature(), failure to update feature");
					return -1;
				}
				// Move on to the next centroid
				dFeatureX = dNextX;
				dFeatureY = dNextY;
			}
			else
			{
				ReportBasedOnVerbosity(-1, "Problem encountered while sorting INSIDE features, aborting.");
				return -1;
			}
		}
		else
		{
			// pInsideFeatures->SMC_GetFeature(i, &tFeature);
			sprintf (chComment, "(Milling INSIDE Feature %d of %d)", i+1, nFeatureCount);
			if (!GenerateFeatureGCode (pFile, chComment, NULL, eCutInsideFeature, pInsideFeatures, bInsideCutDirCCW, bOutsideCutDirCW, bChainMoves, dMinNodeCompliance, i, dCutHeight, dCutSpeed, dRapidMoveHeight, dRapidMoveSpeed, dPlungeSpeed, dRetractSpeed))
			{
				sprintf (chReport_GC, "GenerateFeatureGCode(INSIDE.%d) failed!\n", i);
				PostReport(chReport_GC);
				return -1;
			}
		}
	}
	sprintf (chReport_GC, "\nCompleted %d GCode INSIDE features, lines output: %d", nFeatureCount, m_nGCodeLinesOutput);
	ReportBasedOnVerbosity(-1, chReport_GC);

 	// Cut ON features
	dFeatureX = 0.0;
	dFeatureY = 0.0;
	nFeatureCount = pOnFeatures->SMC_GetSize();
	for (i=0; i<nFeatureCount; i++)
	{
		if (INI_Tool_Path_SortOnCentroids)
		{
			// Sort based on proximity to last centroid
			nClosestIndex = FindClosestAvailableFeature (pOnFeatures, dFeatureX, dFeatureY, &dNextX, &dNextY);
			if (-1 != nClosestIndex)
			{
				sprintf (chComment, "(Milling ON Feature %d of %d)", nClosestIndex+1, nFeatureCount);
				if (!GenerateFeatureGCode_II (pFile, chComment, NULL, eCutOnFeature, pOnFeatures, bInsideCutDirCCW, bOutsideCutDirCW, bChainMoves, dMinNodeCompliance, nClosestIndex, dCutHeight, dCutSpeed, dRapidMoveHeight, dRapidMoveSpeed, dPlungeSpeed, dRetractSpeed))
				{
					sprintf (chReport_GC, "GenerateFeatureGCode(ON.%d) failed!\n", i);
					ReportBasedOnVerbosity(-1, chReport_GC);
					return -1;
				}

				pOnFeatures->SMC_GetFeature(nClosestIndex, &tFeature);
				tFeature.m_bAvailable = false;
				// Update the feature
				if (!pOnFeatures->SMC_UpdateFeature(nClosestIndex, tFeature))
				{
					ReportBasedOnVerbosity(-1, "SMC_UpdateFeature(), failure to update feature");
					return -1;
				}
				// Move to the next centroid
				dFeatureX = dNextX;
				dFeatureY = dNextY;
			}
			else
			{
				ReportBasedOnVerbosity(-1, "Problem encountered while sorting ON features, aborting.");
				return -1;
			}
		}
		else
		{
		 	sprintf (chComment, "(Milling ON Feature %d of %d)", i+1, nFeatureCount);
		 	if (!GenerateFeatureGCode_II (pFile, chComment, NULL, eCutOnFeature, pOnFeatures, bInsideCutDirCCW, bOutsideCutDirCW, bChainMoves, dMinNodeCompliance, i, dCutHeight, dCutSpeed, dRapidMoveHeight, dRapidMoveSpeed, dPlungeSpeed, dRetractSpeed))
		 	{
		 		sprintf (chReport_GC, "GenerateFeatureGCode(ON.%d) failed!\n", i);
		 		ReportBasedOnVerbosity(-1, chReport_GC);
		 		return -1;
		 	}
		}
	}
	sprintf (chReport_GC, "Completed %d GCode ON features, lines output: %d", nFeatureCount, m_nGCodeLinesOutput);
	ReportBasedOnVerbosity(-1, chReport_GC);

	// Cut outside feature
	dFeatureX = 0.0;
	dFeatureY = 0.0;
	nFeatureCount = pOutsideFeatures->SMC_GetSize();
	for (i=0; i<nFeatureCount; i++)
	{
		if (INI_Tool_Path_SortOnCentroids)
		{
			// Sort based on proximity to last centroid
			nClosestIndex = FindClosestAvailableFeature (pOutsideFeatures, dFeatureX, dFeatureY, &dNextX, &dNextY);
			if (-1 != nClosestIndex)
			{
				sprintf (chComment, "(Milling OUTSIDE Feature %d of %d)", nClosestIndex+1, nFeatureCount);
				if (!GenerateFeatureGCode (pFile, chComment, NULL, eCutOutsideFeature, pOutsideFeatures, bInsideCutDirCCW, bOutsideCutDirCW, bChainMoves, dMinNodeCompliance, nClosestIndex, dCutHeight, dCutSpeed, dRapidMoveHeight, dRapidMoveSpeed, dPlungeSpeed, dRetractSpeed))
				{
					sprintf (chReport_GC, "GenerateFeatureGCode(OUTSIDE.%d) failed!\n", nClosestIndex);
					PostReport(chReport_GC);
					return -1;
				}

				pOutsideFeatures->SMC_GetFeature(nClosestIndex, &tFeature);
				tFeature.m_bAvailable = false;
				// Update the feature
				if (!pOutsideFeatures->SMC_UpdateFeature(nClosestIndex, tFeature))
				{
					ReportBasedOnVerbosity(-1, "SMC_UpdateFeature(), failure to update feature");
					return -1;
				}
				// Move to the next centroid
				dFeatureX = dNextX;
				dFeatureY = dNextY;
			}
			else
			{
				ReportBasedOnVerbosity(-1, "Problem encountered while sorting OUTSIDE features, aborting.");
				return -1;
			}
		}
		else
		{
		 	sprintf (chComment, "(Milling OUTSIDE Feature %d of %d)", i+1, nFeatureCount);
		 	if (!GenerateFeatureGCode (pFile, chComment, NULL, eCutOutsideFeature, pOutsideFeatures, bInsideCutDirCCW, bOutsideCutDirCW, bChainMoves, dMinNodeCompliance, i, dCutHeight, dCutSpeed, dRapidMoveHeight, dRapidMoveSpeed, dPlungeSpeed, dRetractSpeed))
		 	{
		 		sprintf (chReport_GC, "GenerateFeatureGCode(OUTSIDE.%d) failed!\n", i);
		 		PostReport(chReport_GC);
		 		return -1;
		 	}
		}
	}
	sprintf (chReport_GC, "Completed %d GCode OUTSIDE features, lines output: %d", nFeatureCount, m_nGCodeLinesOutput);
	ReportBasedOnVerbosity(-1, chReport_GC);


	sprintf (LineToOutput, "%%\n");
	fputs (LineToOutput, pFile);
	// Close file
	fclose (pFile);

	return m_nGCodeLinesOutput;
}


bool GCode_Output::GenerateFeatureGCode (FILE *pFile, char *pDesc, RECT_OBJECT *pBlockRect, FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeatures, bool bInsideCutDirCCW, bool bOutsideCutDirCW, bool bChainMoves, double dMinNodeCompliance, int nGroupIndex, double dCutHeight, double dCutSpeed, double dRapidMoveHeight, double dRapidMoveSpeed, double dPlungeSpeed, double dRetractSpeed)
{
	CONNECTED_ELEMENT	tDxfSegment;
	char LineToOutput[200];
//	char chReport[200];

	if ((eCutInsideFeature != eFeatureCutType) &&
		(eCutOutsideFeature != eFeatureCutType) &&
		(eCutOnFeature != eFeatureCutType))
	assert(false);

	// Output the feature's description
	sprintf (LineToOutput, "%s\n", pDesc);
	fputs (LineToOutput, pFile);

	// Start in lower left corner
	double dCurX = 0.0;
	double dCurY = 0.0;
	double dCurZ = -0.0;

	// Accel & Decel for every segment until told otherwise
 //	sprintf (LineToOutput, "N%d G00 X%3.4f Y%3.4f\n", m_nGCodeLinesOutput++, dCurX, dCurY);
//	fputs (LineToOutput, pFile);

	// Get the Map's pointer
	 STL_MAP_CLASS *pFeatureMap = pFeatures;

	bool bCutCCW = false;

	if (eCutInsideFeature == eFeatureCutType)
	{
		bCutCCW = (bInsideCutDirCCW == true) ? false : true;
//		if (bInsideCutDirCCW == true)
//			bCutCCW = false;
//		else
//			bCutCCW = true;
	}
	else if (eCutOutsideFeature == eFeatureCutType)
	{
		bCutCCW = (bOutsideCutDirCW == true) ? true : false;
//		if (bOutsideCutDirCW == true)
//			bCutCCW = false;
//		else
//			bCutCCW = true;
	}
	else
		bCutCCW = false;

	// Make certain the indexed feature is available
	if (pFeatureMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
	{
		PostReport("GenerateFeatureGCode()..Not that many features in map");
	 	return false;
	}

	// Get the requested feature
	FEATURE_ELEMENT tFeature;
	pFeatureMap->SMC_GetFeature(nGroupIndex, &tFeature);

	if (tFeature.m_nNumberSegments < 1)
	{
		PostReport("GenerateFeatureGCode()..Feature does not contain any segments");
		return false;
	}

	// Get the feature's SmartArray pointer
	STL_CONNECTED_SEGS_CLASS *pToolPathSegmentClass = tFeature.m_pSaToolPathFinishCut;
	int nTotalElements = pToolPathSegmentClass->SVC_Size();

	// Get first segment
	tDxfSegment = pToolPathSegmentClass->SVC_GetElement(0);
	int nSegmentIndex;
	int nFinishingEnd;

	// Move based upon cut direction,


	if (bCutCCW)
	{
		sprintf (chReport_GC, "   DBG1: bCutCCW=%d, tDxfSegment.ucCcwRotationStartEndpoint=%d\n", bCutCCW, tDxfSegment.ucCcwRotationStartEndpoint);
		ReportBasedOnVerbosity(-1, chReport_GC);

		if (1 == tDxfSegment.ucCcwRotationStartEndpoint)
		{
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the first segment & work forward
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
		else
		{
			// This feature needs to index backwards to do CCW rotation
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the last segment & work backwards
			nSegmentIndex = tDxfSegment.nA_ConnectedSegmentIndex;
			nFinishingEnd = (tDxfSegment.ucA_ConnectedSegmentEnd == 2) ? 1 : 2;
		}
	}
	else
	{
		sprintf (chReport_GC, "   DBG2: bCutCCW=%d, tDxfSegment.ucCcwRotationStartEndpoint=%d\n", bCutCCW, tDxfSegment.ucCcwRotationStartEndpoint);
		ReportBasedOnVerbosity(-1, chReport_GC);
		if (1 == tDxfSegment.ucCcwRotationStartEndpoint)
		{
			// This feature needs to index backwards to do CW rotation
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the last segment & work backwards
			nSegmentIndex = tDxfSegment.nA_ConnectedSegmentIndex;
			nFinishingEnd = (tDxfSegment.ucA_ConnectedSegmentEnd == 2) ? 1 : 2;
		}
		else
		{
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the first segment & work forward
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
	}


#if 0	
	// 3/28/2017 - This was the original
	if (bCutCCW)
	{
		sprintf (chReport_GC, "   DBG1: bCutCCW=%d, tDxfSegment.ucCcwRotationStartEndpoint=%d\n", bCutCCW, tDxfSegment.ucCcwRotationStartEndpoint);
		ReportBasedOnVerbosity(-1, chReport_GC);

		if (1 == tDxfSegment.ucCcwRotationStartEndpoint)
		{
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the first segment & work forward
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
		else
		{
			// This feature needs to index backwards to do CCW rotation
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the last segment & work backwards
			nSegmentIndex = tDxfSegment.nA_ConnectedSegmentIndex;
			nFinishingEnd = (tDxfSegment.ucA_ConnectedSegmentEnd == 2) ? 1 : 2;
		}
	}
	else
	{
		sprintf (chReport_GC, "   DBG2: bCutCCW=%d, tDxfSegment.ucCcwRotationStartEndpoint=%d\n", bCutCCW, tDxfSegment.ucCcwRotationStartEndpoint);
		ReportBasedOnVerbosity(-1, chReport_GC);
		if (1 == tDxfSegment.ucCcwRotationStartEndpoint)
		{
			// This feature needs to index backwards to do CW rotation
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the last segment & work backwards
			nSegmentIndex = tDxfSegment.nA_ConnectedSegmentIndex;
			nFinishingEnd = (tDxfSegment.ucA_ConnectedSegmentEnd == 2) ? 1 : 2;
		}
		else
		{
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			// Start with the first segment & work forward
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
	}
#endif

	// Move to starting point & plunge down to cut height
	sprintf (LineToOutput, "N%d G00 X%3.4f Y%3.4f F#102\n", m_nGCodeLinesOutput++, dCurX, dCurY);
 	fputs (LineToOutput, pFile);

	dCurZ = dCutHeight;
	sprintf (LineToOutput, "N%d G01 Z%3.4f F#101\n", m_nGCodeLinesOutput++, dCurZ);
	fputs (LineToOutput, pFile);

	// Cut the path
	double dStartX, dStartY, dEndX, dEndY;
	int nNextSegmentIndex;
	int nNextFinishingEnd;

	bool bChainingMovesOn = false;
	double dCompliance;
	bool bFinishingEndpointCompliant;

	for (int nSegmentsDone=1; nSegmentsDone<=nTotalElements; nSegmentsDone++)
	{
		tDxfSegment = pToolPathSegmentClass->SVC_GetElement(nSegmentIndex);
//		TRACE("Segment[%d]: Ax: %lf, Ay: %lf, Bx: %lf, By: %lf\n", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy);

		if (1 == nFinishingEnd)
		{
			dStartX = tDxfSegment.dBx;
			dStartY = tDxfSegment.dBy;
			dEndX = tDxfSegment.dAx;
			dEndY = tDxfSegment.dAy;
			dCompliance = tDxfSegment.dA_ChainedMoveCompliance;
			nNextSegmentIndex = tDxfSegment.nA_ConnectedSegmentIndex;
			nNextFinishingEnd = (tDxfSegment.ucA_ConnectedSegmentEnd == 2) ? 1 : 2;
		}
		else
		{
			dStartX = tDxfSegment.dAx;
			dStartY = tDxfSegment.dAy;
			dEndX = tDxfSegment.dBx;
			dEndY = tDxfSegment.dBy;
			dCompliance = tDxfSegment.dB_ChainedMoveCompliance;
			nNextSegmentIndex = tDxfSegment.nB_ConnectedSegmentIndex;
			nNextFinishingEnd = (tDxfSegment.ucB_ConnectedSegmentEnd == 2) ? 1 : 2;
		}

		// -- Node compliancy for chaining moves --
		if (dMinNodeCompliance <= dCompliance)
			bFinishingEndpointCompliant = true;
		else
			bFinishingEndpointCompliant = false;

		// If this is the last segment & chaining is active, turn it off to force decceleration by forcing non-compliance
		if ((nSegmentsDone == nTotalElements) && bChainingMovesOn)
			bFinishingEndpointCompliant = false;

		// Or if this is the last segment & chaining is enabled, don't allow turning it on since no completion GCODE will exist.
		if ((nSegmentsDone == nTotalElements) && bChainMoves)
			bFinishingEndpointCompliant = false;

		// Verify position
		assert (fabs(dCurX - dStartX) < CONNECT_EPSILON);
		assert (fabs(dCurY - dStartY) < CONNECT_EPSILON);

		if (tDxfSegment.eSegmentType == eLine)
		{
			if (bChainMoves && !bChainingMovesOn && bFinishingEndpointCompliant)
			{
				sprintf (chReport_GC, "   Segment[%d]: LINE(G01) - Turning ON chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(3, chReport_GC);
				sprintf (LineToOutput, "N%d G61\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = true;
			}

			sprintf (chReport_GC, "   Segment[%d]: LINE(G01) - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
			ReportBasedOnVerbosity(3, chReport_GC);

			// Move to end point
			dCurX = dEndX;
			dCurY = dEndY;
			sprintf (LineToOutput, "N%d G01 X%3.4f Y%3.4f F#103\n", m_nGCodeLinesOutput++, dCurX, dCurY);
			fputs (LineToOutput, pFile);

			if (bChainMoves && bChainingMovesOn && !bFinishingEndpointCompliant)
			{
				sprintf (chReport_GC, "   Segment[%d]: LINE(G01) - Turning OFF chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = false;
			}
		}
		else if (tDxfSegment.eSegmentType == eCircle)
		{

			sprintf (chReport_GC, "   DBG3: CIRCLE() - bCutCCW=%d, bChainingMovesOn=%d, nFinishingEnd=%d\n", bCutCCW, bChainingMovesOn, nFinishingEnd);
			ReportBasedOnVerbosity(-1, chReport_GC);

			if (bChainingMovesOn)
			{
				sprintf (chReport_GC, "   Segment[%d]: CIRCLE(G02/G03) - Turning OFF chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = false;
			}
			if (1 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: CIRCLE(G02)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(2, chReport_GC);
			}
 			else if (2 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: CIRCLE(G03)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(2, chReport_GC);
			}
	 		else
				assert(false);

			// Move to end point
			dCurX = dEndX;
			dCurY = dEndY;

//			if (1 == nFinishingEnd)
//				sprintf_s (LineToOutput, "N%d G02 X%3.4f Y%3.4f I%3.4f J%3.4f F%3.4f\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dBx), (tDxfSegment.dRy-tDxfSegment.dBy), dCutSpeed);
//			else
//				sprintf_s (LineToOutput, "N%d G03 X%3.4f Y%3.4f I%3.4f J%3.4f F%3.4f\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dAx), (tDxfSegment.dRy-tDxfSegment.dAy), dCutSpeed);
			if (1 == nFinishingEnd)
				sprintf (LineToOutput, "N%d G02 I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, (tDxfSegment.dRx-tDxfSegment.dBx), (tDxfSegment.dRy-tDxfSegment.dBy));
			else
				sprintf (LineToOutput, "N%d G03 I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, (tDxfSegment.dRx-tDxfSegment.dAx), (tDxfSegment.dRy-tDxfSegment.dAy));
			fputs (LineToOutput, pFile);
		}
		else if (tDxfSegment.eSegmentType == eArc)
		{
			if (bChainMoves && !bChainingMovesOn && bFinishingEndpointCompliant)
			{
				if (3 < GCODE_VERBOSITY)
				{
					sprintf (chReport_GC, "   Segment[%d]: ARC(G02/G03) - Turning ON chained moves...", nSegmentIndex);
					PostReport(chReport_GC);
				}
  				sprintf (LineToOutput, "N%d G61\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = true;
			}

			if (1 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G02)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(2, chReport_GC);
			}
 			else if (2 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G03)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(2, chReport_GC);
			}
 			else
				assert(false);

			// Move to end point
			dCurX = dEndX;
			dCurY = dEndY;

			// Verify concentricity of arc
			ARC_DATA tArcData;
			tArcData.dX1 = tDxfSegment.dAx;
			tArcData.dY1 = tDxfSegment.dAy;
			tArcData.dX2 = tDxfSegment.dBx;
			tArcData.dY2 = tDxfSegment.dBy;
			tArcData.dRx = tDxfSegment.dRx;
			tArcData.dRy = tDxfSegment.dRy;
			tArcData.lDxfEntityNumber = tDxfSegment.lDxfEntityNumber;
			if (!VerifyArcCenterPoint(&tArcData, ARC_CONCENTRICITY, ARC_CONCENTRICITY/10.0))
			{
				PostReport("GenerateFeatureGCode::VerifyArcCenterPoint() failed - attempting to re-align...");
				// ToDo: re-align intercepts
#if 1
				CIRCLE_OBJECT tCircleA, tCircleB;
				POINT_OBJECT tAdjPoint1, tAdjPoint2;
			//	dRad1 = sqrt ((dForcedStartX-tDxfSegment.dRx)*(dForcedStartX-tDxfSegment.dRx) + (dForcedStartY-tDxfSegment.dRy)*(dForcedStartY-tDxfSegment.dRy));
			//	dRad2 = sqrt ((tDxfSegment.dAx-tDxfSegment.dRx)*(tDxfSegment.dAx-tDxfSegment.dRx) + (tDxfSegment.dAy-tDxfSegment.dRy)*(tDxfSegment.dAy-tDxfSegment.dRy));
				tCircleA.dXc = tDxfSegment.dAx;
				tCircleA.dYc = tDxfSegment.dAy;
				tCircleA.dRadius = tDxfSegment.dRadius;
				tCircleB.dXc = tDxfSegment.dBx;
				tCircleB.dYc = tDxfSegment.dBy;
				tCircleB.dRadius = tDxfSegment.dRadius;
				// Calculate new arc center
				bool bSts = CalculateCircleCircleIntersection_II(&tCircleA, &tCircleB, &tAdjPoint1, &tAdjPoint2);
				assert(bSts);

				double dDistFromOrig_1 = sqrt ((tAdjPoint1.dX-tDxfSegment.dRx)*(tAdjPoint1.dX-tDxfSegment.dRx) + (tAdjPoint1.dY-tDxfSegment.dRy)*(tAdjPoint1.dY-tDxfSegment.dRy));
				double dDistFromOrig_2 = sqrt ((tAdjPoint2.dX-tDxfSegment.dRx)*(tAdjPoint2.dX-tDxfSegment.dRx) + (tAdjPoint2.dY-tDxfSegment.dRy)*(tAdjPoint2.dY-tDxfSegment.dRy));
		//		sprintf (chReport_GC, " *** Segment[DXF#:%ld]: ARC() - AdjPt1 Len: %lf, AdjPt1 Len: %lf\n", tArcData.lDxfEntityNumber, dDistFromOrig_1, dDistFromOrig_2);
		//		ReportBasedOnVerbosity(-1, chReport_GC);

				// Update new center
				if (dDistFromOrig_1 <= dDistFromOrig_2)
				{
					tDxfSegment.dRx = tAdjPoint1.dX;
					tDxfSegment.dRy = tAdjPoint1.dY;
				}
				else
				{
					tDxfSegment.dRx = tAdjPoint2.dX;
					tDxfSegment.dRy = tAdjPoint2.dY;
				}
				tArcData.dRx = tDxfSegment.dRx;
				tArcData.dRy = tDxfSegment.dRy;

				if (!VerifyArcCenterPoint(&tArcData, ARC_CONCENTRICITY, ARC_CONCENTRICITY/10.0))
				{
					PostReport("Post-Correction GenerateFeatureGCode::VerifyArcCenterPoint() failed\n");
					assert (false);
				}
				PostReport("Re-alignment successful\n");
#endif
			}

			if (1 == nFinishingEnd)
				sprintf (LineToOutput, "N%d G02 X%3.4f Y%3.4f I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dBx), (tDxfSegment.dRy-tDxfSegment.dBy));
			else
				sprintf (LineToOutput, "N%d G03 X%3.4f Y%3.4f I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dAx), (tDxfSegment.dRy-tDxfSegment.dAy));
			fputs (LineToOutput, pFile);
			if (bChainMoves && bChainingMovesOn && !bFinishingEndpointCompliant)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G02/G03) - Turning OFF chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = false;
			}

		}
		else
			assert(false);

		nSegmentIndex = nNextSegmentIndex;
		nFinishingEnd = nNextFinishingEnd;
	}

	if (bChainingMovesOn)
	{
		if (3 < GCODE_VERBOSITY)
		{
 			PostReport("   Feature complete - turning OFF chained moves...");
		}
	 	sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
		fputs (LineToOutput, pFile);
	}

	// Move up
	dCurZ = dRapidMoveHeight;
	sprintf (LineToOutput, "N%d G00 Z%3.4f F%3.4f\n", m_nGCodeLinesOutput++, dCurZ, dRetractSpeed);
	fputs (LineToOutput, pFile);

	sprintf (chReport_GC, "Completed %s GCode, number lines output: %d", pDesc, m_nGCodeLinesOutput);
	ReportBasedOnVerbosity(1, chReport_GC);
	return true;
}

bool GCode_Output::GenerateFeatureGCode_II (FILE *pFile, char *pDesc, RECT_OBJECT *pBlockRect, FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeatures, bool bInsideCutDirCCW, bool bOutsideCutDirCW, bool bChainMoves, double dMinNodeCompliance, int nGroupIndex, double dCutHeight, double dCutSpeed, double dRapidMoveHeight, double dRapidMoveSpeed, double dPlungeSpeed, double dRetractSpeed)
{
//	char chReport[200];
	CONNECTED_ELEMENT	tDxfSegment;
	char LineToOutput[200];
	
	if (eCutOnFeature != eFeatureCutType)
		assert(false);
 
 	// Output the description
//	sprintf (chReport, "GCode File: %s\n", pDesc);
//	PostReport(chReport);
	sprintf (LineToOutput, "%s\n", pDesc);
	fputs (LineToOutput, pFile);
	 
	// Start in lower left corner
	double dCurX = 0.0;
	double dCurY = 0.0;
	double dCurZ = -0.0;
 
 	// Get the Map's pointer
	STL_MAP_CLASS *pFeatureMap = pFeatures;

	bool bCutCCW = false;
	
	// Make certain the indexed feature is available
	if (pFeatureMap->SMC_GetSize() < (unsigned int)nGroupIndex)
	{
		PostReport("GenerateFeatureGCode_II()..Not that many features in map");
		return false;
	}

	// Get the requested feature
	FEATURE_ELEMENT tFeature;
	pFeatureMap->SMC_GetFeature(nGroupIndex, &tFeature);

	if (tFeature.m_nNumberSegments < 1)
	{
		PostReport("GenerateFeatureGCode_II()..Feature does not contain any segments\n");
		return false;
	}

	// Get the feature's SmartArray pointer
	STL_CONNECTED_SEGS_CLASS *pToolPathSegmentClass = tFeature.m_pSaToolPathFinishCut;
	int nTotalElements = pToolPathSegmentClass->SVC_Size();

	// Get first segment
	tDxfSegment = pToolPathSegmentClass->SVC_GetElement(0);
	int nSegmentIndex;
	int nFinishingEnd;
	if (bCutCCW)
	{
//		if (2 == nTotalElements)
//		{
//			// Both ends will connect to the 2nd. segment
//		}
		if ((-1 != tDxfSegment.nA_ConnectedSegmentIndex) && (-1 != tDxfSegment.nB_ConnectedSegmentIndex))
		{
			// Closed loop, start with A end
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
		else if (-1 == tDxfSegment.nA_ConnectedSegmentIndex)
		{
			// Path starts from A end
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
		else
		{
			// Path starts from B end
			dCurX = tDxfSegment.dBx;
			dCurY = tDxfSegment.dBy;
			nSegmentIndex = 0;
			nFinishingEnd = 1;
		}
 	}
	else
	{
		// No distinction between CW & CCW for now
		if ((-1 != tDxfSegment.nA_ConnectedSegmentIndex) && (-1 != tDxfSegment.nB_ConnectedSegmentIndex))
		{
			// Closed loop, start with A end
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
		else if (-1 == tDxfSegment.nA_ConnectedSegmentIndex)
		{
			// Path starts from A end
			dCurX = tDxfSegment.dAx;
			dCurY = tDxfSegment.dAy;
			nSegmentIndex = 0;
			nFinishingEnd = 2;
		}
		else
		{
			// Path starts from B end
			dCurX = tDxfSegment.dBx;
			dCurY = tDxfSegment.dBy;
			nSegmentIndex = 0;
			nFinishingEnd = 1;
		}
 	}

	// Move to starting point & plunge down to cut height
 //	sprintf_s (LineToOutput, "N%d G00 X%3.4f Y%3.4f F%3.4f\n", m_nGCodeLinesOutput++, dCurX, dCurY, dRapidMoveSpeed);
	sprintf (LineToOutput, "N%d G00 X%3.4f Y%3.4f F#102\n", m_nGCodeLinesOutput++, dCurX, dCurY);
	fputs (LineToOutput, pFile);

 	dCurZ = dCutHeight;
	sprintf (LineToOutput, "N%d G01 Z%3.4f F#101\n", m_nGCodeLinesOutput++, dCurZ);
	fputs (LineToOutput, pFile);
	
	// Cut the path
	double dStartX, dStartY, dEndX, dEndY;
	int nNextSegmentIndex;
	int nNextFinishingEnd;

	bool bChainingMovesOn = false;
	double dCompliance;
	bool bFinishingEndpointCompliant;

//	CToolPaths *pToolPath = new CToolPaths();

	for (int nSegmentsDone=1; nSegmentsDone<=nTotalElements; nSegmentsDone++)
	{
		tDxfSegment = pToolPathSegmentClass->SVC_GetElement(nSegmentIndex);
//		TRACE("Segment[%d]: Ax: %lf, Ay: %lf, Bx: %lf, By: %lf\n", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy);
	
		if (1 == nFinishingEnd)
		{
			dStartX = tDxfSegment.dBx;
			dStartY = tDxfSegment.dBy;
			dEndX = tDxfSegment.dAx;
			dEndY = tDxfSegment.dAy;
			dCompliance = tDxfSegment.dA_ChainedMoveCompliance;
			nNextSegmentIndex = tDxfSegment.nA_ConnectedSegmentIndex;
			nNextFinishingEnd = (tDxfSegment.ucA_ConnectedSegmentEnd == 2) ? 1 : 2;
		}
		else
		{
			dStartX = tDxfSegment.dAx;
			dStartY = tDxfSegment.dAy;
			dEndX = tDxfSegment.dBx;
			dEndY = tDxfSegment.dBy;
			dCompliance = tDxfSegment.dB_ChainedMoveCompliance;
			nNextSegmentIndex = tDxfSegment.nB_ConnectedSegmentIndex;
			nNextFinishingEnd = (tDxfSegment.ucB_ConnectedSegmentEnd == 2) ? 1 : 2;
		}

		// -- Node compliancy for chaining moves --
		if (dMinNodeCompliance <= dCompliance)
			bFinishingEndpointCompliant = true;
		else
			bFinishingEndpointCompliant = false;

		// If this is the last segment & chaining is active, turn it off to force decceleration by forcing non-compliance
		if ((nSegmentsDone == nTotalElements) && bChainingMovesOn)
			bFinishingEndpointCompliant = false;

		// Or if this is the last segment & chaining is enabled, don't allow turning it on since no completion GCODE will exist.
		if ((nSegmentsDone == nTotalElements) && bChainMoves)
			bFinishingEndpointCompliant = false;

		// Verify position
		assert (fabs(dCurX - dStartX) < CONNECT_EPSILON);
		assert (fabs(dCurY - dStartY) < CONNECT_EPSILON);
		
		// This forces equivalence of starting point to last ending point - 04/08/2013
		double dForcedStartX = dCurX;
		double dForcedStartY = dCurY;

		if (tDxfSegment.eSegmentType == eLine)
		{
			if (bChainMoves && !bChainingMovesOn && bFinishingEndpointCompliant)
			{
				if (5 < GCODE_VERBOSITY)
				{
					sprintf (chReport_GC, "   Segment[%d]: LINE(G01) - Turning ON chained moves...", nSegmentIndex);
					PostReport(chReport_GC);
				}
				sprintf (LineToOutput, "N%d G61\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = true;
			}
//			if (bChainMoves && bChainingMovesOn && !bFinishingEndpointCompliant)
//			{
//				TRACE("   Segment[%d]: LINE(G01) - Turning OFF chained moves...\n", nSegmentIndex);
// 				sprintf_s (LineToOutput, "N%d G61\n", m_nGCodeLinesOutput++);
//				fputs (LineToOutput, pFile);
//				bChainingMovesOn = FALSE;
//			}
			
			sprintf (chReport_GC, "   Segment[%d]: LINE(G01) - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
			ReportBasedOnVerbosity(2, chReport_GC);

			// Move to end point
			dCurX = dEndX;
			dCurY = dEndY;
 			sprintf (LineToOutput, "N%d G01 X%3.4f Y%3.4f F#103\n", m_nGCodeLinesOutput++, dCurX, dCurY);
			fputs (LineToOutput, pFile);
			if (bChainMoves && bChainingMovesOn && !bFinishingEndpointCompliant)
			{
				sprintf (chReport_GC, "   Segment[%d]: LINE(G01) - Turning OFF chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = false;
			}
		}
		else if (tDxfSegment.eSegmentType == eCircle)
		{
			if (bChainingMovesOn)
			{
				sprintf (chReport_GC, "   Segment[%d]: CIRCLE(G02/G03) - Turning OFF chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = false;
			}
			if (1 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: CIRCLE(G02)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(2, chReport_GC);
			}
		 	else if (2 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: CIRCLE(G03)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(2, chReport_GC);
			}
 			else
				assert(false);
	
			// Move to end point
			dCurX = dEndX;
			dCurY = dEndY;
 	
	//		if (1 == nFinishingEnd)
	//			sprintf_s (LineToOutput, "N%d G02 X%3.4f Y%3.4f I%3.4f J%3.4f F%3.4\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dBx), (tDxfSegment.dRy-tDxfSegment.dBy), dCutSpeed);
	//		else
	//			sprintf_s (LineToOutput, "N%d G03 X%3.4f Y%3.4f I%3.4f J%3.4f F%3.4\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dAx), (tDxfSegment.dRy-tDxfSegment.dAy), dCutSpeed);
			if (1 == nFinishingEnd)
				sprintf (LineToOutput, "N%d G02 I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, (tDxfSegment.dRx-tDxfSegment.dBx), (tDxfSegment.dRy-tDxfSegment.dBy));
			else
				sprintf (LineToOutput, "N%d G03 I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, (tDxfSegment.dRx-tDxfSegment.dAx), (tDxfSegment.dRy-tDxfSegment.dAy));
			fputs (LineToOutput, pFile);
		}
		else if (tDxfSegment.eSegmentType == eArc)
		{
			if (bChainMoves && !bChainingMovesOn && bFinishingEndpointCompliant)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G02/G03) - Turning ON chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(4, chReport_GC);
				sprintf (LineToOutput, "N%d G61\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = true;
			}
//			if (bChainMoves && bChainingMovesOn && !bFinishingEndpointCompliant)
//			{
//				TRACE("   Segment[%d]: ARC(G02/G03) - Turning OFF chained moves...\n", nSegmentIndex);
// 				sprintf_s (LineToOutput, "N%d G61\n", m_nGCodeLinesOutput++);
//				fputs (LineToOutput, pFile);
//				bChainingMovesOn = FALSE;
//			}

			if (1 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G02)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(3, chReport_GC);
			}
			else if (2 == nFinishingEnd)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G03)  - Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, EndPoint: %d", nSegmentIndex, tDxfSegment.dAx, tDxfSegment.dAy, tDxfSegment.dBx, tDxfSegment.dBy, nFinishingEnd);
				ReportBasedOnVerbosity(3, chReport_GC);
			}
			else
				assert(false);
	
			// Move to end point
			dCurX = dEndX;
			dCurY = dEndY;

			// Here goes endpoint alignment - 04/08/2013
			CIRCLE_OBJECT tCircleA, tCircleB;
			POINT_OBJECT tAdjPoint1, tAdjPoint2;
			double dRad1, dRad2;

	 		if (1 == nFinishingEnd)
			{
				dRad1 = sqrt ((dForcedStartX-tDxfSegment.dRx)*(dForcedStartX-tDxfSegment.dRx) + (dForcedStartY-tDxfSegment.dRy)*(dForcedStartY-tDxfSegment.dRy));
				dRad2 = sqrt ((tDxfSegment.dAx-tDxfSegment.dRx)*(tDxfSegment.dAx-tDxfSegment.dRx) + (tDxfSegment.dAy-tDxfSegment.dRy)*(tDxfSegment.dAy-tDxfSegment.dRy));
				tCircleA.dXc = tDxfSegment.dAx;
				tCircleA.dYc = tDxfSegment.dAy;
				tCircleA.dRadius = dRad2;
				tCircleB.dXc = dForcedStartX;
				tCircleB.dYc = dForcedStartY;
				tCircleB.dRadius = dRad1;
 			}
			else
			{
				dRad1 = sqrt ((dForcedStartX-tDxfSegment.dRx)*(dForcedStartX-tDxfSegment.dRx) + (dForcedStartY-tDxfSegment.dRy)*(dForcedStartY-tDxfSegment.dRy));
				dRad2 = sqrt ((tDxfSegment.dBx-tDxfSegment.dRx)*(tDxfSegment.dBx-tDxfSegment.dRx) + (tDxfSegment.dBy-tDxfSegment.dRy)*(tDxfSegment.dBy-tDxfSegment.dRy));
				tCircleA.dXc = dForcedStartX;
				tCircleA.dYc = dForcedStartY;
				tCircleA.dRadius = dRad1;
				tCircleB.dXc = tDxfSegment.dBx;
				tCircleB.dYc = tDxfSegment.dBy;
				tCircleB.dRadius = dRad2;
	 		}
		 	
//			assert(pToolPath);
	 		bool bSts = CalculateCircleCircleIntersection_II(&tCircleA, &tCircleB, &tAdjPoint1, &tAdjPoint2);
	 		assert(bSts);
		//	bSts = CalculateCircleCircleIntersection(&tCircleA, &tCircleB, &tAdjPoint1, &tAdjPoint2);
		//	assert(bSts);
 
			double dDistFromOrig_1 = sqrt ((tAdjPoint1.dX-tDxfSegment.dRx)*(tAdjPoint1.dX-tDxfSegment.dRx) + (tAdjPoint1.dY-tDxfSegment.dRy)*(tAdjPoint1.dY-tDxfSegment.dRy));
			double dDistFromOrig_2 = sqrt ((tAdjPoint2.dX-tDxfSegment.dRx)*(tAdjPoint2.dX-tDxfSegment.dRx) + (tAdjPoint2.dY-tDxfSegment.dRy)*(tAdjPoint2.dY-tDxfSegment.dRy));
			sprintf (chReport_GC, " *** Segment[%d]: ARC() - AdjPt1 Len: %lf, AdjPt1 Len: %lf\n", nSegmentIndex, dDistFromOrig_1, dDistFromOrig_2);
			ReportBasedOnVerbosity(3, chReport_GC);

			double dNewX, dNewY;
			if (dDistFromOrig_1 <= dDistFromOrig_2)
			{
				dNewX = tAdjPoint1.dX;
				dNewY = tAdjPoint1.dY;
			}
			else
			{
				dNewX = tAdjPoint2.dX;
				dNewY = tAdjPoint2.dY;
			}

			// This forces equivalence of starting point to last ending point - 04/08/2013
			if (1 == nFinishingEnd)
			{
			 	sprintf (chReport_GC, " *** Segment[%d]: ARC() - Orig_RadX: %lf, Adj_RadX: %lf, Orig_RadY: %lf, Adj_RadY: %lf", nSegmentIndex, (tDxfSegment.dRx-tDxfSegment.dBx), (dNewX-dForcedStartX), (tDxfSegment.dRy-tDxfSegment.dBy), (dNewY-dForcedStartY));
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G02 X%3.4f Y%3.4f I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, dCurX, dCurY, (dNewX-dForcedStartX), (dNewY-dForcedStartY));
			}
			else
			{
				sprintf (chReport_GC, " *** Segment[%d]: ARC() - Orig_RadX: %lf, Adj_RadX: %lf, Orig_RadY: %lf, Adj_RadY: %lf", nSegmentIndex, (tDxfSegment.dRx-tDxfSegment.dAx), (dNewX-dForcedStartX), (tDxfSegment.dRy-tDxfSegment.dAy), (dNewY-dForcedStartY));
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G03 X%3.4f Y%3.4f I%3.4f J%3.4f F#103\n", m_nGCodeLinesOutput++, dCurX, dCurY, (dNewX-dForcedStartX), (dNewY-dForcedStartY));
			}
			fputs (LineToOutput, pFile);

//			if (1 == nFinishingEnd)
//				sprintf_s (LineToOutput, "N%d G02 X%3.4f Y%3.4f I%3.4f J%3.4f F%3.4f\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dBx), (tDxfSegment.dRy-tDxfSegment.dBy), dCutSpeed);
//			else
//				sprintf_s (LineToOutput, "N%d G03 X%3.4f Y%3.4f I%3.4f J%3.4f F%2.4f\n", m_nGCodeLinesOutput++, dCurX, dCurY, (tDxfSegment.dRx-tDxfSegment.dAx), (tDxfSegment.dRy-tDxfSegment.dAy), dCutSpeed);
//			fputs (LineToOutput, pFile);

			// Latest chaining implementation turns Off after last chained move
			if (bChainMoves && bChainingMovesOn && !bFinishingEndpointCompliant)
			{
				sprintf (chReport_GC, "   Segment[%d]: ARC(G02/G03) - Turning OFF chained moves...", nSegmentIndex);
				ReportBasedOnVerbosity(2, chReport_GC);
				sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
				fputs (LineToOutput, pFile);
				bChainingMovesOn = false;
			}
		}
		else
			assert(false);

		nSegmentIndex = nNextSegmentIndex;
		nFinishingEnd = nNextFinishingEnd;
	}
	
//	delete pToolPath;

	if (bChainingMovesOn)
	{
		sprintf (chReport_GC, "   Feature complete - turning OFF chained moves...");
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (LineToOutput, "N%d G62\n", m_nGCodeLinesOutput++);
		fputs (LineToOutput, pFile);
	}

	// Move up
	dCurZ = dRapidMoveHeight;
	sprintf (LineToOutput, "N%d G00 Z%3.4f F%3.4f\n", m_nGCodeLinesOutput++, dCurZ, dRetractSpeed);
	fputs (LineToOutput, pFile);

	sprintf (chReport_GC, "Completed %s GCode, number lines output: %d", pDesc, m_nGCodeLinesOutput);
	ReportBasedOnVerbosity(1, chReport_GC);
 
	return true;
}

bool GCode_Output::CalculateCircleCircleIntersection(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2)
{
	double dX0, dY0, dX1, dY1, dR0, dR1;

	dX0 = pCircle1->dXc;
	dY0 = pCircle1->dYc;
	dR0 = pCircle1->dRadius;

	dX1 = pCircle2->dXc;
	dY1 = pCircle2->dYc;
	dR1 = pCircle2->dRadius;

	// 2/26/2011 - Make centers equal if they are sufficiently close
	double dHorizDiff = fabs (dY1 - dY0);
	if (dHorizDiff < EQUIVALENCY_EPSILON)
	{
		dY0 = (dY0 + dY1)/2.0;
		dY1 = dY0;
	}

	double dVertDiff = fabs (dX1 - dX0);
	if (dVertDiff < EQUIVALENCY_EPSILON)
	{
		dX0 = (dX0 + dX1)/2.0;
		dX1 = dX0;
	}

//	dX0 = m_dX0;
//	dY0 = m_dY0;
//	dX1 = m_dX1;
//	dY1 = m_dY1;
//	dR0 = m_dR0;
//	dR1 = m_dR1;

// Test case that yields (-1.0, 9.0) & (7.0, 5.0)
//	dX0 = 2.0;
//	dY0 = 5.0;
//	dX1 = 6.0;
//	dY1 = 13.0;
//	dR0 = 5.0;
//	dR1 = sqrt (65.0);

	// Make certain intersection is possible
	double dDistBetweenCenters = sqrt ((dX1-dX0)*(dX1-dX0) + (dY1-dY0)*(dY1-dY0));
	double dSumOfRadius = dR0 + dR1;

	if ((dX1==dX0) && (dY1==dY0))
	{
		PostReport ("Circles are on center\n");
		assert(false);
		return false;
	}

	if (dSumOfRadius < dDistBetweenCenters)
	{
		PostReport ("Circles are non-intersecting (too separated)\n");
		double dVrr_X, dVrr_Y;				// Vector from center of C1 to center of C2.
		double duVrr_X, duVrr_Y;			// Unit vector based on Vrr
		POINT_OBJECT tPtC1, tPtC2;			// Point of interest on C1 & C2.

		// Vector from center of C1 to center of C2.
		dVrr_X = dX1 - dX0;
		dVrr_Y = dY1 - dY0;
	 	// Unit vector of Vrr.
		duVrr_X = dVrr_X / dDistBetweenCenters;
		duVrr_Y = dVrr_Y / dDistBetweenCenters;
		// Subtended intersection point on C1.
		tPtC1.dX = dX0 + (dR0 * duVrr_X);
		tPtC1.dY = dY0 + (dR0 * duVrr_Y);
		// Subtended intersection point on C2.
		tPtC2.dX = dX1 - (dR1 * duVrr_X);
		tPtC2.dY = dY1 - (dR1 * duVrr_Y);

		double dDistBetweenPoints = sqrt ((tPtC2.dX-tPtC1.dX)*(tPtC2.dX-tPtC1.dX) + (tPtC2.dY-tPtC1.dY)*(tPtC2.dY-tPtC1.dY));
		if (dDistBetweenPoints < 1.0E-3)
		{
			// Use midpoint
			pPoint1->dX = (tPtC2.dX+tPtC1.dX) / 2.0;
			pPoint1->dY  = (tPtC2.dY+tPtC1.dY) / 2.0;
			pPoint2->dX  = pPoint1->dX;
			pPoint2->dY  = pPoint1->dY;
			return true;
		}
		else
		{
			PostReport ("Tangential intersections did not meet acceptance ctiteria\n");
			return false;
		}
	}

	double dDiffOfRadius = fabs (dR0 - dR1);
	if (dDistBetweenCenters < dDiffOfRadius)
	{
		PostReport ("Circles are non-intersecting (self contained)\n");
		// Which circle is contained ?
		double dVrr_X, dVrr_Y;				// Vector from center of larger to center of smaller circle.
		double duVrr_X, duVrr_Y;			// Unit vector based on Vrr
		POINT_OBJECT tPtC1, tPtC2;			// Point of interest on C1 & C2.

		if (dR0 < dR1)
		{
			// Circle1 is within Circle2
			dVrr_X = dX0 - dX1;
			dVrr_Y = dY0 - dY1;
	 	}
		else
		{
			dVrr_X = dX1 - dX0;
			dVrr_Y = dY1 - dY0;
		}
		// Unit vector from center of larger to center of smaller circle.
		duVrr_X = dVrr_X / dDistBetweenCenters;
		duVrr_Y = dVrr_Y / dDistBetweenCenters;
		// Probable intersection point on C1.
		tPtC1.dX = dX0 + (dR0 * duVrr_X);
		tPtC1.dY = dY0 + (dR0 * duVrr_Y);
		// Probable intersection point on C2.
		tPtC2.dX = dX1 + (dR1 * duVrr_X);
		tPtC2.dY = dY1 + (dR1 * duVrr_Y);

		double dDistBetweenPoints = sqrt ((tPtC2.dX-tPtC1.dX)*(tPtC2.dX-tPtC1.dX) + (tPtC2.dY-tPtC1.dY)*(tPtC2.dY-tPtC1.dY));
		if (dDistBetweenPoints < 1.0E-3)
		{
			// Use midpoint
			pPoint1->dX = (tPtC2.dX+tPtC1.dX) / 2.0;
			pPoint1->dY  = (tPtC2.dY+tPtC1.dY) / 2.0;
			pPoint2->dX  = pPoint1->dX;
			pPoint2->dY  = pPoint1->dY;
			return true;
		}
		else
		{
			PostReport ("Tangential intersections did not meet acceptance ctiteria\n");
			return false;
		}
	}

	double dA = -2.0 * dX0;
	double dB = -2.0 * dY0;
	double dC = (dX0 * dX0) + (dY0 * dY0) - (dR0 * dR0);
	double dAA = -2.0 * dX1;
	double dBB = -2.0 * dY1;
	double dCC = (dX1 * dX1) + (dY1 * dY1) - (dR1 * dR1);

	// Horizontal circles reduce to the following;
	if (dY0 == dY1)
	{
		double dX_1 = (dC - dCC) / (dAA - dA);
		double dX_2 = dX_1;

		double dTermA = 1.0;
		double dTermB = dB;
		double dTermC = (dX_1 * dX_1) + (dA * dX_1) + dC;

		double dTemp0 = (dTermB * dTermB) - (4.0 * dTermA * dTermC);
#if 0
		double dTemp1 = sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC));
		double dTemp2 = -dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC));
		double dTemp3 = dTemp2 / ((2.0 * dTermA));
#endif
		double dY_1, dY_2;
		if (dTemp0 < 0.0)
		{
			if (-1.0E-10 < dTemp0)
			{
				dY_1 = -dTermB / (2.0 * dTermA);
				dY_2 = dY_1;
			}
			else
				assert(false);
		}
		else
		{
			dY_1 = (-dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
			dY_2 = (-dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
		}


#if 0
		if (isnan(dTemp1))
			assert(false);
		if (isnan(dTemp2))
			assert(false);
		if (isnan(dTemp3))
			assert(false);
#endif

//		double dY_1 = (-dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
//		double dY_2 = (-dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);

		if (isnan(dY_1))
			assert(false);
		if (isnan(dY_2))
			assert(false);

		pPoint1->dX = dX_1;
		pPoint1->dY = dY_1;
		pPoint2->dX = dX_2;
		pPoint2->dY = dY_2;
		return true;
	}

	// Vertical circles reduce to the following;
	if (dX0 == dX1)
	{
		double dY_1 = (dC - dCC) / (dBB - dB);
		double dY_2 = dY_1;

		double dTermA = 1.0;
		double dTermB = dA;
		double dTermC = (dY_1 * dY_1) + (dB * dY_1) + dC;
		double dX_1 = (-dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
		double dX_2 = (-dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);

		pPoint1->dX = dX_1;
		pPoint1->dY = dY_1;
		pPoint2->dX = dX_2;
		pPoint2->dY = dY_2;
		return true;
	}


	double dApp = (dA - dAA) / (dBB - dB);
	double dBpp = (dC - dCC) / (dBB - dB);

	double dTermA = 1.0 + (dApp * dApp);
	double dTermB = dA + (2.0 * dApp * dBpp) + (dApp * dB);
	double dTermC = (dBpp * dBpp) + (dB * dBpp) + dC;

	double dRadicalTerm = (dTermB * dTermB) - (4.0 * dTermA * dTermC );

	double dX_1 = (-dTermB + sqrt(dRadicalTerm)) / (2.0 * dTermA);
//	double dX_1 = (-dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
	double dY_1 = (dApp * dX_1) + dBpp;

	double dX_2 = (-dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
	double dY_2 = (dApp * dX_2) + dBpp;

 	pPoint1->dX = dX_1;
	pPoint1->dY = dY_1;
	pPoint2->dX = dX_2;
	pPoint2->dY = dY_2;

	return true;
}

bool GCode_Output::CalculateCircleCircleIntersection_II(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2)
{
	double dX0, dY0, dX1, dY1, dR0, dR1;

	dX0 = pCircle1->dXc;
	dY0 = pCircle1->dYc;
	dR0 = pCircle1->dRadius;

	dX1 = pCircle2->dXc;
	dY1 = pCircle2->dYc;
	dR1 = pCircle2->dRadius;

	// Make certain circles are not concentric
	if ((dX1==dX0) && (dY1==dY0))
	{
		ReportBasedOnVerbosity(-1, "Circles are on center\n");
		assert(false);
		return false;
	}

	// Make certain intersection is possible
	double dDistBetweenCenters = sqrt ((dX1-dX0)*(dX1-dX0) + (dY1-dY0)*(dY1-dY0));
	double dSumOfRadius = dR0 + dR1;
	double dDiffOfRadius = fabs (dR0 - dR1);
	double dDelta = dDistBetweenCenters - dSumOfRadius;

	if (fabs(dDelta) <= CIRCLE_PROXIMITY)
	{
		double dVrr_X, dVrr_Y;				// Vector from center of C1 to center of C2.
		double duVrr_X, duVrr_Y;			// Unit vector based on Vrr
		POINT_OBJECT tPtC1, tPtC2;			// Point of interest on C1 & C2.

		// Vector from center of C1 to center of C2.
		dVrr_X = dX1 - dX0;
		dVrr_Y = dY1 - dY0;
		// Unit vector of Vrr.
		duVrr_X = dVrr_X / dDistBetweenCenters;
		duVrr_Y = dVrr_Y / dDistBetweenCenters;

		// Subtended intersection point on C1 & C2.
		tPtC1.dX = dX0 + (dR0 * duVrr_X);
		tPtC1.dY = dY0 + (dR0 * duVrr_Y);
		tPtC2.dX = dX1 - (dR1 * duVrr_X);
		tPtC2.dY = dY1 - (dR1 * duVrr_Y);

		if (0.0 == dDelta)
		{
			ReportBasedOnVerbosity(2, "CalculateCircleCircleIntersection_II(), circles are tangent to each other.");
			pPoint1->dX = tPtC1.dX;
			pPoint1->dY = tPtC1.dY;
			pPoint2->dX = tPtC2.dX;
			pPoint2->dY = tPtC2.dY;
			return true;
		}
		else
		{
			ReportBasedOnVerbosity(2, "CalculateCircleCircleIntersection_II(), circles likely tangent to each other.");
			// Choose mid-point of these points
			pPoint1->dX = (tPtC2.dX+tPtC1.dX) / 2.0;
			pPoint1->dY  = (tPtC2.dY+tPtC1.dY) / 2.0;
			pPoint2->dX  = pPoint1->dX;
			pPoint2->dY  = pPoint1->dY;

	//		assert(false);
	//		return false;
			return true;
		}
	}
	else if (dSumOfRadius < dDistBetweenCenters)
	{
		ReportBasedOnVerbosity(-1, "Circles are non-intersecting (too separated)\n");
		assert(false);
		return false;
	}
	else if (dDistBetweenCenters < dDiffOfRadius)
	{
		ReportBasedOnVerbosity(-1, "Circles are non-intersecting (self contained)\n");
		assert(false);
		return false;
	}

#if 0

	double dDistBetweenCenters = sqrt ((dX1-dX0)*(dX1-dX0) + (dY1-dY0)*(dY1-dY0));
	double dSumOfRadius = dR0 + dR1;
	double dDelta = dDistBetweenCenters - dSumOfRadius;

	if (fabs(dDelta) <= EQUIVALENCY_EPSILON)
	{
		if (0.0 == dDelta)
		{
			ReportBasedOnVerbosity(2, "Circles are tangent to each other.\n");
			double dVrr_X, dVrr_Y;				// Vector from center of C1 to center of C2.
			double duVrr_X, duVrr_Y;			// Unit vector based on Vrr
			POINT_OBJECT tPtC1, tPtC2;			// Point of interest on C1 & C2.

			// Vector from center of C1 to center of C2.
			dVrr_X = dX1 - dX0;
			dVrr_Y = dY1 - dY0;
			// Unit vector of Vrr.
			duVrr_X = dVrr_X / dDistBetweenCenters;
			duVrr_Y = dVrr_Y / dDistBetweenCenters;

			// Subtended intersection point on C1.
			pPoint1->dX = dX0 + (dR0 * duVrr_X);
			pPoint1->dY = dY0 + (dR0 * duVrr_Y);
			// Subtended intersection point on C2.
			pPoint2->dX = dX1 - (dR1 * duVrr_X);
			pPoint2->dY = dY1 - (dR1 * duVrr_Y);
			return true;
		}
		ReportBasedOnVerbosity(-1, "Circles are likely tangent to each other.\n");
		assert(false);
		return false;
	}

	if (dSumOfRadius < dDistBetweenCenters)
	{
		ReportBasedOnVerbosity(-1, "Circles are non-intersecting (too separated)\n");
		assert(false);
		return false;
	}
	double dDiffOfRadius = fabs (dR0 - dR1);
	if (dDistBetweenCenters < dDiffOfRadius)
	{
		ReportBasedOnVerbosity(-1, "Circles are non-intersecting (self contained)\n");
		assert(false);
		return false;
	}
#endif

	double dC3 = (dR0*dR0) - (dR1*dR1) + (dX1*dX1) + (dY1*dY1) - (dX0*dX0) - (dY0*dY0);

	// C0 = 2(X1 - X0)
	// C1 = 2(Y1 - Y0)
	double dC0 = 2.0 * (dX1 - dX0);
	double dC1 = 2.0 * (dY1 - dY0);

	// Determine which constant's denominator is larger
	if (fabs(dX1 - dX0) < fabs(dY1 -dY0))
	{
		// Solve for x, y = (C3 - C0*x) / C1
		double dTermA = 1.0 + ((dC0 * dC0) / (dC1 * dC1));

		double dTemp1 = (2.0 * dY0 * dC0) / dC1;
		double dTemp2 = (2.0 * dC0 * dC3) / (dC1 * dC1);
		double dTemp3 = 2.0 * dX0;
		double dTermB = dTemp1 - dTemp2 - dTemp3;

		double dC4 = (dC3 * dC3) / (dC1 * dC1);
		double dC5 = (2.0 * dY0 * dC3) / dC1;
		double dC6 = (dX0*dX0) + (dY0*dY0) - (dR0*dR0);
		double dTermC = dC4 - dC5 + dC6;

		// Solve via the quadratic equation
		if (((dTermB * dTermB) - (4.0 * dTermA * dTermC)) < 0.0)
		{
			assert (false);
		}
		double dTemp4 = -dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC));
		double dTemp5 = -dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC));
		double d_X1 = dTemp4 / ((2.0 * dTermA));
		double d_X2 = dTemp5 / ((2.0 * dTermA));

		double d_Y1 = (dC3 - (dC0 * d_X1)) / dC1;
		double d_Y2 = (dC3 - (dC0 * d_X2)) / dC1;

		pPoint1->dX = d_X1;
		pPoint1->dY = d_Y1;
		pPoint2->dX = d_X2;
		pPoint2->dY = d_Y2;

		return true;
	}
	else
	{
		// Solve for Y, x = (C3 - C1*y) / C0
		double dTermA = 1.0 + ((dC1 * dC1) / (dC0 * dC0));

		double dTemp1 = (2.0 * dX0 * dC1) / dC0;
		double dTemp2 = (2.0 * dC1 * dC3) / (dC0 * dC0);
		double dTemp3 = 2.0 * dY0;
		double dTermB = dTemp1 - dTemp2 - dTemp3;

		double dC4 = (dC3 * dC3) / (dC0 * dC0);
		double dC5 = (2.0 * dX0 * dC3) / dC0;
		double dC6 = (dX0*dX0) + (dY0*dY0) - (dR0*dR0);
		double dTermC = dC4 - dC5 + dC6;

		// Solve via the quadratic equation
		if (((dTermB * dTermB) - (4.0 * dTermA * dTermC)) < 0.0)
		{
			assert (false);
		}
		double dTemp4 = -dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC));
		double dTemp5 = -dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC));
		double d_Y1 = dTemp4 / ((2.0 * dTermA));
		double d_Y2 = dTemp5 / ((2.0 * dTermA));

		double d_X1 = (dC3 - (dC1 * d_Y1)) / dC0;
		double d_X2 = (dC3 - (dC1 * d_Y2)) / dC0;

		pPoint1->dX = d_X1;
		pPoint1->dY = d_Y1;
		pPoint2->dX = d_X2;
		pPoint2->dY = d_Y2;

		return true;
	}
 }


bool GCode_Output::PostReport(const char *chReport)
{
	cout << chReport << std::endl;
	return true;
}

bool GCode_Output::ReportBasedOnVerbosity(int nVerbosity, const char *chReport)
{
	// INI values range between 0 and 5
	// 1 <= nVerbosity <= 5
	if (nVerbosity <= INI_Verbosity_GCode_Output)
	{
		cout << chReport << std::endl;
		return true;
	}
	else
		return false;
}

bool GCode_Output::VerifyArcCenterPoint(ARC_DATA *tArcData, double dAbsError, double dRelError)
{
	double dX1, dY1, dX2, dY2, dRx, dRy;

	dX1 = tArcData->dX1;
	dY1 = tArcData->dY1;
	dX2 = tArcData->dX2;
	dY2 = tArcData->dY2;
	dRx = tArcData->dRx;
	dRy = tArcData->dRy;

	double dCalcRadius1 = sqrt ((dX1-dRx)*(dX1-dRx) + (dY1-dRy)*(dY1-dRy));
	double dCalcRadius2 = sqrt ((dX2-dRx)*(dX2-dRx) + (dY2-dRy)*(dY2-dRy));
	double dRadiusDelta = dCalcRadius2 - dCalcRadius1;
	double dRadiusAbsDelta = fabs(dRadiusDelta);

	bool bRetStatus = true;
	if (dAbsError < dRadiusAbsDelta)
	{
		PostReport("GCode_Output::VerifyArcCenterPoint()...Absolute error outside bounds");
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Abs Err: %3.6E", tArcData->lDxfEntityNumber, dRadiusAbsDelta);
		ReportBasedOnVerbosity(-1, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Ax: %lf, Ay: %lf", tArcData->lDxfEntityNumber, tArcData->dX1, tArcData->dY1);
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Bx: %lf, By: %lf", tArcData->lDxfEntityNumber, tArcData->dX2, tArcData->dY2);
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Rx: %lf, Ry: %lf", tArcData->lDxfEntityNumber, tArcData->dRx, tArcData->dRy);
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Radius1: %lf",  tArcData->lDxfEntityNumber, dCalcRadius1);
		ReportBasedOnVerbosity(-1, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Radius2: %lf",  tArcData->lDxfEntityNumber,dCalcRadius2);
		ReportBasedOnVerbosity(-1, chReport_GC);
		bRetStatus = false;
	}

	if (dRelError < (dRadiusAbsDelta / ((dCalcRadius1+dCalcRadius2)/2.0)))
	{
		PostReport("GCode_Output::VerifyArcCenterPoint()...Relative error outside bounds");
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Rel Err: %3.6E", tArcData->lDxfEntityNumber, (dRadiusAbsDelta / ((dCalcRadius1+dCalcRadius2)/2.0)));
		ReportBasedOnVerbosity(-1, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Ax: %lf, Ay: %lf", tArcData->lDxfEntityNumber, tArcData->dX1, tArcData->dY1);
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Bx: %lf, By: %lf", tArcData->lDxfEntityNumber, tArcData->dX2, tArcData->dY2);
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Rx: %lf, Ry: %lf", tArcData->lDxfEntityNumber, tArcData->dRx, tArcData->dRy);
		ReportBasedOnVerbosity(2, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Radius1: %lf",  tArcData->lDxfEntityNumber, dCalcRadius1);
		ReportBasedOnVerbosity(-1, chReport_GC);
		sprintf (chReport_GC, "GCode_Output::VerifyArcCenterPoint(DXF# %ld)... Radius2: %lf",  tArcData->lDxfEntityNumber,dCalcRadius2);
		ReportBasedOnVerbosity(-1, chReport_GC);
		bRetStatus = false;
	}

	return bRetStatus;
}


