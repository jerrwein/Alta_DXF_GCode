//============================================================================
// Name        : Main.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <iostream>
#include <assert.h>
#include <vector>
#include <map>

#include "minIni.h"
#include "enumsBase.h"
#include "STL_VectorBase.h"
#include "STL_MapBase.h"
#include "DXF_FileInput.h"
#include "ToolPaths.h"
#include "GL/freeglut.h"
#include "GL/gl.h"
#include "OpenGL_DisplayPaths.h"
#include "GCode_Output.h"

float 	INI_Tool_Diameter = 0.25;
float 	INI_Tool_Height = 1.25;
bool 	INI_Tool_Rotation_CW = true;
bool 	INI_Path_Blending_Enabled = true;
float 	INI_Path_Blending_Radius = 0.125;
float 	INI_Feed_Rates_Transport = 200.0;
float 	INI_Feed_Rates_Cut = 25.0;
float 	INI_Feed_Rates_Plunge = 15.0;
float 	INI_Feed_Rates_Retract = 50.0;
float 	INI_Tool_Path_TransportHeight = 0.2345;
float 	INI_Tool_Path_CutHeight = -0.1234;
bool	INI_Tool_Path_SortOnCentroids = true;
bool	INI_Tool_Path_InsideCutDirCCW = true;
bool	INI_Tool_Path_OutsideCutDirCW = true;
bool 	INI_Part_Offset_Enabled = false;
float 	INI_Part_Offset_X_Offset = 0.0;
float 	INI_Part_Offset_Y_Offset = 0.0;
float 	INI_Part_Offset_Z_Offset = 0.0;
int		INI_Verbosity_DXF_Input = 5;
int		INI_Verbosity_ToolPaths = 5;
int		INI_Verbosity_OpenGL = 5;
int		INI_Verbosity_GCode_Output = 5;

using namespace std;

bool ReadIniParams(void);

