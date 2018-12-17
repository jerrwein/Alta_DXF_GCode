#include <iostream>
#include <vector>
#include <map>

#include "enumsBase.h"
#include "STL_VectorBase.h"
#include "STL_MapBase.h"

using namespace std;

STL_MAP_CLASS::STL_MAP_CLASS()
{
	//	cout << "STL_MAP_CLASS: IN CONSTRUCTOR" << std::endl;
}

int STL_MAP_CLASS::SMC_AddFeaturePtr(int nKey, FEATURE_ELEMENT *pFeature)
{
	std::pair<std::map<int, FEATURE_ELEMENT*>::iterator, bool> retPair;

	retPair = m_FeaturePtrMap.insert (T_IntFeaturePtrPair(nKey, pFeature));

	if (retPair.second == false)
	{
		cout << "STL_MAP_CLASS::SMC_AddFeature() FAILURE" << std::endl;
		return -1;
	}
	cout << "STL_MAP_CLASS::SMC_AddFeature() success..." << std::endl;
	return 1;
}

int STL_MAP_CLASS::SMC_AddFeature(int nKey, FEATURE_ELEMENT rFeature)
{
	std::pair<std::map<int, FEATURE_ELEMENT>::iterator, bool> retPair;

//	SMC_ReportStatus();

	retPair = m_FeatureMap.insert (T_IntFeaturePair(nKey, rFeature));

	if (retPair.second == false)
	{
		cout << "STL_MAP_CLASS::SMC_AddFeature() FAILURE" << std::endl;
		return -1;
	}
// cout << "STL_MAP_CLASS::SMC_AddFeature() success..." << std::endl;
//	SMC_ReportStatus();
	return 1;
}

bool STL_MAP_CLASS::SMC_GetFeaturePtr(int nKey, FEATURE_ELEMENT *ppFeature)
{
//	int nVal1 = m_FeatureMap.at(2)->m_nFeatureIndex;
//	int nVal2 = m_FeatureMap.at(nKey)->m_nFeatureIndex;
// ffdd	FEATURE_ELEMENT *pFeature1 = m_FeaturePtrMap.at(2);
	ppFeature = m_FeaturePtrMap.at(3);
	ppFeature = m_FeaturePtrMap.at(nKey);

	return true;
}

bool STL_MAP_CLASS::SMC_GetFeature(int nKey, FEATURE_ELEMENT *pFeature)
{
//	int nVal1 = m_FeatureMap.at(2)->m_nFeatureIndex;
//	int nVal2 = m_FeatureMap.at(nKey)->m_nFeatureIndex;
//	FEATURE_ELEMENT rFeature1 = m_FeatureMap.at(2);
//	*pFeature = m_FeatureMap.at(3);
	*pFeature = m_FeatureMap.at(nKey);

 	return true;
}

bool STL_MAP_CLASS::SMC_UpdateFeature(int nKey, FEATURE_ELEMENT rFeature)
{
	m_FeatureMap.at(nKey) = rFeature;

 	return true;
}
int STL_MAP_CLASS::SMC_RemoveFeature(int nKey)
{
//	m_UnconnectedElementArray.pop_back();
	return 1;
}

int STL_MAP_CLASS::SMC_RemoveAll(void)
{
	m_FeatureMap.clear();
	return 1;
}

size_t STL_MAP_CLASS::SMC_GetSize(void)
{
	return m_FeatureMap.size();
}

int STL_MAP_CLASS::SMC_ReportStatus(void)
{
	cout << "The STL_MapBase report\n";
	cout << "Size: " << m_FeatureMap.size() << std::endl;
//	cout << "Capacity: " << m_UnconnectedElementArray.capacity() << std::endl;
	cout << "End of report\n";
	return 1;
}

 
