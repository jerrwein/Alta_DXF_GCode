#include <iostream>
#include <vector>

#include "enumsBase.h"
#include "STL_VectorBase.h"

using namespace std;

STL_VECTOR_CLASS::STL_VECTOR_CLASS()
{
	m_UnconnectedElementArray.reserve(5);
}

int STL_VECTOR_CLASS::SVC_SetElement(size_t nIndex, UNCONNECTED_ELEMENT rElem)
{
	m_UnconnectedElementArray.at(nIndex) = rElem;
	return 1;
}

UNCONNECTED_ELEMENT STL_VECTOR_CLASS::SVC_GetElement(size_t nIndex)
{
	return m_UnconnectedElementArray.at(nIndex);
}

int STL_VECTOR_CLASS::SVC_AddElement(UNCONNECTED_ELEMENT cElem)
{
	m_UnconnectedElementArray.push_back(cElem);
	return 1;
}

int STL_VECTOR_CLASS::SVC_RemoveLastElement(void)
{
	m_UnconnectedElementArray.pop_back();
	return 1;
}

void STL_VECTOR_CLASS::SVC_RemoveAllElements(void)
{
	m_UnconnectedElementArray.empty();
	return;
}

size_t	STL_VECTOR_CLASS::SVC_Size(void)
{
	return m_UnconnectedElementArray.size();
}

int STL_VECTOR_CLASS::SVC_ReportStatus(void)
{
	cout << "The STL_VectorBase report\n";
	cout << "Size: " << m_UnconnectedElementArray.size() << std::endl;
	cout << "Capacity: " << m_UnconnectedElementArray.capacity() << std::endl;
	cout << "End of report\n";
	return 1;
}

// Connected Class

STL_CONNECTED_SEGS_CLASS::STL_CONNECTED_SEGS_CLASS()
{
	m_ConnectedElementArray.reserve(5);
}

bool STL_CONNECTED_SEGS_CLASS::SVC_SetElement(size_t nIndex, CONNECTED_ELEMENT rElem)
{
	// SVC_ReportStatus();
	m_ConnectedElementArray.at(nIndex) = rElem;
	return true;
}

CONNECTED_ELEMENT STL_CONNECTED_SEGS_CLASS::SVC_GetElement(size_t nIndex)
{
	return m_ConnectedElementArray.at(nIndex);
}

int STL_CONNECTED_SEGS_CLASS::SVC_AddElement(CONNECTED_ELEMENT rElem)
{
	m_ConnectedElementArray.push_back(rElem);
	return 1;
}

int STL_CONNECTED_SEGS_CLASS::SVC_RemoveLastElement(void)
{
	m_ConnectedElementArray.pop_back();
	return 1;
}

void STL_CONNECTED_SEGS_CLASS::SVC_RemoveAllElements(void)
{
	m_ConnectedElementArray.empty();
	return;
}

size_t STL_CONNECTED_SEGS_CLASS::SVC_Size(void)
{
	return m_ConnectedElementArray.size();
}

void STL_CONNECTED_SEGS_CLASS::SVC_ReportStatus(void)
{
	cout << "The STL_ConnectedVectorBase report\n";
	cout << "Size: " << m_ConnectedElementArray.size() << std::endl;
	cout << "Capacity: " << m_ConnectedElementArray.capacity() << std::endl;
	cout << "End of report\n";
}



STL_OPENGL_CLASS::STL_OPENGL_CLASS()
{
	m_OpenGL_ElementArray.reserve(5);
}

bool STL_OPENGL_CLASS::SVC_SetElement(size_t nIndex, OPENGL_ELEMENT rElem)
{
//	SVC_ReportStatus();
	m_OpenGL_ElementArray.at(nIndex) = rElem;
	return true;
}

OPENGL_ELEMENT STL_OPENGL_CLASS::SVC_GetElement(size_t nIndex)
{
	return m_OpenGL_ElementArray.at(nIndex);
}

int STL_OPENGL_CLASS::SVC_AddElement(OPENGL_ELEMENT rElem)
{
	m_OpenGL_ElementArray.push_back(rElem);
	return 1;
}

int STL_OPENGL_CLASS::SVC_RemoveLastElement(void)
{
	m_OpenGL_ElementArray.pop_back();
	return 1;
}

void STL_OPENGL_CLASS::SVC_RemoveAllElements(void)
{
	m_OpenGL_ElementArray.empty();
	return;
}

size_t STL_OPENGL_CLASS::SVC_Size(void)
{
	return m_OpenGL_ElementArray.size();
}

void STL_OPENGL_CLASS::SVC_ReportStatus(void)
{
	cout << "The STL_OpenGLVectorBase report\n";
	cout << "Size: " << m_OpenGL_ElementArray.size() << std::endl;
	cout << "Capacity: " << m_OpenGL_ElementArray.capacity() << std::endl;
	cout << "End of report\n";
}


