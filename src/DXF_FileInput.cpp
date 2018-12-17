#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "enumsBase.h"
#include "STL_VectorBase.h"
#include "DXF_FileInput.h"

using namespace std;

char		chReport[400];

extern int 	INI_Verbosity_DXF_Input;

DXF_FILE_IO_CLASS::DXF_FILE_IO_CLASS()
{
	m_szLineLen = 255;
	m_pDxfFile = NULL;
	m_pOutputFile = NULL;
	m_lDxfLineCount = 0l;
	m_Vector = 0;
	m_eDxfVersion = UNDEFINED;
	m_bEntitiesFieldFound = 0;

	m_lDxfArcsExtracted = 0l;
	m_lDxfLinesExtracted = 0l;
	m_lDxfCirclesExtracted = 0l;

	m_pJunkData = (char *)malloc(m_szLineLen+1);
	if (NULL == m_pJunkData)
	{
		cout << "DXF_FILE_IO_CLASS() - malloc() failed!" << std::endl;
	}
}

int DXF_FILE_IO_CLASS::DXF_IO_OpenFile(const char *pFile)
{
	m_pDxfFile = fopen(pFile, "r");
	if (!m_pDxfFile)
		return 0;
	else
		return 1;
}

int DXF_FILE_IO_CLASS::DXF_IO_SkipLines (int nLinesToSkip, long *pLineCount)
{
	for (int nLines=0; nLines < nLinesToSkip; nLines++)
	{
		getline (&m_pJunkData, &m_szLineLen, m_pDxfFile);
		(*pLineCount)++;
	}
	return nLinesToSkip;
}

long DXF_FILE_IO_CLASS::DXF_IO_SeekToLine (const char *pData, long lMaxLines, long *pLineCount)
{
	long 		lLinesTaken = 0;
	std::string strData;
	while ((0 < getline (&m_pJunkData, &m_szLineLen, m_pDxfFile)) && (lLinesTaken++ < lMaxLines))
	{
		(*pLineCount)++;
		strData.assign(m_pJunkData);
		strData.erase(strData.find('\n'));
		if (std::string::npos != strData.find(pData, 0))
			return 	lLinesTaken;
	};
	return lLinesTaken;
}

int DXF_FILE_IO_CLASS::DXF_IO_InputParameterLong (const char *pFormat, long *pLongParam, const char *pLine)
{
	int nFieldsScanned = sscanf (pLine, pFormat, pLongParam);
	if (1 != nFieldsScanned)
	{
		cout << "FAULT - looking for long parameter in string" << std::endl;
		return 0;
	}
	return 1;
}

int DXF_FILE_IO_CLASS::DXF_IO_InputParameterString (const char *pFormat, char *pDestString, size_t sizeDestString, const char *pSrcLine)
{
	int nFieldsScanned = sscanf (pSrcLine, pFormat, pDestString, sizeDestString);
	if (1 != nFieldsScanned)
	{
		cout << "FAULT - looking for strig parameter in string" << std::endl;
		return 0;
	}
	return 1;
}

int DXF_FILE_IO_CLASS::DXF_IO_InputParameterDouble (double *pDoubleVal, const char *pLine)
{
	int nFieldsScanned = sscanf (pLine, "%lf", pDoubleVal);
	if (1 != nFieldsScanned)
	{
		cout << "FAULT - looking for double parameter in string: " << pLine << std::endl;
		return 0;
	}
	return 1;
}

