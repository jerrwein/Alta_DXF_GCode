#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "enumsBase.h"
#include "STL_VectorBase.h"
#include "STL_MapBase.h"
#include "DoubleVector.h"
#include "ToolPaths.h"

#define TOOL_PATH_VERBOSITY 6

using namespace std;

#define TP_ARC_CONCENTRICITY 1.0E-13

#define EQUIVALENCY_EPSILON 1.0E-7
#define ANGULAR_EPSILON 1.0E-9
#define ROOT_EPSILON 1.0E-9

extern int INI_Verbosity_ToolPaths;

char		chReport_TP[400];

//#define CONNECT_EPSILON 1.0E-5
//#define CONNECT_EPSILON 1.0E-3
//#define CONNECT_EPSILON 4.0E-2
#define CONNECT_EPSILON 3.0E-4

CToolPaths::CToolPaths(void)
{
	m_dRoughToolDiameter = 0.4;
	m_dFinishToolDiameter = 0.4;
}

CToolPaths::~CToolPaths(void)
{
}

int CToolPaths::ExtractDxfFeatures (FEATURE_CUT_TYPE eFeatureType,
				    STL_MAP_CLASS *pFeaturesMap,
				    STL_VECTOR_CLASS *pUnconnectedClass,
				    double dToolDiam)
{
	float fLengthPathA, fLengthPathB;

	std::string strCutType;
	switch(eFeatureType)
	{
		case eCutInsideFeature: strCutType = "CUT_INSIDE"; break;
		case eCutOutsideFeature: strCutType = "CUT_OUTSIDE"; break;
		case eCutOnFeature: strCutType = "CUT_ON"; break;
		default: strCutType = "*UNKNOWN"; break;
	};


	// Remove any possible tFeatures from the CMap before proceeding
	size_t nFeatureCount = pFeaturesMap->SMC_GetSize();
	if (eFeatureType == eCutInsideFeature)
	{
		sprintf (chReport_TP, "Cut INSIDE Feature map, initial count: %ld", pFeaturesMap->SMC_GetSize());
		ReportBasedOnVerbosity(3, chReport_TP);
		pFeaturesMap->SMC_RemoveAll();
		sprintf (chReport_TP, "Cut INSIDE Feature map, post-RemoveAll() count: %ld", pFeaturesMap->SMC_GetSize());
		ReportBasedOnVerbosity(3, chReport_TP);
	}
	else if (eFeatureType == eCutOutsideFeature)
	{
		sprintf (chReport_TP, "Cut OUTSIDE Feature map, initial count: %ld", pFeaturesMap->SMC_GetSize());
		ReportBasedOnVerbosity(3, chReport_TP);
		pFeaturesMap->SMC_RemoveAll();
		sprintf (chReport_TP, "Cut OUTSIDE Feature map, post-RemoveAll() count: %ld", pFeaturesMap->SMC_GetSize());
		ReportBasedOnVerbosity(3, chReport_TP);
 	}
	else if (eFeatureType == eCutOnFeature)
	{
		sprintf (chReport_TP, "Cut ON Feature map, initial count: %ld", pFeaturesMap->SMC_GetSize());
		ReportBasedOnVerbosity(3, chReport_TP);
		pFeaturesMap->SMC_RemoveAll();
		sprintf (chReport_TP, "Cut ON Feature map, post-RemoveAll() count: %ld", pFeaturesMap->SMC_GetSize());
		ReportBasedOnVerbosity(3, chReport_TP);
  	}
	else
	{
		ReportBasedOnVerbosity(-1, "ExtractDxfFeatures() - Illegal CUT type!");
		return -1;
	}

	// Find & mark all connections between segments
	if (!BBB_FindAndTagAllConnections (pUnconnectedClass))
		return -1;

	// Get number of remaining unconnected segments
	int nRemainingSegsBeforeExtract = CountRemainingNonGroupedUnconnectedDxfSegments (eFeatureType, pUnconnectedClass);
	int nRemainingSegsAfterExtract;
	int nSegmentsUsed;
	int nGroupIndex = 0;

	while (0 < nRemainingSegsBeforeExtract)
	{
		// Create a grouped DXF segment class for the next feature extraction
		STL_CONNECTED_SEGS_CLASS *pConnectedSegmentsClass = new STL_CONNECTED_SEGS_CLASS();

		// Attempt extraction and initialize feature data if successful
		nSegmentsUsed = ExtractGroupedDxfSegments_III (nGroupIndex, eFeatureType, pUnconnectedClass, pConnectedSegmentsClass);
 		if (0 < nSegmentsUsed)
		{
			FEATURE_ELEMENT tNewFeature;
			tNewFeature.m_nFeatureIndex = nGroupIndex;
			tNewFeature.m_eFeatureCutType = eFeatureType;
			tNewFeature.m_nNumberSegments = nSegmentsUsed;
	 		tNewFeature.m_pSaUnconnectedDxfSegments = NULL;
			tNewFeature.m_pSaConnectedDxfSegments = pConnectedSegmentsClass;

			// Calculate the starting points for CCW rotations
			CalculateFeaturesCcwRotationStartPoints (&tNewFeature, eFeatureType);
 
			// Create new SArrays for the tool paths
			if ((eFeatureType == eCutInsideFeature) || (eFeatureType == eCutOutsideFeature))
			{
				// Create new SArrays for the alternate tool paths
				tNewFeature.m_pSaToolPathA = new STL_CONNECTED_SEGS_CLASS();
				tNewFeature.m_pSaToolPathB = new STL_CONNECTED_SEGS_CLASS();
			}
			tNewFeature.m_pSaToolPathFinishCut = new STL_CONNECTED_SEGS_CLASS();

			// Add the feature to map
			pFeaturesMap->SMC_AddFeature(nGroupIndex, tNewFeature);
 
			// Calculate boundaries, centroid, alternate tool paths and their lengths
			if (!CalculateMapFeatureBoundary (eFeatureType, pFeaturesMap, nGroupIndex))
				return -1;
 			if (!CalculateMapFeatureCentroid (eFeatureType, pFeaturesMap, nGroupIndex))
				return -1;
 			if ((eFeatureType == eCutInsideFeature) || (eFeatureType == eCutOutsideFeature))
			{
		//		GenerateMapFeatureToolPaths (eFeatureType, pFeaturesMap, nGroupIndex, DEFAULT_LOOP_OFFSETS);
				GenerateMapFeatureToolPaths (eFeatureType, pFeaturesMap, nGroupIndex, dToolDiam/2.0);
				CalculateMapFeatureToolPathLengths (eFeatureType, pFeaturesMap, nGroupIndex, &fLengthPathA, &fLengthPathB);
				sprintf (chReport_TP, "LoopA: %f, LoopB: %f\n", fLengthPathA, fLengthPathB);
				ReportBasedOnVerbosity (2, chReport_TP); /* HappyHair */
 
				// Based on alternate loop lengths, generate final tool path
//				UpdateToolDiameters();
				GenerateMapFeatureToolPath (eFeatureType, pFeaturesMap, nGroupIndex++, dToolDiam/2.0);
			}
			else if (eFeatureType == eCutOnFeature)
			{
				if (!GenerateMapOnFeatureToolPath (eFeatureType, pFeaturesMap, nGroupIndex++))
					return -1;
			}
		}
		else if (-1 == nSegmentsUsed)
		{
			// Failure to connect segment
			delete pConnectedSegmentsClass;
			assert(false);
		}
		else
		{
			// No segments remaining - delete the unused SArray
			delete pConnectedSegmentsClass;
			break;
		}

		// A few safety checks...
 		nRemainingSegsAfterExtract = GetRemainingNonGroupedUnconnectedDxfSegments (eFeatureType, pUnconnectedClass);
		assert (nRemainingSegsAfterExtract == (nRemainingSegsBeforeExtract-nSegmentsUsed));

		nRemainingSegsBeforeExtract = nRemainingSegsAfterExtract;
// Happy-hair		DumpSegmentSmartArray ("Extraced and Grouped", pConnectedSegmentsClass);

		nFeatureCount = pFeaturesMap->SMC_GetSize();
		sprintf (chReport_TP, "Current feature map, count: %ld", nFeatureCount);
		ReportBasedOnVerbosity(3, chReport_TP);
	};

	DumpFeatureMap (eFeatureType, pFeaturesMap);

	return nFeatureCount;
}



//int CToolPaths::ExtractDxfFeatures (FEATURE_CUT_TYPE eFeatureType,
//				    STL_MAP_CLASS *pFeaturesMap,
//				    std::vector <UNCONNECTED_ELEMENT> *pUnconnectedArray,
//				    double dToolDiameter)
//{
//	return 1;
//}

bool CToolPaths::BBB_FindAndTagAllConnections (STL_VECTOR_CLASS *pUnconnectedClass)
{
	UNCONNECTED_ELEMENT 	tPrimarySegment;
	UNCONNECTED_ELEMENT 	tSecondarySegment;
	double					dDeltaX, dDeltaY;
 	int						nPrimaryAConnectionsFound;
	int						nPrimaryBConnectionsFound;
	int						nConnsFound = 0;
//	char		chReport[200];

	size_t nSize = pUnconnectedClass->SVC_Size();
	if (nSize < 1)
		return true;

	// Sort through the array looking for connections
	for (size_t i=0; i<nSize; i++)
	{
		// Get the primary segment - accepting only lines and arcs
		tPrimarySegment = pUnconnectedClass->SVC_GetElement(i);
		if (eCircle == tPrimarySegment.eSegmentType)
			continue;
		
		// Update primary segment to default of no connections found
		tPrimarySegment.nA_ConnectedSegmentIndex = -1;
		tPrimarySegment.ucA_ConnectedSegmentsEnd = 0;
		tPrimarySegment.nB_ConnectedSegmentIndex = -1;
		tPrimarySegment.ucB_ConnectedSegmentsEnd = 0;
		pUnconnectedClass->SVC_SetElement(i, tPrimarySegment);

		nPrimaryAConnectionsFound = 0;
		nPrimaryBConnectionsFound = 0;
	
		// DEBUG Only
//		if (tPrimarySegment.lDxfEntityNumber == 74)
//		{
//			sprintf (chReport_TP, "FindAndTagAllConnections(), Processing DXF #: %ld", tPrimarySegment.lDxfEntityNumber);
//			ReportBasedOnVerbosity(-1, chReport_TP);
//		}

		for (size_t j=0; j<nSize; j++)
		{
			// Don't compare identical segments
			if (j==i)
				continue;
			// Get the secondary segment
			tSecondarySegment = pUnconnectedClass->SVC_GetElement(j);
			
			// Accept only lines and arcs
			if (eCircle == tSecondarySegment.eSegmentType)
				continue;

		 	// Is secondary's A end connected to primary's A end ?
			dDeltaX = fabs (tPrimarySegment.dAx  - tSecondarySegment.dAx);
			dDeltaY = fabs (tPrimarySegment.dAy - tSecondarySegment.dAy);
			if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
			{
				nConnsFound++;
				sprintf (chReport_TP, "FindAndTagAllConnections(%d), Prim[%ld]A-Sec[%ld]A Connected X delta: %3.4E, Y delta: %3.4E", nConnsFound, i, j, dDeltaX, dDeltaY);
				ReportBasedOnVerbosity(3, chReport_TP);
				// Update the new target
				nPrimaryAConnectionsFound++;
				if (1 < nPrimaryAConnectionsFound)
				{
					sprintf (chReport_TP, "FindAndTagAllConnections(), Excess connections found at (%3.4E, %3.4E), aborting...", tPrimarySegment.dAx, tPrimarySegment.dAy);
					ReportBasedOnVerbosity(-1, chReport_TP);
					return -1;
				}
				// Update primary segment
				tPrimarySegment.nA_ConnectedSegmentIndex = j;
				tPrimarySegment.ucA_ConnectedSegmentsEnd = 1;
				pUnconnectedClass->SVC_SetElement(i, tPrimarySegment);
			}
		 
			// Is secondary's B end connected to primary's A end ?
			dDeltaX = fabs (tPrimarySegment.dAx - tSecondarySegment.dBx);
			dDeltaY = fabs (tPrimarySegment.dAy - tSecondarySegment.dBy);
			if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
			{
				nConnsFound++;
				sprintf (chReport_TP, "FindAndTagAllConnections(%d), Prim[%ld]A-Sec[%ld]B Connected X delta: %3.4E, Y delta: %3.4E", nConnsFound, i, j, dDeltaX, dDeltaY);
				ReportBasedOnVerbosity(3, chReport_TP);
				// Update the new target
				nPrimaryAConnectionsFound++;
				if (1 < nPrimaryAConnectionsFound)
				{
					sprintf (chReport_TP, "FindAndTagAllConnections(), Excess connections found at (%3.4E, %3.4E), aborting...", tPrimarySegment.dAx, tPrimarySegment.dAy);
					ReportBasedOnVerbosity(-1, chReport_TP);
					return -1;
				}
				// Update primary segment
				tPrimarySegment.nA_ConnectedSegmentIndex = j;
				tPrimarySegment.ucA_ConnectedSegmentsEnd = 2;
				pUnconnectedClass->SVC_SetElement(i, tPrimarySegment);
			}

			// Is secondary's A end connected to primary's B end ?
			dDeltaX = fabs (tPrimarySegment.dBx - tSecondarySegment.dAx);
			dDeltaY = fabs (tPrimarySegment.dBy - tSecondarySegment.dAy);
			if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
			{
				nConnsFound++;
				sprintf (chReport_TP, "FindAndTagAllConnections(%d), Prim[%ld]B-Sec[%ld]A Connected X delta: %3.4E, Y delta: %3.4E", nConnsFound, i, j, dDeltaX, dDeltaY);
				ReportBasedOnVerbosity(3, chReport_TP);
				// Update the new target
				nPrimaryBConnectionsFound++;
				// PROBLEM ?
				if (1 < nPrimaryBConnectionsFound)
				{
					sprintf (chReport_TP, "FindAndTagAllConnections(), Excess connections found at (%3.4E, %3.4E), aborting...", tPrimarySegment.dAx, tPrimarySegment.dAy);
					ReportBasedOnVerbosity(-1, chReport_TP);
					return -1;
				}
				// Update primary segment
				tPrimarySegment.nB_ConnectedSegmentIndex = j;
				tPrimarySegment.ucB_ConnectedSegmentsEnd = 1;
				pUnconnectedClass->SVC_SetElement(i, tPrimarySegment);
			}

			// Is secondary's B end connected to primary's B end ?
			dDeltaX = fabs (tPrimarySegment.dBx - tSecondarySegment.dBx);
			dDeltaY = fabs (tPrimarySegment.dBy - tSecondarySegment.dBy);
			if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
			{
				nConnsFound++;
				sprintf (chReport_TP, "FindAndTagAllConnections(%d), Prim[%ld]B-Sec[%ld]B Connected X delta: %3.4E, Y delta: %3.4E", nConnsFound, i, j, dDeltaX, dDeltaY);
 				ReportBasedOnVerbosity(3, chReport_TP);
				// Update the new target
				nPrimaryBConnectionsFound++;
				// PROBLEM ?
				if (1 < nPrimaryBConnectionsFound)
				{
					sprintf (chReport_TP, "FindAndTagAllConnections(), Excess connections found at (%3.4E, %3.4E), aborting...", tPrimarySegment.dAx, tPrimarySegment.dAy);
					ReportBasedOnVerbosity(-1, chReport_TP);
					return -1;
				}
				// Update primary segment
				tPrimarySegment.nB_ConnectedSegmentIndex = j;
				tPrimarySegment.ucB_ConnectedSegmentsEnd = 2;
				pUnconnectedClass->SVC_SetElement(i, tPrimarySegment);
			}
		}

		if (nPrimaryAConnectionsFound != 1)
		{
			sprintf (chReport_TP, "FindAndTagAllConnections(), *** No Primary-A connections found for index %ld (%3.4E, %3.4E)", i, tPrimarySegment.dAx, tPrimarySegment.dAy);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
		if (nPrimaryBConnectionsFound != 1)
		{
			sprintf (chReport_TP, "FindAndTagAllConnections(), *** No Primary-B connections found for index %ld (%3.4E, %3.4E)", i, tPrimarySegment.dAx, tPrimarySegment.dAy);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
 	}
	return true;
}

long CToolPaths::CountRemainingNonGroupedUnconnectedDxfSegments (FEATURE_CUT_TYPE eType, STL_VECTOR_CLASS *pUnconnectedClass)
{
	long  lRemaining = 0;
	UNCONNECTED_ELEMENT tSegment;

	if (eCutInsideFeature == eType)
	{
		size_t nSize = pUnconnectedClass->SVC_Size();
		for (size_t n=0; n<nSize; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				lRemaining++;
		}
	}
	else if (eCutOutsideFeature == eType)
	{
		size_t nSize = pUnconnectedClass->SVC_Size();
		for (size_t n=0; n<nSize; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				lRemaining++;
		}
	}
	else if (eCutOnFeature == eType)
	{
		size_t nSize = pUnconnectedClass->SVC_Size();
		for (size_t n=0; n<nSize; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				lRemaining++;
		}
	}
	else
		lRemaining = -1;

	return lRemaining;
}

long CToolPaths::GetNextNonGroupedSegmentIndex (FEATURE_CUT_TYPE eType, long lStartingIndex, STL_VECTOR_CLASS *pUnconnectedClass)
{
	UNCONNECTED_ELEMENT tSegment;
	if (eCutInsideFeature == eType)
	{
		size_t nMaxIndex = pUnconnectedClass->SVC_Size() - 1;
		for (size_t n=lStartingIndex; n<=nMaxIndex; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				return n;
		}
	}
	else if (eCutOutsideFeature == eType)
	{
		size_t nMaxIndex = pUnconnectedClass->SVC_Size() - 1;
		for (size_t n=lStartingIndex; n<=nMaxIndex; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				return n;
		}
	}
	else if (eCutOnFeature == eType)
	{
		size_t nMaxIndex = pUnconnectedClass->SVC_Size() - 1;
		for (size_t n=lStartingIndex; n<=nMaxIndex; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				return n;
		}
	}
	return -1;
}

bool CToolPaths::AddSegmentToFeatureMap (UNCONNECTED_ELEMENT *tUnconnectedSegment, STL_CONNECTED_SEGS_CLASS *pSortedConnectedClass, int nGroupedArrayIndex, int nGroupIndex)
{
	CONNECTED_ELEMENT	tConnectedSegment;

	tConnectedSegment.eSegmentType = tUnconnectedSegment->eSegmentType;
	tConnectedSegment.lDxfEntityNumber = tUnconnectedSegment->lDxfEntityNumber;
	tConnectedSegment.nFeatureIndex = nGroupIndex;
	tConnectedSegment.dAx = tUnconnectedSegment->dAx;
	tConnectedSegment.dAy = tUnconnectedSegment->dAy;
	tConnectedSegment.dBx = tUnconnectedSegment->dBx;
	tConnectedSegment.dBy = tUnconnectedSegment->dBy;
	tConnectedSegment.dRadius = tUnconnectedSegment->dRadius;
	tConnectedSegment.dRx = tUnconnectedSegment->dRx;
	tConnectedSegment.dRy = tUnconnectedSegment->dRy;
	tConnectedSegment.dStartAngle = tUnconnectedSegment->dStartAngle;
	tConnectedSegment.dEndAngle = tUnconnectedSegment->dEndAngle;

	// And these fields are not yet known.
	tConnectedSegment.nA_ConnectedSegmentIndex = -1;
	tConnectedSegment.nB_ConnectedSegmentIndex = -1;
	tConnectedSegment.ucA_ConnectedSegmentEnd = 0;
	tConnectedSegment.ucB_ConnectedSegmentEnd = 0;
 	// Place the element into the new connected & sorted array
 	pSortedConnectedClass->SVC_AddElement(tConnectedSegment);

	return true;
}

bool CToolPaths::FindAttachedSegment (FEATURE_CUT_TYPE eFeatureCutType, double dTargetX, double dTargetY, int nStartingIndex, int nAvoidSecondaryIndex, int *pConnectedsIndex, int *pConnectedsEnd, STL_VECTOR_CLASS *pUnconnectedClass)
{
	UNCONNECTED_ELEMENT tSegmentOfInterest;
	double				dDeltaX, dDeltaY;
	int					nLoops = 0;
//	char				chReport[200];

	if ((eCutInsideFeature != eFeatureCutType) &&
		(eCutOutsideFeature != eFeatureCutType) &&
		(eCutOnFeature != eFeatureCutType))
	assert(false);

	// Get the next available non-grouped segment
	int nNextIndex = GetNextNonGroupedSegmentIndex (eCutInsideFeature, nStartingIndex, pUnconnectedClass);

	// Find out how many non-grouped segments will need to be processed
	int nNonGroupedSegments = CountRemainingNonGroupedUnconnectedDxfSegments (eFeatureCutType, pUnconnectedClass);
	if (nNonGroupedSegments < 1)
		return false;

	bool bConnectionFound;
	while (++nLoops <= nNonGroupedSegments)
	{
		// Return FALSE if no remaining segments
		if (-1 == nNextIndex)
			return false;

		// Get the next available non-grouped segment
		tSegmentOfInterest = pUnconnectedClass->SVC_GetElement(nNextIndex);

		// Accept only open segments - LINES & ARCS (no circles)
		if ((eLine != tSegmentOfInterest.eSegmentType) && (eArc != tSegmentOfInterest.eSegmentType))
		{
			// Keep advancing through the array
			nNextIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, nNextIndex+1, pUnconnectedClass);
			// Avoid using matching Primary/Secondary indexes
			if (nNextIndex == nAvoidSecondaryIndex)
			{
				nNextIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, nNextIndex+1, pUnconnectedClass);
				if (-1 == nNextIndex)
				{
					sprintf (chReport_TP, "BIG problem TBD\n");
					ReportBasedOnVerbosity(-1, chReport_TP);
					assert(false);
		//			// Report on last connected segment member
		//			*pNumConnectedSegments = nNumConnectedSegments;
		//			*pUnconnectedsIndex = nLastConnectedIndex;
		//			*pUnconnectedsEnd = nLastConnectedEnd;
					return false;
				}
			}
			continue;
		}

		bConnectionFound = false;
		// Is segment's A end connected to working segment's end ?
		dDeltaX = fabs (dTargetX - tSegmentOfInterest.dAx);
		dDeltaY = fabs (dTargetY - tSegmentOfInterest.dAy);
		sprintf (chReport_TP, "PrimA-SecA-%d Connect X delta: %3.4E, Y delta: %3.4E", nNextIndex, dDeltaX, dDeltaY);
		ReportBasedOnVerbosity(2, chReport_TP);
		if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
		{
		//	sprintf (chReport_TP, "PrimA-SecA-%d Connect X delta: %3.4E, Y delta: %3.4E", nNextIndex, dDeltaX, dDeltaY);
		//	ReportBasedOnVerbosity(2, chReport_TP);
			ReportBasedOnVerbosity(4, "PrimA-SecA Connection found!");
			*pConnectedsEnd = 1;
			bConnectionFound = true;
		}

		// Is secondary's B end connected to working segment's end ?
		dDeltaX = fabs (dTargetX - tSegmentOfInterest.dBx);
		dDeltaY = fabs (dTargetY - tSegmentOfInterest.dBy);
		sprintf (chReport_TP, "PrimA-SecB-%d Connect X delta: %3.4E, Y delta: %3.4E", nNextIndex, dDeltaX, dDeltaY);
		ReportBasedOnVerbosity(2, chReport_TP);

		if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
		{
			// Better not have connected to A end also!
			assert (!bConnectionFound);
		//	sprintf (chReport_TP, "PrimA-SecB-%d Connect X delta: %3.4E, Y delta: %3.4E", nNextIndex, dDeltaX, dDeltaY);
		//	ReportBasedOnVerbosity(2, chReport_TP);
			ReportBasedOnVerbosity(4, "PrimA-SecB Connection found!");
			*pConnectedsEnd = 2;
			bConnectionFound = true;
		}

 		if (bConnectionFound)
		{
			*pConnectedsIndex = nNextIndex;
			return true;
		}
		else
		{
			nNextIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, nNextIndex+1, pUnconnectedClass);
			// Avoid using matching Primary/Secondary indexes
			if (nNextIndex == nAvoidSecondaryIndex)
			{
				nNextIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, nNextIndex+1, pUnconnectedClass);
				if (-1 == nNextIndex)
				{
					assert(false);
	 			}
			}
		}
 	};
	return false;
}

bool CToolPaths::BBB_IsFeaturePathClosed (FEATURE_CUT_TYPE eFeatureCutType, int nStartingSegmentIndex, int *pNumConnectedSegments, int *pUnconnectedsIndex, int *pUnconnectedsEnd, STL_VECTOR_CLASS *pUnconnectedClass)
{
	UNCONNECTED_ELEMENT tStartingSegment;
	UNCONNECTED_ELEMENT tSegmentOfInterest;
//	int					nLastConnectedIndex = -1;
//	int					nLastConnectedEnd = -1;
	int					nNumConnectedSegments = 1;

	if ((eCutInsideFeature != eFeatureCutType) && (eCutOutsideFeature != eFeatureCutType) && (eCutOnFeature != eFeatureCutType))
		assert(false);

	// Get the next starting non-grouped segment
	tStartingSegment = pUnconnectedClass->SVC_GetElement(nStartingSegmentIndex);

	// Is this a self-closing segment, and not in need of grouping ?
	if (eCircle == tStartingSegment.eSegmentType)
	{
		ReportBasedOnVerbosity(4, "BBB_IsFeaturePathClosed(), closed loop CIRCLE found during processing unconnected segments, returning as grouped");
		*pUnconnectedsIndex = -1;
		*pUnconnectedsEnd = 0;
		*pNumConnectedSegments = 1;
		return true;
	}

	// Get the next available non-grouped segment's index
	int nNextIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, nStartingSegmentIndex+1, pUnconnectedClass);
	if (-1 == nNextIndex)
	{
		// Report on last open segment member
		*pUnconnectedsIndex = nStartingSegmentIndex;
		*pUnconnectedsEnd = 0;
		pNumConnectedSegments = 0;
		return false;
	}

	int				nLastConnectedSegmentsIndex;
	unsigned char	ucLastConnectedSegmentsEnd;
	bool			bConnectionFound;
	bool			bBothStartingEndsExamined = false;

	// Go to the B end and see if it's attached to another segment
	if (0 <= tStartingSegment.nB_ConnectedSegmentIndex)
	{
		nNumConnectedSegments++;
		nLastConnectedSegmentsIndex = tStartingSegment.nB_ConnectedSegmentIndex;
		ucLastConnectedSegmentsEnd = tStartingSegment.ucB_ConnectedSegmentsEnd;
		bConnectionFound = true;
	}
	else if (0 <= tStartingSegment.nA_ConnectedSegmentIndex)
	{
		// Both ends now considered
		bBothStartingEndsExamined = true;
		nNumConnectedSegments++;
		nLastConnectedSegmentsIndex = tStartingSegment.nA_ConnectedSegmentIndex;
		ucLastConnectedSegmentsEnd = tStartingSegment.ucA_ConnectedSegmentsEnd;
		bConnectionFound = true;
	}
	else
	{
		*pNumConnectedSegments = 1;
		*pUnconnectedsIndex = nStartingSegmentIndex;
		*pUnconnectedsEnd = 0;
		return false;
	}

	while (bConnectionFound)
	{
		// Get the connected segment
		tSegmentOfInterest = pUnconnectedClass->SVC_GetElement(nLastConnectedSegmentsIndex);
		if (1 == ucLastConnectedSegmentsEnd)
		{
			// Is opposite end (B) of the new segment connected ?
			if (0 <= tSegmentOfInterest.nB_ConnectedSegmentIndex)
			{
				// Have we come back to the origination ?
				if (nStartingSegmentIndex == tSegmentOfInterest.nB_ConnectedSegmentIndex)
				{
					ReportBasedOnVerbosity(4, "Closed loop connection found");
					*pNumConnectedSegments = nNumConnectedSegments;
					return true;
				}
				else
				{
					nNumConnectedSegments++;
					nLastConnectedSegmentsIndex = tSegmentOfInterest.nB_ConnectedSegmentIndex;
					ucLastConnectedSegmentsEnd = tSegmentOfInterest.ucB_ConnectedSegmentsEnd;
				}
			}
			else
			{
				if (bBothStartingEndsExamined)
					bConnectionFound = false;
				else
				{
					// Re-position start, go back to the A end and see if it's attached to another segment
					nStartingSegmentIndex = nLastConnectedSegmentsIndex;
					if (0 <= tStartingSegment.nA_ConnectedSegmentIndex)
					{
						nNumConnectedSegments++;
						nLastConnectedSegmentsIndex = tStartingSegment.nA_ConnectedSegmentIndex;
						ucLastConnectedSegmentsEnd = tStartingSegment.ucA_ConnectedSegmentsEnd;
//						ucLastConnectedSegmentsEnd = tSegmentOfInterest.ucA_ConnectedSegmentsEnd;
					}
					bBothStartingEndsExamined = true;
				}
			}
		}
		else
		{
			// Is opposite end (A) also connected ?
			if (0 <= tSegmentOfInterest.nA_ConnectedSegmentIndex)
			{
				// Have we come back to the origination ?
				if (nStartingSegmentIndex == tSegmentOfInterest.nA_ConnectedSegmentIndex)
				{
					ReportBasedOnVerbosity (4, "Closed loop connection found\n");
					*pNumConnectedSegments = nNumConnectedSegments;
					return true;
				}
				else
				{
					nNumConnectedSegments++;
					nLastConnectedSegmentsIndex = tSegmentOfInterest.nA_ConnectedSegmentIndex;
					ucLastConnectedSegmentsEnd = tSegmentOfInterest.ucA_ConnectedSegmentsEnd;
				}
			}
			else
			{
				if (bBothStartingEndsExamined)
					bConnectionFound = false;
				else
				{
					// Re-position start, go back to the A end and see if it's attached to another segment
					nStartingSegmentIndex = nLastConnectedSegmentsIndex;
					if (0 <= tStartingSegment.nA_ConnectedSegmentIndex)
					{
						nNumConnectedSegments++;
						nLastConnectedSegmentsIndex = tStartingSegment.nA_ConnectedSegmentIndex;
						ucLastConnectedSegmentsEnd = tStartingSegment.ucA_ConnectedSegmentsEnd;
//						ucLastConnectedSegmentsEnd = tSegmentOfInterest.ucA_ConnectedSegmentsEnd;
					}
					bBothStartingEndsExamined = true;
				}
			}
		}
 	};

 	// Report on last connected segment member
	*pNumConnectedSegments = nNumConnectedSegments;
	*pUnconnectedsIndex = nLastConnectedSegmentsIndex;
	*pUnconnectedsEnd = ucLastConnectedSegmentsEnd;
	return false;
}

