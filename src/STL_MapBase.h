typedef struct
{
	long lDxfEntityNumber;
	double dRx;
	double dRy;
	double dRadius;
	double dStartAngle;
	double dEndAngle;
	double dX1;
	double dY1;
	double dX2;
	double dY2;
} ARC_DATA;

typedef struct
{
	FEATURE_CUT_TYPE						m_eFeatureCutType;
	int										m_nFeatureIndex;
	int										m_nNumberSegments;
	class STL_VECTOR_CLASS 					*m_pSaUnconnectedDxfSegments;
	class STL_CONNECTED_SEGS_CLASS			*m_pSaConnectedDxfSegments;
	class STL_CONNECTED_SEGS_CLASS			*m_pSaToolPathA;
	class STL_CONNECTED_SEGS_CLASS			*m_pSaToolPathB;
	class STL_CONNECTED_SEGS_CLASS			*m_pSaToolPathRoughCut;
	class STL_CONNECTED_SEGS_CLASS			*m_pSaToolPathFinishCut;
	RECT_OBJECT								m_BoundingRect;
	double									m_dToolPathLengthA;
	double									m_dToolPathLengthB;
	double									m_dCentroidX;
	double									m_dCentroidY;
	bool									m_bAvailable;
} FEATURE_ELEMENT;


typedef std::pair<int, FEATURE_ELEMENT> T_IntFeaturePair;
typedef std::pair<int, FEATURE_ELEMENT*> T_IntFeaturePtrPair;

class STL_MAP_CLASS 
{
private:
	std::map <int, FEATURE_ELEMENT*> m_FeaturePtrMap;
	std::map <int, FEATURE_ELEMENT> m_FeatureMap;
public:
	STL_MAP_CLASS();
	int 	SMC_AddFeaturePtr(int nKey, FEATURE_ELEMENT *pFeature);
	bool	SMC_GetFeaturePtr(int nKey, FEATURE_ELEMENT *ppFeature);
	bool 	SMC_UpdateFeature(int nKey, FEATURE_ELEMENT rFeature);
	int 	SMC_AddFeature(int nKey, FEATURE_ELEMENT rFeature);
	bool	SMC_GetFeature(int nKey, FEATURE_ELEMENT *pFeature);
	int 	SMC_RemoveFeature(int nKey);
	int 	SMC_RemoveAll(void);
	size_t 	SMC_GetSize(void);
 	int 	SMC_ReportStatus(void);
};
