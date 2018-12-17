#pragma once

typedef enum {
	UNDEFINED = 0,
	ACAD_LT,
	ACAD_12,
	ACAD_13,
	ACAD_14,
	ACAD_15,	// R2000
	UNKNOWN,
	MAX_VERSIONS,
} DXF_VERSION;
 
typedef enum
{
	eLine = 1,
	eCircle,
	eArc,
	eSubtendedArcLine
} SEGMENT_TYPE;

typedef enum
{
	eCutUnknown = 0,
	eCutInsideFeature,
	eCutOutsideFeature,
	eCutOnFeature
} FEATURE_CUT_TYPE;

 
