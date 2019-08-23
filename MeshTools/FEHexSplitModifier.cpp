#include "stdafx.h"
#include "FESplitModifier.h"
#include <MeshLib/FEFaceEdgeList.h>

FEHexSplitModifier::FEHexSplitModifier() : FEModifier("Split")
{
	m_smoothSurface = false;
}

void FEHexSplitModifier::DoSurfaceSmoothing(bool b)
{
	m_smoothSurface = b;
}

FEMesh* FEHexSplitModifier::Apply(FEMesh* pm)
{
	// make sure we are dealing with a hex mesh
	if (pm->IsType(FE_HEX8) == false) return 0;

	// build the edge table of the mesh (each edge will add a node)
	FEEdgeList ET(*pm);
	FEElementEdgeList EET(*pm, ET);

	// build the face table (each face will add a node)
	FEFaceTable FT(*pm);
	FEElementFaceList EFL(*pm, FT);

	// get the mesh item counts
	int NN0 = pm->Nodes();
	int NC0 = ET.size();
	int NF0 = FT.size();
	int NE0 = pm->Elements();

	// each node, edge, face, and element will create a new node
	int NN1 = NN0 + NC0 + NF0 + NE0;

	// each element will be split in eight
	int NE1 = 8*NE0;

	// create new mesh
	FEMesh* pmnew = new FEMesh;
	pmnew->Create(NN1, NE1);

	// build face-edge table
	FEFaceEdgeList FET(*pm, ET);

	// assign nodes
	int n = 0;
	for (int i=0; i<NN0; ++i, ++n)
	{
		FENode& n1 = pmnew->Node(n);
		FENode& n0 = pm->Node(i);
		n1 = n0;
	}

	for (int i=0; i<NC0; ++i, ++n)
	{
		FENode& n1 = pmnew->Node(n);

		pair<int,int>& edge = ET[i];
		FENode& na = pm->Node(edge.first);
		FENode& nb = pm->Node(edge.second);

		n1.r = (na.r + nb.r)*0.5;
	}

	for (int i=0; i<NF0; ++i, ++n)
	{
		FENode& n1 = pmnew->Node(n);

		FEFace& face = FT[i];

		vec3d r0 = pm->Node(face.n[0]).r;
		vec3d r1 = pm->Node(face.n[1]).r;
		vec3d r2 = pm->Node(face.n[2]).r;
		vec3d r3 = pm->Node(face.n[3]).r;

		n1.r = (r0 + r1 + r2 + r3)*0.25;
	}

	for (int i=0; i<NE0; ++i, ++n)
	{
		FENode& n1 = pmnew->Node(n);

		FEElement& el = pm->Element(i);
		vec3d r(0,0,0);
		for (int j=0; j<8; ++j) r += pm->Node(el.m_node[j]).r;
		r *= 0.125;

		n1.r = r;
	}

	const int LUT[8][8] = {
	{  0,  8, 24, 11, 16, 20, 26, 23},
	{  8,  1,  9, 24, 20, 17, 21, 26 },
	{ 11, 24, 10,  3, 23, 26, 22, 19 },
	{ 24,  9,  2, 10, 26, 21, 18, 22 },
	{ 16, 20, 26, 23,  4, 12, 25, 15 },
	{ 20, 17, 21, 26, 12,  5, 13, 25 },
	{ 23, 26, 22, 19, 15, 25, 14,  7 },
	{ 26, 21, 18, 22, 25, 13,  6, 14 }};

	// create new elements
	int m[27];
	for (int i=0; i<NE0; ++i)
	{
		FEElement& el0 = pm->Element(i);
		vector<int>& eel = EET[i];
		vector<int>& fel = EFL[i];

		for (int j=0; j< 8; ++j) m[     j] = el0.m_node[j];
		for (int j=0; j<12; ++j) m[ 8 + j] = NN0 + eel[j];
		for (int j=0; j< 6; ++j) m[20 + j] = NN0 + NC0 + fel[j];
		m[26] = NN0 + NC0 + NF0 + i;

		for (int j=0; j<8; ++j)
		{
			FEElement& el = pmnew->Element(i*8 + j);
			el.m_gid = el0.m_gid;
			el.SetType(FE_HEX8);

			for (int k=0; k<8; ++k) el.m_node[k] = m[LUT[j][k]];
		}
	}

	pmnew->RebuildMesh();

	if (m_smoothSurface)
	{
		// The face table lists all faces (including all internal ones). We only need
		// to loop over exterior faces, so we need to figure out which exterior face
		// corresponds to the face in the face table
		int NF = pm->Faces();
		for (int i=0; i<NF; ++i) 
		{
			FEFace& face = pm->Face(i);
			face.m_ntag = -1;

			for (int j=0; j<NF0; ++j)
			{
				FEFace& fj = FT[j];
				if (fj == face)
				{
					face.m_ntag = j;
					break;
				}
			}

			assert(face.m_ntag != -1);
		}

		for (int i=0; i<pmnew->Nodes(); ++i) pmnew->Node(i).m_ntag = 0;
		vector<vec3d> p(NN0, vec3d(0,0,0));
		for (int i=0; i<NF; ++i)
		{
			FEFace& face = pm->Face(i);
			int ne = face.Edges();
			for (int j=0; j<ne; ++j)
			{
				pmnew->Node(face.n[j]).m_ntag++;
				
				int ej = FET[i][j];
				pair<int, int>& e = ET[ej];
				pmnew->Node(NN0 + ej).m_ntag++;

				p[e.first ] += pmnew->Node(NN0 + ej).r;
				p[e.second] += pmnew->Node(NN0 + ej).r;

				p[face.n[j]] += pmnew->Node(NN0 + NC0 + face.m_ntag).r;
			}
		}

		for (int i=0; i<NN0; ++i)
		{
			FENode& ni = pmnew->Node(i);
			double m = (double) ni.m_ntag;
			if (m != 0.0)
			{
				ni.r = (p[i]/m + ni.r*(m - 3))/m;
			}
		}

		for (int i = 0; i<NF; ++i)
		{
			FEFace& face = pm->Face(i);
			int ne = face.Edges();
			for (int j = 0; j<ne; ++j)
			{
				int ej = FET[i][j];
				pmnew->Node(NN0 + ej).r += pmnew->Node(NN0 + NC0 + face.m_ntag).r*0.5;
			}
		}

		for (int i=0; i<NC0; ++i) 
		{
			FENode& node = pmnew->Node(NN0 + i);
			if (node.m_ntag != 0)
			{
				pmnew->Node(NN0 + i).r *= 0.5;
			}
		}

		pmnew->UpdateNormals();
	}

	return pmnew;
}