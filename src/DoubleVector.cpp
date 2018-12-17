#include "DoubleVector.h"

#include <math.h>

CDoubleVector::CDoubleVector()
{
	m_dX = 0.0;
	m_dY = 0.0;
	m_dZ = 0.0;
}

CDoubleVector::CDoubleVector (double dX, double dY, double dZ)
{
	m_dX = dX;
	m_dY = dY;
	m_dZ = dZ;
}

CDoubleVector CDoubleVector::operator +(CDoubleVector &vec2)
{
	CDoubleVector v1;
	v1.m_dX = m_dX + vec2.m_dX;
	v1.m_dY = m_dY + vec2.m_dY;
	v1.m_dZ = m_dZ + vec2.m_dZ;
	return v1;
}

CDoubleVector CDoubleVector::operator -(CDoubleVector &vec2)
{
	CDoubleVector v1;
	v1.m_dX = m_dX - vec2.m_dX;
	v1.m_dY = m_dY - vec2.m_dY;
	v1.m_dZ = m_dZ - vec2.m_dZ;
	return v1;
}

CDoubleVector CDoubleVector::operator *(double dFactor)
{
	CDoubleVector v1;
	v1.m_dX = m_dX * dFactor;
	v1.m_dY = m_dY * dFactor;
	v1.m_dZ = m_dZ * dFactor;
	return v1;
}

double CDoubleVector::DotProduct (CDoubleVector v1, CDoubleVector v2)
{
	return ((v1.m_dX * v2.m_dX) + (v1.m_dY * v2.m_dY) + (v1.m_dZ * v2.m_dZ));
}

double CDoubleVector::DotProduct2 (CDoubleVector v1, CDoubleVector v2)
{
	return ((v1.m_dX * v2.m_dX) + (v1.m_dY * v2.m_dY) + (v1.m_dZ * v2.m_dZ));
}

CDoubleVector CDoubleVector::CrossProduct (CDoubleVector v1, CDoubleVector v2)
{
	CDoubleVector vResult;

    vResult.m_dX  = (v1.m_dY * v2.m_dZ) - (v1.m_dZ * v2.m_dY);
    vResult.m_dY  = (v1.m_dZ * v2.m_dX) - (v1.m_dX * v2.m_dZ);
    vResult.m_dZ  = (v1.m_dX * v2.m_dY) - (v1.m_dY * v2.m_dX);

	return vResult;
}

CDoubleVector CDoubleVector::CrossProduct2 (CDoubleVector v1, CDoubleVector v2)
{
	CDoubleVector vResult;

    vResult.m_dX  = (v1.m_dY * v2.m_dZ) - (v1.m_dZ * v2.m_dY);
    vResult.m_dY  = (v1.m_dZ * v2.m_dX) - (v1.m_dX * v2.m_dZ);
    vResult.m_dZ  = (v1.m_dX * v2.m_dY) - (v1.m_dY * v2.m_dX);

	return vResult;
}

double CDoubleVector::Magnitude (void)
{
	return sqrt (pow(m_dX,2) + pow(m_dY,2) + pow(m_dZ,2));
}

CDoubleVector CDoubleVector::Normalize(void)
{
	double dMagnitude = sqrt (pow(m_dX,2) + pow(m_dY,2) + pow(m_dZ,2));
	CDoubleVector vResult;
	if (0 <= dMagnitude)
	{
		vResult.m_dX = m_dX / dMagnitude;
		vResult.m_dY = m_dY / dMagnitude;
		vResult.m_dZ = m_dZ / dMagnitude;
	}
	return vResult;
}

CDoubleVector CDoubleVector::Scale (double dScale)
{
	CDoubleVector vResult;

	vResult.m_dX = m_dX * dScale;
	vResult.m_dY = m_dY * dScale;
	vResult.m_dZ = m_dZ * dScale;

	return vResult;
}
