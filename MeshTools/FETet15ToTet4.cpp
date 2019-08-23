#include "stdafx.h"
#include "FEModifier.h"

//-----------------------------------------------------------------------------
//! Convert a tet10 to a tet4 mesh by eliminating all the edge nodes
FEMesh* FETet15ToTet4::Apply(FEMesh* pm)
{
	// make sure the mesh is a tet10 mesh
	if (pm->IsType(FE_TET15) == false) return 0;

	// get the number of items
	int NN = pm->Nodes();
	int NE = pm->Elements();
	int NF = pm->Faces();
	int NC = pm->Edges();

	// count the number of corner nodes
	for (int i=0; i<NN; ++i) pm->Node(i).m_ntag = -1;
	for (int i=0; i<NE; ++i)
	{
		FEElement& el = pm->Element(i);
		for (int j=0; j<4; ++j) pm->Node(el.m_node[j]).m_ntag = 1;
	}
	int nn = 0;
	for (int i=0; i<NN; ++i)
	{
		FENode& ni = pm->Node(i);
		if (ni.m_ntag == 1) 
		{
			ni.m_ntag = nn++;
		}
	}

	// allocate a new mesh
	FEMesh* pnew = new FEMesh;
	pnew->Create(nn, NE, NF, NC);

	// create the nodes
	nn = 0;
	for (int i=0; i<NN; ++i)
	{
		FENode& n0 = pm->Node(i);
		if (n0.m_ntag >= 0)
		{
			FENode& n1 = pnew->Node(nn++);
			n1.r = n0.r;
			n1.m_gid = n0.m_gid;
		}
	}

	// create the elements
	for (int i=0; i<NE; ++i)
	{
		FEElement& e0 = pm->Element(i);
		FEElement& e1 = pnew->Element(i);
		e1 = e0;

		e1.m_gid = e0.m_gid;

		e1.SetType(FE_TET4);
		e1.m_node[0] = pm->Node(e0.m_node[0]).m_ntag;
		e1.m_node[1] = pm->Node(e0.m_node[1]).m_ntag;
		e1.m_node[2] = pm->Node(e0.m_node[2]).m_ntag;
		e1.m_node[3] = pm->Node(e0.m_node[3]).m_ntag;
	}

	// create the new faces
	for (int i=0; i<NF; ++i)
	{
		FEFace& f0 = pm->Face(i);
		FEFace& f1 = pnew->Face(i);

		f1.SetType(FE_FACE_TRI3);
		f1.m_gid = f0.m_gid;
		f1.m_sid = f0.m_sid;
		f1.n[0] = pm->Node(f0.n[0]).m_ntag;
		f1.n[1] = pm->Node(f0.n[1]).m_ntag;
		f1.n[2] = pm->Node(f0.n[2]).m_ntag;
		f1.m_elem[0] = f0.m_elem[0];
		f1.m_elem[1] = f0.m_elem[1];
		f1.m_nbr[0] = f0.m_nbr[0];
		f1.m_nbr[1] = f0.m_nbr[1];
		f1.m_nbr[2] = f0.m_nbr[2];
	}

	// create the new edges
	for (int i=0; i<NC; ++i)
	{
		FEEdge& e0 = pm->Edge(i);
		FEEdge& e1 = pnew->Edge(i);

		e1.SetType(FE_EDGE2);
		e1.n[0] = pm->Node(e0.n[0]).m_ntag;
		e1.n[1] = pm->Node(e0.n[1]).m_ntag;
		e1.n[2] = -1;
		e1.m_gid = e0.m_gid;
		e1.m_nbr[0] = e0.m_nbr[0];
		e1.m_nbr[1] = e0.m_nbr[1];
		e1.m_elem = e1.m_elem;
	}

	pnew->MarkExteriorNodes();
	pnew->UpdateEdgeNeighbors();
	pnew->UpdateNormals();

	return pnew;
}