#include "FEElementLibrary.h"


vector<FEElemTraits> FEElementLibrary::m_lib;

void FEElementLibrary::addElement(int ntype, int nshape, int nclass, int nodes, int faces, int edges)
{
	FEElemTraits t = {ntype, nshape, nclass, nodes, faces, edges};
	m_lib.push_back(t);
}

void FEElementLibrary::InitLibrary()
{
	m_lib.clear();

	// NOTE: When adding new elements make sure to set the faces to zero for shells, and the edges to zero for solids.
	addElement(FE_INVALID_ELEMENT_TYPE, 0, 0, 0, 0, 0);
	addElement(FE_HEX8   , ELEM_HEX  , ELEM_SOLID,  8, 6, 0);
	addElement(FE_TET4   , ELEM_TET  , ELEM_SOLID,  4, 4, 0);
	addElement(FE_PENTA6 , ELEM_PENTA, ELEM_SOLID,  6, 5, 0);
	addElement(FE_QUAD4  , ELEM_QUAD , ELEM_SHELL,  4, 0, 4);
	addElement(FE_TRI3   , ELEM_TRI  , ELEM_SHELL,  3, 0, 3);
	addElement(FE_BEAM2  , ELEM_LINE , ELEM_BEAM ,  2, 0, 0);
	addElement(FE_HEX20  , ELEM_HEX  , ELEM_SOLID, 20, 6, 0);
	addElement(FE_QUAD8  , ELEM_QUAD , ELEM_SHELL,  8, 0, 4);
	addElement(FE_BEAM3  , ELEM_LINE , ELEM_BEAM ,  3, 0, 0);
	addElement(FE_TET10  , ELEM_TET  , ELEM_SOLID, 10, 4, 0);
	addElement(FE_TRI6   , ELEM_TRI  , ELEM_SHELL,  6, 0, 3);
	addElement(FE_TET15  , ELEM_TET  , ELEM_SOLID, 15, 4, 0);
	addElement(FE_HEX27  , ELEM_HEX  , ELEM_SOLID, 27, 6, 0);
	addElement(FE_TRI7   , ELEM_TRI  , ELEM_SHELL,  7, 0, 3);
	addElement(FE_QUAD9  , ELEM_QUAD , ELEM_SHELL,  9, 0, 4);
	addElement(FE_PENTA15, ELEM_PENTA, ELEM_SOLID, 15, 5, 0);
	addElement(FE_PYRA5  , ELEM_PYRA , ELEM_SOLID,  5, 5, 0);
	addElement(FE_TET20  , ELEM_TET  , ELEM_SOLID, 20, 4, 0);
	addElement(FE_TRI10  , ELEM_TRI  , ELEM_SHELL, 10, 0, 3);
	addElement(FE_TET5   , ELEM_TET  , ELEM_SOLID,  5, 4, 0);
}

const FEElemTraits* FEElementLibrary::GetTraits(int type)
{
	int ntype = (int) type;
	if ((ntype >= 0) && (ntype < m_lib.size()))
	{
		return &m_lib[ntype];
	}
	else
	{
		assert(false);
		return 0;
	}
}