int main(int argc, char** argv)
{
	string strDxfFile;
	string strNgcFile;
	if (1 == argc)
	{
		cout << "Arg[0] = " << argv[0] << endl;
	}
	else if (2 == argc)
	{
//		cout << "Arg[0] = " << argv[0] << endl;
//		cout << "Arg[1] = " << argv[1] << endl;
		string strDxfPath("./");
		string strNgcPath("./");
//		string strNgcPath("/home/jerry/CNC_Files/");
		string strBaseName(argv[1]);
		strDxfFile = strDxfPath + strBaseName + ".dxf";
		strNgcFile = strNgcPath + strBaseName + ".ngc";
		cout << std::endl << "DxfToGCode version 1.18" << endl;
		cout << "File << " << strDxfFile << endl;
		cout << "File >> " << strNgcFile << endl;
	}

	class CToolPaths myToolPaths;
	class DXF_FILE_IO_CLASS myDxfFileClass;

	class STL_VECTOR_CLASS 	tUnconnectedDxfInsideElementsClass;
	class STL_VECTOR_CLASS 	tUnconnectedDxfOutsideElementsClass;
	class STL_VECTOR_CLASS 	tUnconnectedDxfOnElementsClass;

	class STL_MAP_CLASS	mySTLMapClass;

	class STL_MAP_CLASS	myCutInsideFeatureMap;
	class STL_MAP_CLASS	myCutOutsideFeatureMap;
	class STL_MAP_CLASS	myCutOnFeatureMap;

	class OpenGL_DisplayPaths myOpenGlDisplayClass;

	class GCode_Output myGCodeClass;
	if(!ReadIniParams())
	{
		cout << "Failed to read INI file..." << std::endl;
		return -1;
	}

	// Open DXF file
//	myDxfFileClass.DXF_IO_OpenFile("ProtoCut1.dxf");
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("Gusset_FourCutouts_Mirror_2.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("ON_FourLinesConnected.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("OUT_TwoGroups_FourLinesConnected.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("IN_OUT_TwoGroups_FourLinesConnected.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("OUT_OneArcThreeLinesConnected.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("OUT_IN_ON_FourGroups_FourLinesConnected.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("OUT_IN_ON_MultipleGroups_ArcsLinesCirclesConnected.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("Gusset_FourCutouts_1D.dxf"))
//	if (1 != myDxfFileClass.DXF_IO_OpenFile("Gusset_FourCutouts_4A.dxf"))
	if (1 != myDxfFileClass.DXF_IO_OpenFile(strDxfFile.data()))
	{
		cout << "Failed to open DXF file..." << std::endl;
		return -1;
	}

	// Read in DXF data - populate inner, outer & on 'unconnected' elements
	myDxfFileClass.DXF_IO_ReadFile(500000, &tUnconnectedDxfInsideElementsClass, &tUnconnectedDxfOutsideElementsClass, &tUnconnectedDxfOnElementsClass);

//	tUnconnectedDxfInsideElementsClass.SVC_ReportStatus();
//	tUnconnectedDxfOutsideElementsClass.SVC_ReportStatus();
//	tUnconnectedDxfOnElementsClass.SVC_ReportStatus();

	cout << std::endl << "DXF Arcs extracted: " << myDxfFileClass.DXF_IO_getArcsExtracted() << std::endl;
	cout << "DXF Lines extracted: " << myDxfFileClass.DXF_IO_getLinesExtracted() << std::endl;
	cout << "DXF Circles extracted: " << myDxfFileClass.DXF_IO_getCirclesExtracted() << std::endl << std::endl;

	 // Begin processing - 
	if (-1 == myToolPaths.ExtractDxfFeatures (eCutInsideFeature, &myCutInsideFeatureMap, &tUnconnectedDxfInsideElementsClass, (double)INI_Tool_Diameter))
		return -1;
	if (-1 == myToolPaths.ExtractDxfFeatures (eCutOutsideFeature, &myCutOutsideFeatureMap, &tUnconnectedDxfOutsideElementsClass, (double)INI_Tool_Diameter))
		return -1;
	if (-1 == myToolPaths.ExtractDxfFeatures (eCutOnFeature, &myCutOnFeatureMap, &tUnconnectedDxfOnElementsClass, (double)INI_Tool_Diameter))
		return -1;

	cout << "INSIDE entities: " << myCutInsideFeatureMap.SMC_GetSize() << std::endl;
	cout << "OUTSIDE entities: " << myCutOutsideFeatureMap.SMC_GetSize() << std::endl;
	cout << "ON entities: " << myCutOnFeatureMap.SMC_GetSize() << std::endl;

	// OpenGL Display;
	myOpenGlDisplayClass.DisplayAllPaths(&myCutInsideFeatureMap, &myCutOutsideFeatureMap, &myCutOnFeatureMap);
	myOpenGlDisplayClass.DisplayCutPaths(&myCutInsideFeatureMap, &myCutOutsideFeatureMap, &myCutOnFeatureMap);

	// Output GCode
 //	myGCodeClass.Set_BaseFileName (strNgcFile.data());
 	myGCodeClass.Set_PathFileName (strNgcFile);
	myGCodeClass.OutputGCodeFile (&myCutInsideFeatureMap, &myCutOutsideFeatureMap, &myCutOnFeatureMap, INI_Tool_Path_InsideCutDirCCW, INI_Tool_Path_OutsideCutDirCW, false, 0.9000, INI_Tool_Path_CutHeight, INI_Feed_Rates_Cut, INI_Tool_Path_TransportHeight, INI_Feed_Rates_Transport, INI_Feed_Rates_Plunge, INI_Feed_Rates_Retract);

	return 0;
}

bool ReadIniParams(void)
{
	minIni 	ini("DXF_GCode.ini");
	string 	sIniStr;
	long 	nIniVal;
	float	fIniVal;
	bool	bIniVal;
	int 	nBadReads = 0;
	
	// Verbosities
	nIniVal = ini.getl ("Verbosity", "DXF_Input", -1);
	if (-1 != nIniVal)
		INI_Verbosity_DXF_Input = nIniVal;
	else
		nBadReads++;

	nIniVal = ini.getl ("Verbosity", "ToolPath", -1);
	if (-1 != nIniVal)
		INI_Verbosity_ToolPaths = nIniVal;
	else
		nBadReads++;

	nIniVal = ini.getl ("Verbosity", "OpenGL", -1);
	if (-1 != nIniVal)
		INI_Verbosity_OpenGL = nIniVal;
	else
		nBadReads++;

	nIniVal = ini.getl ("Verbosity", "GCode_Output", -1);
	if (-1 != nIniVal)
		INI_Verbosity_GCode_Output = nIniVal;
	else
		nBadReads++;

	// Tool info
	fIniVal = ini.getf ("Tool", "Diameter", -1.0);
	if (-1.0 != fIniVal)
		INI_Tool_Diameter = fIniVal;
	else
		nBadReads++;

	fIniVal = ini.getf ("Tool", "Height", -1.0);
	if (-1.0 != fIniVal)
		INI_Tool_Height = fIniVal;
	else
		nBadReads++;

	bIniVal = ini.getbool("Tool", "Rotation_CW", false);
	INI_Tool_Rotation_CW = bIniVal;

	// Feed Rates
	fIniVal = ini.getf ("Feed_Rates", "Transport", -1.0);
	if (-1.0 != fIniVal)
		INI_Feed_Rates_Transport = fIniVal;
	else
		nBadReads++;
	fIniVal = ini.getf ("Feed_Rates", "Cut", -1.0);
	if (-1.0 != fIniVal)
		INI_Feed_Rates_Cut = fIniVal;
	else
		nBadReads++;
	fIniVal = ini.getf ("Feed_Rates", "Plunge", -1.0);
	if (-1.0 != fIniVal)
		INI_Feed_Rates_Plunge = fIniVal;
	else
		nBadReads++;
	fIniVal = ini.getf ("Feed_Rates", "Retact", -1.0);
	if (-1.0 != fIniVal)
		INI_Feed_Rates_Retract = fIniVal;
	else
		nBadReads++;

	// Tool Path info
	bIniVal = ini.getbool("Tool_Path", "SortBaseOnCentriods", true);
	INI_Tool_Path_SortOnCentroids = bIniVal;

	fIniVal = ini.getf ("Tool_Path", "TransportHeight", -1.0);
	if (-1.0 != fIniVal)
		INI_Tool_Path_TransportHeight = fIniVal;
	else
		nBadReads++;

	fIniVal = ini.getf ("Tool_Path", "CutHeight", -1.0);
	if (-1.0 != fIniVal)
		INI_Tool_Path_CutHeight = fIniVal;
	else
		nBadReads++;

	bIniVal = ini.getbool("Tool_Path", "InsideCutDirCCW", true);
	INI_Tool_Path_InsideCutDirCCW = bIniVal;

	bIniVal = ini.getbool("Tool_Path", "OutsideCutDirCW", true);
	INI_Tool_Path_OutsideCutDirCW = bIniVal;

	if (3 <= nBadReads)
		return false;
	else
		return true;
}
#if 0
float 	INI_Tool_Diameter = 0.5;
float 	INI_Tool_Height = 1.25;
bool 	INI_Tool_Rotation_CW = true;
bool 	INI_Path_Blending_Enabled = true;
float 	INI_Path_Blending_Radius = 0.125;
float 	INI_Feed_Rates_Transport = 200.0
float 	INI_Feed_Rates_Cut = 25.0
float 	INI_Feed_Rates_Plunge = 15.0
float 	INI_Feed_Rates_Retract = 50.0
float 	INI_Tool_Path_TransportHeight = -0.875
float 	INI_Tool_Path_CutHeight = -1.25
bool 	INI_Part_Offset_Enabled = false
float 	INI_Part_Offset_X_Offset = 0.0
float 	INI_Part_Offset_Y_Offset = 0.0
float 	INI_Part_Offset_Z_Offset = 0.0
int	INI_Verbosity_DXF_Input = 3;
int	INI_Verbosity_ToolPath = 3;
int	INI_Verbosity_GCode_Output = 3;
#endif



