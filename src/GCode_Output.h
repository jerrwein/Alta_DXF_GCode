class GCode_Output
{
	private:
		std::string	m_strPathFileName;
		float 		m_fPartWidth;
		float 		m_fPartHeight;
		float 		m_fOffsetX;
		float 		m_fOffsetY;
		int 		m_nGCodeLinesOutput;
	public:
		GCode_Output();
		bool	Set_PathFileName(std::string strBaseFile);
		bool	Get_PathFileName(std::string *pStrBaseFile);
		bool	MarkAllAsAvailableFeatures(STL_MAP_CLASS *pOutsideFeatures);
		int		FindClosestAvailableFeature(STL_MAP_CLASS *pFeatures, double dX, double dY, double *pNextX, double *pNextY);
		int		OutputGCodeFile (STL_MAP_CLASS *pInsideFeatures, STL_MAP_CLASS *pOutsideFeatures, STL_MAP_CLASS *pOnFeatures, bool bInsideCutDirCCW, bool bOutsideCutDirCW, bool bChainMoves, double dMinNodeCompliance, double dCutHeight, double dCutSpeed, double dRapidMoveHeight, double dRapidMoveSpeed, double dPlungeSpeed, double dRetractSpeed);
		bool	GenerateFeatureGCode (FILE *pFile, char *pDesc, RECT_OBJECT *pBlockRect, FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeatures, bool bInsideCutDirCCW, bool bOutsideCutDirCW, bool bChainMoves, double dMinNodeCompliance, int nGroupIndex, double dCutHeight, double dCutSpeed, double dRapidMoveHeight, double dRapidMoveSpeed, double dPlungeSpeed, double dRetractSpeed);
		bool 	GenerateFeatureGCode_II (FILE *pFile, char *pDesc, RECT_OBJECT *pBlockRect, FEATURE_CUT_TYPE eFeatureCutType, STL_MAP_CLASS *pFeatures, bool bInsideCutDirCCW, bool bOutsideCutDirCW, bool bChainMoves, double dMinNodeCompliance, int nGroupIndex, double dCutHeight, double dCutSpeed, double dRapidMoveHeight, double dRapidMoveSpeed, double dPlungeSpeed, double dRetractSpeed);
		bool	CalculateCircleCircleIntersection(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2);
		bool	CalculateCircleCircleIntersection_II(CIRCLE_OBJECT *pCircle1, CIRCLE_OBJECT *pCircle2, POINT_OBJECT *pPoint1, POINT_OBJECT *pPoint2);
		bool	PostReport(const char *chReport);
  		bool	ReportBasedOnVerbosity(int nVerbosity, const char *chReport);
  		bool	VerifyArcCenterPoint(ARC_DATA *tArcData, double dAbsError, double dRelError);
};




