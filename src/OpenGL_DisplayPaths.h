typedef struct
{
	float fXc;
	float fYc;
	float fRadius;
} OBJ_CIRCLE;

typedef struct
{
	float fXc;
	float fYc;
	float fRadius;
	float fStartAngle;
	float fEndAngle;
	float fZ;
} OBJ_ARC;

class OpenGL_DisplayPaths
{
	private:
		float m_fPartWidth;
		float m_fPartHeight;
	//	float m_fPartAspectRatio;
		float m_fOffsetX;
		float m_fOffsetY;
	public:
				OpenGL_DisplayPaths();
		void 	DisplayAllPaths(STL_MAP_CLASS *pInsideFeaturesMap, STL_MAP_CLASS *pOutsideFeaturesMap, STL_MAP_CLASS *pOnFeaturesMap);
		void 	DisplayCutPaths(STL_MAP_CLASS *pInsideFeaturesMap, STL_MAP_CLASS *pOutsideFeaturesMap, STL_MAP_CLASS *pOnFeaturesMap);
		bool	PostReport(const char *chReport);
		bool	ReportBasedOnVerbosity(int nVerbosity, const char *chReport);
};

class COpenGL
{
	public:
		COpenGL();
		COpenGL(STL_OPENGL_CLASS *pSaPart, STL_OPENGL_CLASS *pSaLoop1, STL_OPENGL_CLASS *pSaLoop2, float fWidth, float fHeight, float fOffsetX, float fOffsetY, float fScale);
		COpenGL(STL_OPENGL_CLASS *pMaterialBlockSegments, STL_OPENGL_CLASS *pGCodeSegments, float fWidth, float fHeight, float fOffsetX, float fOffsetY, float fScale);
		virtual ~COpenGL();

		int CreateGlutGLWindow_2(char *pTitle, int width, int height, float fPartWidth, float fPartHeight, int bits, bool fullscreenflag);
		static void Glut_DrawScene(void);
		static void	GLUT_reshape(int w, int h);
		static void GLUT_mouse(int button, int state, int x, int y);
		static void	GLUT_MouseMovement(int x , int y);
		static void GLUT_MouseWheel(int wheel, int dir, int x, int y);
		static void Glut_keyboard(unsigned char key, int x, int y);
		static void	Glut_special(int key, int x, int y);

		void 		DrawArc (float fScale, OBJ_ARC *pArc, int nSegments, float cRed, float cGreen, float cBlue);
		void static DrawArc (float fScale, OBJ_ARC *pArc, float fOffsetUnitsX, float fOffsetUnitsY, float cRed, float cGreen, float cBlue);
		void static	DrawArc (float fScale, OBJ_ARC *pArc, float fOffsetUnitsX, float fOffsetUnitsY, int nSegments, float cRed, float cGreen, float cBlue);
		void static	DrawArc2D (OBJ_ARC *pArc, float fOffsetUnitsX, float fOffsetUnitsY, float fZ, int nSegments, float cRed, float cGreen, float cBlue);
		void static	GLUT_ScrollLeft(int x, int y);
		void static	GLUT_ScrollRight(int x, int y);
		void static	GLUT_ScrollUp(int x, int y);
		void static	GLUT_ScrollDown(int x, int y);
		void static	GLUT_RePosition(float fDeltaX, float fDeltaY);
		void static	GLUT_ZoomIn(int x, int y);
		void static	GLUT_ZoomOut(int x, int y);
		bool static	ReportBasedOnVerbosity(int nVerbosity, const char *chReport);
//		bool        ReportBasedOnVerbosity(int nVerbosity, const char *chReport);

		bool	m_bActive;			// Window Active Flag Set To TRUE By Default
		static bool	m_bKeys[256];		// Array Used For The Keyboard Routine

		bool		m_bFullscreen;			// Fullscreen Flag Set To Fullscreen Mode By Default
// ddd		HGLRC		m_hRC;					// Permanent Rendering Context
// ddd		HINSTANCE	m_hInstance;			// Holds The Instance Of The Application

		unsigned long m_lFontList;

		GLuint		m_FontBaseDisplayList;	// Base Display List For The Font Set
// ddd		GLYPHMETRICSFLOAT m_gmf[256];		// Storage For Information About Our Outline Font Characters


		// Drawing specific
		static GLfloat m_fViewportOffsetX;
		static GLfloat m_fViewportOffsetY;
		static GLfloat m_fViewportWidth;
		static GLfloat m_fViewportHeight;
		static GLfloat m_fViewPortAspectRatio;
		static GLfloat m_fPartAspectRatio;

		static float m_fXRotation;
		static float m_fYRotation;
		static float m_fZRotation;

		static long  m_lCountX;
		static long  m_lCountY;
		static long  m_lCountZ;

		static float m_fRotationFactorX;
		static float m_fRotationFactorY;
		static float m_fRotationFactorZ;

		float m_fScaleFactor;


		static STL_OPENGL_CLASS *m_pSaSortedSegments;
		static STL_OPENGL_CLASS *m_pSaLoop1Segments;
		static STL_OPENGL_CLASS *m_pSaLoop2Segments;

		static STL_OPENGL_CLASS *m_pSaMaterialBlockSegments;
		static STL_OPENGL_CLASS *m_pSaGCodeSegments;
};


