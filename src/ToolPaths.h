// #pragma once

class CToolPaths
{
public:
	CToolPaths(void);
	~CToolPaths(void);
//	int ExtractDxfFeatures (FEATURE_CUT_TYPE eFeatureType, CMap  <int, int, FEATURE, FEATURE&> *pFeatureMap, CSmartArrayUnconnectedSegments *pUnconnectedSegments, double dToolDiam);
	int 	ExtractDxfFeatures (FEATURE_CUT_TYPE eFeatureType, STL_MAP_CLASS *pFeaturesMap, STL_VECTOR_CLASS *pUnconnectedClass, double dToolDiam);
	//int		ExtractDxfFeatures (FEATURE_CUT_TYPE eFeatureType, STL_MAP_CLASS *pFeaturesMap, std::vector <UNCONNECTED_ELEMENT> *pUnconnectedArray, double dToolDiam);
	bool 	BBB_FindAndTagAllConnections (STL_VECTOR_CLASS *pUnconnectedClass);
	long	CountRemainingNonGroupedUnconnectedDxfSegments (FEATURE_CUT_TYPE eCutType, STL_VECTOR_CLASS *pUnconnectedClass);
	bool 	FindAttachedSegment (FEATURE_CUT_TYPE eFeatureCutType, double dTargetX, double dTargetY, int nStartingIndex, int nAvoidSecondaryIndex, int *pConnectedsIndex, int *pConnectedsEnd, STL_VECTOR_CLASS *pUnconnectedClass);
	bool 	BBB_IsFeaturePathClosed (FEATURE_CUT_TYPE eFeatureCutType, int nStartingSegmentIndex, int *pNumConnectedSegments, int *pUnconnectedsIndex, int *pUnconnectedsEnd, STL_VECTOR_CLASS *pUnconnectedClass);
	long	GetNextNonGroupedSegmentIndex (FEATURE_CUT_TYPE eType, long zStartingIndex, STL_VECTOR_CLASS *pUnconnectedClass);
	bool 	AddSegmentToFeatureMap (UNCONNECTED_ELEMENT *tUnconnectedSegment, STL_CONNECTED_SEGS_CLASS *pSortedConnectedClass, int nGroupedArrayIndex, int nGroupIndex);
	int 	ExtractGroupedDxfSegments_III (int nGroupIndex, FEATURE_CUT_TYPE eFeatureCutType, STL_VECTOR_CLASS *pUnconnectedClass, STL_CONNECTED_SEGS_CLASS *pSortedConnectedClass);
	bool 	ConnectAttachedSegments (int nGroupedArrayNextIndex, CONNECTED_ELEMENT *pSegment1, CONNECTED_ELEMENT *pSegment2,  int nPrimarysWorkingEnd, int nSecondarysConnectedEnd);
	bool 	CalculateFeaturesCcwRotationStartPoints (FEATURE_ELEMENT *pFeature, FEATURE_CUT_TYPE eFeatureCutType);
	bool	DecomposeArc_II (ARC_DATA *tArcData, ARC_DATA tDecomposedArcs[], int nMaxElements, int *pArcs);
	bool 	CalculateMapFeatureBoundary (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex);
	bool 	CalculateMapFeatureCentroid (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex);
	bool 	CalculateSmartArraySegmentBoundaries (STL_CONNECTED_SEGS_CLASS *pConnectedClass);
	bool	GenerateMapFeatureToolPaths (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex, double dLoopOffset);
	void	DumpSegmentSmartArray (const char *cText, STL_CONNECTED_SEGS_CLASS *pConnectedClass);
	bool	CalculateOffsetSegmentIntersections (STL_CONNECTED_SEGS_CLASS *pConnectedClass);
	bool	CalculateMapFeatureToolPathLengths (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex, float *pLengthA, float *pLengthB);
	bool	ConnectSegments (CONNECTED_ELEMENT *pSeg1, CONNECTED_ELEMENT *pSeg2, double dInterceptX, double dInterceptY);
	bool	UpdateConnectedSegments (int nIndex, int nMaxSize, CONNECTED_ELEMENT *pSeg1, CONNECTED_ELEMENT *pSeg2, STL_CONNECTED_SEGS_CLASS *pConnectedLoopClass);
	bool	CalculateCircleLineSeparationDistDoubleVector(CIRCLE_OBJECT *pCircle, LINE_OBJECT *pLine, double *pVal);
	bool	CalculateLineCircleIntersection(LINE_OBJECT *pLine, CIRCLE_OBJECT *pCircle, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2);
	bool	CalculateLineCircleIntersection_II(LINE_OBJECT *pLine, CIRCLE_OBJECT *pCircle, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2);
	bool	FindLineCircleTangentialMidpoint (LINE_OBJECT Line, CIRCLE_OBJECT Circle, POINT_OBJECT *pPoint);
	bool	CalculateCircleCircleIntersection(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2);
	bool	CalculateCircleCircleIntersection_II(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2);
	bool	VerifyArcCenterPoint(ARC_DATA *tArcData, double dAbsError, double dRelError);
	bool 	GenerateMapFeatureToolPath (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex, double dLoopOffset);
	bool	CalculateOffsetSegmentIntersectionCompliances (STL_CONNECTED_SEGS_CLASS *pConnectedClass);
	bool	GenerateMapOnFeatureToolPath (FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeaturesMap, int nGroupIndex);
	int		GetRemainingNonGroupedUnconnectedDxfSegments (FEATURE_CUT_TYPE eType, STL_VECTOR_CLASS *pUnconnectedClass);
	void	DumpFeatureMap (FEATURE_CUT_TYPE eType, STL_MAP_CLASS *pFeatureMap);
	bool	PostReport(const char *chReport);
	bool	ReportBasedOnVerbosity (int nVerbosity, const char *chReport);
private:
	double m_dRoughToolDiameter;
	double m_dFinishToolDiameter;
};
