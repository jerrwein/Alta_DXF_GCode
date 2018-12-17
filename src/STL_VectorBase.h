typedef struct
{
 	double dX;
	double dY;
} POINT_OBJECT;

typedef struct
{
	double	dAx;
	double	dAy;
	double	dBx;
	double	dBy;
	long	lEntityNumber;
	double	dSlope;
	double	dIntercept;
	bool	bVerticalLine;
} LINE_OBJECT;

typedef struct
{
	long lEntityNumber;
	double dXc;
	double dYc;
	double dRadius;
} CIRCLE_OBJECT;

typedef struct
{
	double	dLeft;
	double	dBottom;
	double	dRight;
	double	dTop;
} RECT_OBJECT;

typedef enum
{
	eSolid = 1,
	eShortDash,
	eLongDash
} GL_LINE_TYPE;

typedef struct tagUnconnectedElement
{
	SEGMENT_TYPE		eSegmentType;
	FEATURE_CUT_TYPE 	eFeatureCutType;	
	long   				lDxfEntityNumber;
	double 			dAx;
	double 			dAy;
	double 			dBx;
	double 			dBy;
	double 			dRx;
	double 			dRy;
	double 			dRadius;
	double 			dStartAngle;
	double 			dEndAngle;
	int				nA_ConnectedSegmentIndex;
	unsigned char	ucA_ConnectedSegmentsEnd;
	int				nB_ConnectedSegmentIndex;
	unsigned char	ucB_ConnectedSegmentsEnd;
	long			lFeatureGroupIndex;
} UNCONNECTED_ELEMENT;

typedef struct tagConnectedElement
{
	SEGMENT_TYPE	eSegmentType;
	long			lDxfEntityNumber;
	int				nFeatureIndex;
	int				nSegmentIndex;
	double			dAx;
	double			dAy;
	double			dBx;
	double			dBy;
	double			dRadius;
	double			dRx;
	double			dRy;
	double			dStartAngle;
	double			dEndAngle;
	double			dAngularOrientation;
	unsigned char	ucStartEndpointAtInitialSort;
	unsigned char	ucCcwRotationStartEndpoint;
	int				nA_ConnectedSegmentIndex;
	unsigned char	ucA_ConnectedSegmentEnd;
	double			dA_ChainedMoveCompliance;
	int				nB_ConnectedSegmentIndex;
	unsigned char	ucB_ConnectedSegmentEnd;
	double			dB_ChainedMoveCompliance;
	double			dLineSlope;
	double			dLineIntercept;
	bool			bVerticalLine;
	RECT_OBJECT		rBoundaries;
} CONNECTED_ELEMENT;

typedef struct tagOpenGlElement
{
	SEGMENT_TYPE	eSegmentType;
	GL_LINE_TYPE	eLinePattern;
	float			fLineWidth;
	float			fAx;
	float			fAy;
	float			fAz;
	float			fBx;
	float			fBy;
	float			fBz;
	float			fRadius;
	float			fRx;
	float			fRy;
	float			fStartAngle;
	float			fEndAngle;
} OPENGL_ELEMENT;

class STL_CONNECTED_SEGS_CLASS
{
	private:
		std::vector <CONNECTED_ELEMENT> m_ConnectedElementArray;
		CONNECTED_ELEMENT m_UnconnectedElementStruct;
	public:
		STL_CONNECTED_SEGS_CLASS();
		CONNECTED_ELEMENT SVC_GetElement(size_t nIndex);
		bool	SVC_SetElement(size_t nIndex, CONNECTED_ELEMENT rElem);
		int 	SVC_AddElement(CONNECTED_ELEMENT cElem);
		int 	SVC_RemoveLastElement(void);
		void 	SVC_RemoveAllElements(void);
		void 	SVC_ReportStatus(void);
		size_t	SVC_Size(void);
};

class STL_VECTOR_CLASS 
{
	private:
		std::vector <UNCONNECTED_ELEMENT> m_UnconnectedElementArray;
		UNCONNECTED_ELEMENT m_UnconnectedElementStruct;
	public:
		STL_VECTOR_CLASS();
		UNCONNECTED_ELEMENT SVC_GetElement(size_t nIndex);
		int		SVC_SetElement(size_t nIndex, UNCONNECTED_ELEMENT rElem);
		int 	SVC_AddElement(UNCONNECTED_ELEMENT cElem);
		int 	SVC_RemoveLastElement(void);
		void 	SVC_RemoveAllElements(void);
		int 	SVC_ReportStatus(void);
		size_t	SVC_Size(void);
};

class STL_OPENGL_CLASS
{
	private:
		std::vector <OPENGL_ELEMENT> m_OpenGL_ElementArray;
	public:
			STL_OPENGL_CLASS();
			OPENGL_ELEMENT SVC_GetElement(size_t nIndex);
			bool	SVC_SetElement(size_t nIndex, OPENGL_ELEMENT rElem);
			int 	SVC_AddElement(OPENGL_ELEMENT cElem);
			int 	SVC_RemoveLastElement(void);
			void 	SVC_RemoveAllElements(void);
			void 	SVC_ReportStatus(void);
			size_t	SVC_Size(void);
};

