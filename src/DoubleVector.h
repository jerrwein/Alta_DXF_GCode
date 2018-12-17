class CDoubleVector
{
	public:
		double m_dX;
		double m_dY;
		double m_dZ;
	public:

		CDoubleVector();
		CDoubleVector(double dX, double dY, double dZ);
		CDoubleVector operator +(CDoubleVector &vec2);
		CDoubleVector operator -(CDoubleVector &vec2);
		CDoubleVector operator *(double dFactor);
//		~CDoubleVector();
		
		double DotProduct(CDoubleVector v1, CDoubleVector v2);
		static double DotProduct2 (CDoubleVector v1, CDoubleVector v2);
		
		CDoubleVector CrossProduct (CDoubleVector v1, CDoubleVector v2);
		static CDoubleVector CrossProduct2 (CDoubleVector v1, CDoubleVector v2);
	
		double Magnitude(void);
		CDoubleVector Normalize(void);
		CDoubleVector Scale (double dScale);
};