int CToolPaths::ExtractGroupedDxfSegments_III (int nGroupIndex, FEATURE_CUT_TYPE eFeatureCutType, STL_VECTOR_CLASS *pUnconnectedClass, STL_CONNECTED_SEGS_CLASS *pSortedConnectedClass)
{
	UNCONNECTED_ELEMENT tPrimaryUnconnectedSegment;
	UNCONNECTED_ELEMENT tSecondaryUnconnectedSegment;
	CONNECTED_ELEMENT	tConnectedSegment1;
	CONNECTED_ELEMENT	tConnectedSegment2;

	double	dDeltaX, dDeltaY;
	bool	bConnectionFound;
	int		nBeginningSecondaryIndex;
	int		nAvoidSecondaryIndex;
 	int		nGroupedArrayNextIndex = 0;
	int		nSegmentsPlacedInGroup = 0;
// 	char	chReport[200];

	// ASSERT() only required for initial confirmation.
	if ((eCutInsideFeature != eFeatureCutType) && (eCutOutsideFeature != eFeatureCutType) && (eCutOnFeature != eFeatureCutType))
		assert(false);

	// Remove any possible segments from the sorted connected array before proceeding
	size_t zSize = pSortedConnectedClass->SVC_Size();
	sprintf (chReport_TP, "ExtractGroupedDxfSegments(): Connected sorted segment array, initial array size: %ld", zSize);
	ReportBasedOnVerbosity(4, chReport_TP);
	pSortedConnectedClass->SVC_RemoveAllElements();
	zSize = pSortedConnectedClass->SVC_Size();
	sprintf (chReport_TP, "ExtractGroupedDxfSegments(): Connected sorted segment array, post-RemoveAll() array size: %ld", zSize);
	ReportBasedOnVerbosity(4, chReport_TP);

	// Step #1a, Find out how many non-grouped segments will need to be processed
	long nNonGroupedSegments = CountRemainingNonGroupedUnconnectedDxfSegments (eFeatureCutType, pUnconnectedClass);
	if (nNonGroupedSegments < 1)
		return 0;

	// Step #1b, Find & get the first non-grouped primary segment to start the connection routing with (using starting index of 0)
	int  nFirstUnconnectedSegmentIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, 0, pUnconnectedClass);
	if (-1 == nFirstUnconnectedSegmentIndex)
	{
		ReportBasedOnVerbosity(3, "ExtractGroupedDxfSegments(), No non-grouped unconnected segments available.");
		return 0;
	}
	tPrimaryUnconnectedSegment = pUnconnectedClass->SVC_GetElement(nFirstUnconnectedSegmentIndex);

	// Step #1c, Consider the cases of only one remaining unconnected segment
	if (nNonGroupedSegments == 1)
	{
		// One unconnected segment left - is it a closed circle ?
		if (eCircle == tPrimaryUnconnectedSegment.eSegmentType)
		{
			// Confirmed CIRCLE, add it to the map and mark it as grouped
			AddSegmentToFeatureMap (&tPrimaryUnconnectedSegment, pSortedConnectedClass, nGroupedArrayNextIndex, nGroupIndex);
			// Mark the non-grouped segment as grouped at this time
			tPrimaryUnconnectedSegment.lFeatureGroupIndex = nGroupIndex;
			// QUESTION This...
			pUnconnectedClass->SVC_SetElement(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
// was...	pUnconnectedSegments->SetAt(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
	 		return 1;
		}

		// It's not a closed circle - if it's an ON feature it does not have to be closed
		else if ((eCutOnFeature == eFeatureCutType))
		{
// was...	AddSegmentToFeatureMap (&tPrimaryUnconnectedSegment, pSaSortedSegments, nGroupedArrayNextIndex, nGroupNumber);
			AddSegmentToFeatureMap (&tPrimaryUnconnectedSegment, pSortedConnectedClass, nGroupedArrayNextIndex, nGroupIndex);
  			// Mark the non-grouped segment as grouped at this time
			tPrimaryUnconnectedSegment.lFeatureGroupIndex = nGroupIndex;
			// QUESTION This...
			pUnconnectedClass->SVC_SetElement(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
			return 1;
 		}
		else
		{
			ReportBasedOnVerbosity (-1,"CUT_INSIDE or CUT_OUTSIDE feature found to be not closed, returning failure");
			return 0;
		}
	}

	// Step #1d, get the second non-grouped segment with which to begin connection routing
	int nSecondUnconnectedSegmentIndex = GetNextNonGroupedSegmentIndex (eFeatureCutType, nFirstUnconnectedSegmentIndex+1l, pUnconnectedClass);
	if (-1 == nSecondUnconnectedSegmentIndex)
	{
		ReportBasedOnVerbosity (2, "ExtractGroupedDxfSegments(): , Two non-grouped unconnected segments available");
	 	return 0;
	}

	// Step #2 - Is this a closed path and not in need of further grouping ?
	int nUnconnectedsIndex = -2;
	int nUnconnectedsEnd = -0;
	int nNumConnectedSegments;
	bool bIsClosedPath = BBB_IsFeaturePathClosed (eFeatureCutType, nFirstUnconnectedSegmentIndex, &nNumConnectedSegments, &nUnconnectedsIndex, &nUnconnectedsEnd, pUnconnectedClass);

	if (bIsClosedPath && (nNumConnectedSegments == 1))
	{
		ReportBasedOnVerbosity (3, "Likely closed loop CIRCLE or ARC found during processing unconnected segments, returning as grouped");
		// Confirm this segment is a CIRCLE, add it to the map and mark it as grouped
		if (eCircle == tPrimaryUnconnectedSegment.eSegmentType)
		{
			AddSegmentToFeatureMap (&tPrimaryUnconnectedSegment, pSortedConnectedClass, nGroupedArrayNextIndex, nGroupIndex);
			// Mark the non-grouped segment as grouped at this time
			tPrimaryUnconnectedSegment.lFeatureGroupIndex = nGroupIndex;
			// QUESTION This...
			pUnconnectedClass->SVC_SetElement(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
// was...	pUnconnectedSegments->SetAt(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
	 	}
		return 1;
	}
	if (!bIsClosedPath && (nNumConnectedSegments == 1))
	{
		ReportBasedOnVerbosity (3, "Likely unattached  CIRCLE or ARC found during processing unconnected segments, returning as grouped");
		// TODO: Confirm this segment and add it to the feature map & mark it as grouped
		AddSegmentToFeatureMap (&tPrimaryUnconnectedSegment, pSortedConnectedClass, nGroupedArrayNextIndex, nGroupIndex);
		// Mark the non-grouped segment as grouped at this time
		tPrimaryUnconnectedSegment.lFeatureGroupIndex = nGroupIndex;
		// QUESTION This...
		pUnconnectedClass->SVC_SetElement(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
// was..pUnconnectedSegments->SetAt(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
		return 1;
 	}

	// Step #3, All CUT_INSIDE & CUT_OUTSIDE features must be closed
	if (!bIsClosedPath && ((eCutInsideFeature == eFeatureCutType) || (eCutOutsideFeature == eFeatureCutType)))
	{
		UNCONNECTED_ELEMENT tUnconnecetedSegment;
		tUnconnecetedSegment = pUnconnectedClass->SVC_GetElement(nUnconnectedsIndex);
		sprintf (chReport_TP, "CUT_INSIDE or CUT_OUTSIDE feature found to be not closed, discontinuity at %3.4f, %3.4f or %3.4f, %3.4f\n", tUnconnecetedSegment.dAx, tUnconnecetedSegment.dAy,  tUnconnecetedSegment.dBx, tUnconnecetedSegment.dBy);
		ReportBasedOnVerbosity (3, chReport_TP);
		ReportBasedOnVerbosity (3, "CUT_INSIDE or CUT_OUTSIDE feature found to be not closed, returning failure");
		return 0;
	}

	// Step #3b, get the second non-grouped segment with which to begin connection routing
	//	int nSecondUnconnectedSegmentIndex = NextNonGroupedSegmentIndex (eFeatureCutType, nFirstUnconnectedSegmentIndex+1, pUnconnectedSegments);
//		if (-1 == nSecondUnconnectedSegmentIndex)
//		{
//			TRACE("ExtractGroupedDxfSegments_III(), Two non-grouped unconnected segments unavailable\n");
//			return 0;
//		}

	double dOriginationX, dOriginationY;
	double dPrimarysWorkingEndX;
	double dPrimarysWorkingEndY;
	int nPrimarysWorkingEnd;

	// Step #4, based upon open/closed paths determine originating segment and position.
	if (bIsClosedPath)
	{
		// Closed path - Begin grouping all unconnected segments using the primary originally used
		dOriginationX = tPrimaryUnconnectedSegment.dAx;
		dOriginationY = tPrimaryUnconnectedSegment.dAy;
		dPrimarysWorkingEndX = tPrimaryUnconnectedSegment.dBx;
		dPrimarysWorkingEndY = tPrimaryUnconnectedSegment.dBy;
		nPrimarysWorkingEnd = 2;
		nAvoidSecondaryIndex = nFirstUnconnectedSegmentIndex;
		nBeginningSecondaryIndex = nSecondUnconnectedSegmentIndex;
 	}
	else
	{
		// Open path - Begin grouping all unconnected segments starting with the unconnected segment found above (to preserve sort order)
		tPrimaryUnconnectedSegment = pUnconnectedClass->SVC_GetElement(nUnconnectedsIndex);
		nPrimarysWorkingEnd = nUnconnectedsEnd;
		nAvoidSecondaryIndex = nUnconnectedsIndex;
		if (1 == nPrimarysWorkingEnd)
		{
			dOriginationX = tPrimaryUnconnectedSegment.dBx;
			dOriginationY = tPrimaryUnconnectedSegment.dBy;
			dPrimarysWorkingEndX = tPrimaryUnconnectedSegment.dAx;
			dPrimarysWorkingEndY = tPrimaryUnconnectedSegment.dAy;
		}
		else
		{
			dOriginationX = tPrimaryUnconnectedSegment.dAx;
			dOriginationY = tPrimaryUnconnectedSegment.dAy;
			dPrimarysWorkingEndX = tPrimaryUnconnectedSegment.dBx;
			dPrimarysWorkingEndY = tPrimaryUnconnectedSegment.dBy;
		}
		// Go back to the first unconnected segment for an open path
		nBeginningSecondaryIndex = nFirstUnconnectedSegmentIndex;
	}

	sprintf (chReport_TP, "Primary segment's origination point, %lf, %lf\n", dOriginationX, dOriginationY);
	ReportBasedOnVerbosity(3, chReport_TP);

	// Step #5, seed the sorted array with the first originating segment
	tConnectedSegment1.eSegmentType = tPrimaryUnconnectedSegment.eSegmentType;
	tConnectedSegment1.lDxfEntityNumber = tPrimaryUnconnectedSegment.lDxfEntityNumber;
	tConnectedSegment1.nFeatureIndex = nGroupIndex;
	tConnectedSegment1.dAx = tPrimaryUnconnectedSegment.dAx;
	tConnectedSegment1.dAy = tPrimaryUnconnectedSegment.dAy;
	tConnectedSegment1.dBx = tPrimaryUnconnectedSegment.dBx;
	tConnectedSegment1.dBy = tPrimaryUnconnectedSegment.dBy;
	tConnectedSegment1.dRadius = tPrimaryUnconnectedSegment.dRadius;
	tConnectedSegment1.dRx = tPrimaryUnconnectedSegment.dRx;
	tConnectedSegment1.dRy = tPrimaryUnconnectedSegment.dRy;
	tConnectedSegment1.dStartAngle = tPrimaryUnconnectedSegment.dStartAngle;
	tConnectedSegment1.dEndAngle = tPrimaryUnconnectedSegment.dEndAngle;
	// And these fields are not yet known.
	tConnectedSegment1.dA_ChainedMoveCompliance = -1.0;
	tConnectedSegment1.dB_ChainedMoveCompliance = -1.0;
	tConnectedSegment1.nA_ConnectedSegmentIndex = -1;
	tConnectedSegment1.nB_ConnectedSegmentIndex = -1;
	tConnectedSegment1.ucA_ConnectedSegmentEnd = 0;
	tConnectedSegment1.ucB_ConnectedSegmentEnd = 0;

	// Place the originating element back into the new connected & sorted array

	// QUESTION this...
	pSortedConnectedClass->SVC_AddElement(tConnectedSegment1);
	//	pSortedConnectedClass->SVC_SetElement(, tConnectedSegment1);
	nGroupedArrayNextIndex++;

	nSegmentsPlacedInGroup++;

	// Mark the non-grouped segment as grouped at this time
	tPrimaryUnconnectedSegment.lFeatureGroupIndex = nGroupIndex;
	if (bIsClosedPath)
	{
		pUnconnectedClass->SVC_SetElement(nFirstUnconnectedSegmentIndex, tPrimaryUnconnectedSegment);
	}
	else
	{
		pUnconnectedClass->SVC_SetElement(nUnconnectedsIndex, tPrimaryUnconnectedSegment);
 	}

	// Step #6, Starting with the next segment after the originating one, work through
	// the available non-grouped segments looking for connections to the working
	// segment's end. Build the sorted grouped segment SArray with these newly grouped segments

	int nLoops = 0;
	int nAttachedSecondaryIndex;
	int nAttachedSecondaryEnd;

	while (++nLoops <= (nNonGroupedSegments))
	{
		bConnectionFound = FindAttachedSegment (eFeatureCutType, dPrimarysWorkingEndX, dPrimarysWorkingEndY, nBeginningSecondaryIndex, nAvoidSecondaryIndex, &nAttachedSecondaryIndex, &nAttachedSecondaryEnd, pUnconnectedClass);

		if (!bConnectionFound && bIsClosedPath)
		{
	 		sprintf (chReport_TP, "Connection failure at X:%lf, Y:%lf", dPrimarysWorkingEndX, dPrimarysWorkingEndY);
			PostReport(chReport_TP);
			return -1;
		}

		if (!bConnectionFound && !bIsClosedPath)
		{
			// a.) Open path now complete, tie first & last segments together (even though not really connected) for routing purposes
			// b.) Open path now complete, disconnect first & last segments together (even though not really connected) for routing purposes
			if (1 < nSegmentsPlacedInGroup)
			{
				tConnectedSegment1 = pSortedConnectedClass->SVC_GetElement(0);
				if (1 == nUnconnectedsEnd)
				{
// a.)				tConnectedSegment1.nB_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
// a.)				tConnectedSegment1.ucB_ConnectedSegmentEnd = nPrimarysWorkingEnd;
					tConnectedSegment1.nB_ConnectedSegmentIndex = -1;
					tConnectedSegment1.ucB_ConnectedSegmentEnd = -1;
				}
				else
				{
// a.)				tConnectedSegment1.nA_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
// a.)				tConnectedSegment1.ucA_ConnectedSegmentEnd = nPrimarysWorkingEnd;
					tConnectedSegment1.nA_ConnectedSegmentIndex = -1;
					tConnectedSegment1.ucA_ConnectedSegmentEnd = -1;
				}

				tConnectedSegment2 = pSortedConnectedClass->SVC_GetElement(nGroupedArrayNextIndex-1);
				if (1 == nPrimarysWorkingEnd)
				{
// a.)				tConnectedSegment2.nA_ConnectedSegmentIndex = 0;
// a.)				tConnectedSegment2.ucA_ConnectedSegmentEnd = (1 == nUnconnectedsEnd) ? 2 : 1;
					tConnectedSegment2.nA_ConnectedSegmentIndex = -1;
					tConnectedSegment2.ucA_ConnectedSegmentEnd = -1;
				}
				else
				{
// a.)				tConnectedSegment2.nB_ConnectedSegmentIndex = 0;
// a.)				tConnectedSegment2.ucB_ConnectedSegmentEnd = (1 == nUnconnectedsEnd) ? 2 : 1;
					tConnectedSegment2.nB_ConnectedSegmentIndex = -1;
					tConnectedSegment2.ucB_ConnectedSegmentEnd = -1;
				}

				// Update both segments
				pSortedConnectedClass->SVC_SetElement(0, tConnectedSegment1);
				pSortedConnectedClass->SVC_SetElement(nGroupedArrayNextIndex-1, tConnectedSegment2);
			}
			return nSegmentsPlacedInGroup;
		}

		// Get the attached secondary segment
		tSecondaryUnconnectedSegment = pUnconnectedClass->SVC_GetElement(nAttachedSecondaryIndex);

		// Initialize sorted secondary segment
		tConnectedSegment2.eSegmentType = tSecondaryUnconnectedSegment.eSegmentType;
		tConnectedSegment2.lDxfEntityNumber = tSecondaryUnconnectedSegment.lDxfEntityNumber;
		tConnectedSegment2.nFeatureIndex = nGroupIndex;
		tConnectedSegment2.dAx = tSecondaryUnconnectedSegment.dAx;
		tConnectedSegment2.dAy = tSecondaryUnconnectedSegment.dAy;
		tConnectedSegment2.dBx = tSecondaryUnconnectedSegment.dBx;
		tConnectedSegment2.dBy = tSecondaryUnconnectedSegment.dBy;
		tConnectedSegment2.dRadius = tSecondaryUnconnectedSegment.dRadius;
		tConnectedSegment2.dRx = tSecondaryUnconnectedSegment.dRx;
		tConnectedSegment2.dRy = tSecondaryUnconnectedSegment.dRy;
		tConnectedSegment2.dStartAngle = tSecondaryUnconnectedSegment.dStartAngle;
		tConnectedSegment2.dEndAngle = tSecondaryUnconnectedSegment.dEndAngle;

		// And these fields are not yet known.
		tConnectedSegment2.dA_ChainedMoveCompliance = -1.0;
		tConnectedSegment2.dB_ChainedMoveCompliance = -1.0;

		// Get the attached segment & tie the two segments together
		tConnectedSegment1 = pSortedConnectedClass->SVC_GetElement(nGroupedArrayNextIndex-1);
		ConnectAttachedSegments (nGroupedArrayNextIndex, &tConnectedSegment1, &tConnectedSegment2, nPrimarysWorkingEnd, nAttachedSecondaryEnd);

		// Update primary & secondary sorted segment data
		pSortedConnectedClass->SVC_SetElement(nGroupedArrayNextIndex-1, tConnectedSegment1);
		// Question this...
		pSortedConnectedClass->SVC_AddElement(tConnectedSegment2);
//		pSortedConnectedClass->SVC_SetElement(nGroupedArrayNextIndex++, tConnectedSegment2);
		nGroupedArrayNextIndex++;
		nSegmentsPlacedInGroup++;

		// a.) Mark the non-grouped secondary segment as grouped
		// b.) The secondary element now becomes the primary element
		// c.) Set the searching index to avoid
		tSecondaryUnconnectedSegment.lFeatureGroupIndex = nGroupIndex;
		pUnconnectedClass->SVC_SetElement(nAttachedSecondaryIndex, tSecondaryUnconnectedSegment);
		tPrimaryUnconnectedSegment = pUnconnectedClass->SVC_GetElement(nAttachedSecondaryIndex);
		nAvoidSecondaryIndex = nAttachedSecondaryIndex;

		// Update the primary's working point
		if (1 == nAttachedSecondaryEnd)	// (A)
		{
			dPrimarysWorkingEndX = tPrimaryUnconnectedSegment.dBx;
			dPrimarysWorkingEndY = tPrimaryUnconnectedSegment.dBy;
			nPrimarysWorkingEnd = 2;
		}
		else
		{
			dPrimarysWorkingEndX = tPrimaryUnconnectedSegment.dAx;
			dPrimarysWorkingEndY = tPrimaryUnconnectedSegment.dAy;
			nPrimarysWorkingEnd = 1;
		}

		if (bIsClosedPath)
		{
			// Has route returned back to origination ?
			dDeltaX = fabs (dPrimarysWorkingEndX - dOriginationX);
			dDeltaY = fabs (dPrimarysWorkingEndY - dOriginationY);
			if ((dDeltaX < CONNECT_EPSILON) && (dDeltaY < CONNECT_EPSILON))
			{
				sprintf (chReport_TP, "Prim-Orig Connect X delta: %3.4E, Y delta: %3.4E\n", dDeltaX, dDeltaY);
				ReportBasedOnVerbosity(3, chReport_TP);
				ReportBasedOnVerbosity (4, "Closed loop connection found");
			 	if (2 <= nSegmentsPlacedInGroup)
				{
					// Tie last segment back to the first
					tConnectedSegment1 = pSortedConnectedClass->SVC_GetElement(0);
					tConnectedSegment1.nA_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
					tConnectedSegment1.ucA_ConnectedSegmentEnd = nPrimarysWorkingEnd;
					tConnectedSegment2 = pSortedConnectedClass->SVC_GetElement(nGroupedArrayNextIndex-1);

// *** THIS changes connection operation
//					tConnectedSegment2.nA_ConnectedSegmentIndex = 0;
//					if (1 == nPrimarysWorkingEnd)
//						tConnectedSegment2.ucA_ConnectedSegmentEnd = 1;
//					else
//						tConnectedSegment2.ucB_ConnectedSegmentEnd = 1;

					if (1 == nPrimarysWorkingEnd)
					{
						tConnectedSegment2.nA_ConnectedSegmentIndex = 0;
						tConnectedSegment2.ucA_ConnectedSegmentEnd = 1;
					}
					else
					{
						tConnectedSegment2.nB_ConnectedSegmentIndex = 0;
						tConnectedSegment2.ucB_ConnectedSegmentEnd = 1;
					}
// *** THIS changes connection operation


					// Update both segments
					pSortedConnectedClass->SVC_SetElement(0, tConnectedSegment1);
					pSortedConnectedClass->SVC_SetElement(nGroupedArrayNextIndex-1, tConnectedSegment2);
				}
				return nSegmentsPlacedInGroup;
			}
		}


		// Advance to the next secondary index
//		nBeginningSecondaryIndex = 	NextNonGroupedSegmentIndex (eFeatureCutType, nBeginningSecondaryIndex+1);
//		if (-1 == nBeginningSecondaryIndex)
//		{
//			TRACE("ExtractGroupedDxfSegments_III(), No more non-grouped unconnected segments available\n");
//			return 0;
//		}

	};	// end of while() loop

 	// Should have completed in for loop
	return -1;
}

bool CToolPaths::ConnectAttachedSegments (int nGroupedArrayNextIndex, CONNECTED_ELEMENT *pSegment1, CONNECTED_ELEMENT *pSegment2,  int nPrimarysWorkingEnd, int nSecondarysConnectedEnd)
{
	if (1 == nPrimarysWorkingEnd)	// (A end)
	{
		if (1 == nSecondarysConnectedEnd)
		{
			// PrimaryA <-> SecondaryA
			pSegment1->nA_ConnectedSegmentIndex = nGroupedArrayNextIndex;
			pSegment1->ucA_ConnectedSegmentEnd = 1;
			pSegment2->nA_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
			pSegment2->ucA_ConnectedSegmentEnd = 1;
		}
		else
		{
			// PrimaryA <-> SecondaryB
			pSegment1->nA_ConnectedSegmentIndex = nGroupedArrayNextIndex;
			pSegment1->ucA_ConnectedSegmentEnd = 2;
			pSegment2->nB_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
			pSegment2->ucB_ConnectedSegmentEnd = 1;
		}
	}
	else
	{
		if (1 == nSecondarysConnectedEnd)
		{
			// PrimaryB <-> SecondaryA
			pSegment1->nB_ConnectedSegmentIndex = nGroupedArrayNextIndex;
			pSegment1->ucB_ConnectedSegmentEnd = 1;
			pSegment2->nA_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
			pSegment2->ucA_ConnectedSegmentEnd = 2;
		}
		else
		{
			// PrimaryB <-> SecondaryB
			pSegment1->nB_ConnectedSegmentIndex = nGroupedArrayNextIndex;
			pSegment1->ucB_ConnectedSegmentEnd = 2;
			pSegment2->nB_ConnectedSegmentIndex = nGroupedArrayNextIndex-1;
			pSegment2->ucB_ConnectedSegmentEnd = 2;
		}
	}
	return true;
}

bool CToolPaths::CalculateFeaturesCcwRotationStartPoints (FEATURE_ELEMENT *pFeature, FEATURE_CUT_TYPE eFeatureCutType)
{
	CONNECTED_ELEMENT	tPrimarySegment;
	CONNECTED_ELEMENT	tSecondarySegment;
	STL_CONNECTED_SEGS_CLASS *pSaCenterlinePath = pFeature->m_pSaConnectedDxfSegments;
//	char chReport[200];

//	STL_VECTOR_CLASS *pUnconnectedClass, STL_CONNECTED_SEGS_CLASS *pSortedConnectedClass


	int nNumSegments = pFeature->m_nNumberSegments;
	if (nNumSegments < 2)
	{
// CIRCLE_MODS
		if (nNumSegments == 1)
		{
			tPrimarySegment = pSaCenterlinePath->SVC_GetElement(0);
			// Is this a circle ?
			if(tPrimarySegment.eSegmentType == eCircle)
			{
				// This is a guess ?
				tPrimarySegment.ucCcwRotationStartEndpoint = 1;
				pSaCenterlinePath->SVC_SetElement(0, tPrimarySegment);
				return true;
			}
			else if (tPrimarySegment.eSegmentType == eLine)
			{
				tPrimarySegment.ucStartEndpointAtInitialSort = 1;
				pSaCenterlinePath->SVC_SetElement (0, tPrimarySegment);
//				sprintf (chReport_TP, "BIG problem TBD\n");
				ReportBasedOnVerbosity(3, "CalculateFeaturesCcwRotationStartPoints(), feature contains only (1) segment.");
				return true;
			}
			else if (tPrimarySegment.eSegmentType == eArc)
			{
				tPrimarySegment.ucStartEndpointAtInitialSort = 1;
				pSaCenterlinePath->SVC_SetElement (0, tPrimarySegment);
				ReportBasedOnVerbosity(3, "CalculateFeaturesCcwRotationStartPoints(), feature contains only (1) segment.");
				return true;
			}
			else
			{
				ReportBasedOnVerbosity(-1, "CalculateFeaturesCcwRotationStartPoints().feature contains only (1) segment.");
				assert(false);
			}
		}
		else
		{
			ReportBasedOnVerbosity(-1, "CalculateFeaturesCcwRotationStartPoints()..Feature does not contain enough (2) segments.");
			return false;
		}
	}

	// JMW toDo: these angular orientations are probably not needed - consider removing them (cals done via decomposed arcs below)

	// Determine first segment's orientation angle
	tPrimarySegment = pSaCenterlinePath->SVC_GetElement(0);

	int nWorkingEnd;
	if (tPrimarySegment.nA_ConnectedSegmentIndex == -1)
	{
		// A end is unconnected and will be the starting point, B must be connected
		assert(tPrimarySegment.nB_ConnectedSegmentIndex != -1);
		tPrimarySegment.ucStartEndpointAtInitialSort = 1;
		nWorkingEnd = 2;
	}
	else if (tPrimarySegment.nB_ConnectedSegmentIndex == -1)
	{
		// B end is unconnected and will be the starting point, A must be connected
		assert(tPrimarySegment.nA_ConnectedSegmentIndex != -1);
		tPrimarySegment.ucStartEndpointAtInitialSort = 2;
		nWorkingEnd = 1;
	}
	else
	{
		// 11-17/2011 - Problem may lie here

		// Neither A or B end is unconnected, assume a closed segment with A end as the starting point
		tPrimarySegment.ucStartEndpointAtInitialSort = 1;
	//	tPrimarySegment.ucStartEndpointAtInitialSort = 2;
		nWorkingEnd = 2;
	}
	pSaCenterlinePath->SVC_SetElement (0, tPrimarySegment);

// CCW sort change - 07/05/2010

	// Now work the remaining segments
	for (int j=1; j<nNumSegments; j++)
	{
 		tSecondarySegment = pSaCenterlinePath->SVC_GetElement(j);

		if (1 == nWorkingEnd)
		{
			if (1 == tPrimarySegment.ucA_ConnectedSegmentEnd) // Primary-A connects to Secondary-A end
			{
//				dSecondarysAngle = atan2 (tSecondarySegment.dBy - tSecondarySegment.dAy, tSecondarySegment.dBx - tSecondarySegment.dAx);
				tSecondarySegment.ucStartEndpointAtInitialSort = 1;
				nWorkingEnd = 2;
			}
			else					// Primary-A connects to Secondary-B end
			{
//				dSecondarysAngle = atan2 (tSecondarySegment.dAy - tSecondarySegment.dBy, tSecondarySegment.dAx - tSecondarySegment.dBx);
				tSecondarySegment.ucStartEndpointAtInitialSort = 2;
				nWorkingEnd = 1;
			}
		}
		else
		{
			if (1 == tPrimarySegment.ucB_ConnectedSegmentEnd) // Primary-B connects to Secondary-A end
			{
//				dSecondarysAngle = atan2 (tSecondarySegment.dBy - tSecondarySegment.dAy, tSecondarySegment.dBx - tSecondarySegment.dAx);
				tSecondarySegment.ucStartEndpointAtInitialSort = 1;
				nWorkingEnd = 2;
			}
			else					// Primary-B connects to Secondary-B end
			{
//				dSecondarysAngle = atan2 (tSecondarySegment.dAy - tSecondarySegment.dBy, tSecondarySegment.dAx - tSecondarySegment.dBx);
				tSecondarySegment.ucStartEndpointAtInitialSort = 2;
				nWorkingEnd = 1;
			}
		}

//		if (dSecondarysAngle < 0.0)
//			dSecondarysAngle += 2.0*MATH_PI;

		// Update secondary element
//		tSecondarySegment.dAngularOrientation = (dSecondarysAngle * (180.0 / MATH_PI));
		pSaCenterlinePath->SVC_SetElement (j, tSecondarySegment);

		// Secondary segment now becomes primary
		tPrimarySegment = pSaCenterlinePath->SVC_GetElement(j);
	}

	// With rotational orientations done, determine loop direction (CW / CCW)
	double dAngularSum = 0;
	double dAngleBackRotation;

	// Decompose the entire loop into a sequence of line segments to be used to calculate angular rotations
	CONNECTED_ELEMENT	tSubArcLineSegment;
	STL_CONNECTED_SEGS_CLASS *pSaDecomposedSegments = new STL_CONNECTED_SEGS_CLASS();

	double dOrientationAngle;
// JMW ffdd 08/27/2014	int nLastIndex;
	int n;

	for (int i=0; i<nNumSegments; i++)
	{
 		tPrimarySegment = pSaCenterlinePath->SVC_GetElement(i);
		if (tPrimarySegment.eSegmentType == eLine)
		{
			tSubArcLineSegment.eSegmentType = eLine;
			tSubArcLineSegment.lDxfEntityNumber = tPrimarySegment.lDxfEntityNumber;
			tSubArcLineSegment.dAx = tPrimarySegment.dAx;
			tSubArcLineSegment.dAy = tPrimarySegment.dAy;
			tSubArcLineSegment.dBx = tPrimarySegment.dBx;
			tSubArcLineSegment.dBy = tPrimarySegment.dBy;

			// Configure starting point (A / B)
			tSubArcLineSegment.ucStartEndpointAtInitialSort = tPrimarySegment.ucStartEndpointAtInitialSort;

			// Calculate orientation angle
			if (1 == tSubArcLineSegment.ucStartEndpointAtInitialSort)
			{
				// Segment starts on A end
				dOrientationAngle = atan2 (tSubArcLineSegment.dBy - tSubArcLineSegment.dAy, tSubArcLineSegment.dBx - tSubArcLineSegment.dAx);
			}
			else
			{
				// Segment starts on B end
				dOrientationAngle = atan2 (tSubArcLineSegment.dAy - tSubArcLineSegment.dBy, tSubArcLineSegment.dAx - tSubArcLineSegment.dBx);
			}
			if (dOrientationAngle < 0.0)
				dOrientationAngle += 2.0*M_PI;
			tSubArcLineSegment.dAngularOrientation = (dOrientationAngle * (180.0 / M_PI));

			// Insert the element into the CSmatArray
// JMW ffdd 08/27/2014	nLastIndex = pSaDecomposedSegments->SVC_AddElement(tSubArcLineSegment);
			pSaDecomposedSegments->SVC_AddElement(tSubArcLineSegment);
		}
		else if (tPrimarySegment.eSegmentType == eArc)
		{
			// Decompose the arc into distinct quadrant line segments
			ARC_DATA tArcData;
			ARC_DATA tDecomposedArcs[6];
			// Setup arc data
			tArcData.dX1 = tPrimarySegment.dAx;
			tArcData.dY1 = tPrimarySegment.dAy;
			tArcData.dX2 = tPrimarySegment.dBx;
			tArcData.dY2 = tPrimarySegment.dBy;
			tArcData.dRx = tPrimarySegment.dRx;
			tArcData.dRy = tPrimarySegment.dRy;
			tArcData.dRadius = tPrimarySegment.dRadius;
			tArcData.dStartAngle = tPrimarySegment.dStartAngle;
			tArcData.dEndAngle = tPrimarySegment.dEndAngle;

			int nNumDecomposedArcs = -1;
			DecomposeArc_II (&tArcData, tDecomposedArcs, 6, &nNumDecomposedArcs);
		 	sprintf (chReport_TP, "CalculateFeaturesCcwRotationStartPoints(): Number of decomposed arcs: %d", nNumDecomposedArcs);
			ReportBasedOnVerbosity(4, chReport_TP);

			// Arc is decomposed from Sart -> End, reverse the ordering of the decomposed segments if arc started on B end
			if (2 == tPrimarySegment.ucStartEndpointAtInitialSort)
			{
				tSubArcLineSegment.eSegmentType = eLine;
				tSubArcLineSegment.lDxfEntityNumber = tPrimarySegment.lDxfEntityNumber;
				for (n=nNumDecomposedArcs-1; 0<=n; n--)
				{
					tSubArcLineSegment.dAx = tDecomposedArcs[n].dX1;
					tSubArcLineSegment.dAy = tDecomposedArcs[n].dY1;
					tSubArcLineSegment.dBx = tDecomposedArcs[n].dX2;
					tSubArcLineSegment.dBy = tDecomposedArcs[n].dY2;

					// Configure starting point (A / B)
					tSubArcLineSegment.ucStartEndpointAtInitialSort = tPrimarySegment.ucStartEndpointAtInitialSort;
					sprintf (chReport_TP, "RawLinesOfArcSubtends, DXF: %ld, Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, Starting end: %d", tPrimarySegment.lDxfEntityNumber, tSubArcLineSegment.dAx, tSubArcLineSegment.dAy, tSubArcLineSegment.dBx, tSubArcLineSegment.dBy, tSubArcLineSegment.ucStartEndpointAtInitialSort);
					ReportBasedOnVerbosity(2, chReport_TP);

					// Calculate orientation angle
					dOrientationAngle = atan2 (tSubArcLineSegment.dAy - tSubArcLineSegment.dBy, tSubArcLineSegment.dAx - tSubArcLineSegment.dBx);
					if (dOrientationAngle < 0.0)
						dOrientationAngle += 2.0*M_PI;
					tSubArcLineSegment.dAngularOrientation = (dOrientationAngle * (180.0 / M_PI));

					// Insert the element into the CSmatArray
// JMW ffdd 08/27/2014			nLastIndex = pSaDecomposedSegments->SVC_AddElement(tSubArcLineSegment);
					pSaDecomposedSegments->SVC_AddElement(tSubArcLineSegment);
					// nLastIndex will not be valid here
				}
			}
			else
			{
				tSubArcLineSegment.eSegmentType = eLine;
				tSubArcLineSegment.lDxfEntityNumber = tPrimarySegment.lDxfEntityNumber;
				for (n=0; n<nNumDecomposedArcs; n++)
				{
					tSubArcLineSegment.dAx = tDecomposedArcs[n].dX1;
					tSubArcLineSegment.dAy = tDecomposedArcs[n].dY1;
					tSubArcLineSegment.dBx = tDecomposedArcs[n].dX2;
					tSubArcLineSegment.dBy = tDecomposedArcs[n].dY2;

					// Configure starting point (A / B)
					tSubArcLineSegment.ucStartEndpointAtInitialSort = tPrimarySegment.ucStartEndpointAtInitialSort;
					sprintf (chReport_TP, "RawLinesOfArcSubtends, DXF: %ld, Ax: %lf, Ay: %lf, Bx: %lf, By: %lf, Starting end: %d", tPrimarySegment.lDxfEntityNumber, tSubArcLineSegment.dAx, tSubArcLineSegment.dAy, tSubArcLineSegment.dBx, tSubArcLineSegment.dBy, tSubArcLineSegment.ucStartEndpointAtInitialSort);
					ReportBasedOnVerbosity(2, chReport_TP);

					// Calculate orientation angle
					dOrientationAngle = atan2 (tSubArcLineSegment.dBy - tSubArcLineSegment.dAy, tSubArcLineSegment.dBx - tSubArcLineSegment.dAx);
					if (dOrientationAngle < 0.0)
						dOrientationAngle += 2.0*M_PI;
					tSubArcLineSegment.dAngularOrientation = (dOrientationAngle * (180.0 / M_PI));

					// Insert the element into the CSmatArray
// JMW ffdd 08/27/2014			nLastIndex = pSaDecomposedSegments->SVC_AddElement(tSubArcLineSegment);
					pSaDecomposedSegments->SVC_AddElement(tSubArcLineSegment);
					// nLastIndex will not be valid here
				}
			}
		}
	}

	// How many decomposed segments ?
	int nArraySize = pSaDecomposedSegments->SVC_Size();

	// Sum up all the differential angles of rotation
	double dAngleRotatedSecondary;
	for (n=0; n<nArraySize; n++)
	{
 		tPrimarySegment = pSaDecomposedSegments->SVC_GetElement(n);
		// Secondary wraps back to first segment on the last iteration
		if (n < (nArraySize-1))
			tSecondarySegment = pSaDecomposedSegments->SVC_GetElement(n+1);
		else
			tSecondarySegment = pSaDecomposedSegments->SVC_GetElement(0);

		// Rotate primary back to zero degrees
		dAngleBackRotation = 360.0 - tPrimarySegment.dAngularOrientation;
		dAngleRotatedSecondary = tSecondarySegment.dAngularOrientation + dAngleBackRotation;
		if (360.0 <= dAngleRotatedSecondary)
			dAngleRotatedSecondary -= 360.0;
		assert (dAngleRotatedSecondary < 360.0);

		// Line segments cannot rotate more than 180 degrees
		if (180.0 < dAngleRotatedSecondary)
			dAngleRotatedSecondary -= 360.0;

		dAngularSum += dAngleRotatedSecondary;
	}

	sprintf (chReport_TP, "Feature's angular sum: %lf", dAngularSum);
	ReportBasedOnVerbosity(4, chReport_TP);

	if (dAngularSum < 0.0)
	{
		if (fabs(dAngularSum + 360.0) < ANGULAR_EPSILON)
		{
			sprintf (chReport_TP, "Feature's angular sum: %lf, considered a full CW rotation\n", dAngularSum);
			ReportBasedOnVerbosity(4, chReport_TP);
			for (int n=0; n<nNumSegments; n++)
			{
				// Incorrect guess - reverse & update the segment's starting point flag
				tPrimarySegment = pSaCenterlinePath->SVC_GetElement(n);
				if (1 == tPrimarySegment.ucStartEndpointAtInitialSort)
					tPrimarySegment.ucCcwRotationStartEndpoint = 2;
				else
					tPrimarySegment.ucCcwRotationStartEndpoint = 1;
				pSaCenterlinePath->SVC_SetElement (n, tPrimarySegment);
			}
		}
		else
		{
			if (eCutOnFeature == eFeatureCutType)
			{
				// Determine that this is not a closed loop feature - if so it's OK that it fails the 360 test
				for (int n=0; n<nNumSegments; n++)
				{
					// Incorrect guess - reverse & update the segment's starting point flag
					tPrimarySegment = pSaCenterlinePath->SVC_GetElement(n);
					if (1 == tPrimarySegment.ucStartEndpointAtInitialSort)
						tPrimarySegment.ucCcwRotationStartEndpoint = 2;
					else
						tPrimarySegment.ucCcwRotationStartEndpoint = 1;
					pSaCenterlinePath->SVC_SetElement (n, tPrimarySegment);
				}
			}
			else
				assert(false);
		}
	}
	else
	{
		if (fabs(dAngularSum - 360.0) < ANGULAR_EPSILON)
		{
			sprintf (chReport_TP, "Feature's angular sum: %lf, considered a full CCW rotation\n", dAngularSum);
			ReportBasedOnVerbosity(4, chReport_TP);
			for (int n=0; n<nNumSegments; n++)
			{
				// Correct guess - update the segment's starting point flag
				tPrimarySegment = pSaCenterlinePath->SVC_GetElement(n);
				tPrimarySegment.ucCcwRotationStartEndpoint = tPrimarySegment.ucStartEndpointAtInitialSort;
				pSaCenterlinePath->SVC_SetElement (n, tPrimarySegment);
			}
		}
		else
		{
			if (eCutOnFeature == eFeatureCutType)
			{
				// Determine that this is not a closed loop feature - if so it's OK that it fails the 360 test
				for (int n=0; n<nNumSegments; n++)
				{
					tPrimarySegment = pSaCenterlinePath->SVC_GetElement(n);
					tPrimarySegment.ucCcwRotationStartEndpoint = tPrimarySegment.ucStartEndpointAtInitialSort;
					pSaCenterlinePath->SVC_SetElement (n, tPrimarySegment);
				}
			}
			else
				assert(false);
		}
	}

	delete pSaDecomposedSegments;

	return true;
}

bool CToolPaths::DecomposeArc_II (ARC_DATA *tArcData, ARC_DATA tDecomposedArcs[], int nMaxElements, int *pArcs)
{
	double dAng_SweepNeeded = tArcData->dEndAngle - tArcData->dStartAngle;

	if (dAng_SweepNeeded <= 0.0)
		dAng_SweepNeeded += 360.0;

	int nSegmentsNeeded = -1;
	if (dAng_SweepNeeded <= 90.0)
		nSegmentsNeeded = 1;
	else if (dAng_SweepNeeded <= 180.0)
		nSegmentsNeeded = 2;
	else if (dAng_SweepNeeded <= 270.0)
		nSegmentsNeeded = 3;
	else if (dAng_SweepNeeded <= 360.0)
		nSegmentsNeeded = 4;
	else
		assert(false);

	int nArrayIndex = 0;
	if (nSegmentsNeeded == 1)
	{
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle =tArcData->dStartAngle;
		tDecomposedArcs[nArrayIndex].dEndAngle = tArcData->dEndAngle;

		// Use orignal points since no decompostion needed
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dX1;
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dY1;
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dX2;
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dY2;
		// Return number of elements used
		*pArcs = 1;
		return true;
	}
	else if (nSegmentsNeeded == 2)
	{
		double dAngPerSubArc = dAng_SweepNeeded / 2.0;
		double dStart = tArcData->dStartAngle;
		double dEnd = tArcData->dStartAngle + dAngPerSubArc;

		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dX1;
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dY1;
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dRx + tArcData->dRadius * cos (dEnd * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dRy + tArcData->dRadius * sin (dEnd * M_PI / 180.0);

		dStart = tArcData->dStartAngle + dAngPerSubArc;
		dEnd = tArcData->dEndAngle;
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dRx + tArcData->dRadius * cos (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dRy + tArcData->dRadius * sin (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dX2;
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dY2;

		// Return number of elements used
		*pArcs = 2;
		return true;
	}
	else if (nSegmentsNeeded == 3)
	{
		double dAngPerSubArc = dAng_SweepNeeded / 3.0;
		double dStart = tArcData->dStartAngle;
		double dEnd = tArcData->dStartAngle + dAngPerSubArc;

		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dX1;
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dY1;
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dRx + tArcData->dRadius * cos (dEnd * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dRy + tArcData->dRadius * sin (dEnd * M_PI / 180.0);

		dStart = tArcData->dStartAngle + dAngPerSubArc;
		dEnd = tArcData->dStartAngle + (2.0*dAngPerSubArc);
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dRx + tArcData->dRadius * cos (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dRy + tArcData->dRadius * sin (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dRx + tArcData->dRadius * cos (dEnd * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dRy + tArcData->dRadius * sin (dEnd * M_PI / 180.0);

		dStart = tArcData->dStartAngle + (2.0*dAngPerSubArc);
		dEnd = tArcData->dEndAngle  + tArcData->dEndAngle;
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = tArcData->dStartAngle + dAngPerSubArc;
		tDecomposedArcs[nArrayIndex].dEndAngle = tArcData->dStartAngle + dAngPerSubArc + dAngPerSubArc;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dRx + tArcData->dRadius * cos (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dRy + tArcData->dRadius * sin (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dX2;
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dY2;

		// Return number of elements used
		*pArcs = 3;
		return true;
	}
	else if (nSegmentsNeeded == 4)
	{
		double dAngPerSubArc = dAng_SweepNeeded / 4.0;
		double dStart = tArcData->dStartAngle;
		double dEnd = tArcData->dStartAngle + dAngPerSubArc;

		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dX1;
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dY1;
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dRx + tArcData->dRadius * cos (dEnd * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dRy + tArcData->dRadius * sin (dEnd * M_PI / 180.0);

		dStart = tArcData->dStartAngle + dAngPerSubArc;
		dEnd = tArcData->dStartAngle + (2.0*dAngPerSubArc);
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dRx + tArcData->dRadius * cos (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dRy + tArcData->dRadius * sin (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dRx + tArcData->dRadius * cos (dEnd * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dRy + tArcData->dRadius * sin (dEnd * M_PI / 180.0);

		dStart = tArcData->dStartAngle + (2.0*dAngPerSubArc);
		dEnd = tArcData->dStartAngle + (3.0*dAngPerSubArc);
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = dStart;
		tDecomposedArcs[nArrayIndex].dEndAngle = dEnd;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dRx + tArcData->dRadius * cos (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dRy + tArcData->dRadius * sin (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dRx + tArcData->dRadius * cos (dEnd * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dRy + tArcData->dRadius * sin (dEnd * M_PI / 180.0);

		dStart = tArcData->dStartAngle + (3.0*dAngPerSubArc);
		dEnd = tArcData->dEndAngle;
		tDecomposedArcs[nArrayIndex].dRadius = tArcData->dRadius;
		tDecomposedArcs[nArrayIndex].dStartAngle = tArcData->dStartAngle + dAngPerSubArc;
		tDecomposedArcs[nArrayIndex].dEndAngle = tArcData->dStartAngle + dAngPerSubArc + dAngPerSubArc;
		tDecomposedArcs[nArrayIndex].dX1 = tArcData->dRx + tArcData->dRadius * cos (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dY1 = tArcData->dRy + tArcData->dRadius * sin (dStart * M_PI / 180.0);
		tDecomposedArcs[nArrayIndex].dX2 = tArcData->dX2;
		tDecomposedArcs[nArrayIndex++].dY2 = tArcData->dY2;

		// Return number of elements used
		*pArcs = 4;
		return true;
	}
	else
	{
		assert(false);
		return false;
	}
}

bool CToolPaths::CalculateMapFeatureBoundary (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex)
{


	if ((eCutInsideFeature != eFeatureCutType) && (eCutOutsideFeature != eFeatureCutType) && (eCutOnFeature != eFeatureCutType))
		assert(false);

	// Ensure it's a valid feature index
	int nFeatureCount = pFeaturesMap->SMC_GetSize();
	if (nFeatureCount <= nGroupIndex)
	{
 		PostReport("CalculateMapFeatureBoundary()..Not that many features in map\n");
		return false;
	}

	// Get the requested feature
	FEATURE_ELEMENT tFeature;
	if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
	{
		PostReport("CalculateMapFeatureBoundary(), Failure to get FEATURE\n");
		return false;
	}
	if (tFeature.m_nNumberSegments < 1)
	{
		PostReport("CalculateMapFeatureBoundary(), Feature does not contain any segments\n");
		return false;
	}

	// Calculate all centerline tool path segment boundaries (boundary info placed in segment structure)
	assert (tFeature.m_pSaConnectedDxfSegments);
	if (!CalculateSmartArraySegmentBoundaries (tFeature.m_pSaConnectedDxfSegments))
	{
		PostReport("CalculateMapFeatureBoundary()..CalculateSmartArraySegmentBoundaries() FAILED\n");
		return false;
	}

	// Given all the segment boundaries, find bounding region of all centerline segments (Feature)
	int nSaMaxSize = tFeature.m_pSaConnectedDxfSegments->SVC_Size();

	// Seed initial conditions
	double dMinX = 1.0E25, dMinY = 1.0E25;
	double dMaxX = -1.0E25, dMaxY = -1.0E25;

	CONNECTED_ELEMENT tSegment;

	// Find bounding region of Feature
	for (int j=0; j<nSaMaxSize; j++)
	{
		tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(j);

		// Check all boundary regions
		if (tSegment.rBoundaries.dLeft < dMinX)
			dMinX = tSegment.rBoundaries.dLeft;
		if (tSegment.rBoundaries.dBottom < dMinY)
			dMinY = tSegment.rBoundaries.dBottom;
		if (dMaxX < tSegment.rBoundaries.dRight)
			dMaxX = tSegment.rBoundaries.dRight;
		if (dMaxY < tSegment.rBoundaries.dTop)
			dMaxY = tSegment.rBoundaries.dTop;
 	}

	// Update boundary
	tFeature.m_BoundingRect.dLeft = dMinX;
	tFeature.m_BoundingRect.dBottom = dMinY;
	tFeature.m_BoundingRect.dRight = dMaxX;
	tFeature.m_BoundingRect.dTop = dMaxY;

	// Update feature map
	if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
	{
		PostReport("CalculateMapFeatureBoundary(), SMC_SetFeature() FAILED\n");
		return false;
	}

	return true;
}

bool CToolPaths::CalculateMapFeatureCentroid (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex)
{
	// TODO: Consolidate this into a common block
	if (eCutInsideFeature == eFeatureCutType)
	{
		int nFeatureCount = pFeaturesMap->SMC_GetSize();
		if (nFeatureCount <= nGroupIndex)
		{
			PostReport("CalculateMapFeatureCentroid()..Not that many features in map\n");
			return false;
		}
		// Get the requested feature
		FEATURE_ELEMENT tFeature;
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureBoundary(), Failure to get FEATURE\n");
			return false;
		}

// CIRCLE_MODS
		CONNECTED_ELEMENT	tSegment;
		if (tFeature.m_nNumberSegments == 1)
		{
			tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(0);
			if (tSegment.eSegmentType == eCircle)
			{
				tFeature.m_dCentroidX = tSegment.dRx;
				tFeature.m_dCentroidY = tSegment.dRy;

				// Update the map's feature
				if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
				{
					PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
					return false;
				}
				return true;
			}
			else
			{
				PostReport("CalculateMapFeatureCentroid()..Feature does not contain qualified segments\n");
				return false;
			}
		}

		if (tFeature.m_nNumberSegments < 2)
		{
			PostReport("CalculateMapFeatureCentroid()..Feature does not contain enough (2) segments\n");
			return false;
		}

		double dSumX = 0.0, dSumY = 0.0;
		int nPoints = 0;

		for (int j=0; j<tFeature.m_nNumberSegments; j++)
		{
			tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(j);
			if ((tSegment.eSegmentType == eLine) || (tSegment.eSegmentType == eArc))
			{
				dSumX += tSegment.dAx;
				dSumX += tSegment.dBx;
				dSumY += tSegment.dAy;
				dSumY += tSegment.dBy;
				nPoints += 2;
			}
			else
				assert(false);
		}

		double dMeanX = dSumX / (double)nPoints;
		double dMeanY = dSumY / (double)nPoints;
		tFeature.m_dCentroidX = dMeanX;
		tFeature.m_dCentroidY = dMeanY;

		// Update map
		if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
		{
			PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
			return false;
		}
 	}
	else if (eCutOutsideFeature == eFeatureCutType)
	{
		int nFeatureCount = pFeaturesMap->SMC_GetSize();
		if (nFeatureCount <= nGroupIndex)
		{
			PostReport("CalculateMapFeatureCentroid()..Not that many features in map\n");
			return false;
		}
		// Get the requested feature
		FEATURE_ELEMENT tFeature;
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureBoundary(), Failure to get FEATURE\n");
			return false;
		}

// CIRCLE_MODS
		CONNECTED_ELEMENT	tSegment;
		if (tFeature.m_nNumberSegments == 1)
		{
			tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(0);
			if (tSegment.eSegmentType == eCircle)
			{
				tFeature.m_dCentroidX = tSegment.dRx;
				tFeature.m_dCentroidY = tSegment.dRy;

				// Update the map's feature
				if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
				{
					PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
					return false;
				}
				return true;
			}
			else
			{
				PostReport("CalculateMapFeatureCentroid()..Feature does not contain qualified segments\n");
				return false;
			}
		}

		if (tFeature.m_nNumberSegments < 2)
		{
			PostReport("CalculateMapFeatureCentroid()..Feature does not contain enough (2) segments\n");
			return false;
		}
		double dSumX = 0.0, dSumY = 0.0;
		int nPoints = 0;

		for (int j=0; j<tFeature.m_nNumberSegments; j++)
		{
			tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(j);
			if ((tSegment.eSegmentType == eLine) || (tSegment.eSegmentType == eArc))
			{
				dSumX += tSegment.dAx;
				dSumX += tSegment.dBx;
				dSumY += tSegment.dAy;
				dSumY += tSegment.dBy;
				nPoints += 2;
			}
			else
				assert(false);
		}

		double dMeanX = dSumX / (double)nPoints;
		double dMeanY = dSumY / (double)nPoints;
		tFeature.m_dCentroidX = dMeanX;
		tFeature.m_dCentroidY = dMeanY;

		// Update map
		if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
		{
			PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
			return false;
		}
	}
	else if (eCutOnFeature == eFeatureCutType)
	{
		int nFeatureCount = pFeaturesMap->SMC_GetSize();
		if (nFeatureCount <= nGroupIndex)
		{
			PostReport("CalculateMapFeatureCentroid()..Not that many features in map\n");
			return false;
		}
		// Get the requested feature
		FEATURE_ELEMENT tFeature;
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureBoundary(), Failure to get FEATURE\n");
			return false;
		}

// CIRCLE_MODS
		CONNECTED_ELEMENT	tSegment;
//		if (tFeature.m_nNumberSegments == 1)
//		{
			tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(0);
			if (tSegment.eSegmentType == eCircle)
			{
				tFeature.m_dCentroidX = tSegment.dRx;
				tFeature.m_dCentroidY = tSegment.dRy;

				// Update the map's feature
				if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
				{
					PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
					return false;
				}
				return true;
			}
			else
			{
				double dSumX = 0.0, dSumY = 0.0;
				int nPoints = 0;

				for (int j=0; j<tFeature.m_nNumberSegments; j++)
				{
					tSegment = tFeature.m_pSaConnectedDxfSegments->SVC_GetElement(j);
					if ((tSegment.eSegmentType == eLine) || (tSegment.eSegmentType == eArc))
					{
						dSumX += tSegment.dAx;
						dSumX += tSegment.dBx;
						dSumY += tSegment.dAy;
						dSumY += tSegment.dBy;
						nPoints += 2;
					}
					else
						assert(false);
				}

				double dMeanX = dSumX / (double)nPoints;
				double dMeanY = dSumY / (double)nPoints;
				tFeature.m_dCentroidX = dMeanX;
				tFeature.m_dCentroidY = dMeanY;

				// Update map
				if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
				{
					PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
					return false;
				}
			}
//		}
//		if (tFeature.m_nNumberSegments < 2)
//		{
//			TRACE("CalculateMapFeatureCentroid()..Feature does not contain enough (2) segments\n");
//			return FALSE;
//		}

	}
	else
		assert(false);

	return true;
}

bool CToolPaths::CalculateSmartArraySegmentBoundaries (STL_CONNECTED_SEGS_CLASS *pConnectedClass)
{
//	char chReport[200];
	int nSaMaxIndex = pConnectedClass->SVC_Size();

	double dMinX, dMinY, dMaxX, dMaxY;
	CONNECTED_ELEMENT tSegment;

	for (int j=0; j<nSaMaxIndex; j++)
	{
		tSegment = pConnectedClass->SVC_GetElement(j);

		if (tSegment.eSegmentType == eLine)
		{
			tSegment.rBoundaries.dLeft = (tSegment.dAx < tSegment.dBx) ? tSegment.dAx : tSegment.dBx;
			tSegment.rBoundaries.dBottom = (tSegment.dAy < tSegment.dBy) ? tSegment.dAy : tSegment.dBy;
			tSegment.rBoundaries.dRight = (tSegment.dAx < tSegment.dBx) ? tSegment.dBx : tSegment.dAx;
			tSegment.rBoundaries.dTop = (tSegment.dAy < tSegment.dBy) ? tSegment.dBy : tSegment.dAy;
		}
		else if (tSegment.eSegmentType == eCircle)
		{
// CIRCLE_MODS
			tSegment.rBoundaries.dLeft = tSegment.dRx - tSegment.dRadius;
			tSegment.rBoundaries.dBottom = tSegment.dRy - tSegment.dRadius;
			tSegment.rBoundaries.dRight = tSegment.dRx + tSegment.dRadius;
			tSegment.rBoundaries.dTop = tSegment.dRy + tSegment.dRadius;
		}
		else if (tSegment.eSegmentType == eArc)
		{
			// Decompose the arc into distinct quadrant line segments
			ARC_DATA tArcData;
			ARC_DATA tDecomposedArcs[6];
			tArcData.lDxfEntityNumber = tSegment.lDxfEntityNumber;
			tArcData.dX1 = tSegment.dAx;
			tArcData.dY1 = tSegment.dAy;
			tArcData.dX2 = tSegment.dBx;
			tArcData.dY2 = tSegment.dBy;
			tArcData.dRx = tSegment.dRx;
			tArcData.dRy = tSegment.dRy;
			tArcData.dRadius = tSegment.dRadius;
			tArcData.dStartAngle = tSegment.dStartAngle;
			tArcData.dEndAngle = tSegment.dEndAngle;

			int nNumDecomposedArcs = -1;
			assert (DecomposeArc_II (&tArcData, tDecomposedArcs, 6, &nNumDecomposedArcs));
			sprintf (chReport_TP, "CalculateFeaturesCcwRotationStartPoints(): Number of decomposed arcs: %d\n", nNumDecomposedArcs);
			ReportBasedOnVerbosity(3, chReport_TP);

			// Seed initial conditions using mid-point of endpoints
			dMinX = dMaxX = (tSegment.dAx + tSegment.dBx) / 2.0;
			dMinY = dMaxY = (tSegment.dAy + tSegment.dBy) / 2.0;

			// Seed initial conditions
			dMinX = dMinY = 1.0E25;
			dMaxX = dMaxY = -1.0E25;

			for (int n=0; n<nNumDecomposedArcs; n++)
			{
				sprintf (chReport_TP, "RawLinesAndArcSubtends, DXF: %ld.%d, X1: %lf, Y1: %lf, X2: %lf, Y2: %lf", tSegment.lDxfEntityNumber, n+1, tDecomposedArcs[n].dX1, tDecomposedArcs[n].dY1, tDecomposedArcs[n].dX2, tDecomposedArcs[n].dY2);
				ReportBasedOnVerbosity(3, chReport_TP);
				// Check all minimums
				if (tDecomposedArcs[n].dX1 < dMinX)
					dMinX = tDecomposedArcs[n].dX1;
				if (tDecomposedArcs[n].dX2 < dMinX)
					dMinX = tDecomposedArcs[n].dX2;
				if (tDecomposedArcs[n].dY1 < dMinY)
					dMinY = tDecomposedArcs[n].dY1;
				if (tDecomposedArcs[n].dY2 < dMinY)
					dMinY = tDecomposedArcs[n].dY2;
					// Check all maximums
				if (dMaxX < tDecomposedArcs[n].dX1)
					dMaxX = tDecomposedArcs[n].dX1;
				if (dMaxX < tDecomposedArcs[n].dX2)
					dMaxX = tDecomposedArcs[n].dX2;
				if (dMaxY < tDecomposedArcs[n].dY1)
					dMaxY = tDecomposedArcs[n].dY1;
				if (dMaxY < tDecomposedArcs[n].dY2)
					dMaxY = tDecomposedArcs[n].dY2;
			}
			tSegment.rBoundaries.dLeft = dMinX;
			tSegment.rBoundaries.dBottom = dMinY;
			tSegment.rBoundaries.dRight = dMaxX;
			tSegment.rBoundaries.dTop = dMaxY;
		}
		else
			assert(false);

		// Update the segment
		pConnectedClass->SVC_SetElement(j, tSegment);
	}
	return true;
}

bool CToolPaths::GenerateMapFeatureToolPaths (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex, double dLoopOffset)
{
	CONNECTED_ELEMENT tNextSortedSegment;
	CONNECTED_ELEMENT tSegmentsLoopA;
	CONNECTED_ELEMENT tSegmentsLoopB;
	FEATURE_ELEMENT tFeature;
//	char chReport[200];

	if (eCutInsideFeature == eFeatureCutType)
	{
		if (pFeaturesMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
		{
			PostReport("GenerateMapFeatureToolPaths()..Not that many features in map\n");
			return false;
		}

		// Get the requested feature
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureBoundary(), Failure to get FEATURE\n");
			return false;
		}

		if (tFeature.m_nNumberSegments < 1)
		{
			PostReport("GenerateMapFeatureToolPaths()..Feature does not contain enough (1) segments\n");
			return false;
		}
	}
	else if (eCutOutsideFeature == eFeatureCutType)
	{
		if (pFeaturesMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
		{
			PostReport("GenerateMapFeatureToolPaths()..Not that many features in map\n");
			return false;
		}
		// Get the requested feature
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureBoundary(), Failure to get FEATURE\n");
			return false;
		}
		if (tFeature.m_nNumberSegments < 1)
		{
			PostReport("GenerateMapFeatureToolPaths()..Feature does not contain enough (1) segments\n");
			return false;
		}
	}
	else
		assert(false);

	// Get the centerline, pathA and pathB pointers
	STL_CONNECTED_SEGS_CLASS  *pSaCenterLineSegments = tFeature.m_pSaConnectedDxfSegments;
	STL_CONNECTED_SEGS_CLASS *pSaToolPathA = tFeature.m_pSaToolPathA;
	STL_CONNECTED_SEGS_CLASS *pSaToolPathB = tFeature.m_pSaToolPathB;
	assert (pSaToolPathA);
	assert (pSaToolPathB);

	// Remove any possible Loop-A elements
	int nArraySize = pSaToolPathA->SVC_Size();
//	TRACE("GenerateMapFeatureToolPath(), initial Loop-A SArray size: %d\n", nArraySize);
	pSaToolPathA->SVC_RemoveAllElements();
	nArraySize = pSaToolPathA->SVC_Size();
//	TRACE("GenerateMapFeatureToolPath(), post-RemoveAll() Loop-A SArray size: %d\n", nArraySize);

	// Remove any possible Loop-B elements
	nArraySize = pSaToolPathB->SVC_Size();
//	TRACE("GenerateMapFeatureToolPath(), initial Loop-B SArray size: %d\n", nArraySize);
	pSaToolPathB->SVC_RemoveAllElements();
	nArraySize = pSaToolPathB->SVC_Size();

// JMW ffdd 08/27/2014 - supress compiler warning
	nArraySize = nArraySize;

	CDoubleVector dvSegment, dvZ, dvLoopOffset;
	CDoubleVector dvA, dvAxZ, dvAxAxZ;

	// The unit Z vector
	dvZ.m_dX = 0.0;
	dvZ.m_dY = 0.0;
	dvZ.m_dZ = 1.0;

//	// Refresh tool diameter from radio buttons
//	UpdateToolDiameter();

 	int nNumberSegments = pSaCenterLineSegments->SVC_Size();

	if (1 == nNumberSegments)
	{
		tNextSortedSegment = pSaCenterLineSegments->SVC_GetElement(0);
		if (tNextSortedSegment.eSegmentType == eCircle)
		{
			tSegmentsLoopA.eSegmentType = eCircle;
			tSegmentsLoopA.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoopA.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoopA.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
			tSegmentsLoopA.dRx = tNextSortedSegment.dRx;
			tSegmentsLoopA.dRy = tNextSortedSegment.dRy;
			tSegmentsLoopA.dAx = tNextSortedSegment.dAx;
			tSegmentsLoopA.dAy = tNextSortedSegment.dAy + dLoopOffset;
			tSegmentsLoopA.dBx = tNextSortedSegment.dBx;
			tSegmentsLoopA.dBy = tNextSortedSegment.dBy + dLoopOffset;
			tSegmentsLoopA.dRadius = tNextSortedSegment.dRadius + dLoopOffset;
			tSegmentsLoopA.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoopA.dB_ChainedMoveCompliance = -1.0;
			pSaToolPathA->SVC_AddElement(tSegmentsLoopA);

			tSegmentsLoopB.eSegmentType = eCircle;
			tSegmentsLoopB.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoopB.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoopB.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
			tSegmentsLoopB.dRx = tNextSortedSegment.dRx;
			tSegmentsLoopB.dRy = tNextSortedSegment.dRy;
			tSegmentsLoopB.dAx = tNextSortedSegment.dAx;
			tSegmentsLoopB.dAy = tNextSortedSegment.dAy - dLoopOffset;
			tSegmentsLoopB.dBx = tNextSortedSegment.dBx;
			tSegmentsLoopB.dBy = tNextSortedSegment.dBy - dLoopOffset;
			tSegmentsLoopB.dRadius = tNextSortedSegment.dRadius - dLoopOffset;
			tSegmentsLoopB.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoopB.dB_ChainedMoveCompliance = -1.0;
			pSaToolPathB->SVC_AddElement(tSegmentsLoopB);
			return true;
		}
		return false;
	}
	for (int j=0; j<nNumberSegments; j++)
	{
		tNextSortedSegment = pSaCenterLineSegments->SVC_GetElement(j);
		if (tNextSortedSegment.eSegmentType == eLine)
		{
			// Construct a vector on top of the line segment and cross it with Z to get the perpendicular
			if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
			{
				// Starting with A-end
				dvSegment.m_dX = (tNextSortedSegment.dBx - tNextSortedSegment.dAx);
				dvSegment.m_dY = (tNextSortedSegment.dBy - tNextSortedSegment.dAy);
			}
			else
			{
				// Starting with B-end
				dvSegment.m_dX = (tNextSortedSegment.dAx - tNextSortedSegment.dBx);
				dvSegment.m_dY = (tNextSortedSegment.dAy - tNextSortedSegment.dBy);
			}

			dvSegment.m_dZ = 0.0;

// 2/1/2011	D3DXVECTOR3 vCrossProd;
			CDoubleVector dvCrossProd;


// 2/1/2011	D3DXVec3Cross(&vCrossProd, &vSegment, &vZ);
			dvCrossProd = CDoubleVector::CrossProduct2 (dvSegment, dvZ);

// 2/1/2011	D3DXVECTOR3 vNormalizedXProd;

//ttty			D3DXVec3Normalize (&vNormalized, &vCrossProd);
// 2/1/2011	vNormalizedXProd = vCrossProd / D3DXVec3Length(&vCrossProd);
			CDoubleVector dvNormalizedXProd = dvCrossProd.Normalize();

			dvLoopOffset = dvNormalizedXProd * dLoopOffset;
//			vLoopOffset = ((float)dLoopOffset) * Normalize(CrossProduct(vSegment, vZ));

			// Construct line segment to the right
			tSegmentsLoopA.eSegmentType = eLine;
			tSegmentsLoopA.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoopA.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoopA.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
// Experiment
			tSegmentsLoopA.nA_ConnectedSegmentIndex = -1;
			tSegmentsLoopA.nB_ConnectedSegmentIndex = -1;
// Experiment
			tSegmentsLoopA.dAx = tNextSortedSegment.dAx + dvLoopOffset.m_dX;
			tSegmentsLoopA.dAy = tNextSortedSegment.dAy + dvLoopOffset.m_dY;
			tSegmentsLoopA.dBx = tNextSortedSegment.dBx + dvLoopOffset.m_dX;
			tSegmentsLoopA.dBy = tNextSortedSegment.dBy + dvLoopOffset.m_dY;
			sprintf (chReport_TP, "GenerateMapFeatureToolPaths(), LoopA-line[%d]: Ax: %f, Ay: %f, Bx: %f By: %f", j, tSegmentsLoopA.dAx, tSegmentsLoopA.dAy, tSegmentsLoopA.dBx, tSegmentsLoopA.dBy);
			ReportBasedOnVerbosity(2, chReport_TP);

	 		// Calculate slopes & intercepts, watching out for vertical lines
			if (EQUIVALENCY_EPSILON < fabs(tSegmentsLoopA.dAx-tSegmentsLoopA.dBx))
			{
				tSegmentsLoopA.dLineSlope = (tSegmentsLoopA.dBy - tSegmentsLoopA.dAy) / (tSegmentsLoopA.dBx - tSegmentsLoopA.dAx);
				tSegmentsLoopA.dLineIntercept = tSegmentsLoopA.dAy - (tSegmentsLoopA.dLineSlope * tSegmentsLoopA.dAx);
				tSegmentsLoopA.bVerticalLine = false;
				sprintf (chReport_TP, "GenerateMapFeatureToolPaths(), LoopA-line[%d]: m: %f, b: %f", j, tSegmentsLoopA.dLineSlope, tSegmentsLoopA.dLineIntercept);
				ReportBasedOnVerbosity(2, chReport_TP);
			}
			else
			{
				sprintf (chReport_TP, "GenerateMapFeatureToolPaths(), LoopA-line[%d]: VerticalLine, m: INF", j);
				ReportBasedOnVerbosity(2, chReport_TP);
		 		tSegmentsLoopA.bVerticalLine = true;
			}

			tSegmentsLoopA.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoopA.dB_ChainedMoveCompliance = -1.0;
			pSaToolPathA->SVC_AddElement(tSegmentsLoopA);

			// Construct a second line segment to the left
			tSegmentsLoopB.eSegmentType = eLine;
			tSegmentsLoopB.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoopB.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoopB.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
// Experiment
			tSegmentsLoopB.nA_ConnectedSegmentIndex = -1;
			tSegmentsLoopB.nB_ConnectedSegmentIndex = -1;
// Experiment
			tSegmentsLoopB.dAx = tNextSortedSegment.dAx - dvLoopOffset.m_dX;
			tSegmentsLoopB.dAy = tNextSortedSegment.dAy - dvLoopOffset.m_dY;
			tSegmentsLoopB.dBx = tNextSortedSegment.dBx - dvLoopOffset.m_dX;
			tSegmentsLoopB.dBy = tNextSortedSegment.dBy - dvLoopOffset.m_dY;
			sprintf (chReport_TP, "GenerateMapFeatureToolPaths(), LoopB-line[%d]: Ax: %f, Ay: %f, Bx: %f By: %f", j, tSegmentsLoopB.dAx, tSegmentsLoopB.dAy, tSegmentsLoopB.dBx, tSegmentsLoopB.dBy);
			ReportBasedOnVerbosity(2, chReport_TP);

			// Calculate slopes & intercepts, watching out for vertical lines
			if (EQUIVALENCY_EPSILON < fabs(tSegmentsLoopB.dAx-tSegmentsLoopB.dBx))
//			if (tSegmentsLoopB.dAx != tSegmentsLoopB.dBx)
			{
				tSegmentsLoopB.dLineSlope = (tSegmentsLoopB.dBy - tSegmentsLoopB.dAy) / (tSegmentsLoopB.dBx - tSegmentsLoopB.dAx);
				tSegmentsLoopB.dLineIntercept = tSegmentsLoopB.dAy - (tSegmentsLoopB.dLineSlope * tSegmentsLoopB.dAx);
				tSegmentsLoopB.bVerticalLine = false;
				sprintf (chReport_TP, "GenerateMapFeatureToolPaths(), LoopB-line[%d]: m: %f, b: %f", j, tSegmentsLoopB.dLineSlope, tSegmentsLoopB.dLineIntercept);
				ReportBasedOnVerbosity(2, chReport_TP);
			}
			else
			{
			 	sprintf (chReport_TP, "GenerateMapFeatureToolPaths(), LoopB-line[%d]: VerticalLine, m: INF", j);
			 	ReportBasedOnVerbosity(2, chReport_TP);
			 	tSegmentsLoopB.bVerticalLine = true;
			}

			tSegmentsLoopB.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoopB.dB_ChainedMoveCompliance = -1.0;
			pSaToolPathB->SVC_AddElement(tSegmentsLoopB);
		}
		else if (tNextSortedSegment.eSegmentType == eArc)
		{
			// *** Construct an arc segment to the right ***
			tSegmentsLoopA.eSegmentType = eArc;
			tSegmentsLoopA.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoopA.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoopA.dRx = tNextSortedSegment.dRx;
			tSegmentsLoopA.dRy = tNextSortedSegment.dRy;
			tSegmentsLoopA.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;

			// Is arc curving to left ?
			if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
			{
				// Extend the radius a bit to guarantee intersecting segments
				// 2/26/2011	tSegmentsLoopA.dRad = tNextSortedSegment.dRad + dLoopOffset;
				tSegmentsLoopA.dRadius = 1.00001 * (tNextSortedSegment.dRadius + dLoopOffset);
				//	tSegmentsLoopA.dRad = 1.0001 * (tNextSortedSegment.dRad + (m_dToolDiameter/2.0));
			}
			else
			{
				// Extend the radius a bit to guarantee intersecting segments
				// 2/26/2011	tSegmentsLoopA.dRad = tNextSortedSegment.dRad - dLoopOffset;
				tSegmentsLoopA.dRadius = 1.00001 * (tNextSortedSegment.dRadius - dLoopOffset);
				//	tSegmentsLoopA.dRad = 1.0001 * (tNextSortedSegment.dRad - (m_dToolDiameter/2.0));
			}
 			// Calculate new projected endpoints
			CDoubleVector dvOrig, dvNew;
			dvOrig.m_dX = (tNextSortedSegment.dAx - tNextSortedSegment.dRx);
			dvOrig.m_dY = (tNextSortedSegment.dAy - tNextSortedSegment.dRy);
			dvOrig.m_dZ = 0.0;
			dvNew = dvOrig * (tSegmentsLoopA.dRadius / tNextSortedSegment.dRadius);
			tSegmentsLoopA.dAx = tNextSortedSegment.dRx + dvNew.m_dX;
			tSegmentsLoopA.dAy = tNextSortedSegment.dRy + dvNew.m_dY;

			dvOrig.m_dX = (tNextSortedSegment.dBx - tNextSortedSegment.dRx);
			dvOrig.m_dY = (tNextSortedSegment.dBy - tNextSortedSegment.dRy);
			dvNew = dvOrig * (tSegmentsLoopA.dRadius / tNextSortedSegment.dRadius);
			tSegmentsLoopA.dBx = tNextSortedSegment.dRx + dvNew.m_dX;
			tSegmentsLoopA.dBy = tNextSortedSegment.dRy + dvNew.m_dY;

			tSegmentsLoopA.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoopA.dB_ChainedMoveCompliance = -1.0;
			pSaToolPathA->SVC_AddElement(tSegmentsLoopA);

			// *** Construct a second arc segment to the left ***
			tSegmentsLoopB.eSegmentType = eArc;
			tSegmentsLoopB.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoopB.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoopB.dRx = tNextSortedSegment.dRx;
			tSegmentsLoopB.dRy = tNextSortedSegment.dRy;
			tSegmentsLoopB.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;

			// Is arc curving to left ?
			if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
			{
				// Extend the radius a bit to guarantee intersecting segments
				// 2/26/2011	tSegmentsLoopB.dRad = tNextSortedSegment.dRad - dLoopOffset;
				tSegmentsLoopB.dRadius = 1.00001 * (tNextSortedSegment.dRadius - dLoopOffset);
				//	tSegmentsLoopB.dRad = 1.0001 * (tNextSortedSegment.dRad - (m_dToolDiameter/2.0));
			}
			else
			{
				// Extend the radius a bit to guarantee intersecting segments
				// 2/26/2011	tSegmentsLoopB.dRad = tNextSortedSegment.dRad + dLoopOffset;
				tSegmentsLoopB.dRadius = 1.00001 * (tNextSortedSegment.dRadius + dLoopOffset);
				//	tSegmentsLoopB.dRad = 1.0001 * (tNextSortedSegment.dRad + (m_dToolDiameter/2.0));
			}

			// Calculate new projected endpoints
			dvOrig.m_dX = (tNextSortedSegment.dAx - tNextSortedSegment.dRx);
			dvOrig.m_dY = (tNextSortedSegment.dAy - tNextSortedSegment.dRy);
			dvOrig.m_dZ = 0.0f;
			dvNew = dvOrig * (tSegmentsLoopB.dRadius / tNextSortedSegment.dRadius);
			tSegmentsLoopB.dAx = tNextSortedSegment.dRx + dvNew.m_dX;
			tSegmentsLoopB.dAy = tNextSortedSegment.dRy + dvNew.m_dY;

			dvOrig.m_dX = (tNextSortedSegment.dBx - tNextSortedSegment.dRx);
			dvOrig.m_dY = (tNextSortedSegment.dBy - tNextSortedSegment.dRy);
			dvNew = dvOrig * (tSegmentsLoopB.dRadius / tNextSortedSegment.dRadius);
			tSegmentsLoopB.dBx = tNextSortedSegment.dRx + dvNew.m_dX;
			tSegmentsLoopB.dBy = tNextSortedSegment.dRy + dvNew.m_dY;

			tSegmentsLoopB.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoopB.dB_ChainedMoveCompliance = -1.0;
			pSaToolPathB->SVC_AddElement(tSegmentsLoopB);
		}
	}

	// (No connected end info at this point)
	DumpSegmentSmartArray ("Loop-A Unconnected", pSaToolPathA);
	DumpSegmentSmartArray ("Loop-B Unconnected", pSaToolPathB);

 	// Calculate all intersections for loop-A
	if (!CalculateOffsetSegmentIntersections (pSaToolPathA))
	{
		PostReport("GenerateMapFeatureToolPaths()..CalculateOffsetSegmentIntersections(LoopA) failed\n");
		return false;
	}

	// (Should be valid connected end info at this point)
	DumpSegmentSmartArray ("Loop-A1 Connected", pSaToolPathA);

	// Calculate all intersections for loop-B
	if (!CalculateOffsetSegmentIntersections (pSaToolPathB))
	{
		PostReport("GenerateMapFeatureToolPaths()..CalculateOffsetSegmentIntersections(LoopA) failed\n");
		return false;
	}

	// (Should be valid connected end info at this point)
	DumpSegmentSmartArray ("Loop-B Connected", pSaToolPathB);

	return true;
}

void CToolPaths::DumpSegmentSmartArray (const char *cText, STL_CONNECTED_SEGS_CLASS *pConnectedClass)
{
//	char chReport_[300];
	int nMaxSize = pConnectedClass->SVC_Size();

	CONNECTED_ELEMENT tSegment;

	sprintf (chReport_TP, "\n -- Array dump (%s), Number elements: %d --\n", cText, nMaxSize);
	ReportBasedOnVerbosity(2, chReport_TP);

	for (int j=0; j<nMaxSize; j++)
	{
		tSegment = pConnectedClass->SVC_GetElement(j);
 	 	if (tSegment.eSegmentType == eArc)
		{
			sprintf (chReport_TP, "Index: %d (%s), DXF: %ld, Ax: %f, Ay: %f, Bx: %f By: %f, CompA: %f CompB: %f, Sort-StartPoint: %d", j, (tSegment.eSegmentType == eLine ? "LINE" : "ARC "), tSegment.lDxfEntityNumber, tSegment.dAx, tSegment.dAy, tSegment.dBx, tSegment.dBy, tSegment.dA_ChainedMoveCompliance, tSegment.dB_ChainedMoveCompliance, tSegment.ucStartEndpointAtInitialSort);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
		else
		{
			sprintf (chReport_TP, "Index: %d (%s), DXF: %ld, Ax: %f, Ay: %f, Bx: %f By: %f, CompA: %f CompB: %f", j, (tSegment.eSegmentType == eLine ? "LINE" : "ARC "), tSegment.lDxfEntityNumber, tSegment.dAx, tSegment.dAy, tSegment.dBx, tSegment.dBy, tSegment.dA_ChainedMoveCompliance, tSegment.dB_ChainedMoveCompliance);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
		sprintf (chReport_TP, "                 DXF: %ld, A_ConIndex: %d(%s), B_ConIndex: %d(%s)", tSegment.lDxfEntityNumber, tSegment.nA_ConnectedSegmentIndex, (tSegment.ucA_ConnectedSegmentEnd==1) ? "A" : "B", tSegment.nB_ConnectedSegmentIndex, (tSegment.ucB_ConnectedSegmentEnd==1) ? "A" : "B");
		ReportBasedOnVerbosity(2, chReport_TP);
 	}
}

bool CToolPaths::CalculateOffsetSegmentIntersections (STL_CONNECTED_SEGS_CLASS *pConnectedClass)
{
	CONNECTED_ELEMENT	tSegment1;
	CONNECTED_ELEMENT	tSegment2;
	double				dInterceptX, dInterceptY;

 	int nMaxSegmentsSize = pConnectedClass->SVC_Size();

	// Do loop-A
	for (int j=0; j<nMaxSegmentsSize; j++)
	{
		tSegment1 = pConnectedClass->SVC_GetElement(j);
		// Wrap back to the beginning of the map for the final element
		if (j < nMaxSegmentsSize-1)
			tSegment2 = pConnectedClass->SVC_GetElement(j+1);
		else
			tSegment2 = pConnectedClass->SVC_GetElement(0);

		// Calculate intersection, both segments are lines
		if (tSegment1.eSegmentType==eLine && tSegment2.eSegmentType==eLine)
		{
			if (!tSegment1.bVerticalLine && !tSegment2.bVerticalLine)
			{
				// Neither segment is a vertical line - are they running the same slope?
				double dSlopeDelta = fabs (tSegment1.dLineSlope - tSegment2.dLineSlope);
		//		if (dSlopeDelta == 0.0)
				if (dSlopeDelta < EQUIVALENCY_EPSILON)
				{
					// Do they share a common point? - there are for possible connections
					if ((fabs(tSegment1.dAx-tSegment2.dAx) < EQUIVALENCY_EPSILON) && (fabs(tSegment1.dAy-tSegment2.dAy) < EQUIVALENCY_EPSILON))
					{
						dInterceptX = (tSegment1.dAx+tSegment2.dAx)/2.0;
						dInterceptY = (tSegment1.dAy+tSegment2.dAy)/2.0;
					}
					else if ((fabs(tSegment1.dAx-tSegment2.dBx) < EQUIVALENCY_EPSILON) && (fabs(tSegment1.dAy-tSegment2.dBy) < EQUIVALENCY_EPSILON))
					{
						dInterceptX = (tSegment1.dAx+tSegment2.dBx)/2.0;
						dInterceptY = (tSegment1.dAy+tSegment2.dBy)/2.0;
					}
					else if ((fabs(tSegment1.dBx-tSegment2.dAx) < EQUIVALENCY_EPSILON) && (fabs(tSegment1.dBy-tSegment2.dAy) < EQUIVALENCY_EPSILON))
					{
						dInterceptX = (tSegment1.dBx+tSegment2.dAx)/2.0;
						dInterceptY = (tSegment1.dBy+tSegment2.dAy)/2.0;
					}
					else if ((fabs(tSegment1.dBx-tSegment2.dBx) < EQUIVALENCY_EPSILON) && (fabs(tSegment1.dBy-tSegment2.dBy) < EQUIVALENCY_EPSILON))
					{
						dInterceptX = (tSegment1.dBx+tSegment2.dBx)/4.0;
						dInterceptY = (tSegment1.dBy+tSegment2.dBy)/2.0;
					}
					else
					{
						PostReport("CalculateOffsetSegmentIntersections(), failed to find intersection of co-linear lines\n");
						assert(false);
					}
				}
				else
				{
					dInterceptX = (tSegment2.dLineIntercept - tSegment1.dLineIntercept) / (tSegment1.dLineSlope - tSegment2.dLineSlope);
					dInterceptY = (tSegment1.dLineSlope * dInterceptX) + tSegment1.dLineIntercept;
				}
			}
			else if (tSegment1.bVerticalLine && !tSegment2.bVerticalLine)
			{
				// Segment #1 is a vertical line
				dInterceptX = tSegment1.dAx;
				dInterceptY = (tSegment2.dLineSlope * dInterceptX) + tSegment2.dLineIntercept;
			}
			else if (!tSegment1.bVerticalLine && tSegment2.bVerticalLine)
			{
				// Segment #2 is a vertical line
				dInterceptX = tSegment2.dAx;
				dInterceptY = (tSegment1.dLineSlope * dInterceptX) + tSegment1.dLineIntercept;
			}
			else
			{
				// Both are vertical - are they co-linear?
				double dVertDiff = fabs (((tSegment2.dAx+tSegment2.dBx)/2.0) - ((tSegment1.dAx+tSegment1.dBx)/2.0));
				if (dVertDiff < EQUIVALENCY_EPSILON)
				{
					// Do they share a common point? - there are for possible connections
					if (fabs(tSegment1.dAy - tSegment2.dAy) < EQUIVALENCY_EPSILON)
					{
						dInterceptX = (tSegment1.dAx+tSegment1.dBx+tSegment2.dAx+tSegment2.dBx)/4.0;
						dInterceptY = (tSegment1.dAy+tSegment2.dAy)/2.0;
					}
					else if (fabs(tSegment1.dAy - tSegment2.dBy) < EQUIVALENCY_EPSILON)
					{
						dInterceptX = (tSegment1.dAx+tSegment1.dBx+tSegment2.dAx+tSegment2.dBx)/4.0;
						dInterceptY = (tSegment1.dAy+tSegment2.dBy)/2.0;
					}
					else if (fabs(tSegment1.dBy - tSegment2.dAy) < EQUIVALENCY_EPSILON)
					{
						dInterceptX = (tSegment1.dAx+tSegment1.dBx+tSegment2.dAx+tSegment2.dBx)/4.0;
						dInterceptY = (tSegment1.dBy+tSegment2.dAy)/2.0;
					}
					else if (fabs(tSegment1.dBy - tSegment2.dBy) < EQUIVALENCY_EPSILON)
					{
						dInterceptX = (tSegment1.dAx+tSegment1.dBx+tSegment2.dAx+tSegment2.dBx)/4.0;
						dInterceptY = (tSegment1.dBy+tSegment2.dBy)/2.0;
					}
					else
					{
						PostReport("CalculateOffsetSegmentIntersections(), failed to find intersection of vertical lines\n");
						assert(false);
					}
				}
				else
				{
					PostReport("CalculateOffsetSegmentIntersections(), vertical lines are not co-linear\n");
					assert(false);
				}
			}

			// Connect the segments at the intersection point and update all connection info
			if (!ConnectSegments (&tSegment1, &tSegment2, dInterceptX, dInterceptY))
			{
				PostReport("CalculateOffsetSegmentIntersections()..ConnectSegments() failed\n");
				assert(false);
			}

	 		// Update both elements - wrap to the beginning at the end of the map
	 		if (!UpdateConnectedSegments (j, nMaxSegmentsSize, &tSegment1, &tSegment2, pConnectedClass))
			{
	 			PostReport("CalculateOffsetSegmentIntersections()..UpdateConnectedSegments() failed\n");
				assert(false);
			}
		}
		else if (tSegment1.eSegmentType==eLine && tSegment2.eSegmentType==eArc)
		{
			// Segment #1 is a line, segment #2 is an arc
			CIRCLE_OBJECT	Circle;
			LINE_OBJECT		Line;
//			double			dDistance;

			// Pre-calculate the separation distance
			if (1 == tSegment1.ucStartEndpointAtInitialSort)
			{
				Line.dAx = tSegment1.dAx;
				Line.dAy = tSegment1.dAy;
				Line.dBx = tSegment1.dBx;
				Line.dBy = tSegment1.dBy;
			}
			else
			{
				Line.dAx = tSegment1.dBx;
				Line.dAy = tSegment1.dBy;
				Line.dBx = tSegment1.dAx;
				Line.dBy = tSegment1.dAy;
			}

			Circle.dXc = tSegment2.dRx;
			Circle.dYc = tSegment2.dRy;
			Circle.dRadius = tSegment2.dRadius;

			// Is line-arc intersection possible
// TODO: Remove			ASSERT (CalculateCircleLineSeparationDist (&Circle, &Line, &dDistance));
			double dDistDoubleVector = -1.0E6;
			assert (CalculateCircleLineSeparationDistDoubleVector (&Circle, &Line, &dDistDoubleVector));
//			if (dDistance <= Circle.dRadius)
			if (dDistDoubleVector <= Circle.dRadius)
			{
				POINT_OBJECT Point1, Point2;
				assert (CalculateLineCircleIntersection_II (&Line, &Circle, &Point1, &Point2));
				// Which intersection point is closest to the line's endpoint
				double dSep1 = pow((Point1.dX-Line.dBx),2) + pow((Point1.dY-Line.dBy),2);
				double dSep2 = pow((Point2.dX-Line.dBx),2) + pow((Point2.dY-Line.dBy),2);
				if (dSep1 < dSep2)
				{
					dInterceptX = Point1.dX;
					dInterceptY = Point1.dY;
				}
				else
				{
					dInterceptX = Point2.dX;
					dInterceptY = Point2.dY;
				}
 			}
			else
			{
				// Todo: tangential radiuses
				POINT_OBJECT MidPoint;
				if (!FindLineCircleTangentialMidpoint (Line, Circle, &MidPoint))
				{
					PostReport("CalculateOffsetSegmentIntersections()..FindLineCircleTangentialMidPoint() failed\n");
					assert(false);
				}

				// Is point close enough to the line's B endpoint ?
				double dSep = pow((MidPoint.dX-Line.dBx),2) + pow((MidPoint.dY-Line.dBy),2);
				if (dSep < CONNECT_EPSILON)
				{
					dInterceptX = MidPoint.dX;
					dInterceptY = MidPoint.dY;
				}
				else
				{
					PostReport("CalculateOffsetSegmentIntersections()..Midpoint to far from line's endpoint\n");
					assert(false);
				}
			}

			// Connect the segments at the intersection point and update all connection info
			if (!ConnectSegments (&tSegment1, &tSegment2, dInterceptX, dInterceptY))
			{
				PostReport("CalculateOffsetSegmentIntersections()..ConnectSegments() failed\n");
				assert(false);
			}

			// Calculate new rotation angle for arc
			if (1 == tSegment2.ucStartEndpointAtInitialSort)
			{
				double dAngle = atan2 (tSegment2.dAy-Circle.dYc, tSegment2.dAx-Circle.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment2.dStartAngle = dAngle * (180.0 / M_PI);
			}
			else
			{
				double dAngle = atan2 (tSegment2.dBy-Circle.dYc, tSegment2.dBx-Circle.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment2.dEndAngle = dAngle * (180.0 / M_PI);
			}

 			// Update both elements - wrap to the beginning at the end of the map
	 		if (!UpdateConnectedSegments (j, nMaxSegmentsSize, &tSegment1, &tSegment2, pConnectedClass))
			{
	 			PostReport("CalculateOffsetSegmentIntersections()..UpdateConnectedSegments() failed\n");
	 			assert(false);
			}
		}
		else if (tSegment1.eSegmentType==eArc && tSegment2.eSegmentType==eLine)
		{
			// Segment #1 is an arc, segment #2 is a line
			CIRCLE_OBJECT	Circle;
			LINE_OBJECT		Line;
//			double			dDistance;

			// Pre-calculate the separation distance
			Circle.dXc = tSegment1.dRx;
			Circle.dYc = tSegment1.dRy;
			Circle.dRadius = tSegment1.dRadius;
			if (1 == tSegment2.ucStartEndpointAtInitialSort)
			{
				Line.dAx = tSegment2.dAx;
				Line.dAy = tSegment2.dAy;
				Line.dBx = tSegment2.dBx;
				Line.dBy = tSegment2.dBy;
			}
			else
			{
				Line.dAx = tSegment2.dBx;
				Line.dAy = tSegment2.dBy;
				Line.dBx = tSegment2.dAx;
				Line.dBy = tSegment2.dAy;
			}

			// Is arc-line intersection possible ?
// TODO: Remove			ASSERT (CalculateCircleLineSeparationDist (&Circle, &Line, &dDistance));
			double dDistDoubleVector = -1.0E6;
			assert (CalculateCircleLineSeparationDistDoubleVector (&Circle, &Line, &dDistDoubleVector));
//			if (dDistance <= Circle.dRadius)
			if (dDistDoubleVector <= Circle.dRadius)
			{
				POINT_OBJECT Point1, Point2;
				assert (CalculateLineCircleIntersection_II (&Line, &Circle, &Point1, &Point2));
				// Which point is closest to the line's endpoint
				double dSep1 = pow((Point1.dX-Line.dAx),2) + pow((Point1.dY-Line.dAy),2);
				double dSep2 = pow((Point2.dX-Line.dAx),2) + pow((Point2.dY-Line.dAy),2);
				if (dSep1 < dSep2)
				{
					dInterceptX = Point1.dX;
					dInterceptY = Point1.dY;
				}
				else
				{
					dInterceptX = Point2.dX;
					dInterceptY = Point2.dY;
				}
			}
			else
			{
				// Todo: tangential radiuses
				POINT_OBJECT MidPoint;
				if (!FindLineCircleTangentialMidpoint (Line, Circle, &MidPoint))
				{
					PostReport("CalculateOffsetSegmentIntersections()..FindLineCircleTangentialMidPoint() failed\n");
					assert(false);
				}

				// Is point close enough to the line's A endpoint ?
				double dSep = pow((MidPoint.dX-Line.dAx),2) + pow((MidPoint.dY-Line.dAy),2);
				if (dSep < CONNECT_EPSILON)
				{
					dInterceptX = MidPoint.dX;
					dInterceptY = MidPoint.dY;
				}
				else
				{
					PostReport("CalculateOffsetSegmentIntersections()..Midpoint to far from line's endpoint\n");
					assert(false);
				}
			}

			// Now, connect the segments at the intersection point and update all connection info
			if (!ConnectSegments (&tSegment1, &tSegment2, dInterceptX, dInterceptY))
			{
				PostReport("CalculateOffsetSegmentIntersections()..ConnectSegments() failed\n");
				assert(false);
			}

			// Calculate new rotation angle for arc
			if (1 == tSegment1.ucStartEndpointAtInitialSort)
			{
				double dAngle = atan2 (tSegment1.dBy-Circle.dYc, tSegment1.dBx-Circle.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment1.dEndAngle = dAngle * (180.0 / M_PI);
			}
			else
			{
				double dAngle = atan2 (tSegment1.dAy-Circle.dYc, tSegment1.dAx-Circle.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment1.dStartAngle = dAngle * (180.0 / M_PI);
			}

 			// Update both elements - wrap to the beginning at the end of the map
	 		if (!UpdateConnectedSegments (j, nMaxSegmentsSize, &tSegment1, &tSegment2, pConnectedClass))
			{
	 			PostReport("CalculateOffsetSegmentIntersections()..UpdateConnectedSegments() failed\n");
	 			assert(false);
			}
		}
		else if (tSegment1.eSegmentType==eArc && tSegment2.eSegmentType==eArc)
		{
			CIRCLE_OBJECT Circle1, Circle2;
			POINT_OBJECT Point1, Point2;
			Circle1.dXc = tSegment1.dRx;
			Circle1.dYc = tSegment1.dRy;
			Circle1.dRadius = tSegment1.dRadius;
			Circle2.dXc = tSegment2.dRx;
			Circle2.dYc = tSegment2.dRy;
			Circle2.dRadius = tSegment2.dRadius;

			// Is arc-arc intersection possible ?
			if ((Circle1.dXc==Circle2.dXc) && (Circle1.dYc==Circle2.dYc) && (Circle1.dRadius==Circle2.dRadius))
			{
				// Circles are duplicates (chained arcs)
				double dSep;
				if (1 == tSegment1.ucStartEndpointAtInitialSort)
				{
					if (1 == tSegment2.ucStartEndpointAtInitialSort)
					{
						// Does 1.B meet 2.A ?
						dSep = pow((tSegment1.dBx - tSegment2.dAx),2) + pow((tSegment1.dBy - tSegment2.dAy),2);
						if (1.0E-3 < dSep)
							assert(false);
						Point1.dX = Point2.dX = tSegment2.dAx;
						Point1.dY = Point2.dY = tSegment2.dAy;
					}
					else
					{
						// Does 1.B meet 2.B ?
						dSep = pow((tSegment1.dBx - tSegment2.dBx),2) + pow((tSegment1.dBy - tSegment2.dBy),2);
						if (1.0E-3 < dSep)
							assert(false);
						Point1.dX = Point2.dX = tSegment2.dBx;
						Point1.dY = Point2.dY = tSegment2.dBy;
					}
				}
				else
				{
					if (1 == tSegment2.ucStartEndpointAtInitialSort)
					{
						// Does 1.A meet 2.A ?
						dSep = pow((tSegment1.dAx - tSegment2.dAx),2) + pow((tSegment1.dAy - tSegment2.dAy),2);
						if (1.0E-3 < dSep)
							assert(false);
						Point1.dX = Point2.dX = tSegment2.dAx;
						Point1.dY = Point2.dY = tSegment2.dAy;
					}
					else
					{
						// Does 1.A meet 2.B ?
						dSep = pow((tSegment1.dAx - tSegment2.dBx),2) + pow((tSegment1.dAy - tSegment2.dBy),2);
						if (1.0E-3 < dSep)
							assert(false);
						Point1.dX = Point2.dX = tSegment2.dBx;
						Point1.dY = Point2.dY = tSegment2.dBy;
					}
				}
			}
			else
			{
				if (!CalculateCircleCircleIntersection_II(&Circle1, &Circle2, &Point1, &Point2))
				{
					assert(false);
					return false;
				}

			}

			// Which point is closest to the arc's original endpoint ?
			double dSep1, dSep2;
			if (1 == tSegment1.ucStartEndpointAtInitialSort)
			{
				dSep1 = pow((Point1.dX-tSegment1.dBx),2) + pow((Point1.dY-tSegment1.dBy),2);
				dSep2 = pow((Point2.dX-tSegment1.dBx),2) + pow((Point2.dY-tSegment1.dBy),2);
			}
			else
			{
				dSep1 = pow((Point1.dX-tSegment1.dAx),2) + pow((Point1.dY-tSegment1.dAy),2);
				dSep2 = pow((Point2.dX-tSegment1.dAx),2) + pow((Point2.dY-tSegment1.dAy),2);
			}
			if (dSep1 < dSep2)
			{
				dInterceptX = Point1.dX;
				dInterceptY = Point1.dY;
			}
			else
			{
				dInterceptX = Point2.dX;
				dInterceptY = Point2.dY;
			}

			// Connect the segments at the intersection point and update all connection info
			if (!ConnectSegments (&tSegment1, &tSegment2, dInterceptX, dInterceptY))
			{
				PostReport("CalculateOffsetSegmentIntersections()..ConnectSegments() failed\n");
				assert(false);
			}

 			// Calculate new rotation angle for arc #1
			if (1 == tSegment1.ucStartEndpointAtInitialSort)
			{
				double dAngle = atan2 (tSegment1.dBy-Circle1.dYc, tSegment1.dBx-Circle1.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment1.dEndAngle = dAngle * (180.0 / M_PI);
			}
			else
			{
				double dAngle = atan2 (tSegment1.dAy-Circle1.dYc, tSegment1.dAx-Circle1.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment1.dStartAngle = dAngle * (180.0 / M_PI);
			}

			// Calculate new rotation angle for arc #2
			if (1 == tSegment2.ucStartEndpointAtInitialSort)
			{
				double dAngle = atan2 (tSegment2.dAy-Circle2.dYc, tSegment2.dAx-Circle2.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment2.dStartAngle = dAngle * (180.0 / M_PI);
			}
			else
			{
				double dAngle = atan2 (tSegment2.dBy-Circle2.dYc, tSegment2.dBx-Circle2.dXc);
				if (dAngle < 0.0)
					dAngle += 2.0*M_PI;
				tSegment2.dEndAngle = dAngle * (180.0 / M_PI);
			}

			// Update both elements - wrap to the beginning at the end of the map
	 		if (!UpdateConnectedSegments (j, nMaxSegmentsSize, &tSegment1, &tSegment2, pConnectedClass))
			{
	 			PostReport("CalculateOffsetSegmentIntersections()..UpdateConnectedSegments() failed\n");
	 			assert(false);
			}
 		}
		else
			assert(false);
	}

	return true;
}

bool CToolPaths::ConnectSegments (CONNECTED_ELEMENT *pSeg1, CONNECTED_ELEMENT *pSeg2, double dInterceptX, double dInterceptY)
{
	if (1 == pSeg1->ucStartEndpointAtInitialSort)
	{
		pSeg1->dBx = dInterceptX;
		pSeg1->dBy = dInterceptY;
		pSeg1->ucB_ConnectedSegmentEnd = pSeg2->ucStartEndpointAtInitialSort;
	}
	else
	{
		pSeg1->dAx = dInterceptX;
		pSeg1->dAy = dInterceptY;
		pSeg1->ucA_ConnectedSegmentEnd = pSeg2->ucStartEndpointAtInitialSort;
	}
	if (1 == pSeg2->ucStartEndpointAtInitialSort)
	{
		pSeg2->dAx = dInterceptX;
		pSeg2->dAy = dInterceptY;
		pSeg2->ucA_ConnectedSegmentEnd = (1 == pSeg1->ucStartEndpointAtInitialSort) ? 2 : 1;
	}
	else
	{
		pSeg2->dBx = dInterceptX;
		pSeg2->dBy = dInterceptY;
		pSeg2->ucB_ConnectedSegmentEnd = (1 == pSeg1->ucStartEndpointAtInitialSort) ? 2 : 1;
	}
	return true;
}

bool CToolPaths::UpdateConnectedSegments (int nIndex, int nMaxSize, CONNECTED_ELEMENT *pSeg1, CONNECTED_ELEMENT *pSeg2, STL_CONNECTED_SEGS_CLASS *pConnectedLoopClass)
{
	if (nIndex < (nMaxSize-1))
	{
		if (1 == pSeg1->ucStartEndpointAtInitialSort)
			pSeg1->nB_ConnectedSegmentIndex = nIndex + 1;
		else
			pSeg1->nA_ConnectedSegmentIndex = nIndex + 1;

		if (1 == pSeg2->ucStartEndpointAtInitialSort)
			pSeg2->nA_ConnectedSegmentIndex = nIndex;
		else
			pSeg2->nB_ConnectedSegmentIndex = nIndex;

		pConnectedLoopClass->SVC_SetElement(nIndex+1, *pSeg2);
	}
	else
	{
		if (1 == pSeg1->ucStartEndpointAtInitialSort)
			pSeg1->nB_ConnectedSegmentIndex = 0;
		else
			pSeg1->nA_ConnectedSegmentIndex = 0;

		if (1 == pSeg2->ucStartEndpointAtInitialSort)
			pSeg2->nA_ConnectedSegmentIndex = nMaxSize-1;
		else
			pSeg2->nB_ConnectedSegmentIndex = nMaxSize-1;

		pConnectedLoopClass->SVC_SetElement(0, *pSeg2);
	}
	pConnectedLoopClass->SVC_SetElement(nIndex, *pSeg1);

	return true;
}

bool CToolPaths::CalculateCircleLineSeparationDistDoubleVector(CIRCLE_OBJECT *pCircle, LINE_OBJECT *pLine, double *pVal)
{
	// Odd cases first
	if (pLine->dBx != pLine->dAx)
	{
		// Calculate slope & intercept
		double dLm = (pLine->dBy - pLine->dAy) / (pLine->dBx - pLine->dAx);
		double dLb = pLine->dAy - dLm*pLine->dAx;

		// Does line pass through center of circle ?
		double dTestY = dLm * pCircle->dXc + dLb;
		if (dTestY == pCircle->dYc)
		{
			*pVal = 0.0;
			return true;
		}
	}
	else
	{
		// Does the vertical line pass through center of circle ?
		if (pLine->dAx == pCircle->dXc)
		{
			*pVal = 0.0;
			return true;
		}
	}

	// Vector from Line-A to circle's center
	CDoubleVector dvA ((pCircle->dXc-pLine->dAx), (pCircle->dYc-pLine->dAy), 0.0);

	// Vector from Line-A to Line-B
	CDoubleVector dvB ((pLine->dBx-pLine->dAx), (pLine->dBy-pLine->dAy), 0.0);

	// Unit vector B
	CDoubleVector dvUnitB = dvB.Normalize();

	// Vector from Line-A to perpendicular intercept
	CDoubleVector dvMidB;
	CDoubleVector dvPerpInt;	// Vector from perpendicular intercept (vMidB) to circle's center

 	// 1.) AdotB = |vA||vB| cos(o)
	// 2.) vMidB = |vMidB| vUnitB
	// 3.) cos(o) = |vMidB| / |vA|
	// #3 -> #2: vMidB = |vA| cos(o) vUnitB
	// #1 ->   : vMidB = (AdotB / |vB|) vUnitB

	double dA_dot_B = CDoubleVector::DotProduct2 (dvA, dvB);
	double dMagB = dvB.Magnitude();
	double dScale = dA_dot_B / dMagB;
	dvMidB = dvUnitB * dScale;

	dvPerpInt = dvA - dvMidB;

	*pVal = dvPerpInt.Magnitude();
	return true;
}

bool CToolPaths::CalculateLineCircleIntersection(LINE_OBJECT *pLine, CIRCLE_OBJECT *pCircle, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2)
{
	// Rule out non-intersecting vertical line cases
//	BOOL bVerticalLine = (pLine->dAx == pLine->dBx);
// 11-14-2011
	double dDeltaX = fabs (pLine->dAx - pLine->dBx);
	if (dDeltaX < CONNECT_EPSILON)
//	if (bVerticalLine)
// 11-14-2011
	{
		double dCircleMinX = pCircle->dXc - pCircle->dRadius;
		double dCircleMaxX = pCircle->dXc + pCircle->dRadius;
		double dX = pLine->dAx;
		// 08/01/13 fix
//		double dX = (pLine->dAx + pLine->dBx) / 2.0;

		if ((dCircleMinX == dX) || (dX == dCircleMaxX))
		{
			// Vertical line lies on left or right edge of circle
			pPoint1->dX = dX;
			pPoint2->dX = dX;
			pPoint1->dY = pCircle->dYc;
			pPoint2->dY = pCircle->dYc;
			return true;
		}
		// 07/29/13 fix
		else if ((fabs(dCircleMinX-dX) < CONNECT_EPSILON) || (fabs(dCircleMaxX-dX) < CONNECT_EPSILON))
		{
			pPoint1->dX = dX;
			pPoint2->dX = dX;
			pPoint1->dY =  pCircle->dYc;
			pPoint2->dY =  pCircle->dYc;
			return true;
		}
		// 07/29/13 fix
		else if ((dCircleMinX < dX) && (dX < dCircleMaxX))
		{
			// Vertical line lies within circle
			pPoint1->dX = dX;
			pPoint2->dX = dX;
			pPoint2->dY = pCircle->dYc + sqrt((pCircle->dRadius*pCircle->dRadius) - ((dX-pCircle->dXc)*(dX-pCircle->dXc)));
			pPoint1->dY = pCircle->dYc - sqrt((pCircle->dRadius*pCircle->dRadius) - ((dX-pCircle->dXc)*(dX-pCircle->dXc)));;
			return true;
		}
		else
		{
			PostReport ("CalculateLineCircleIntersection(), vertical line does not intersect circle, no intersection possible\n");
			return false;
		}
	}

	// Rule out non-intersecting horizontal line cases
//	BOOL bHorizLine = (pLine->dAy == pLine->dBy);
// 11-14-2011
	double dDeltaY = fabs (pLine->dAy - pLine->dBy);
	if (dDeltaY < CONNECT_EPSILON)
//	if (bHorizLine)
// 11-14-2011
	{
		double dCircleMinY = pCircle->dYc - pCircle->dRadius;
		double dCircleMaxY = pCircle->dYc + pCircle->dRadius;
		double dY = pLine->dBy;

		if ((dCircleMinY == dY) || (dY == dCircleMaxY))
		{
			pPoint1->dX = pCircle->dXc;
			pPoint2->dX = pCircle->dXc;
			pPoint1->dY = dY;
			pPoint2->dY = dY;
			return true;
		}
		// 07/29/13 fix
		else if ((fabs(dCircleMinY-dY) < CONNECT_EPSILON) || (fabs(dCircleMaxY-dY) < CONNECT_EPSILON))
		{
			pPoint1->dX = pCircle->dXc;
			pPoint2->dX = pCircle->dXc;
			pPoint1->dY = dY;
			pPoint2->dY = dY;
			return true;
		}

		else if ((dCircleMinY < dY) && (dY < dCircleMaxY))
		{
			pPoint1->dY = dY;
			pPoint2->dY = dY;
			pPoint2->dX = pCircle->dXc + sqrt((pCircle->dRadius*pCircle->dRadius) - ((dY-pCircle->dYc)*(dY-pCircle->dYc)));
			pPoint1->dX = pCircle->dXc - sqrt((pCircle->dRadius*pCircle->dRadius) - ((dY-pCircle->dYc)*(dY-pCircle->dYc)));;
			return true;
		}
		else
		{
			PostReport ("CalculateLineCircleIntersection(), horizontal line does not intersect circle, no intersection possible\n");
			return false;
		}
	}

	double dX0 = pCircle->dXc;
	double dY0 = pCircle->dYc;
	double dR0 = pCircle->dRadius;
	double dLm = (pLine->dBy - pLine->dAy) / (pLine->dBx - pLine->dAx);
	double dLb = pLine->dAy - dLm*pLine->dAx;

//	double dCheckY = 5.139049*dLm + dLb;

	// Calculate intercepts
	double dA = -2.0 * dX0;
	double dB = -2.0 * dY0;
	double dC = (dX0 * dX0) + (dY0 * dY0) - (dR0 * dR0);

	double dTermA = 1.0 + dLm*dLm;
	double dTermB = dA + dB*dLm + 2.0*dLm*dLb;
	double dTermC = dLb*dLb + dB*dLb + dC;

	double dRadicalTerm = (dTermB * dTermB) - (4.0 * dTermA * dTermC );

	if ((-ROOT_EPSILON<=dRadicalTerm) && (dRadicalTerm<=ROOT_EPSILON))
	{
		PostReport ("CalculateLineCircleIntersection(), radical term will be corrected to zero, single point tangential intersection likely\n");
		dRadicalTerm = 0.0;
	}
	else if (dRadicalTerm < 0.0)
	{
		PostReport ("CalculateLineCircleIntersection(), radical term is less than zero, no intersection possible\n");
		return false;
	}

	double dX_1 = (-dTermB + sqrt(dRadicalTerm)) / (2.0 * dTermA);
	double dY_1 = (dLm * dX_1) + dLb;

	double dX_2 = (-dTermB - sqrt(dRadicalTerm)) / (2.0 * dTermA);
	double dY_2 = (dLm * dX_2) + dLb;

	pPoint1->dX = dX_1;
	pPoint1->dY = dY_1;
	pPoint2->dX = dX_2;
	pPoint2->dY = dY_2;

	return true;
}

bool CToolPaths::CalculateLineCircleIntersection_II(LINE_OBJECT *pLine, CIRCLE_OBJECT *pCircle, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2)
{
	// Watch out for vertical lines - slope calculation will fail
	bool bVerticalLine = (pLine->dAx == pLine->dBx);
	double dSlopeCheck = 0.0;
	if (!bVerticalLine)
	{
		// Very steep slope lines will also be troublesome
		dSlopeCheck = (pLine->dBy - pLine->dAy) / (pLine->dBx - pLine->dAx);
		if ((-175.0 < dSlopeCheck) && (dSlopeCheck < 175.0))
		{
			// Now check for bad horizontal line conditions
			bool bHorizLine = (pLine->dAy == pLine->dBy);
			if (bHorizLine)
			{
				double dCircleMinY = pCircle->dYc - pCircle->dRadius;
				double dCircleMaxY = pCircle->dYc + pCircle->dRadius;
				double dMeanY = (pLine->dAy + pLine->dBy) / 2.0;

				if ((dCircleMinY == dMeanY) || (dMeanY == dCircleMaxY))
				{
					pPoint1->dX = pCircle->dXc;
					pPoint2->dX = pCircle->dXc;
					pPoint1->dY = dMeanY;
					pPoint2->dY = dMeanY;
					return true;
				}
				else if ((fabs(dCircleMinY-dMeanY) < CONNECT_EPSILON) || (fabs(dCircleMaxY-dMeanY) < CONNECT_EPSILON))
				{
					pPoint1->dX = pCircle->dXc;
					pPoint2->dX = pCircle->dXc;
					pPoint1->dY = dMeanY;
					pPoint2->dY = dMeanY;
					return true;
				}
				else if ((dCircleMinY < dMeanY) && (dMeanY < dCircleMaxY))
				{
					pPoint1->dY = dMeanY;
					pPoint2->dY = dMeanY;
					pPoint2->dX = pCircle->dXc + sqrt((pCircle->dRadius*pCircle->dRadius) - ((dMeanY-pCircle->dYc)*(dMeanY-pCircle->dYc)));
					pPoint1->dX = pCircle->dXc - sqrt((pCircle->dRadius*pCircle->dRadius) - ((dMeanY-pCircle->dYc)*(dMeanY-pCircle->dYc)));;
					return true;
				}
				else
				{
					PostReport ("CalculateLineCircleIntersection_II(), horizontal line does not intersect circle, no intersection possible\n");
					return false;
				}
			}
			else
			{
				// Calculate interception(s) based on line slope/intercept
				double dX0 = pCircle->dXc;
				double dY0 = pCircle->dYc;
				double dR0 = pCircle->dRadius;
				double dLm = (pLine->dBy - pLine->dAy) / (pLine->dBx - pLine->dAx);
				double dLb = pLine->dAy - dLm*pLine->dAx;

				double dA = -2.0 * dX0;
				double dB = -2.0 * dY0;
				double dC = (dX0 * dX0) + (dY0 * dY0) - (dR0 * dR0);

				double dTermA = 1.0 + dLm*dLm;
				double dTermB = dA + dB*dLm + 2.0*dLm*dLb;
				double dTermC = dLb*dLb + dB*dLb + dC;

				double dRadicalTerm = (dTermB * dTermB) - (4.0 * dTermA * dTermC );

				if ((-ROOT_EPSILON<=dRadicalTerm) && (dRadicalTerm<=ROOT_EPSILON))
				{
					PostReport ("CalculateLineCircleIntersection_II(), radical term will be corrected to zero, single point tangential intersection likely\n");
					dRadicalTerm = 0.0;
				}
				else if (dRadicalTerm < 0.0)
				{
					PostReport ("CalculateLineCircleIntersection_II(), radical term is less than zero, no intersection possible\n");
					return false;
				}

				double dX_1 = (-dTermB + sqrt(dRadicalTerm)) / (2.0 * dTermA);
				double dY_1 = (dLm * dX_1) + dLb;

				double dX_2 = (-dTermB - sqrt(dRadicalTerm)) / (2.0 * dTermA);
				double dY_2 = (dLm * dX_2) + dLb;

				pPoint1->dX = dX_1;
				pPoint1->dY = dY_1;
				pPoint2->dX = dX_2;
				pPoint2->dY = dY_2;

				return true;
			}
		}
	}

//	double dSlopeCheck = (pLine->dBy - pLine->dAy) / (pLine->dBx - pLine->dAx);
//	PostReport ("CalculateLineCircleIntersection_II(), Very steep line - intersection may not be possible\n");
//	sprintf (chReport_TP, "CalculateLineCircleIntersection_II(), Slope = %3.6E\n", dSlopeCheck);
//	ReportBasedOnVerbosity(-1, chReport_TP);

	// Vertical line or too steep - assume vertical line
	double dCircleMinX = pCircle->dXc - pCircle->dRadius;
	double dCircleMaxX = pCircle->dXc + pCircle->dRadius;
	double dMeanX = (pLine->dAx + pLine->dBx) / 2.0;

	if ((dCircleMinX == dMeanX) || (dMeanX == dCircleMaxX))
	{
		// Vertical line lies on left or right edge of circle
		pPoint1->dX = dMeanX;
		pPoint2->dX = dMeanX;
		pPoint1->dY = pCircle->dYc;
		pPoint2->dY = pCircle->dYc;
		return true;
	}
	else if ((fabs(dCircleMinX-dMeanX) < CONNECT_EPSILON) || (fabs(dCircleMaxX-dMeanX) < CONNECT_EPSILON))
	{
		// Vertical line lies very close to left or right edge of circle
		pPoint1->dX = dMeanX;
		pPoint2->dX = dMeanX;
		pPoint1->dY =  pCircle->dYc;
		pPoint2->dY =  pCircle->dYc;
		return true;
	}
	else if ((dCircleMinX < dMeanX) && (dMeanX < dCircleMaxX))
	{
		// Vertical line lies within circle
		pPoint1->dX = dMeanX;
		pPoint2->dX = dMeanX;
		pPoint2->dY = pCircle->dYc + sqrt((pCircle->dRadius*pCircle->dRadius) - ((dMeanX-pCircle->dXc)*(dMeanX-pCircle->dXc)));
		pPoint1->dY = pCircle->dYc - sqrt((pCircle->dRadius*pCircle->dRadius) - ((dMeanX-pCircle->dXc)*(dMeanX-pCircle->dXc)));
		return true;
	}
	else
	{
		PostReport ("CalculateLineCircleIntersection_II(), vertical line does not intersect circle, no intersection possible\n");
		sprintf (chReport_TP, "CalculateLineCircleIntersection_II(), Line (%3.6E, %3.6E) (%3.6E, %3.6E)\n",
                 pLine->dAx, pLine->dAy, pLine->dBx, pLine->dBy);
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CalculateLineCircleIntersection_II(), Circle (%3.6E, %3.6E, %3.6E)\n",
                 pCircle->dXc, pCircle->dYc, pCircle->dRadius);
		ReportBasedOnVerbosity(-1, chReport_TP);
		return false;
	}
}

bool CToolPaths::FindLineCircleTangentialMidpoint (LINE_OBJECT Line, CIRCLE_OBJECT Circle, POINT_OBJECT *pPoint)
{
	// ToDo : Verify line lies outside circle

	// Find the furthest line end point from the circle
//	D3DVECTOR v1;		// Vector from Line-A to circle's center
//	D3DXVECTOR3 v1;		// Vector from Line-A to circle's center
	CDoubleVector dv1;
	dv1.m_dX = (Circle.dXc - Line.dAx);
	dv1.m_dY = (Circle.dYc - Line.dAy);
	dv1.m_dZ = 0.0;

//	D3DVECTOR v2;		// Vector from Line-B to circle's center
//	D3DXVECTOR3 v2;		// Vector from Line-B to circle's center
	CDoubleVector dv2;
	dv2.m_dX = (Circle.dXc - Line.dBx);
	dv2.m_dY = (float)(Circle.dYc - Line.dBy);
	dv2.m_dZ = 0.0;

//	double dMag1 = (double)D3DXVec3Length(&v1);
//	double dMag2 = (double)D3DXVec3Length(&v2);
	double dMag1 = dv1.Magnitude();
	double dMag2 = dv2.Magnitude();

	if (dMag2 < dMag1)
	{
		// Work off A end of Line
//		D3DVECTOR vAC;			// Vector from Line-A to circle's center
//		D3DVECTOR vAB;			// Vector from Line-A to Line-B
//		D3DVECTOR vUnitAB;		// Unit vector AB
//		D3DVECTOR vAPerp;		// Vector from Line-A to perpendicular intercept
//		D3DVECTOR vCPerp;		// Vector from circle's center to perpendicular intercept (vAPerp)
//		D3DVECTOR vUnitCPerp;	// Unit vector from circle's center to perpendicular intercept (vAPerp)
//		D3DVECTOR vCRadialPoint;// Point on circle's radius (point #2)

//		D3DXVECTOR3 vAC;			// Vector from Line-A to circle's center
//		D3DXVECTOR3 vAB;			// Vector from Line-A to Line-B
//		D3DXVECTOR3 vUnitAB;		// Unit vector AB
//		D3DXVECTOR3 vAPerp;		// Vector from Line-A to perpendicular intercept
//		D3DXVECTOR3 vCPerp;		// Vector from circle's center to perpendicular intercept (vAPerp)
//		D3DXVECTOR3 vUnitCPerp;	// Unit vector from circle's center to perpendicular intercept (vAPerp)
//		D3DXVECTOR3 vCRadialPoint;// Point on circle's radius (point #2)

		CDoubleVector dvAC;			// Vector from Line-A to circle's center
		CDoubleVector dvAB;			// Vector from Line-A to Line-B
		CDoubleVector dvUnitAB;		// Unit vector AB
		CDoubleVector dvAPerp;		// Vector from Line-A to perpendicular intercept
		CDoubleVector dvCPerp;		// Vector from circle's center to perpendicular intercept (vAPerp)
		CDoubleVector dvUnitCPerp;	// Unit vector from circle's center to perpendicular intercept (vAPerp)
		CDoubleVector dvCRadialPoint;// Point on circle's radius (point #2)

		dvAC.m_dX = (Circle.dXc - Line.dAx);
		dvAC.m_dY = (Circle.dYc - Line.dAy);
		dvAC.m_dZ = 0.0;

		dvAB.m_dX = (Line.dBx - Line.dAx);
		dvAB.m_dY = (Line.dBy - Line.dAy);
		dvAB.m_dZ = 0.0;

		// 1.) ACdotAB = |vAC||vAB| cos(o)
		// 2.) vAPerp = |vAPerp| vUnitAB
		// 3.) cos(o) = |vAPerp| / |vAC|

		// #3 -> #2: vAPerp = |vAC| cos(o) vUnitB
		// #1 ->   : vAPerp = (ACdotAB / |vAB|) vUnitB

//		vUnitAB = Normalize(vAB);
//ttty	D3DXVec3Normalize (&vUnitAB, &vAB);
//		vUnitAB = vAB / D3DXVec3Length(&vAB);
		dvUnitAB = dvAB.Normalize();
//		double dMagAB = Magnitude (vAB);
//		double dMagAB = (double)D3DXVec3Length(&vAB);
		double dMagAB = dvAB.Magnitude();

//		double dA_dot_AB = DotProduct (vAC, vAB);
//		double dA_dot_AB = (double)D3DXVec3Dot(&vAC, &vAB);
		double dA_dot_AB = CDoubleVector::DotProduct2(dvAC, dvAB);

//		vAPerp = (float)(dA_dot_AB / dMagAB) * vUnitAB;
		dvAPerp = dvUnitAB * (dA_dot_AB / dMagAB);

		POINT_OBJECT tPtC1, tPtC2;			// Point of interest on C1 & C2.
		tPtC1.dX = Line.dAx + dvAPerp.m_dX;
		tPtC1.dY = Line.dAy + dvAPerp.m_dY;

//		vCPerp = vAPerp - vAC;
		dvCPerp = dvAPerp - dvAC;

//ttty		D3DXVec3Normalize (&vUnitCPerp, &vCPerp);
//		vUnitCPerp = Normalize(vCPerp);
//		vUnitCPerp = vCPerp / D3DXVec3Length(&vCPerp);
		dvUnitCPerp = dvCPerp.Normalize();
//		vCRadialPoint = (float)Circle.dRadius * vUnitCPerp;
		dvCRadialPoint = dvUnitCPerp * Circle.dRadius;

		tPtC2.dX = Circle.dXc + dvCRadialPoint.m_dX;
		tPtC2.dY = Circle.dYc + dvCRadialPoint.m_dY;

		double dDistBetweenPoints = sqrt ((tPtC2.dX-tPtC1.dX)*(tPtC2.dX-tPtC1.dX) + (tPtC2.dY-tPtC1.dY)*(tPtC2.dY-tPtC1.dY));
		if (dDistBetweenPoints < 1.0E-3)
		{
			// Use midpoint
			pPoint->dX = (tPtC2.dX+tPtC1.dX) / 2.0;
			pPoint->dY  = (tPtC2.dY+tPtC1.dY) / 2.0;
			return true;
		}
		else
		{
			PostReport ("Tangential intersections did not meet acceptance ctiteria\n");
			return true;
		}
	}
	else
	{
		// Work off B end of Line
//		D3DVECTOR vBC;			// Vector from Line-B to circle's center
//		D3DVECTOR vBA;			// Vector from Line-B to Line-A
//		D3DVECTOR vUnitBA;		// Unit vector BA
//		D3DVECTOR vBPerp;		// Vector from Line-B to perpendicular intercept
//		D3DVECTOR vCPerp;		// Vector from circle's center to perpendicular intercept (vBPerp)
//		D3DVECTOR vUnitCPerp;	// Unit vector from circle's center to perpendicular intercept (vBPerp)
//		D3DVECTOR vCRadialPoint;// Point on circle's radius (point #2)

//		D3DXVECTOR3 vBC;			// Vector from Line-B to circle's center
//		D3DXVECTOR3 vBA;			// Vector from Line-B to Line-A
//		D3DXVECTOR3 vUnitBA;		// Unit vector BA
//		D3DXVECTOR3 vBPerp;		// Vector from Line-B to perpendicular intercept
//		D3DXVECTOR3 vCPerp;		// Vector from circle's center to perpendicular intercept (vBPerp)
//		D3DXVECTOR3 vUnitCPerp;	// Unit vector from circle's center to perpendicular intercept (vBPerp)
//		D3DXVECTOR3 vCRadialPoint;// Point on circle's radius (point #2)

		CDoubleVector dvBC;			// Vector from Line-B to circle's center
		CDoubleVector dvBA;			// Vector from Line-B to Line-A
		CDoubleVector dvUnitBA;		// Unit vector BA
		CDoubleVector dvBPerp;		// Vector from Line-B to perpendicular intercept
		CDoubleVector dvCPerp;		// Vector from circle's center to perpendicular intercept (vBPerp)
		CDoubleVector dvUnitCPerp;	// Unit vector from circle's center to perpendicular intercept (vBPerp)
		CDoubleVector dvCRadialPoint;// Point on circle's radius (point #2)

		dvBC.m_dX = (Circle.dXc - Line.dBx);
		dvBC.m_dY = (Circle.dYc - Line.dBy);
		dvBC.m_dZ = 0.0;

		dvBA.m_dX = (Line.dAx - Line.dBx);
		dvBA.m_dY = (Line.dAy - Line.dBy);
		dvBA.m_dZ = 0.0;

		// 1.) BCdotBA = |vBC||vBA| cos(o)
		// 2.) vBPerp = |vBPerp| vUnitBA
		// 3.) cos(o) = |vBPerp| / |vBC|

		// #3 -> #2: vBPerp = |vBC| cos(o) vUnitBA
		// #1 ->   : vBPerp = (BCdotBA / |vBA|) vUnitBA

//		vUnitBA = Normalize(vBA);
//ttty		D3DXVec3Normalize (&vUnitBA, &vBA);
//		vUnitBA = vBA / D3DXVec3Length(&vBA);
		dvUnitBA = dvBA.Normalize();

//		double dMagBA = Magnitude (vBA);
//		double dMagBA = (double)D3DXVec3Length (&vBA);
		double dMagBA = dvBA.Magnitude();
//		double dBC_dot_BA = DotProduct (vBC, vBA);
//		double dBC_dot_BA = (double)D3DXVec3Dot(&vBC, &vBA);
		double dBC_dot_BA = CDoubleVector::DotProduct2(dvBC, dvBA);
//		vBPerp = (float)(dBC_dot_BA / dMagBA) * vUnitBA;
		dvBPerp = dvUnitBA * (dBC_dot_BA / dMagBA);

		POINT_OBJECT tPtC1, tPtC2;			// Point of interest on C1 & C2.
		tPtC1.dX = Line.dBx + dvBPerp.m_dX;
		tPtC1.dY = Line.dBy + dvBPerp.m_dY;

		dvCPerp = dvBPerp - dvBC;
//		vUnitCPerp = Normalize(vCPerp);
//ttty		D3DXVec3Normalize (&vUnitCPerp, &vCPerp);
//		vUnitCPerp = vCPerp / D3DXVec3Length (&vCPerp);
		dvUnitCPerp = dvCPerp.Normalize();
		dvCRadialPoint = dvUnitCPerp * Circle.dRadius;

		tPtC2.dX = Circle.dXc + dvCRadialPoint.m_dX;
		tPtC2.dY = Circle.dYc + dvCRadialPoint.m_dY;

		double dDistBetweenPoints = sqrt ((tPtC2.dX-tPtC1.dX)*(tPtC2.dX-tPtC1.dX) + (tPtC2.dY-tPtC1.dY)*(tPtC2.dY-tPtC1.dY));
		if (dDistBetweenPoints < 1.0E-3)
		{
			// Use midpoint
			pPoint->dX = (tPtC2.dX+tPtC1.dX) / 2.0;
			pPoint->dY  = (tPtC2.dY+tPtC1.dY) / 2.0;
			return true;

		}
		else
		{
			PostReport ("Tangential intersections did not meet acceptance ctiteria\n");
			return false;
		}
	}
}

bool CToolPaths::CalculateCircleCircleIntersection_II(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2)
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
		ReportBasedOnVerbosity(-1, "CalculateCircleCircleIntersection_II(), circles are on center - aborting");
		assert(false);
		return false;
	}

	// Make certain intersection is possible
	double dDistBetweenCenters = sqrt ((dX1-dX0)*(dX1-dX0) + (dY1-dY0)*(dY1-dY0));
	double dSumOfRadius = dR0 + dR1;
	double dDiffOfRadius = fabs (dR0 - dR1);
	double dDelta = dDistBetweenCenters - dSumOfRadius;

	if (fabs(dDelta) <= EQUIVALENCY_EPSILON)
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
			ReportBasedOnVerbosity(2, "CalculateCircleCircleIntersection_II(), circles are tangent to each other...");
			pPoint1->dX = tPtC1.dX;
			pPoint1->dY = tPtC1.dY;
			pPoint2->dX = tPtC2.dX;
			pPoint2->dY = tPtC2.dY;
			return true;
		}
		else
		{
			ReportBasedOnVerbosity(-1, "CalculateCircleCircleIntersection_II(), circles are likely tangent to each other...\n");
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
		// Are they really close?
		if (fabs(dDiffOfRadius-dDistBetweenCenters) < CONNECT_EPSILON)
		{
			double dVrr_X, dVrr_Y;				// Vector from center of C1 to center of C2.
			double duVrr_X, duVrr_Y;			// Unit vector based on Vrr
			POINT_OBJECT tPtC0, tPtC1;			// Point of interest on C1 & C2.

			// Is C0 contained in C1
			if (dR0 < dR1)
			{
				// Vector from center of C1 to center of C0.
				dVrr_X = dX0 - dX1;
				dVrr_Y = dY0 - dY1;

			}
			else
			{
				// Vector from center of C0 to center of C1.
				dVrr_X = dX1 - dX0;
				dVrr_Y = dY1 - dY0;
			}

			// Unit vector of Vrr.
			duVrr_X = dVrr_X / dDistBetweenCenters;
			duVrr_Y = dVrr_Y / dDistBetweenCenters;

			// Subtended intersection point on C0 & C1.
			tPtC0.dX = dX0 + (dR0 * duVrr_X);
			tPtC0.dY = dY0 + (dR0 * duVrr_Y);
			tPtC1.dX = dX1 + (dR1 * duVrr_X);
			tPtC1.dY = dY1 + (dR1 * duVrr_Y);

			ReportBasedOnVerbosity(2, "CalculateCircleCircleIntersection_II(), circles are self-contained but close enough...");
			pPoint1->dX = (tPtC0.dX + tPtC1.dX) / 2.0;
			pPoint1->dY = (tPtC0.dY + tPtC1.dY) / 2.0;;
			pPoint2->dX = pPoint1->dX;
			pPoint2->dY = pPoint1->dY;
			return true;
		}

		ReportBasedOnVerbosity(-1, "Circles are non-intersecting (self contained)\n");
		assert(false);
		return false;
	}

	// C0 = 2(X1 - X0)
	// C1 = 2(Y1 - Y0)
	double dC0 = 2.0 * (dX1 - dX0);
	double dC1 = 2.0 * (dY1 - dY0);
	double dC3 = (dR0*dR0) - (dR1*dR1) + (dX1*dX1) + (dY1*dY1) - (dX0*dX0) - (dY0*dY0);

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
			return false;
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
			return false;
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

bool CToolPaths::CalculateCircleCircleIntersection(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2)
{
	// Original - replaced 4/22/13
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
		double dY_1 = (-dTermB + sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);
		double dY_2 = (-dTermB - sqrt((dTermB * dTermB) - (4.0 * dTermA * dTermC ))) / (2.0 * dTermA);

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

bool CToolPaths::VerifyArcCenterPoint(ARC_DATA *tArcData, double dAbsError, double dRelError)
{
	double dX1, dY1, dX2, dY2, dRx, dRy;

	dX1 = tArcData->dX1;
	dY1 = tArcData->dY1;
	dX2 = tArcData->dX2;
	dY2 = tArcData->dY2;
	dRx = tArcData->dRx;
	dRy = tArcData->dRy;

// JMW ffdd 08/27/2014 	
	// long nDxfEntityNum = tArcData->lDxfEntityNumber;

	double dCalcRadius1 = sqrt ((dX1-dRx)*(dX1-dRx) + (dY1-dRy)*(dY1-dRy));
	double dCalcRadius2 = sqrt ((dX2-dRx)*(dX2-dRx) + (dY2-dRy)*(dY2-dRy));
	double dRadiusDelta = dCalcRadius2 - dCalcRadius1;
	double dRadiusAbsDelta = fabs(dRadiusDelta);

//	if (tArcData->lDxfEntityNumber == 81)
//	{
//		sprintf (chReport_TP, "CToolPaths::VACP(DXF# %ld)... Ax: %lf, Ay: %lf\n", tArcData->lDxfEntityNumber, tArcData->dX1, tArcData->dY1);
//		ReportBasedOnVerbosity(-1, chReport_TP);
//		sprintf (chReport_TP, "CToolPaths::VACP(DXF# %ld)... Bx: %lf, By: %lf\n", tArcData->lDxfEntityNumber, tArcData->dX2, tArcData->dY2);
//		ReportBasedOnVerbosity(-1, chReport_TP);
//		sprintf (chReport_TP, "CToolPaths::VACP(DXF# %ld)... Rx: %lf, Ry: %lf\n", tArcData->lDxfEntityNumber, tArcData->dRx, tArcData->dRy);
//		ReportBasedOnVerbosity(-1, chReport_TP);
//	}

	bool bRetStatus = true;
	if (dAbsError < dRadiusAbsDelta)
	{
		PostReport("CToolPaths::VerifyArcCenterPoint()...Absolute error outside bounds\n");
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Abs Err: %3.6E\n", tArcData->lDxfEntityNumber, dRadiusAbsDelta);
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Ax: %lf, Ay: %lf\n", tArcData->lDxfEntityNumber, tArcData->dX1, tArcData->dY1);
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Bx: %lf, By: %lf\n", tArcData->lDxfEntityNumber, tArcData->dX2, tArcData->dY2);
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Rx: %lf, Ry: %lf\n", tArcData->lDxfEntityNumber, tArcData->dRx, tArcData->dRy);
		ReportBasedOnVerbosity(-1, chReport_TP);
		bRetStatus = false;
	}

	if (dRelError < (dRadiusAbsDelta / ((dCalcRadius1+dCalcRadius2)/2.0)))
	{
		PostReport("CToolPaths::VerifyArcCenterPoint()...Relative error outside bounds\n");
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Rel Err: %3.6E\n", tArcData->lDxfEntityNumber, (dRadiusAbsDelta / ((dCalcRadius1+dCalcRadius2)/2.0)));
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Ax: %lf, Ay: %lf\n", tArcData->lDxfEntityNumber, tArcData->dX1, tArcData->dY1);
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Bx: %lf, By: %lf\n", tArcData->lDxfEntityNumber, tArcData->dX2, tArcData->dY2);
		ReportBasedOnVerbosity(-1, chReport_TP);
		sprintf (chReport_TP, "CToolPaths::VerifyArcCenterPoint(DXF# %ld)... Rx: %lf, Ry: %lf\n", tArcData->lDxfEntityNumber, tArcData->dRx, tArcData->dRy);
		ReportBasedOnVerbosity(-1, chReport_TP);
		bRetStatus = false;
	}

	return bRetStatus;
}

bool CToolPaths::CalculateMapFeatureToolPathLengths (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex, float *pLengthA, float *pLengthB)
{
	FEATURE_ELEMENT		tFeature;
	CONNECTED_ELEMENT	tSegmentLoopA;
	CONNECTED_ELEMENT	tSegmentLoopB;

	if (eCutInsideFeature == eFeatureCutType)
	{
		if (pFeaturesMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
		{
			PostReport("CalculateMapFeatureToolPathLengths()..Not that many features in map\n");
			return false;
		}

		// Get the requested feature
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureToolPathLengths(), Failure to get FEATURE\n");
			return false;
		}

		if (tFeature.m_nNumberSegments < 1)
		{
			PostReport("CalculateMapFeatureToolPathLengths()..Feature does not contain enough (1) segments\n");
			return false;
		}
	}
	else if (eCutOutsideFeature == eFeatureCutType)
	{
		if (pFeaturesMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
		{
			PostReport("CalculateMapFeatureToolPathLengths()..Not that many features in map\n");
			return false;
		}
		if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
		{
			PostReport("CalculateMapFeatureToolPathLengths(), Failure to get FEATURE\n");
			return false;
		}
		if (tFeature.m_nNumberSegments < 1)
		{
			PostReport("CalculateMapFeatureToolPathLengths()..Feature does not contain enough (1) segments\n");
			return false;
		}
	}
	else
		assert(false);

	// Get the pathA and pathB pointers
	STL_CONNECTED_SEGS_CLASS  *pSaToolPathA = tFeature.m_pSaToolPathA;
	STL_CONNECTED_SEGS_CLASS  *pSaToolPathB = tFeature.m_pSaToolPathB;
	assert (pSaToolPathA);
	assert (pSaToolPathB);

	// Calculate loop length
	CDoubleVector dvRoute;
	float fLengthLoopA = 0.0;
	float fLengthLoopB = 0.0;

	for (int j=0; j<tFeature.m_nNumberSegments; j++)
	{
 		tSegmentLoopA = pSaToolPathA->SVC_GetElement(j);
		tSegmentLoopB = pSaToolPathB->SVC_GetElement(j);

		if (tSegmentLoopA.eSegmentType == eLine)
		{
			dvRoute.m_dX = (tSegmentLoopA.dBx - tSegmentLoopA.dAx);
			dvRoute.m_dY = (tSegmentLoopA.dBy - tSegmentLoopA.dAy);
			dvRoute.m_dZ = 0.0;
// 2/1/2011	fLengthLoopA += D3DXVec3Length (&vRoute);
			fLengthLoopA += (float)dvRoute.Magnitude();
//			fLengthLoopA += Magnitude (vRoute);

//			if (3 <= TRACE_VERBOSITY)
//				TRACE ("CalculateRouteLengths(), [%d]: Line LengthA: %f, AccumLenA: %f\n", j, dvRoute.Magnitude(), fLengthLoopA);

			dvRoute.m_dX = (tSegmentLoopB.dBx - tSegmentLoopB.dAx);
			dvRoute.m_dY = (tSegmentLoopB.dBy - tSegmentLoopB.dAy);
			dvRoute.m_dZ = 0.0;
//			fLengthLoopB += Magnitude (vRoute);
			fLengthLoopB += (float)dvRoute.Magnitude();

//			if (2 <= TRACE_VERBOSITY)
//				TRACE ("CalculateRouteLengths(), [%d]: Line LengthB: %f, AccumLenB: %f\n", j, dvRoute.Magnitude(), fLengthLoopB);
		}
		else if (tSegmentLoopA.eSegmentType == eCircle)
		{
// CIRCLE_MODS
			double dTotalCircumferenceA = 2.0 * M_PI * tSegmentLoopA.dRadius;
			fLengthLoopA += (float)dTotalCircumferenceA;
//			if (-2 <= TRACE_VERBOSITY)
//				TRACE ("CalculateRouteLengths(), [%d]: Circle circumference A: %f, AccumLenA: %f\n", j, dTotalCircumferenceA, fLengthLoopA);

			double dTotalCircumferenceB = 2.0 * M_PI * tSegmentLoopB.dRadius;
			fLengthLoopB += (float)dTotalCircumferenceB;
//			if (-2 <= TRACE_VERBOSITY)
//				TRACE ("CalculateRouteLengths(), [%d]: Circle circumference B: %f, AccumLenB: %f\n", j, dTotalCircumferenceB, fLengthLoopB);
		}
		else if (tSegmentLoopA.eSegmentType == eArc)
		{
			double dSweepAngleA = tSegmentLoopA.dEndAngle - tSegmentLoopA.dStartAngle;
			if (dSweepAngleA < 0.0)
				dSweepAngleA += 360.0;
			double dTotalCircumferenceA = 2.0 * M_PI * tSegmentLoopA.dRadius;
			double dArcLengthA = (dSweepAngleA/360.0) * dTotalCircumferenceA;
			fLengthLoopA += (float)dArcLengthA;
//			if (2 <= TRACE_VERBOSITY)
//				TRACE ("CalculateRouteLengths(), [%d]: Arc LengthA: %f, AccumLenA: %f\n", j, dArcLengthA, fLengthLoopA);

			double dSweepAngleB = tSegmentLoopB.dEndAngle - tSegmentLoopB.dStartAngle;
			if (dSweepAngleB < 0.0)
				dSweepAngleB += 360.0;
			double dTotalCircumferenceB = 2.0 * M_PI * tSegmentLoopB.dRadius;
			double dArcLengthB = (dSweepAngleB/360.0) * dTotalCircumferenceB;
			fLengthLoopB += (float)dArcLengthB;
//			if (2 <= TRACE_VERBOSITY)
//				TRACE ("CalculateRouteLengths(), [%d]: Arc LengthB: %f, AccumLenB: %f\n", j, dArcLengthB, fLengthLoopB);
		}
		else
			assert(false);
	}

	// Update map
	tFeature.m_dToolPathLengthA = fLengthLoopA;
	tFeature.m_dToolPathLengthB = fLengthLoopB;
	if ((eCutInsideFeature == eFeatureCutType) || (eCutOutsideFeature == eFeatureCutType))
	{
		if (!pFeaturesMap->SMC_UpdateFeature(nGroupIndex, tFeature))
		{
			PostReport("CalculateMapFeatureCentroid(), SMC_SetFeature() FAILED\n");
			return false;
		}
	}
	else
		assert(false);

	*pLengthA = fLengthLoopA;
	*pLengthB = fLengthLoopB;

	return true;
}

bool CToolPaths::GenerateMapFeatureToolPath (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex, double dLoopOffset)
{
	if ((eCutInsideFeature != eFeatureCutType) && (eCutOutsideFeature != eFeatureCutType))
		assert(false);

	// Make certain the indexed feature is available
	if (pFeaturesMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
	{
		PostReport("GenerateMapFeatureToolPath()..Not that many features in map\n");
		return false;
	}
	// Get the requested feature
	FEATURE_ELEMENT tFeature;
	if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
	{
		PostReport("GenerateMapFeatureToolPath(), Failure to get FEATURE\n");
		return false;
	}
	if ((eCutInsideFeature == eFeatureCutType) || (eCutOutsideFeature == eFeatureCutType))
	{
		if (tFeature.m_nNumberSegments < 1)
		{
			PostReport("GenerateMapFeatureToolPath()..Feature does not contain enough/any segments\n");
			return false;
		}
	}

	// Determine which side of centerline tool path will fall on
	unsigned char bySide;
	if (eCutInsideFeature == eFeatureCutType)
	{
		if (tFeature.m_dToolPathLengthA < tFeature.m_dToolPathLengthB)
			bySide = 1;
		else
			bySide = 2;
	}
	else
	{
		if (tFeature.m_dToolPathLengthA < tFeature.m_dToolPathLengthB)
			bySide = 2;
		else
			bySide = 1;
	}

	// Get the centerline, and tool path pointers
	STL_CONNECTED_SEGS_CLASS  *pSaCenterLineSegments = tFeature.m_pSaConnectedDxfSegments;
	STL_CONNECTED_SEGS_CLASS  *pSaOffsetToolPath = tFeature.m_pSaToolPathFinishCut;

	assert (pSaOffsetToolPath);

	// Remove any possible offset loop elements
	int nArraySize = pSaOffsetToolPath->SVC_Size();
//	TRACE("GenerateMapFeatureToolPath(), initial offset loop SArray size: %d\n", nArraySize);
	pSaOffsetToolPath->SVC_RemoveAllElements();
	nArraySize = pSaOffsetToolPath->SVC_Size();
//	TRACE("GenerateMapFeatureToolPath(), post-RemoveAll() offset loop SArray size: %d\n", nArraySize);

// JMW ffdd 08/27/2014 - supress compiler warning
	nArraySize = nArraySize;

	CONNECTED_ELEMENT tNextSortedSegment;
	CONNECTED_ELEMENT tSegmentsLoopA;
	CONNECTED_ELEMENT tSegmentsLoopB;

	int nMaxSegmentsIndex = pSaCenterLineSegments->SVC_Size() - 1;
	if (0 == nMaxSegmentsIndex)
	{
		tNextSortedSegment = pSaCenterLineSegments->SVC_GetElement(0);
		if (tNextSortedSegment.eSegmentType == eCircle)
		{
			if (1 == bySide)
			{
				// Construct a circle to the outside
				tSegmentsLoopA.eSegmentType = eCircle;
				tSegmentsLoopA.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoopA.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoopA.dRx = tNextSortedSegment.dRx;
				tSegmentsLoopA.dRy = tNextSortedSegment.dRy;
				tSegmentsLoopA.nSegmentIndex = 0;
				tSegmentsLoopA.nA_ConnectedSegmentIndex = 0;
				tSegmentsLoopA.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoopA.dB_ChainedMoveCompliance = -1.0;
				tSegmentsLoopA.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
				tSegmentsLoopA.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;

				// This is a guess ?
				tSegmentsLoopA.dRadius = tNextSortedSegment.dRadius + dLoopOffset;

				// Calculate new projected endpoints
				tSegmentsLoopA.dAx = tNextSortedSegment.dAx;
				tSegmentsLoopA.dAy = tNextSortedSegment.dAy + dLoopOffset;
				tSegmentsLoopA.dBx = tNextSortedSegment.dBx;
				tSegmentsLoopA.dBy = tNextSortedSegment.dBy + dLoopOffset;
//				pSaOffsetToolPath->SVC_SetElement(0, tSegmentsLoopA);
				pSaOffsetToolPath->SVC_AddElement(tSegmentsLoopA);
			}
			else
			{
				// Construct a circle to the inside
				tSegmentsLoopB.eSegmentType = eCircle;
				tSegmentsLoopB.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoopB.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoopB.dRx = tNextSortedSegment.dRx;
				tSegmentsLoopB.dRy = tNextSortedSegment.dRy;
				tSegmentsLoopB.nSegmentIndex = 0;
				tSegmentsLoopB.nA_ConnectedSegmentIndex = 0;
				tSegmentsLoopB.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoopB.dB_ChainedMoveCompliance = -1.0;
				tSegmentsLoopB.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
				tSegmentsLoopB.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;

				// This is a guess ?
				tSegmentsLoopB.dRadius = tNextSortedSegment.dRadius - dLoopOffset;

				// Calculate new projected endpoints
				tSegmentsLoopB.dAx = tNextSortedSegment.dAx;
				tSegmentsLoopB.dAy = tNextSortedSegment.dAy - dLoopOffset;
				tSegmentsLoopB.dBx = tNextSortedSegment.dBx;
				tSegmentsLoopB.dBy = tNextSortedSegment.dBy - dLoopOffset;
//				pSaOffsetToolPath->SVC_SetElement(0, tSegmentsLoopB);
				pSaOffsetToolPath->SVC_AddElement(tSegmentsLoopB);
			}
			DumpSegmentSmartArray ("Offset loop (Connected)", pSaOffsetToolPath);
 			return true;
		}
		else
			return false;
	}

	CDoubleVector dvSegment_G, dvZ_G, dvLoopOffset_G;
	CDoubleVector dvA_G, dvAxZ_G, dvAxAxZ_G;
	CDoubleVector dvOrig_G, dvNew_G;

	// The unit Z vector
	dvZ_G.m_dX = 0.0;
	dvZ_G.m_dY = 0.0;
	dvZ_G.m_dZ = 1.0;

	// Process multi-segment loop
	for (int j=0; j<=nMaxSegmentsIndex; j++)
	{
		tNextSortedSegment = pSaCenterLineSegments->SVC_GetElement(j);
		
//		sprintf (chReport_TP, "DEBUG-1 J = %d, MaxJ = %d SegType: %d\n", j, nMaxSegmentsIndex, tNextSortedSegment.eSegmentType);
//		ReportBasedOnVerbosity(-1, chReport_TP);

		if (tNextSortedSegment.eSegmentType == eLine)
		{
			// Construct a vector on top of the line segment and cross it with Z to get the perpendicular
			if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
			{
				// Starting with A-end
				dvSegment_G.m_dX = (float)(tNextSortedSegment.dBx - tNextSortedSegment.dAx);
				dvSegment_G.m_dY = (float)(tNextSortedSegment.dBy - tNextSortedSegment.dAy);
			}
			else
			{
				// Starting with B-end
				dvSegment_G.m_dX = (float)(tNextSortedSegment.dAx - tNextSortedSegment.dBx);
				dvSegment_G.m_dY = (float)(tNextSortedSegment.dAy - tNextSortedSegment.dBy);
			}

			dvSegment_G.m_dZ = 0.0;

			CDoubleVector dvCrossProd_G;
			dvCrossProd_G = CDoubleVector::CrossProduct2 (dvSegment_G, dvZ_G);

			CDoubleVector dvNormalizedXProd_G;
			dvNormalizedXProd_G = dvCrossProd_G.Normalize();
			dvLoopOffset_G = dvNormalizedXProd_G * dLoopOffset;

			if (1 == bySide)
			{
				// Construct line segment to the right
				tSegmentsLoopA.eSegmentType = eLine;
				tSegmentsLoopA.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoopA.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoopA.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
// Experiment
				tSegmentsLoopA.nA_ConnectedSegmentIndex = -1;
				tSegmentsLoopA.nB_ConnectedSegmentIndex = -1;
// Experiment
				tSegmentsLoopA.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
				tSegmentsLoopA.dAx = tNextSortedSegment.dAx + dvLoopOffset_G.m_dX;
				tSegmentsLoopA.dAy = tNextSortedSegment.dAy + dvLoopOffset_G.m_dY;
				tSegmentsLoopA.dBx = tNextSortedSegment.dBx + dvLoopOffset_G.m_dX;
				tSegmentsLoopA.dBy = tNextSortedSegment.dBy + dvLoopOffset_G.m_dY;
//				if (2 <= TRACE_VERBOSITY)
//					TRACE ("GenerateMapFeatureToolPath(), LoopA[%d]: Ax: %f, Ay: %f, Bx: %f By: %f\n", j, tSegmentsLoopA.dAx, tSegmentsLoopA.dAy, tSegmentsLoopA.dBx, tSegmentsLoopA.dBy);

				// Calculate slopes & intercepts, watching out for vertical lines
//				if (tSegmentsLoopA.dAx != tSegmentsLoopA.dBx)
				if (EQUIVALENCY_EPSILON < fabs(tSegmentsLoopA.dAx-tSegmentsLoopA.dBx))
				{
					tSegmentsLoopA.dLineSlope = (tSegmentsLoopA.dBy - tSegmentsLoopA.dAy) / (tSegmentsLoopA.dBx - tSegmentsLoopA.dAx);
					tSegmentsLoopA.dLineIntercept = tSegmentsLoopA.dAy - (tSegmentsLoopA.dLineSlope * tSegmentsLoopA.dAx);
					tSegmentsLoopA.bVerticalLine = false;
//					if (2 <= TRACE_VERBOSITY)
//						TRACE ("GenerateMapFeatureToolPath(), LoopA[%d]: m: %f, b: %f\n", j, tSegmentsLoopA.dLineSlope, tSegmentsLoopA.dLineIntercept);
				}
				else
				{
//					if (2 <= TRACE_VERBOSITY)
//						TRACE ("GenerateMapFeatureToolPath(), LoopA[%d]: VerticalLine, m: INF\n", j);
					tSegmentsLoopA.bVerticalLine = true;
				}

				tSegmentsLoopA.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoopA.dB_ChainedMoveCompliance = -1.0;
				// pSaOffsetToolPath->SVC_SetElement(j, tSegmentsLoopA);
				pSaOffsetToolPath->SVC_AddElement(tSegmentsLoopA);
			}
			else
			{
				// Construct line segment to the left
				tSegmentsLoopB.eSegmentType = eLine;
				tSegmentsLoopB.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoopB.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoopB.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
// Experiment
				tSegmentsLoopB.nA_ConnectedSegmentIndex = -1;
				tSegmentsLoopB.nB_ConnectedSegmentIndex = -1;
// Experiment
				tSegmentsLoopB.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
				tSegmentsLoopB.dAx = tNextSortedSegment.dAx - dvLoopOffset_G.m_dX;
				tSegmentsLoopB.dAy = tNextSortedSegment.dAy - dvLoopOffset_G.m_dY;
				tSegmentsLoopB.dBx = tNextSortedSegment.dBx - dvLoopOffset_G.m_dX;
				tSegmentsLoopB.dBy = tNextSortedSegment.dBy - dvLoopOffset_G.m_dY;
//				if (2 <= TRACE_VERBOSITY)
//					TRACE ("GenerateMapFeatureToolPath(), LoopB[%d]: Ax: %f, Ay: %f, Bx: %f By: %f\n", j, tSegmentsLoopB.dAx, tSegmentsLoopB.dAy, tSegmentsLoopB.dBx, tSegmentsLoopB.dBy);

				// Calculate slopes & intercepts, watching out for vertical lines
//				if (tSegmentsLoopB.dAx != tSegmentsLoopB.dBx)
				if (EQUIVALENCY_EPSILON < fabs(tSegmentsLoopB.dAx-tSegmentsLoopB.dBx))
				{
					tSegmentsLoopB.dLineSlope = (tSegmentsLoopB.dBy - tSegmentsLoopB.dAy) / (tSegmentsLoopB.dBx - tSegmentsLoopB.dAx);
					tSegmentsLoopB.dLineIntercept = tSegmentsLoopB.dAy - (tSegmentsLoopB.dLineSlope * tSegmentsLoopB.dAx);
					tSegmentsLoopB.bVerticalLine = false;
//					if (2 <= TRACE_VERBOSITY)
//						TRACE ("GenerateMapFeatureToolPath(), LoopB[%d]: m: %f, b: %f\n", j, tSegmentsLoopB.dLineSlope, tSegmentsLoopB.dLineIntercept);
				}
				else
				{
//					if (2 <= TRACE_VERBOSITY)
//						TRACE ("GenerateMapFeatureToolPath(), LoopB[%d]: VerticalLine, m: INF\n", j);
					tSegmentsLoopB.bVerticalLine = true;
				}

				tSegmentsLoopB.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoopB.dB_ChainedMoveCompliance = -1.0;
				//pSaOffsetToolPath->SVC_SetElement(j, tSegmentsLoopB);
				pSaOffsetToolPath->SVC_AddElement(tSegmentsLoopB);
			}
		}
		else if (tNextSortedSegment.eSegmentType == eArc)
		{
			if (1 == bySide)
			{
				// Construct an arc segment to the right
				tSegmentsLoopA.eSegmentType = eArc;
				tSegmentsLoopA.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoopA.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoopA.dRx = tNextSortedSegment.dRx;
				tSegmentsLoopA.dRy = tNextSortedSegment.dRy;
				tSegmentsLoopA.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
				tSegmentsLoopA.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;

				// Is arc curving to left ?
				if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
				{
					// Extend the radius a bit to guarantee intersecting segments
					// 2/26/2011	tSegmentsLoopA.dRad = tNextSortedSegment.dRad + dLoopOffset;
					tSegmentsLoopA.dRadius = 1.00001 * (tNextSortedSegment.dRadius + dLoopOffset);
				}
				else
				{
					// Extend the radius a bit to guarantee intersecting segments
					// 2/26/2011	tSegmentsLoopA.dRad = tNextSortedSegment.dRad - dLoopOffset;
					tSegmentsLoopA.dRadius = 1.00001 * (tNextSortedSegment.dRadius - dLoopOffset);
				}

				// Calculate new projected endpoints
				dvOrig_G.m_dX = tNextSortedSegment.dAx - tNextSortedSegment.dRx;
				dvOrig_G.m_dY = tNextSortedSegment.dAy - tNextSortedSegment.dRy;
				dvOrig_G.m_dZ = 0.0;
				dvNew_G = dvOrig_G * (tSegmentsLoopA.dRadius / tNextSortedSegment.dRadius);
				tSegmentsLoopA.dAx = tNextSortedSegment.dRx + dvNew_G.m_dX;
				tSegmentsLoopA.dAy = tNextSortedSegment.dRy + dvNew_G.m_dY;

				dvOrig_G.m_dX = tNextSortedSegment.dBx - tNextSortedSegment.dRx;
				dvOrig_G.m_dY = tNextSortedSegment.dBy - tNextSortedSegment.dRy;
				dvNew_G = dvOrig_G * (tSegmentsLoopA.dRadius / tNextSortedSegment.dRadius);
				tSegmentsLoopA.dBx = tNextSortedSegment.dRx + dvNew_G.m_dX;
				tSegmentsLoopA.dBy = tNextSortedSegment.dRy + dvNew_G.m_dY;

				tSegmentsLoopA.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoopA.dB_ChainedMoveCompliance = -1.0;
				// pSaOffsetToolPath->SVC_SetElement(j, tSegmentsLoopA);

				// Verify concentricity of arc
				ARC_DATA tArcData;
				tArcData.dX1 = tSegmentsLoopA.dAx;
				tArcData.dY1 = tSegmentsLoopA.dAy;
				tArcData.dX2 = tSegmentsLoopA.dBx;
				tArcData.dY2 = tSegmentsLoopA.dBy;
				tArcData.dRx = tSegmentsLoopA.dRx;
				tArcData.dRy = tSegmentsLoopA.dRy;
				tArcData.lDxfEntityNumber = tSegmentsLoopA.lDxfEntityNumber;
				if (!VerifyArcCenterPoint(&tArcData, TP_ARC_CONCENTRICITY, TP_ARC_CONCENTRICITY))
				{
					PostReport("GenerateMapFeatureToolPath::VerifyArcCenterPoint-right() failed\n");
					assert(false);
				}
				// Add the path
				pSaOffsetToolPath->SVC_AddElement(tSegmentsLoopA);
			}
 			else
			{
				// Construct an arc segment to the left
				tSegmentsLoopB.eSegmentType = eArc;
				tSegmentsLoopB.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoopB.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoopB.dRx = tNextSortedSegment.dRx;
				tSegmentsLoopB.dRy = tNextSortedSegment.dRy;
				tSegmentsLoopB.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
				tSegmentsLoopB.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;

				// Is arc curving to left ?
				if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
				{
					// Extend the radius a bit to guarantee intersecting segments
					// 2/26/2011	tSegmentsLoopB.dRad = tNextSortedSegment.dRad - dLoopOffset;
					tSegmentsLoopB.dRadius = 1.00001 * (tNextSortedSegment.dRadius - dLoopOffset);
				}
				else
				{
					// Extend the radius a bit to guarantee intersecting segments
					// 2/26/2011	tSegmentsLoopB.dRad = tNextSortedSegment.dRad + dLoopOffset;
					tSegmentsLoopB.dRadius = 1.00001 * (tNextSortedSegment.dRadius + dLoopOffset);
				}

				// Calculate new projected endpoints
				dvOrig_G.m_dX = (tNextSortedSegment.dAx - tNextSortedSegment.dRx);
				dvOrig_G.m_dY = (tNextSortedSegment.dAy - tNextSortedSegment.dRy);
				dvOrig_G.m_dZ = 0.0;
				dvNew_G = dvOrig_G * (tSegmentsLoopB.dRadius / tNextSortedSegment.dRadius);
				tSegmentsLoopB.dAx = tNextSortedSegment.dRx + dvNew_G.m_dX;
				tSegmentsLoopB.dAy = tNextSortedSegment.dRy + dvNew_G.m_dY;

				dvOrig_G.m_dX = (tNextSortedSegment.dBx - tNextSortedSegment.dRx);
				dvOrig_G.m_dY = (tNextSortedSegment.dBy - tNextSortedSegment.dRy);
				dvNew_G = dvOrig_G * (tSegmentsLoopB.dRadius / tNextSortedSegment.dRadius);
				tSegmentsLoopB.dBx = tNextSortedSegment.dRx + dvNew_G.m_dX;
				tSegmentsLoopB.dBy = tNextSortedSegment.dRy + dvNew_G.m_dY;

				tSegmentsLoopB.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoopB.dB_ChainedMoveCompliance = -1.0;
				// pSaOffsetToolPath->SVC_SetElement(j, tSegmentsLoopB);

				// Verify concentricity of arc
				ARC_DATA tArcData;
				tArcData.dX1 = tSegmentsLoopB.dAx;
				tArcData.dY1 = tSegmentsLoopB.dAy;
				tArcData.dX2 = tSegmentsLoopB.dBx;
				tArcData.dY2 = tSegmentsLoopB.dBy;
				tArcData.dRx = tSegmentsLoopB.dRx;
				tArcData.dRy = tSegmentsLoopB.dRy;
				tArcData.lDxfEntityNumber = tSegmentsLoopB.lDxfEntityNumber;

//				sprintf (chReport_TP, "DEBUG-10 DXF# %ld, X1: %lf, Y1: %lf", tArcData.lDxfEntityNumber, tArcData.dX1, tArcData.dY1);
//				ReportBasedOnVerbosity(-1, chReport_TP);				
//				sprintf (chReport_TP, "DEBUG-11 DXF# %ld, X2: %lf, Y2: %lf", tArcData.lDxfEntityNumber, tArcData.dX2, tArcData.dY2);
//				ReportBasedOnVerbosity(-1, chReport_TP);
//				sprintf (chReport_TP, "DEBUG-12 DXF# %ld, Rx: %lf, Ry: %lf\n", tArcData.lDxfEntityNumber, tArcData.dRx, tArcData.dRy);
//				ReportBasedOnVerbosity(-1, chReport_TP);

				if (!VerifyArcCenterPoint(&tArcData, TP_ARC_CONCENTRICITY, TP_ARC_CONCENTRICITY))
				{
					PostReport("GenerateMapFeatureToolPath::VerifyArcCenterPoint-left() failed\n");
					assert(false);
				}

				// Add the path
				pSaOffsetToolPath->SVC_AddElement(tSegmentsLoopB);
			}
		}
	}		// end of   	for (int j=0; j<=nMaxSegmentsIndex; j++)


	// (No connected end info at this point)
	DumpSegmentSmartArray ("Offset loop (Unconnected)", pSaOffsetToolPath);

 	// Calculate all intersections for offset loop
	if (!CalculateOffsetSegmentIntersections (pSaOffsetToolPath))
	{
		PostReport("GenerateMapFeatureToolPath::CalculateOffsetSegmentIntersections(OffsetLoop) failed\n");
		assert(false);
	}

	// Calculate all node compliances for offset loop
	if (!CalculateOffsetSegmentIntersectionCompliances (pSaOffsetToolPath))
	{
		PostReport("GenerateMapFeatureToolPath::CalculateOffsetSegmentIntersectionCompliances(OffsetLoop) failed\n");
		assert(false);
	}

	// (Should be valid connected end info at this point)
	DumpSegmentSmartArray ("Offset loop (Connected)", pSaOffsetToolPath);

	return true;
}

bool CToolPaths::CalculateOffsetSegmentIntersectionCompliances (STL_CONNECTED_SEGS_CLASS *pConnectedClass)
{
	CONNECTED_ELEMENT	tSegment1;
	CONNECTED_ELEMENT	tSegment2;

	int nSegmentOnesEnd;
	int nSegmentTwosEnd;

	// Get # segments to process
 	int nMaxSegmentIndex = pConnectedClass->SVC_Size() - 1;

	// Do tool path loop
	for (int j=0; j<=nMaxSegmentIndex; j++)
	{
		tSegment1 = pConnectedClass->SVC_GetElement(j);
		// Wrap back to the beginning of the map for the final element
		if (j < nMaxSegmentIndex)
			tSegment2 = pConnectedClass->SVC_GetElement(j+1);
		else
			tSegment2 = pConnectedClass->SVC_GetElement(0);

		// Determine the segment's connecting endpoints
		if (tSegment2.nA_ConnectedSegmentIndex == j)
		{
			nSegmentTwosEnd = 1;
			nSegmentOnesEnd = tSegment2.ucA_ConnectedSegmentEnd;
		}
		else if (tSegment2.nB_ConnectedSegmentIndex == j)
		{
			nSegmentTwosEnd = 2;
			nSegmentOnesEnd = tSegment2.ucB_ConnectedSegmentEnd;
		}
		else
		{
			// For the case of unconnected segments (features), simply set compliances = -1.0
			if (j == nMaxSegmentIndex)
			{
				// Set the first segment's compliance
				if (tSegment1.nA_ConnectedSegmentIndex == -1)
					tSegment1.dA_ChainedMoveCompliance = -1.0;
				else if (tSegment1.nB_ConnectedSegmentIndex == -1)
					tSegment1.dB_ChainedMoveCompliance = -1.0;
				else
					assert(false);

				// Set the second segment's compliance
				if (tSegment2.nA_ConnectedSegmentIndex == -1)
					tSegment2.dA_ChainedMoveCompliance = -1.0;
				else if (tSegment2.nB_ConnectedSegmentIndex == -1)
					tSegment2.dB_ChainedMoveCompliance = -1.0;
				else
					assert(false);

//				if (5 <= TRACE_VERBOSITY)
//				{
//					TRACE("CalculateOffsetSegmentIntersectionCompliances(0): DXF#: %ld, Non-closed segment, setting compliance to -1.0\n", tSegment1.lDxfEntityNumber);
//					TRACE("CalculateOffsetSegmentIntersectionCompliances(0): DXF#: %ld, Non-closed segment, setting compliance to -1.0\n", tSegment2.lDxfEntityNumber);
// 				}
			}
			else
				assert(false);

			// Update segments #1
			pConnectedClass->SVC_SetElement(j, tSegment1);
			pConnectedClass->SVC_SetElement(0, tSegment2);
			continue;
		}

		// Calculate intersection compliance, both segments are lines
		if (tSegment1.eSegmentType==eLine && tSegment2.eSegmentType==eLine)
		{
			// Unit Vector for Line1
			CDoubleVector dvA ((tSegment1.dBx-tSegment1.dAx), (tSegment1.dBy-tSegment1.dAy), 0.0);
			// Correct direction of tangent vector
			if (tSegment1.ucCcwRotationStartEndpoint==2)
				dvA = dvA * -1.0;
			CDoubleVector dvUnitA = dvA.Normalize();

			// Unit Vector for Line2
			CDoubleVector dvB ((tSegment2.dBx-tSegment2.dAx), (tSegment2.dBy-tSegment2.dAy), 0.0);
			// Correct direction of tangent vector
			if (tSegment2.ucCcwRotationStartEndpoint==2)
				dvB = dvB * -1.0;
			// Unit vector #2
			CDoubleVector dvUnitB = dvB.Normalize();

			// vUA dot vUB = |vUA||vUB| cos(o) = cos(o) (perfect compliance == 1)
 			double dUA_dot_UB = CDoubleVector::DotProduct2 (dvUnitA, dvUnitB);

			// Set the compliances
			if (nSegmentOnesEnd==1)
				tSegment1.dA_ChainedMoveCompliance = dUA_dot_UB;
			else
				tSegment1.dB_ChainedMoveCompliance = dUA_dot_UB;

			if (nSegmentTwosEnd==1)
				tSegment2.dA_ChainedMoveCompliance = dUA_dot_UB;
			else
				tSegment2.dB_ChainedMoveCompliance = dUA_dot_UB;

// 			if (5 <= TRACE_VERBOSITY)
//			{
//				TRACE("CalcOffsetSegInterComp(1): DXF#: %ld, Line Seg #1 (%5.3f, %5.3f), (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, tSegment1.dAx, tSegment1.dAy,  tSegment1.dBx, tSegment1.dBy);
//				TRACE("CalcOffsetSegInterComp(1): DXF#: %ld, Line Seg #1 UV: (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, dvUnitA.m_dX, dvUnitA.m_dY);
//				TRACE("CalcOffsetSegInterComp(): LineSeg #1 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment1.dBx, tSegment1.dBy, tSegment1.dB_ChainedMoveCompliance);
//				TRACE("CalcOffsetSegInterComp(1): DXF#: %ld, Line Seg #2 (%5.3f, %5.3f), (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, tSegment2.dAx, tSegment2.dAy,  tSegment2.dBx, tSegment2.dBy);
//				TRACE("CalcOffsetSegInterComp(1): DXF#: %ld, Line Seg #2 UV: (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, dvUnitB.m_dX, dvUnitB.m_dY);
//				TRACE("CalcOffsetSegInterComp(1): LineSeg #2 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment2.dAx, tSegment2.dAy, tSegment2.dB_ChainedMoveCompliance);
//			}
 		}
		else if (tSegment1.eSegmentType==eLine && tSegment2.eSegmentType==eArc)
		{
			// Segment #1 is a line, segment #2 is an arc

			// Line - Unit vector based on direction of CCW rotating tangent vector
			CDoubleVector dvU1;
// Fix 7/6/2010
//			if (tSegment1.ucCcwRotationStartEndpoint==2)
			if (tSegment1.ucCcwRotationStartEndpoint==1)
			{
				CDoubleVector dvA ((tSegment1.dBx-tSegment1.dAx), (tSegment1.dBy-tSegment1.dAy), 0.0);
				dvU1 = dvA.Normalize();
			}
			else
			{
				CDoubleVector dvA ((tSegment1.dAx-tSegment1.dBx), (tSegment1.dAy-tSegment1.dBy), 0.0);
				dvU1 = dvA.Normalize();
			}

//			CDoubleVector dvA ((tSegment1.dBx-tSegment1.dAx), (tSegment1.dBy-tSegment1.dAy), 0.0);
//			// Correct direction of tangent vector
//			if (tSegment1.ucCcwRotationStartEndpoint==2)
//				dvA = dvA * -1.0;
//			CDoubleVector dvU1 = dvA.Normalize();

			// Arc
			double dArcDeltaX, dArcDeltaY, dArcAngle;
			if (nSegmentTwosEnd==1)
			{
				dArcDeltaX = tSegment2.dAx - tSegment2.dRx;
				dArcDeltaY = tSegment2.dAy - tSegment2.dRy;
			}
			else if (nSegmentTwosEnd==2)
			{
				dArcDeltaX = tSegment2.dBx - tSegment2.dRx;
				dArcDeltaY = tSegment2.dBy - tSegment2.dRy;
 			}
			else
				assert(false);

			dArcAngle = atan2 (dArcDeltaY, dArcDeltaX);
			if (dArcAngle < 0.0)
				dArcAngle += 2.0*M_PI;

			CDoubleVector dvU2 (-sin(dArcAngle), cos(dArcAngle), 0.0);
			// Correct for direction of segment #2's tangent vector
			if (tSegment2.ucCcwRotationStartEndpoint==2)
				dvU2 = dvU2 * -1.0;

			// vU1 dot vU2 = |vU1||vU2| cos(o) = cos(o) (perfect compliance == 1)
 			double dU1_dot_U2 = CDoubleVector::DotProduct2 (dvU1, dvU2);

			// Set the compliances
			if (nSegmentOnesEnd==1)
				tSegment1.dA_ChainedMoveCompliance = dU1_dot_U2;
			else
				tSegment1.dB_ChainedMoveCompliance = dU1_dot_U2;

			if (nSegmentTwosEnd==1)
				tSegment2.dA_ChainedMoveCompliance = dU1_dot_U2;
			else
				tSegment2.dB_ChainedMoveCompliance = dU1_dot_U2;

//			if (5 <= TRACE_VERBOSITY)
//			{
//				TRACE("CalcOffsetSegInterComp(2): DXF#: %ld, Line Seg #1 (%5.3f, %5.3f), (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, tSegment1.dAx, tSegment1.dAy,  tSegment1.dBx, tSegment1.dBy);
//				TRACE("CalcOffsetSegInterComp(2): DXF#: %ld, Line Seg #1 UV_b: (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, dvU1.m_dX, dvU1.m_dY);
//				TRACE("CalcOffsetSegInterComp(2): Line Seg #1 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment1.dBx, tSegment1.dBy, tSegment1.dB_ChainedMoveCompliance);
//				TRACE("CalcOffsetSegInterComp(2): DXF#: %ld, Arc Seg #2 (%5.3f, %5.3f), (%5.3f, %5.3f) @ (%5.3f, %5.3f), SA: %3.1f, EA: %3.1f\n", tSegment2.lDxfEntityNumber, tSegment2.dAx, tSegment2.dAy, tSegment2.dBx, tSegment2.dBy, tSegment2.dRx, tSegment2.dRy, tSegment2.dStartAngle, tSegment2.dEndAngle);
//				TRACE("CalcOffsetSegInterComp(2): DXF#: %ld, Arc Seg #1 UV_a: (%5.3f, %5.3f)\n", tSegment2.lDxfEntityNumber, dvU2.m_dX, dvU2.m_dY);
//				TRACE("CalcOffsetSegInterComp(2): Arc Seg #2 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment2.dAx, tSegment2.dAy, tSegment2.dA_ChainedMoveCompliance);
//			}
		}
		else if (tSegment1.eSegmentType==eArc && tSegment2.eSegmentType==eLine)
		{
			// Segment #1 is an arc, segment #2 is a line

			// Arc
			double dArcDeltaX, dArcDeltaY, dArcAngle;
			if (nSegmentOnesEnd==1)
			{
				dArcDeltaX = tSegment1.dAx - tSegment1.dRx;
				dArcDeltaY = tSegment1.dAy - tSegment1.dRy;
			}
			else if (nSegmentOnesEnd==2)
			{
				dArcDeltaX = tSegment1.dBx - tSegment1.dRx;
				dArcDeltaY = tSegment1.dBy - tSegment1.dRy;
 			}
			else
				assert(false);

			dArcAngle = atan2 (dArcDeltaY, dArcDeltaX);
			if (dArcAngle < 0.0)
				dArcAngle += 2.0*M_PI;

			CDoubleVector dvU1 (-sin(dArcAngle), cos(dArcAngle), 0.0);
			// Correct for direction of segment #2's tangent vector
			if (tSegment1.ucCcwRotationStartEndpoint==2)
				dvU1 = dvU1 * -1.0;

			// Line - Unit vector based on direction of CCW rotating tangent vector
			CDoubleVector dvU2;
// Fix 7/6/2010
//			if (tSegment2.ucCcwRotationStartEndpoint==2)
			if (tSegment2.ucCcwRotationStartEndpoint==1)
			{
				CDoubleVector dvB ((tSegment2.dBx-tSegment2.dAx), (tSegment2.dBy-tSegment2.dAy), 0.0);
				dvU2 = dvB.Normalize();
			}
			else
			{
				CDoubleVector dvB ((tSegment2.dAx-tSegment2.dBx), (tSegment2.dAy-tSegment2.dBy), 0.0);
				dvU2 = dvB.Normalize();
			}

			// Line
//			CDoubleVector dvB ((tSegment2.dBx-tSegment2.dAx), (tSegment2.dBy-tSegment2.dAy), 0.0);
//			// Correct direction of tangent vector
//			if (tSegment2.ucCcwRotationStartEndpoint==2)
//				dvB = dvB * -1.0;
//			CDoubleVector dvU2 = dvB.Normalize();

			// vU1 dot vU2 = |vU1||vU2| cos(o) = cos(o) (perfect compliance == 1)
 			double dU1_dot_U2 = CDoubleVector::DotProduct2 (dvU1, dvU2);

			// Set the compliances
			if (nSegmentOnesEnd==1)
				tSegment1.dA_ChainedMoveCompliance = dU1_dot_U2;
			else
				tSegment1.dB_ChainedMoveCompliance = dU1_dot_U2;

			if (nSegmentTwosEnd==1)
				tSegment2.dA_ChainedMoveCompliance = dU1_dot_U2;
			else
				tSegment2.dB_ChainedMoveCompliance = dU1_dot_U2;

// 			if (5 <= TRACE_VERBOSITY)
//			{
//				TRACE("CalcOffsetSegInterComp(3): DXF#: %ld,  Arc Seg #1 (%5.3f, %5.3f), (%5.3f, %5.3f) @ (%5.3f, %5.3f), SA: %3.1f, EA: %3.1f\n", tSegment1.lDxfEntityNumber, tSegment1.dAx, tSegment1.dAy, tSegment1.dBx, tSegment1.dBy, tSegment1.dRx, tSegment1.dRy, tSegment1.dStartAngle, tSegment1.dEndAngle);
//				TRACE("CalcOffsetSegInterComp(3): DXF#: %ld, Arc Seg #2 UV_b: (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, dvU1.m_dX, dvU1.m_dY);
//				TRACE("CalcOffsetSegInterComp(3): Arc Seg #1 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment1.dBx, tSegment1.dBy, tSegment1.dB_ChainedMoveCompliance);
//				TRACE("CalcOffsetSegInterComp(3): DXF#: %ld, Line Seg #2 (%5.3f, %5.3f), (%5.3f, %5.3f)\n", tSegment2.lDxfEntityNumber, tSegment2.dAx, tSegment2.dAy,  tSegment2.dBx, tSegment2.dBy);
//				TRACE("CalcOffsetSegInterComp(3): DXF#: %ld, Line Seg #1 UV_a: (%5.3f, %5.3f)\n", tSegment2.lDxfEntityNumber, dvU2.m_dX, dvU2.m_dY);
//				TRACE("CalcOffsetSegInterComp(3): Line Seg #2 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment2.dAx, tSegment2.dAy, tSegment2.dA_ChainedMoveCompliance);
//			}
		}
		else if (tSegment1.eSegmentType==eArc && tSegment2.eSegmentType==eArc)
		{
			// Four cases
			double dArc1DeltaX, dArc1DeltaY, dArc1Angle;
			double dArc2DeltaX, dArc2DeltaY, dArc2Angle;

			// Arc #1
			if (nSegmentOnesEnd==1)
			{
				dArc1DeltaX = tSegment1.dAx - tSegment1.dRx;
				dArc1DeltaY = tSegment1.dAy - tSegment1.dRy;
			}
			else if (nSegmentOnesEnd==2)
			{
				dArc1DeltaX = tSegment1.dBx - tSegment1.dRx;
				dArc1DeltaY = tSegment1.dBy - tSegment1.dRy;
			}
			else
				assert(false);

			// Arc #2
			if (nSegmentTwosEnd==1)
			{
				dArc2DeltaX = tSegment2.dAx - tSegment2.dRx;
				dArc2DeltaY = tSegment2.dAy - tSegment2.dRy;
			}
			else if (nSegmentTwosEnd==2)
			{
				dArc2DeltaX = tSegment2.dBx - tSegment2.dRx;
				dArc2DeltaY = tSegment2.dBy - tSegment2.dRy;
 			}
			else
				assert(false);

			dArc1Angle = atan2 (dArc1DeltaY, dArc1DeltaX);
			if (dArc1Angle < 0.0)
				dArc1Angle += 2.0*M_PI;

			dArc2Angle = atan2 (dArc2DeltaY, dArc2DeltaX);
			if (dArc2Angle < 0.0)
				dArc2Angle += 2.0*M_PI;

			CDoubleVector dvU1 (-sin(dArc1Angle), cos(dArc1Angle), 0.0);
			CDoubleVector dvU2 (-sin(dArc2Angle), cos(dArc2Angle), 0.0);

			// vU1 dot vU2 = |vU1||vU2| cos(o) = cos(o) (perfect compliance == 1)
 //			double dU1_dot_U2 = CDoubleVector::DotProduct2 (dvU1, dvU2);

			// **** Don't yet understand this ****
//			if (nSegmentOnesEnd == tSegment1.ucCcwRotationStartEndpoint)
//				dU1_dot_U2 *= -1.0;
//			if (nSegmentTwosEnd == tSegment2.ucCcwRotationStartEndpoint)
//				dU1_dot_U2 *= -1.0;

			// **** Don't yet understand this ****
//			if (nSegmentOnesEnd == nSegmentTwosEnd)
//				dU1_dot_U2 *= -1.0;

			// Correct for direction of segment #1's tangent vector
//			if (nSegmentOnesEnd==1 && tSegment1.ucCcwRotationStartEndpoint==2)
//			{
//				dvU1 = dvU1 * -1.0;
//				dU1_dot_U2 *= -1.0;
//			}
//			else if (nSegmentOnesEnd==2 && tSegment1.ucCcwRotationStartEndpoint==2)
//			{
//				dvU1 = dvU1 * -1.0;
//				dU1_dot_U2 *= -1.0;
//			}

			// Correct for direction of segment #1's tangent vector
			if (tSegment1.ucCcwRotationStartEndpoint==2)
				dvU1 = dvU1 * -1.0;

			// Correct for direction of segment #2's tangent vector
			if (tSegment2.ucCcwRotationStartEndpoint==2)
				dvU2 = dvU2 * -1.0;

			// vU1 dot vU2 = |vU1||vU2| cos(o) = cos(o) (perfect compliance == 1)
 			double dU1_dot_U2 = CDoubleVector::DotProduct2 (dvU1, dvU2);

			// Set the compliances
			if (nSegmentOnesEnd==1)
				tSegment1.dA_ChainedMoveCompliance = dU1_dot_U2;
			else
				tSegment1.dB_ChainedMoveCompliance = dU1_dot_U2;

			if (nSegmentTwosEnd==1)
				tSegment2.dA_ChainedMoveCompliance = dU1_dot_U2;
			else
				tSegment2.dB_ChainedMoveCompliance = dU1_dot_U2;

//			if (5 <= TRACE_VERBOSITY)
//			{
//				TRACE("CalculateOffsetSegmentIntersectionCompliances(4): DXF#: %ld, Arc Seg #1 (%5.3f, %5.3f), (%5.3f, %5.3f) @ (%5.3f, %5.3f), SA: %3.1f, EA: %3.1f\n", tSegment1.lDxfEntityNumber, tSegment1.dAx, tSegment1.dAy,  tSegment1.dBx, tSegment1.dBy, tSegment1.dRx, tSegment1.dRy, tSegment1.dStartAngle, tSegment1.dEndAngle);
//				TRACE("CalculateOffsetSegmentIntersectionCompliances(4): DXF#: %ld, Arc Seg #1 UV: (%5.3f, %5.3f)\n", tSegment1.lDxfEntityNumber, dvU1.m_dX, dvU1.m_dY);
//				TRACE("CalculateOffsetSegmentIntersectionCompliances(4): Arc Seg #1 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment1.dBx, tSegment1.dBy, dU1_dot_U2);
//				TRACE("CalculateOffsetSegmentIntersectionCompliances(4): DXF#: %ld, Arc Seg #2 (%5.3f, %5.3f), (%5.3f, %5.3f) @ (%5.3f, %5.3f), SA: %3.1f, EA: %3.1f\n", tSegment2.lDxfEntityNumber, tSegment2.dAx, tSegment2.dAy,  tSegment2.dBx, tSegment2.dBy, tSegment2.dRx, tSegment2.dRy, tSegment2.dStartAngle, tSegment2.dEndAngle);
//				TRACE("CalculateOffsetSegmentIntersectionCompliances(4): DXF#: %ld, Arc Seg #2 UV: (%5.3f, %5.3f)\n", tSegment2.lDxfEntityNumber, dvU2.m_dX, dvU2.m_dY);
//				TRACE("CalculateOffsetSegmentIntersectionCompliances(4): Arc Seg #2 Intersecting Node: (%lf, %lf), Compliance: %lf\n", tSegment2.dAx, tSegment2.dAy, dU1_dot_U2);
//			}
 		}
		else
			assert(false);

		// Update segment #1
		pConnectedClass->SVC_SetElement(j, tSegment1);
		// Wrap back to first element at the end of list
		if (j < nMaxSegmentIndex)
			pConnectedClass->SVC_SetElement(j+1, tSegment2);
		else
			pConnectedClass->SVC_SetElement(0, tSegment2);
	}

	return true;
}

bool CToolPaths::GenerateMapOnFeatureToolPath (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex)
{
	if (eCutOnFeature != eFeatureCutType)
 		assert(false);

	// Make certain the indexed feature is available
	if (pFeaturesMap->SMC_GetSize() <= (unsigned int)nGroupIndex)
	{
		PostReport("GenerateMapFeatureToolPath()..Not that many features in map\n");
		return false;
	}
	// Get the requested feature
	FEATURE_ELEMENT tFeature;
	if (!(pFeaturesMap->SMC_GetFeature(nGroupIndex, &tFeature)))
	{
		PostReport("GenerateMapOnFeatureToolPath(), Failure to get FEATURE\n");
		return false;
	}

	// Get the feature's centerline, and tool path pointers
	STL_CONNECTED_SEGS_CLASS  *pSaCenterLineSegments = tFeature.m_pSaConnectedDxfSegments;
	STL_CONNECTED_SEGS_CLASS  *pSaToolPath = tFeature.m_pSaToolPathFinishCut;
	assert (pSaToolPath);

	// Remove any possible offset loop elements
	int nArraySize = pSaToolPath->SVC_Size();
//	TRACE("GenerateMapFeatureToolPath(), initial loop SArray size: %d\n", nArraySize);
	pSaToolPath->SVC_RemoveAllElements();
	nArraySize = pSaToolPath->SVC_Size();;
//	TRACE("GenerateMapFeatureToolPath(), post-RemoveAll() loop SArray size: %d\n", nArraySize);

// JMW ffdd 08/27/2014 - supress compiler warning
	nArraySize = nArraySize;

	CONNECTED_ELEMENT tNextSortedSegment;
	CONNECTED_ELEMENT tSegmentsLoop;

	int nMaxSegmentsIndex = pSaCenterLineSegments->SVC_Size() - 1;
	// Is this a single segment feature ?
	if (0 == nMaxSegmentsIndex)
	{
		tNextSortedSegment = pSaCenterLineSegments->SVC_GetElement(0);
		if (tNextSortedSegment.eSegmentType == eCircle)
		{
			// Construct a circle to the outside
			tSegmentsLoop.eSegmentType = eCircle;
			tSegmentsLoop.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoop.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoop.dRx = tNextSortedSegment.dRx;
			tSegmentsLoop.dRy = tNextSortedSegment.dRy;
			tSegmentsLoop.nSegmentIndex = 0;
			tSegmentsLoop.nA_ConnectedSegmentIndex = 0;
			tSegmentsLoop.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
			tSegmentsLoop.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
			tSegmentsLoop.dRadius = tNextSortedSegment.dRadius;

			// Calculate new projected endpoints
			tSegmentsLoop.dAx = tNextSortedSegment.dAx;
			tSegmentsLoop.dAy = tNextSortedSegment.dAy;
			tSegmentsLoop.dBx = tNextSortedSegment.dBx;
			tSegmentsLoop.dBy = tNextSortedSegment.dBy;

			// Compliance not relevant for circles
			tSegmentsLoop.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoop.dB_ChainedMoveCompliance = -1.0;

			// Add the element
			pSaToolPath->SVC_AddElement(tSegmentsLoop);
//			pSaToolPath->SVC_SetElement(0, tSegmentsLoop);

		 	DumpSegmentSmartArray ("Offset loop (Connected)", pSaToolPath);
 			return true;
		}
		else if (tNextSortedSegment.eSegmentType == eArc)
		{
			// Construct an arc segment on top of the line segment
			tSegmentsLoop.eSegmentType = eArc;
			tSegmentsLoop.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoop.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoop.dRx = tNextSortedSegment.dRx;
			tSegmentsLoop.dRy = tNextSortedSegment.dRy;
			tSegmentsLoop.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
			tSegmentsLoop.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
			tSegmentsLoop.dRadius = tNextSortedSegment.dRadius;
			tSegmentsLoop.dAx = tNextSortedSegment.dAx;
			tSegmentsLoop.dAy = tNextSortedSegment.dAy;
			tSegmentsLoop.dBx = tNextSortedSegment.dBx;
			tSegmentsLoop.dBy = tNextSortedSegment.dBy;
			tSegmentsLoop.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoop.dB_ChainedMoveCompliance = -1.0;
			pSaToolPath->SVC_AddElement(tSegmentsLoop);
//			pSaToolPath->SVC_SetElement(0, tSegmentsLoop);
		}
		else if (tNextSortedSegment.eSegmentType == eLine)
		{
			tSegmentsLoop.eSegmentType = eLine;
			tSegmentsLoop.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
			tSegmentsLoop.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
			tSegmentsLoop.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
			tSegmentsLoop.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
			tSegmentsLoop.dAx = tNextSortedSegment.dAx;
			tSegmentsLoop.dAy = tNextSortedSegment.dAy;
			tSegmentsLoop.dBx = tNextSortedSegment.dBx;
			tSegmentsLoop.dBy = tNextSortedSegment.dBy;
			tSegmentsLoop.dA_ChainedMoveCompliance = -1.0;
			tSegmentsLoop.dB_ChainedMoveCompliance = -1.0;
			pSaToolPath->SVC_AddElement(tSegmentsLoop);
//			pSaToolPath->SVC_SetElement(0, tSegmentsLoop);
		}
		else
			assert(false);
	}
	else
	{
		// Process multi-segment feature
 		for (int j=0; j<=nMaxSegmentsIndex; j++)
		{
			tNextSortedSegment = pSaCenterLineSegments->SVC_GetElement(j);
			if (tNextSortedSegment.eSegmentType == eLine)
			{
				// Construct a vector on top of the line segment
				if (1 == tNextSortedSegment.ucStartEndpointAtInitialSort)
				{
					// Starting with A-end
				}
				else
				{
					// Starting with B-end
				}
 				tSegmentsLoop.eSegmentType = eLine;
				tSegmentsLoop.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoop.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoop.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
				tSegmentsLoop.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
				tSegmentsLoop.nA_ConnectedSegmentIndex = tNextSortedSegment.nA_ConnectedSegmentIndex;
				tSegmentsLoop.ucA_ConnectedSegmentEnd = tNextSortedSegment.ucA_ConnectedSegmentEnd;
				tSegmentsLoop.nB_ConnectedSegmentIndex = tNextSortedSegment.nB_ConnectedSegmentIndex;
				tSegmentsLoop.ucB_ConnectedSegmentEnd = tNextSortedSegment.ucB_ConnectedSegmentEnd;
				tSegmentsLoop.dAx = tNextSortedSegment.dAx;
				tSegmentsLoop.dAy = tNextSortedSegment.dAy;
				tSegmentsLoop.dBx = tNextSortedSegment.dBx;
				tSegmentsLoop.dBy = tNextSortedSegment.dBy;
				tSegmentsLoop.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoop.dB_ChainedMoveCompliance = -1.0;
//				if (2 <= TRACE_VERBOSITY)
//					TRACE ("GenerateMapFeatureToolPath(), LoopA[%d]: Ax: %f, Ay: %f, Bx: %f By: %f\n", j, tSegmentsLoop.dAx, tSegmentsLoop.dAy, tSegmentsLoop.dBx, tSegmentsLoop.dBy);
				pSaToolPath->SVC_AddElement(tSegmentsLoop);
			}
			else if (tNextSortedSegment.eSegmentType == eArc)
			{
				// Construct an arc segmenton top of the line segment
				tSegmentsLoop.eSegmentType = eArc;
				tSegmentsLoop.lDxfEntityNumber = tNextSortedSegment.lDxfEntityNumber;
				tSegmentsLoop.nFeatureIndex = tNextSortedSegment.nFeatureIndex;
				tSegmentsLoop.dRx = tNextSortedSegment.dRx;
				tSegmentsLoop.dRy = tNextSortedSegment.dRy;
				tSegmentsLoop.ucStartEndpointAtInitialSort = tNextSortedSegment.ucStartEndpointAtInitialSort;
				tSegmentsLoop.ucCcwRotationStartEndpoint = tNextSortedSegment.ucCcwRotationStartEndpoint;
				tSegmentsLoop.nA_ConnectedSegmentIndex = tNextSortedSegment.nA_ConnectedSegmentIndex;
				tSegmentsLoop.ucA_ConnectedSegmentEnd = tNextSortedSegment.ucA_ConnectedSegmentEnd;
				tSegmentsLoop.nB_ConnectedSegmentIndex = tNextSortedSegment.nB_ConnectedSegmentIndex;
				tSegmentsLoop.ucB_ConnectedSegmentEnd = tNextSortedSegment.ucB_ConnectedSegmentEnd;
				tSegmentsLoop.dRadius = tNextSortedSegment.dRadius;
		 		tSegmentsLoop.dAx = tNextSortedSegment.dAx;
				tSegmentsLoop.dAy = tNextSortedSegment.dAy;
				tSegmentsLoop.dBx = tNextSortedSegment.dBx;
				tSegmentsLoop.dBy = tNextSortedSegment.dBy;
				tSegmentsLoop.dA_ChainedMoveCompliance = -1.0;
				tSegmentsLoop.dB_ChainedMoveCompliance = -1.0;
				pSaToolPath->SVC_AddElement(tSegmentsLoop);
//				pSaToolPath->SVC_SetElement(j, tSegmentsLoop);
			}
 		}
	}		// end of   	for (int j=0; j<=nMaxSegmentsIndex; j++)


	// (No connected end info at this point)
//	CString cstrText1("Offset loop (Unconnected)");
//	DumpSegmentSmartArray (cstrText1, pSaToolPath);

 	// Update all connection info
//	if (!CalculateOnPathSegmentConnections (pSaToolPath))
//	{
//		TRACE("GenerateMapFeatureToolPaths()..CalculateOffsetSegmentIntersections(OffsetLoop) failed\n");
//		ASSERT(0);
//	}

	// Calculate all node compliances for offset loop - making sure it's a multi-segment group first
	int nMaxSegsIndex = pSaToolPath->SVC_Size() - 1;
	if (0 < nMaxSegsIndex)
	{
		if (!CalculateOffsetSegmentIntersectionCompliances (pSaToolPath))
		{
			PostReport("GenerateMapOnFeatureToolPath::CalculateOffsetSegmentIntersectionCompliances(OffsetLoop) failed\n");
			assert(false);
		}
	}

	// (Should be valid connected end info at this point)
	DumpSegmentSmartArray ("ON Path loop (Connected)", pSaToolPath);

	return true;
}

int CToolPaths::GetRemainingNonGroupedUnconnectedDxfSegments (FEATURE_CUT_TYPE eType, STL_VECTOR_CLASS *pUnconnectedClass)
{
	int nRemaining = 0;
	UNCONNECTED_ELEMENT tSegment;
	if (eCutInsideFeature == eType)
	{
		int nMaxIndex = pUnconnectedClass->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				nRemaining++;
		}
	}
	else if (eCutOutsideFeature == eType)
	{
		int nMaxIndex = pUnconnectedClass->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				nRemaining++;
		}
	}
	else if (eCutOnFeature == eType)
	{
		int nMaxIndex = pUnconnectedClass->SVC_Size() - 1;
		for (int n=0; n<=nMaxIndex; n++)
		{
			tSegment = pUnconnectedClass->SVC_GetElement(n);
			if (tSegment.lFeatureGroupIndex == -1)
				nRemaining++;
		}
	}
	else
		nRemaining = -1;

	return nRemaining;
}

void CToolPaths::DumpFeatureMap (FEATURE_CUT_TYPE eType, STL_MAP_CLASS *pFeatureMap)
{
	FEATURE_ELEMENT tFeature;
//	char chReport[200];

	if (eCutInsideFeature == eType)
	{
		int nFeatureCount = pFeatureMap->SMC_GetSize();
		sprintf (chReport_TP, "DumpFeatureMap()..INSIDE cut feature map count: %d", nFeatureCount);
		ReportBasedOnVerbosity(2, chReport_TP);

		for (int j=0; j<nFeatureCount; j++)
		{
			pFeatureMap->SMC_GetFeature(j, &tFeature);
			sprintf (chReport_TP, "IN Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity(2, chReport_TP);
			sprintf (chReport_TP, "    Centroid: X=%lf, Y=%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity(2, chReport_TP);
			sprintf (chReport_TP, "    MinX: X=%lf, MinY=%lf, MaxX: X=%lf, MaxY=%lf", tFeature.m_BoundingRect.dLeft, tFeature.m_BoundingRect.dBottom, tFeature.m_BoundingRect.dRight, tFeature.m_BoundingRect.dTop);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
	}
	else if (eCutOutsideFeature == eType)
	{
		int nFeatureCount = pFeatureMap->SMC_GetSize();
		sprintf (chReport_TP, "DumpFeatureMap()..OUTSIDE cut feature map count: %d", nFeatureCount);
		ReportBasedOnVerbosity(2, chReport_TP);

		for (int j=0; j<nFeatureCount; j++)
		{
			pFeatureMap->SMC_GetFeature(j, &tFeature);
		 	sprintf (chReport_TP, "OUT Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity(2, chReport_TP);
			sprintf (chReport_TP, "    Centroid: X=%lf, Y=%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity(2, chReport_TP);
			sprintf (chReport_TP, "    MinX: X=%lf, MinY=%lf, MaxX: X=%lf, MaxY=%lf", tFeature.m_BoundingRect.dLeft, tFeature.m_BoundingRect.dBottom, tFeature.m_BoundingRect.dRight, tFeature.m_BoundingRect.dTop);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
	}
	else if (eCutOnFeature == eType)
	{
		int nFeatureCount = pFeatureMap->SMC_GetSize();
		sprintf (chReport_TP, "DumpFeatureMap()..ON cut feature map count: %d", nFeatureCount);
		ReportBasedOnVerbosity(2, chReport_TP);

		for (int j=0; j<nFeatureCount; j++)
		{
			pFeatureMap->SMC_GetFeature(j, &tFeature);
		 	sprintf (chReport_TP, "ON Feature: %d (%s), Segments: %d", tFeature.m_nFeatureIndex, (tFeature.m_eFeatureCutType == eCutInsideFeature ? "INSIDE_CUT" : "OUTSIDE_CUT"), tFeature.m_nNumberSegments);
			ReportBasedOnVerbosity(2, chReport_TP);
			sprintf (chReport_TP, "    Centroid: X=%lf, Y=%lf", tFeature.m_dCentroidX, tFeature.m_dCentroidY);
			ReportBasedOnVerbosity(2, chReport_TP);
			sprintf (chReport_TP, "    MinX: X=%lf, MinY=%lf, MaxX: X=%lf, MaxY=%lf", tFeature.m_BoundingRect.dLeft, tFeature.m_BoundingRect.dBottom, tFeature.m_BoundingRect.dRight, tFeature.m_BoundingRect.dTop);
			ReportBasedOnVerbosity(2, chReport_TP);
		}
	}
	else
		assert(false);
}

bool CToolPaths::PostReport(const char *chReport)
{
	cout << chReport << std::endl;
	return true;
}

bool CToolPaths::ReportBasedOnVerbosity(int nVerbosity, const char *chReport)
{
	// INI values range between 0 and 5
	// 1 <= nVerbosity <= 5
	if (nVerbosity <= INI_Verbosity_ToolPaths)
	{
		cout << chReport << std::endl;
		return true;
	}
	else
		return false;
}