int DXF_FILE_IO_CLASS::DXF_IO_ReadFile(int n, class STL_VECTOR_CLASS *pInsideVectorClass, class STL_VECTOR_CLASS *pOutsideVectorClass, class STL_VECTOR_CLASS *pOnVectorClass)
{
	std::string 	strMyData;
	std::string 	strMyData2("myMy");
	char  			*pLineData;
	size_t			szLineLen = 255;
//	char			chReport[200];

	UNCONNECTED_ELEMENT 	tUnconnectedSegment;

	if (!m_pDxfFile)
		return 0;

	pLineData = (char *)malloc(szLineLen+1);
	if (NULL == pLineData)
	{
		cout << "DXF_IO_ReadFile() - malloc() failed!" << std::endl;
		return -1;
	}

	m_lCurrentOutsideSegmentCount = 0;
	m_lCurrentInsideSegmentCount = 0;
	m_lCurrentOnSegmentCount = 0;

	m_lCurrentOnSegmentArcCount = 0;
	m_lCurrentOnSegmentLineCount = 0;
	m_lCurrentOnSegmentCircleCount = 0;
	m_lCurrentInSegmentArcCount = 0;
	m_lCurrentInSegmentLineCount = 0;
	m_lCurrentInSegmentCircleCount = 0;
	m_lCurrentOutSegmentArcCount = 0;
	m_lCurrentOutSegmentLineCount = 0;
	m_lCurrentOutSegmentCircleCount = 0;

 	// Remove all segments from the INSIDE vector array before proceeding
	int nInitialCount = pInsideVectorClass->SVC_Size();
	sprintf (chReport, "DXF Cut INSIDE unconnected array, initial count: %d", nInitialCount);
	ReportBasedOnVerbosity (5, chReport);
	pInsideVectorClass->SVC_RemoveAllElements();
	int nCurrentCount = pInsideVectorClass->SVC_Size();
	sprintf (chReport, "DXF Cut INSIDE unconnected array, post-RemoveAll count: %d", nCurrentCount);
	ReportBasedOnVerbosity (5, chReport);

	// Remove all segments from the OUTSIDE vector array before proceeding
	nInitialCount = pOutsideVectorClass->SVC_Size();
	sprintf (chReport, "DXF Cut OUTSIDE unconnected array, initial count: %d", nInitialCount);
	ReportBasedOnVerbosity (5, chReport);
	pOutsideVectorClass->SVC_RemoveAllElements();
	nCurrentCount = pOutsideVectorClass->SVC_Size();
	sprintf (chReport, "DXF Cut OUTSIDE unconnected array, post-RemoveAll count: %d", nCurrentCount);
	ReportBasedOnVerbosity (5, chReport);

	// Remove all segments from the ON vector array before proceeding
	nInitialCount = pOnVectorClass->SVC_Size();
	sprintf (chReport, "DXF Cut ON unconnected array, initial count: %d", nInitialCount);
	ReportBasedOnVerbosity (5, chReport);
	pOnVectorClass->SVC_RemoveAllElements();
	nCurrentCount = pOnVectorClass->SVC_Size();
	sprintf (chReport, "DXF Cut ON unconnected array, post-RemoveAll count: %d", nCurrentCount);
	ReportBasedOnVerbosity (5, chReport);

	ReportBasedOnVerbosity (2, "DXF_IO_ReadFile() - Starting to read file");

	char chAcadVer[20];

	while ((m_lDxfLineCount<n) && (0 < getline(&pLineData, &szLineLen, m_pDxfFile)))
	{
		m_lDxfLineCount++;
		strMyData.assign(pLineData);
		strMyData.erase(strMyData.find('\n'));
		sprintf (chReport, "DXF_IO_ReadFile(%ld) - Line: %s", m_lDxfLineCount, strMyData.data());
		ReportBasedOnVerbosity (8, chReport);

		if ((m_eDxfVersion == UNDEFINED) && (std::string::npos != strMyData.find("$ACADVER", 0)))
		{
			// Unused line
			DXF_IO_SkipLines (1, &m_lDxfLineCount);
		
			// ACAD version info
			getline (&pLineData, &szLineLen, m_pDxfFile);
			m_lDxfLineCount++;
			strMyData.assign(pLineData);
			sprintf (chReport, "DXF_IO_ReadFile(%ld) - ACAD version: %s", m_lDxfLineCount, strMyData.data());
			ReportBasedOnVerbosity (4, chReport);
			if (1 == sscanf (pLineData, "%s", chAcadVer))
			{
				if (0 == strncmp (chAcadVer, "AC1014", 6))
				{
					m_eDxfVersion = ACAD_14;
					ReportBasedOnVerbosity (3, "ACAD version: ACAD_14");
				}
				else if (0 == strncmp (chAcadVer, "AC1015", 6))
				{
					m_eDxfVersion = ACAD_15;
					ReportBasedOnVerbosity (3, "ACAD version: AC1015");
				}
				else
				{
					m_eDxfVersion = UNKNOWN;
					ReportBasedOnVerbosity (3, "ACAD version: UNKNOWN");
				}
			}
		}

		// Look for "ENTITIES" start
		if (!m_bEntitiesFieldFound && (std::string::npos != strMyData.find("ENTITIES", 0)))
		{
			m_bEntitiesFieldFound = 1;
			ReportBasedOnVerbosity (4, "ENTITIES field found");
		}

		if (m_bEntitiesFieldFound)
		{
			// Look for "ENDSEC" designator
			if (std::string::npos != strMyData.find("ENDSEC", 0))
			{
				m_bEntitiesFieldFound = 0;
				ReportBasedOnVerbosity (4, "ENTITIES-ENDSEC field found");
		 		break;
			}
		}

		if (m_bEntitiesFieldFound)
		{
			long lEntityNumber;
			char chLayerInfo[50];
			double dValue;

			if ((std::string::npos != strMyData.find("POLYLINE", 0)) && (std::string::npos == strMyData.find("LWPOLYLINE", 0)))
			{
				if (m_eDxfVersion == UNDEFINED)
				{
					// TBD
				}
				else if (m_eDxfVersion == ACAD_14)
				{
 			
				}
			}
			else if (std::string::npos != strMyData.find("LWPOLYLINE", 0))
			{
				if (m_eDxfVersion == UNDEFINED)
				{
					// TBD
				}
				else if (m_eDxfVersion == ACAD_14)
				{
 				}
			}
			else if (std::string::npos != strMyData.find("LINE", 0))
			{
				if (m_eDxfVersion == UNDEFINED)
				{
					// TBD
			 	}
				else if (m_eDxfVersion == ACAD_14)
				{
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline (&pLineData, &szLineLen, m_pDxfFile);	// Entity Line #
					m_lDxfLineCount++;
				//	strMyData.assign(pLineData);

 					if (!DXF_IO_InputParameterLong ("%lX", &lEntityNumber, pLineData))
					{
						cout << "FAULT - looking for entity number, found: " << pLineData << std::endl;
						return 0;
					}

 					sprintf (chReport, "File line # =  %ld", m_lDxfLineCount-2);
 					ReportBasedOnVerbosity (3, chReport);

 					sprintf (chReport, "LINE - entity# =  %ld", lEntityNumber);
 					ReportBasedOnVerbosity (3, chReport);

					// Search fo pre-cursor to LAYER field
				//	DXF_IO_SeekToLine ("AcDbLine", 100, &m_lDxfLineCount);					
					// Skip 3 more lines to be in position for LAYER info
					DXF_IO_SkipLines (3, &m_lDxfLineCount);
					
					// Layer info (CUT type)
					getline(&pLineData, &szLineLen, m_pDxfFile);
					m_lDxfLineCount++;
					if (!DXF_IO_InputParameterString ("%s", chLayerInfo, sizeof(chLayerInfo), pLineData))
					{
						cout << "FAULT - looking for layer info in line:" << pLineData << std::endl;
						return -1;
					}
					if (0 == strncmp (chLayerInfo, "CUT_OUTSIDE", 11))
					{
						ReportBasedOnVerbosity (3, "LINE - layer# = CUT_OUTSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutOutsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_INSIDE", 10))
					{
						ReportBasedOnVerbosity (3, "LINE - layer# = CUT_INSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutInsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_ON", 7))
					{
						ReportBasedOnVerbosity (3, "LINE - layer# = CUT_ON");
						tUnconnectedSegment.eFeatureCutType = eCutOnFeature;
					}
					else
					{
						sprintf (chReport, "LINE - layer# = %s", chLayerInfo);
						ReportBasedOnVerbosity (3, chReport);
						tUnconnectedSegment.eFeatureCutType = eCutUnknown;
					}

					// Search for pre-cursor to coordinate field
		//			DXF_IO_SeekToLine ("ByLayer", 100, &m_lDxfLineCount);
					// Skip 7 more line to be in position for coordinate info
					DXF_IO_SkipLines (7, &m_lDxfLineCount);
 								
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line X1
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - X1 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dAx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line Y1
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - Y1 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dAy = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line X2
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - X2 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dBx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line Y2
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - Y2 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dBy = dValue;

					m_lDxfLinesExtracted++;

					tUnconnectedSegment.lFeatureGroupIndex = -1;
					tUnconnectedSegment.lDxfEntityNumber = lEntityNumber;
					tUnconnectedSegment.eSegmentType = eLine;

					if (eCutOutsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
						m_lCurrentOutsideSegmentCount++;
						m_lCurrentOutSegmentLineCount++;
						if (1 != pOutsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOutsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutInsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
						m_lCurrentInsideSegmentCount++;
						m_lCurrentInSegmentLineCount++;
						if (1 != pInsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pInsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutOnFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOnSegmentCount++;
						m_lCurrentOnSegmentLineCount++;
						if (1 != pOnVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOnVectorClass->SVC_ReportStatus();
					}
					else
						ReportBasedOnVerbosity (3, "!!! NOT adding line element due existence on unqualified layer !!!");
 				}
				else if (m_eDxfVersion == ACAD_15)
				{
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline (&pLineData, &szLineLen, m_pDxfFile);	// Entity Line #
					m_lDxfLineCount++;
				//	strMyData.assign(pLineData);

 					if (!DXF_IO_InputParameterLong ("%lX", &lEntityNumber, pLineData))
					{
						cout << "FAULT - looking for entity number, found: " << pLineData << std::endl;
						return 0;
					}

 					sprintf (chReport, "File line # =  %ld", m_lDxfLineCount-2);
 					ReportBasedOnVerbosity (3, chReport);

 					sprintf (chReport, "LINE - entity# =  %ld", lEntityNumber);
 					ReportBasedOnVerbosity (3, chReport);

					// Search fo pre-cursor to LAYER field
					DXF_IO_SeekToLine ("AcDbLine", 100, &m_lDxfLineCount);					
					// Skip one more line to be in position for LAYER info
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					
					// Layer info (CUT type)
					getline(&pLineData, &szLineLen, m_pDxfFile);
					m_lDxfLineCount++;
					if (!DXF_IO_InputParameterString ("%s", chLayerInfo, sizeof(chLayerInfo), pLineData))
					{
						cout << "FAULT - looking for layer info in line:" << pLineData << std::endl;
						return -1;
					}
					if (0 == strncmp (chLayerInfo, "CUT_OUTSIDE", 11))
					{
						ReportBasedOnVerbosity (3, "LINE - layer# = CUT_OUTSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutOutsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_INSIDE", 10))
					{
						ReportBasedOnVerbosity (3, "LINE - layer# = CUT_INSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutInsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_ON", 7))
					{
						ReportBasedOnVerbosity (3, "LINE - layer# = CUT_ON");
						tUnconnectedSegment.eFeatureCutType = eCutOnFeature;
					}
					else
					{
						sprintf (chReport, "LINE - layer# = %s", chLayerInfo);
						ReportBasedOnVerbosity (3, chReport);
						tUnconnectedSegment.eFeatureCutType = eCutUnknown;
					}

					// Search for pre-cursor to coordinate field
		//			DXF_IO_SeekToLine ("ByLayer", 100, &m_lDxfLineCount);
					// Skip 7 more line to be in position for coordinate info
					DXF_IO_SkipLines (7, &m_lDxfLineCount);
 								
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line X1
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - X1 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dAx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line Y1
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - Y1 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dAy = dValue;

					DXF_IO_SkipLines (3, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line X2
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - X2 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dBx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Line Y2
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "LINE - Y2 =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dBy = dValue;

					m_lDxfLinesExtracted++;

					tUnconnectedSegment.lFeatureGroupIndex = -1;
					tUnconnectedSegment.lDxfEntityNumber = lEntityNumber;
					tUnconnectedSegment.eSegmentType = eLine;

					if (eCutOutsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
						m_lCurrentOutsideSegmentCount++;
						m_lCurrentOutSegmentLineCount++;
						if (1 != pOutsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOutsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutInsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
						m_lCurrentInsideSegmentCount++;
						m_lCurrentInSegmentLineCount++;
						if (1 != pInsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pInsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutOnFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOnSegmentCount++;
						m_lCurrentOnSegmentLineCount++;
						if (1 != pOnVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOnVectorClass->SVC_ReportStatus();
					}
					else
						ReportBasedOnVerbosity (3, "!!! NOT adding line element due existence on unqualified layer !!!");
				}
			}
			else if (std::string::npos != strMyData.find("ARC", 0))
			{
				if (m_eDxfVersion == UNDEFINED)
				{
					// TBD
			 	}
				else if (m_eDxfVersion == ACAD_14)
				{
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline (&pLineData, &szLineLen, m_pDxfFile);	// Entity Line #
					m_lDxfLineCount++;
				//	strMyData.assign(pLineData);

 					if (!DXF_IO_InputParameterLong ("%lX", &lEntityNumber, pLineData))
 					{
 						cout << "FAULT - looking for entity number, found: " << pLineData << std::endl;
 						return 0;
 					}

 					sprintf (chReport, "File line # =  %ld", m_lDxfLineCount-2);
 					ReportBasedOnVerbosity (3, chReport);

 					sprintf (chReport, "ARC - entity# =  %ld", lEntityNumber);
					ReportBasedOnVerbosity (3, chReport);

					DXF_IO_SkipLines (3, &m_lDxfLineCount);
 		
					// Layer info (CUT type)
					getline(&pLineData, &szLineLen, m_pDxfFile);
					m_lDxfLineCount++;
					if (!DXF_IO_InputParameterString ("%s", chLayerInfo, sizeof(chLayerInfo), pLineData))
					{
						cout << "FAULT - looking for layer info in line:" << pLineData << std::endl;
						return -1;
					}
					if (0 == strncmp (chLayerInfo, "CUT_OUTSIDE", 11))
					{
						ReportBasedOnVerbosity (3, "ARC - layer# = CUT_OUTSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutOutsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_INSIDE", 10))
					{
						ReportBasedOnVerbosity (3, "ARC - layer# = CUT_INSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutInsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_ON", 7))
					{
						ReportBasedOnVerbosity (3, "ARC - layer# = CUT_ON");
						tUnconnectedSegment.eFeatureCutType = eCutOnFeature;
					}
					else
					{
						sprintf (chReport, "ARC - layer# = %s", chLayerInfo);
						ReportBasedOnVerbosity (3, chReport);
						tUnconnectedSegment.eFeatureCutType = eCutUnknown;
					}

					// Search for pre-cursor to coordinate field
					DXF_IO_SeekToLine ("AcDbCircle", 100, &m_lDxfLineCount);					
					// Skip one more line to be in position for coordinate info
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
 								
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc center X
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc Rx =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc center Y
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc Ry =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRy = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc radius
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc Radius =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRadius = dValue;
					
					DXF_IO_SkipLines (3, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc start angle
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc start angle =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dStartAngle = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc end angle
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc end angle =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dEndAngle = dValue;

					m_lDxfArcsExtracted++;

					tUnconnectedSegment.lFeatureGroupIndex = -1;
					tUnconnectedSegment.lDxfEntityNumber = lEntityNumber;
					tUnconnectedSegment.eSegmentType = eArc;
					tUnconnectedSegment.dAx = tUnconnectedSegment.dRx + tUnconnectedSegment.dRadius * cos (tUnconnectedSegment.dStartAngle * M_PI / 180.0);
					tUnconnectedSegment.dAy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius * sin (tUnconnectedSegment.dStartAngle * M_PI / 180.0);
					tUnconnectedSegment.dBx = tUnconnectedSegment.dRx + tUnconnectedSegment.dRadius * cos (tUnconnectedSegment.dEndAngle * M_PI / 180.0);
					tUnconnectedSegment.dBy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius * sin (tUnconnectedSegment.dEndAngle * M_PI / 180.0);

					sprintf (chReport, "ARC - Calculated Start X = %f", tUnconnectedSegment.dAx);
					ReportBasedOnVerbosity (3, chReport);
					sprintf (chReport, "ARC - Calculated Start Y = %f", tUnconnectedSegment.dAy);
					ReportBasedOnVerbosity (3, chReport);
					sprintf (chReport, "ARC - Calculated End X = %f", tUnconnectedSegment.dBx);
					ReportBasedOnVerbosity (3, chReport);
					sprintf (chReport, "ARC - Calculated End Y = %f", tUnconnectedSegment.dBy);
					ReportBasedOnVerbosity (3, chReport);

					if (eCutOutsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOutsideSegmentCount++;
						m_lCurrentOutSegmentArcCount++;
						if (1 != pOutsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOutsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutInsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentInsideSegmentCount++;
						m_lCurrentInSegmentArcCount++;
						if (1 != pInsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pInsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutOnFeature == tUnconnectedSegment.eFeatureCutType)
					{
		 				m_lCurrentOnSegmentCount++;
						m_lCurrentOnSegmentArcCount++;
						if (1 != pOnVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOnVectorClass->SVC_ReportStatus();
					}
					else
						ReportBasedOnVerbosity (3, "!!! NOT adding arc element due existence on unqualified layer !!!");
 				}
				else if (m_eDxfVersion == ACAD_15)
				{
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline (&pLineData, &szLineLen, m_pDxfFile);	// Entity Line #
					m_lDxfLineCount++;
				//	strMyData.assign(pLineData);

 					if (!DXF_IO_InputParameterLong ("%lX", &lEntityNumber, pLineData))
 					{
 						cout << "FAULT - looking for entity number, found: " << pLineData << std::endl;
 						return 0;
 					}

 					sprintf (chReport, "File line # =  %ld", m_lDxfLineCount-2);
 					ReportBasedOnVerbosity (3, chReport);

 					sprintf (chReport, "ARC - entity# =  %ld", lEntityNumber);
					ReportBasedOnVerbosity (3, chReport);

					DXF_IO_SkipLines (3, &m_lDxfLineCount);
 		
					// Layer info (CUT type)
					getline(&pLineData, &szLineLen, m_pDxfFile);
					m_lDxfLineCount++;
					if (!DXF_IO_InputParameterString ("%s", chLayerInfo, sizeof(chLayerInfo), pLineData))
					{
						cout << "FAULT - looking for layer info in line:" << pLineData << std::endl;
						return -1;
					}
					if (0 == strncmp (chLayerInfo, "CUT_OUTSIDE", 11))
					{
						ReportBasedOnVerbosity (3, "ARC - layer# = CUT_OUTSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutOutsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_INSIDE", 10))
					{
						ReportBasedOnVerbosity (3, "ARC - layer# = CUT_INSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutInsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_ON", 7))
					{
						ReportBasedOnVerbosity (3, "ARC - layer# = CUT_ON");
						tUnconnectedSegment.eFeatureCutType = eCutOnFeature;
					}
					else
					{
						sprintf (chReport, "ARC - layer# = %s", chLayerInfo);
						ReportBasedOnVerbosity (3, chReport);
						tUnconnectedSegment.eFeatureCutType = eCutUnknown;
					}

					// Search for pre-cursor to coordinate field
					DXF_IO_SeekToLine ("AcDbCircle", 100, &m_lDxfLineCount);					
					// Skip one more line to be in position for coordinate info
					DXF_IO_SkipLines (1, &m_lDxfLineCount);
 								
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc radius X
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc Rx =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc radius Y
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc Ry =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRy = dValue;

					DXF_IO_SkipLines (3, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc radius
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc Radius =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRadius = dValue;
					
					DXF_IO_SkipLines (3, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc start angle
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc start angle =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dStartAngle = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Arc end angle
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "Arc end angle =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dEndAngle = dValue;

					m_lDxfArcsExtracted++;

					tUnconnectedSegment.lFeatureGroupIndex = -1;
					tUnconnectedSegment.lDxfEntityNumber = lEntityNumber;
					tUnconnectedSegment.eSegmentType = eArc;
					tUnconnectedSegment.dAx = tUnconnectedSegment.dRx + tUnconnectedSegment.dRadius * cos (tUnconnectedSegment.dStartAngle * M_PI / 180.0);
					tUnconnectedSegment.dAy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius * sin (tUnconnectedSegment.dStartAngle * M_PI / 180.0);
					tUnconnectedSegment.dBx = tUnconnectedSegment.dRx + tUnconnectedSegment.dRadius * cos (tUnconnectedSegment.dEndAngle * M_PI / 180.0);
					tUnconnectedSegment.dBy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius * sin (tUnconnectedSegment.dEndAngle * M_PI / 180.0);

					sprintf (chReport, "ARC - Calculated Start X = %f", tUnconnectedSegment.dAx);
					ReportBasedOnVerbosity (3, chReport);
					sprintf (chReport, "ARC - Calculated Start Y = %f", tUnconnectedSegment.dAy);
					ReportBasedOnVerbosity (3, chReport);
					sprintf (chReport, "ARC - Calculated End X = %f", tUnconnectedSegment.dBx);
					ReportBasedOnVerbosity (3, chReport);
					sprintf (chReport, "ARC - Calculated End Y = %f", tUnconnectedSegment.dBy);
					ReportBasedOnVerbosity (3, chReport);

					if (eCutOutsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOutsideSegmentCount++;
						m_lCurrentOutSegmentArcCount++;
						if (1 != pOutsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOutsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutInsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentInsideSegmentCount++;
						m_lCurrentInSegmentArcCount++;
						if (1 != pInsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pInsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutOnFeature == tUnconnectedSegment.eFeatureCutType)
					{
		 				m_lCurrentOnSegmentCount++;
						m_lCurrentOnSegmentArcCount++;
						if (1 != pOnVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOnVectorClass->SVC_ReportStatus();
					}
					else
						ReportBasedOnVerbosity (3, "!!! NOT adding arc element due existence on unqualified layer !!!");
				}
			}
			else if (std::string::npos != strMyData.find("CIRCLE", 0))
			{
				if (m_eDxfVersion == UNDEFINED)
				{
					// TBD
			 	}
				else if (m_eDxfVersion == ACAD_14)
				{
					// Unused line
					DXF_IO_SkipLines (1, &m_lDxfLineCount);

					getline (&pLineData, &szLineLen, m_pDxfFile);	// Entity Line #
					m_lDxfLineCount++;

					if (!DXF_IO_InputParameterLong ("%lX", &lEntityNumber, pLineData))
					{
						cout << "FAULT - looking for entity number, found: " << pLineData << std::endl;
					 	return 0;
					}

					sprintf (chReport, "File line # =  %ld", m_lDxfLineCount-2);
					ReportBasedOnVerbosity (3, chReport);

					sprintf (chReport, "CIRCLE - entity# =  %ld", lEntityNumber);
					ReportBasedOnVerbosity (3, chReport);

 					DXF_IO_SkipLines (3, &m_lDxfLineCount);

					// Layer info (CUT type)
					getline(&pLineData, &szLineLen, m_pDxfFile);
					m_lDxfLineCount++;
					if (!DXF_IO_InputParameterString ("%s", chLayerInfo, sizeof(chLayerInfo), pLineData))
					{
						cout << "FAULT - looking for layer info in line:" << pLineData << std::endl;
						return -1;
					}
					if (0 == strncmp (chLayerInfo, "CUT_OUTSIDE", 11))
					{
						ReportBasedOnVerbosity (3, "CIRCLE - layer# = CUT_OUTSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutOutsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_INSIDE", 10))
					{
						ReportBasedOnVerbosity (3, "CIRCLE - layer# = CUT_INSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutInsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_ON", 7))
					{
						ReportBasedOnVerbosity (3, "CIRCLE - layer# = CUT_ON");
						tUnconnectedSegment.eFeatureCutType = eCutOnFeature;
					}
					else
					{
						sprintf (chReport, "CIRCLE - layer# = %s", chLayerInfo);
						ReportBasedOnVerbosity (3, chReport);
						tUnconnectedSegment.eFeatureCutType = eCutUnknown;
					}

					// Search for pre-cursor to coordinate field
//					DXF_IO_SeekToLine ("ByLayer", 100, &m_lDxfLineCount);
					// Skip one more line to be in position for coordinate info
					DXF_IO_SkipLines (7, &m_lDxfLineCount);
 								
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Center X
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "CIRCLE-X =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Center Y
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "CIRCLE-Y =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRy = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Radius
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "CIRCLE radius =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRadius = dValue;
					 
					m_lDxfCirclesExtracted++;

					tUnconnectedSegment.lFeatureGroupIndex = -1;
					tUnconnectedSegment.lDxfEntityNumber = lEntityNumber;
					tUnconnectedSegment.eSegmentType = eCircle;
					tUnconnectedSegment.dAx = tUnconnectedSegment.dRx;
					tUnconnectedSegment.dAy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius;
					tUnconnectedSegment.dBx = tUnconnectedSegment.dRx;
					tUnconnectedSegment.dBy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius;
					tUnconnectedSegment.dStartAngle = 0.0;
					tUnconnectedSegment.dStartAngle = 360.0;

					if (eCutOutsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOutsideSegmentCount++;
						m_lCurrentOutSegmentCircleCount++;
						if (1 != pOutsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOutsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutInsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentInsideSegmentCount++;
						m_lCurrentInSegmentCircleCount++;
						if (1 != pInsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pInsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutOnFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOnSegmentCount++;
						m_lCurrentOnSegmentCircleCount++;
						if (1 != pOnVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOnVectorClass->SVC_ReportStatus();
					}
					else
						ReportBasedOnVerbosity (3, "!!! NOT adding circle element due existence on unqualified layer !!!");
				}
				else if (m_eDxfVersion == ACAD_15)
				{
					// Unused line
					DXF_IO_SkipLines (1, &m_lDxfLineCount);

					getline (&pLineData, &szLineLen, m_pDxfFile);	// Entity Line #
					m_lDxfLineCount++;

					if (!DXF_IO_InputParameterLong ("%lX", &lEntityNumber, pLineData))
					{
						cout << "FAULT - looking for entity number, found: " << pLineData << std::endl;
					 	return 0;
					}

					sprintf (chReport, "File line # =  %ld", m_lDxfLineCount-2);
					ReportBasedOnVerbosity (3, chReport);

					sprintf (chReport, "CIRCLE - entity# =  %ld", lEntityNumber);
					ReportBasedOnVerbosity (3, chReport);

 					DXF_IO_SkipLines (5, &m_lDxfLineCount);

					// Layer info (CUT type)
					getline(&pLineData, &szLineLen, m_pDxfFile);
					m_lDxfLineCount++;
					if (!DXF_IO_InputParameterString ("%s", chLayerInfo, sizeof(chLayerInfo), pLineData))
					{
						cout << "FAULT - looking for layer info in line:" << pLineData << std::endl;
						return -1;
					}
					if (0 == strncmp (chLayerInfo, "CUT_OUTSIDE", 11))
					{
						ReportBasedOnVerbosity (3, "CIRCLE - layer# = CUT_OUTSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutOutsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_INSIDE", 10))
					{
						ReportBasedOnVerbosity (3, "CIRCLE - layer# = CUT_INSIDE");
						tUnconnectedSegment.eFeatureCutType = eCutInsideFeature;
					}
					else if (0 == strncmp (chLayerInfo, "CUT_ON", 7))
					{
						ReportBasedOnVerbosity (3, "CIRCLE - layer# = CUT_ON");
						tUnconnectedSegment.eFeatureCutType = eCutOnFeature;
					}
					else
					{
						sprintf (chReport, "CIRCLE - layer# = %s", chLayerInfo);
						ReportBasedOnVerbosity (3, chReport);
						tUnconnectedSegment.eFeatureCutType = eCutUnknown;
					}

					// Search for pre-cursor to coordinate field
//					DXF_IO_SeekToLine ("ByLayer", 100, &m_lDxfLineCount);
					// Skip one more line to be in position for coordinate info
					DXF_IO_SkipLines (7, &m_lDxfLineCount);
 								
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Center X
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "CIRCLE-X =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRx = dValue;

					DXF_IO_SkipLines (1, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Center Y
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "CIRCLE-Y =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRy = dValue;

					DXF_IO_SkipLines (3, &m_lDxfLineCount);
					getline(&pLineData, &szLineLen, m_pDxfFile);		// Radius
					m_lDxfLineCount++;
					if (1 != DXF_IO_InputParameterDouble (&dValue, pLineData))
						return -1;
					sprintf (chReport, "CIRCLE radius =  %f", dValue);
					ReportBasedOnVerbosity (3, chReport);
					tUnconnectedSegment.dRadius = dValue;
					 
					m_lDxfCirclesExtracted++;

					tUnconnectedSegment.lFeatureGroupIndex = -1;
					tUnconnectedSegment.lDxfEntityNumber = lEntityNumber;
					tUnconnectedSegment.eSegmentType = eCircle;
					tUnconnectedSegment.dAx = tUnconnectedSegment.dRx;
					tUnconnectedSegment.dAy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius;
					tUnconnectedSegment.dBx = tUnconnectedSegment.dRx;
					tUnconnectedSegment.dBy = tUnconnectedSegment.dRy + tUnconnectedSegment.dRadius;
					tUnconnectedSegment.dStartAngle = 0.0;
					tUnconnectedSegment.dStartAngle = 360.0;

					if (eCutOutsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOutsideSegmentCount++;
						m_lCurrentOutSegmentCircleCount++;
						if (1 != pOutsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOutsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutInsideFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentInsideSegmentCount++;
						m_lCurrentInSegmentCircleCount++;
						if (1 != pInsideVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pInsideVectorClass->SVC_ReportStatus();
					}
					else if (eCutOnFeature == tUnconnectedSegment.eFeatureCutType)
					{
 						m_lCurrentOnSegmentCount++;
						m_lCurrentOnSegmentCircleCount++;
						if (1 != pOnVectorClass->SVC_AddElement(tUnconnectedSegment))
						{
							cout << "Failure to add STL vector..." << std::endl;
							return -1;
						}
//						pOnVectorClass->SVC_ReportStatus();
					}
					else
						ReportBasedOnVerbosity (3, "!!! NOT adding circle element due existence on unqualified layer !!!");
				}
			}
			else
			{
			}
		}
	};
 
	return 0;
}



bool DXF_FILE_IO_CLASS::ReportBasedOnVerbosity(int nVerbosity, const char *chReport)
{
	// INI values range between 0 and 5
	// 1 <= nVerbosity <= 5
	if (nVerbosity <= INI_Verbosity_DXF_Input)
	{
		cout << chReport << std::endl;
		return true;
	}
	else
		return false;
}

int DXF_FILE_IO_CLASS::DXF_IO_CloseFile(void)
{
	if (m_pDxfFile)
	{
		fclose(m_pDxfFile);
		return 1;
	}
	else
		return 0;
}

int DXF_FILE_IO_CLASS::DXF_IO_ReportStatus(void)
{
	return 0;
}
