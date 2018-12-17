/*
 * DXF_FileInput.h
 *
 *  Created on: Apr 11, 2013
 *      Author: jerry
 */

#ifndef DXF_FILEINPUT_H_
#define DXF_FILEINPUT_H_

class DXF_FILE_IO_CLASS
{
private:
	DXF_VERSION 	m_eDxfVersion;
	int			m_bEntitiesFieldFound;
	FILE 		*m_pDxfFile;
	FILE 		*m_pOutputFile;
	long		m_lDxfLineCount;
	int 		m_Vector;

	size_t		m_szLineLen;
	char  		*m_pJunkData;

	long		m_lDxfArcsExtracted;
	long		m_lDxfLinesExtracted;
	long		m_lDxfCirclesExtracted;

	long	m_lCurrentOutsideSegmentCount;
	long	m_lCurrentInsideSegmentCount;
	long	m_lCurrentOnSegmentCount;

	long	m_lCurrentOnSegmentArcCount;
	long	m_lCurrentOnSegmentLineCount;
	long	m_lCurrentOnSegmentCircleCount;
	long	m_lCurrentInSegmentArcCount;
	long	m_lCurrentInSegmentLineCount;
	long	m_lCurrentInSegmentCircleCount;
	long	m_lCurrentOutSegmentArcCount;
	long	m_lCurrentOutSegmentLineCount;
	long	m_lCurrentOutSegmentCircleCount;

public:
			DXF_FILE_IO_CLASS();
	int		DXF_IO_OpenFile (const char *pFile);
	int		DXF_IO_SkipLines (int nLinesToSkip, long *pLineCount);
	long	DXF_IO_SeekToLine (const char *pData, long lMaxLines, long *pLineCount);
	int		DXF_IO_InputParameterLong (const char *pFormat, long *pLongParam, const char *pLine);
	int		DXF_IO_InputParameterString (const char *pFormat, char *pDestString, size_t sizeDestString, const char *pSrcLine);
	int		DXF_IO_InputParameterDouble (double *pDoubleVal, const char *pLine);
	int		DXF_IO_ReadFile (int n, class STL_VECTOR_CLASS *pInsideVectorClass, class STL_VECTOR_CLASS *pOutsideVectorClass, class STL_VECTOR_CLASS *pOnVectorClass);
	int		DXF_IO_CloseFile (void);
 	int		DXF_IO_ReportStatus (void);
 	bool	ReportBasedOnVerbosity (int nVerbosity, const char *chReport);

	long	DXF_IO_getArcsExtracted(void) { return m_lDxfArcsExtracted; }
	long	DXF_IO_getLinesExtracted(void) { return m_lDxfLinesExtracted; }
	long	DXF_IO_getCirclesExtracted(void) { return m_lDxfCirclesExtracted; }
};

#endif /* DXF_FILEINPUT_H_ */
