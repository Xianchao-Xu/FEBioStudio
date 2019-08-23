#include"FEMeshBase.h"
#include <GeomLib/GObject.h>

//-----------------------------------------------------------------------------
FEMeshBase::FEMeshBase()
{
}

//-----------------------------------------------------------------------------
FEMeshBase::~FEMeshBase()
{
}

//-----------------------------------------------------------------------------
// get the local node positions of a face
void FEMeshBase::FaceNodeLocalPositions(const FEFace& f, vec3d* r) const
{
	int nf = f.Nodes();
	for (int i = 0; i<nf; ++i) r[i] = m_Node[f.n[i]].r;
}

//-----------------------------------------------------------------------------
// Tag all faces
void FEMeshBase::TagAllFaces(int ntag)
{
	const int NF = Faces();
	for (int i = 0; i<NF; ++i) Face(i).m_ntag = ntag;
}

//-----------------------------------------------------------------------------
bool FEMeshBase::IsEdge(int n0, int n1)
{
	int NE = Edges();
	for (int i = 0; i<NE; ++i)
	{
		FEEdge& e = Edge(i);
		if ((e.n[0] == n0) && (e.n[1] == n1)) return true;
		if ((e.n[0] == n1) && (e.n[1] == n0)) return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
FEEdge* FEMeshBase::FindEdge(int n0, int n1)
{
	int NE = Edges();
	for (int i = 0; i<NE; ++i)
	{
		FEEdge& e = Edge(i);
		if ((e.n[0] == n0) && (e.n[1] == n1)) return &e;
		if ((e.n[0] == n1) && (e.n[1] == n0)) return &e;
	}
	return 0;
}

//-----------------------------------------------------------------------------
bool FEMeshBase::IsCreaseEdge(int n0, int n1)
{
	int NE = Edges();
	for (int i = 0; i<NE; ++i)
	{
		FEEdge& e = Edge(i);
		if ((e.n[0] == n0) && (e.n[1] == n1)) return (e.m_gid != -1);
		if ((e.n[0] == n1) && (e.n[1] == n0)) return (e.m_gid != -1);
	}
	return false;
}


//-----------------------------------------------------------------------------
// Build the node-face table
void FEMeshBase::BuildNodeFaceTable(vector< vector<int> >& NFT)
{
	int NN = Nodes();
	int NF = Faces();

	// zero nodal valences
	for (int i = 0; i<NN; ++i) m_Node[i].m_ntag = 0;

	// calculate nodal valences
	for (int i = 0; i<NF; ++i)
	{
		FEFace& f = m_Face[i];
		int n = f.Nodes();
		for (int j = 0; j<n; ++j) m_Node[f.n[j]].m_ntag++;
	}

	// allocate node-face-table
	NFT.resize(NN);
	for (int i = 0; i<NN; ++i) NFT[i].reserve(m_Node[i].m_ntag);

	// fill node element table
	for (int i = 0; i<NF; ++i)
	{
		FEFace& f = m_Face[i];
		int n = f.Nodes();
		for (int j = 0; j<n; ++j) NFT[f.n[j]].push_back(i);
	}
}

//-----------------------------------------------------------------------------
// Build the node-edge table
void FEMeshBase::BuildNodeEdgeTable(vector< vector<int> >& NET)
{
	int NN = Nodes();
	int NE = Edges();
	for (int i = 0; i<NN; ++i) m_Node[i].m_ntag = 0;
	for (int i = 0; i<NE; ++i)
	{
		FEEdge& e = Edge(i);
		if (e.n[0] != -1) m_Node[e.n[0]].m_ntag++;
		if (e.n[1] != -1) m_Node[e.n[1]].m_ntag++;
		if (e.n[2] != -1) m_Node[e.n[2]].m_ntag++;
		if (e.n[3] != -1) m_Node[e.n[3]].m_ntag++;
	}

	NET.resize(NN);
	for (int i = 0; i<NN; ++i) NET[i].reserve(m_Node[i].m_ntag);

	for (int i = 0; i<NE; ++i)
	{
		FEEdge& e = Edge(i);
		if (e.n[0] != -1) NET[e.n[0]].push_back(i);
		if (e.n[1] != -1) NET[e.n[1]].push_back(i);
		if (e.n[2] != -1) NET[e.n[2]].push_back(i);
		if (e.n[3] != -1) NET[e.n[3]].push_back(i);
	}
}

//-----------------------------------------------------------------------------
// Updates the bounding box (in local coordinates)
void FEMeshBase::UpdateBox()
{
	FENode* pn = NodePtr();
	if (pn == 0)
	{
		m_box.x0 = m_box.y0 = m_box.z0 = 0;
		m_box.x1 = m_box.y1 = m_box.z1 = 0;
		return;
	}

	m_box.x0 = m_box.x1 = pn->r.x;
	m_box.y0 = m_box.y1 = pn->r.y;
	m_box.z0 = m_box.z1 = pn->r.z;
	for (int i = 0; i<Nodes(); i++, pn++)
	{
		vec3d& r = pn->r;
		if (r.x < m_box.x0) m_box.x0 = r.x;
		if (r.y < m_box.y0) m_box.y0 = r.y;
		if (r.z < m_box.z0) m_box.z0 = r.z;
		if (r.x > m_box.x1) m_box.x1 = r.x;
		if (r.y > m_box.y1) m_box.y1 = r.y;
		if (r.z > m_box.z1) m_box.z1 = r.z;
	}
}

//-----------------------------------------------------------------------------
// Remove faces with tag ntag
void FEMeshBase::RemoveFaces(int ntag)
{
	int n = 0;
	for (int i = 0; i<Faces(); ++i)
	{
		FEFace& f1 = Face(i);
		FEFace& f2 = Face(n);

		if (f1.m_ntag != ntag)
		{
			if (i != n) f2 = f1;
			n++;
		}
	}
	m_Face.resize(n);
}

//-----------------------------------------------------------------------------
// Remove edges with tag ntag
void FEMeshBase::RemoveEdges(int ntag)
{
	int n = 0;
	for (int i = 0; i<Edges(); ++i)
	{
		FEEdge& e1 = Edge(i);
		FEEdge& e2 = Edge(n);

		if (e1.m_ntag != ntag)
		{
			if (i != n) e2 = e1;
			n++;
		}
	}
	m_Edge.resize(n);
}

//-----------------------------------------------------------------------------
// This function assignes group ID's to the mesh' faces based on a smoothing
// angle.
//
void FEMeshBase::AutoSmooth(double w)
{
	int NF = Faces();

	// smoothing threshold
	double eps = (double)cos(w*PI / 180);

	// clear face group ID's
	for (int i = 0; i<NF; ++i)
	{
		FEFace* pf = FacePtr(i);
		pf->m_sid = -1;
	}

	// calculate face normals
	for (int i = 0; i<NF; ++i)
	{
		FEFace* pf = FacePtr(i);

		// calculate the face normals
		vec3d& r0 = Node(pf->n[0]).r;
		vec3d& r1 = Node(pf->n[1]).r;
		vec3d& r2 = Node(pf->n[2]).r;

		pf->m_fn = (r1 - r0) ^ (r2 - r0);
		pf->m_fn.Normalize();
	}


	// stack for tracking unprocessed faces
	vector<FEFace*> stack(NF);
	int ns = 0;

	// process all faces
	int nsg = 0;
	for (int i = 0; i<NF; ++i)
	{
		FEFace* pf = FacePtr(i);
		if (pf->m_sid == -1)
		{
			stack[ns++] = pf;
			while (ns > 0)
			{
				// pop a face
				pf = stack[--ns];

				// mark as processed
				pf->m_sid = nsg;

				// loop over neighbors
				int n = pf->Edges();
				for (int j = 0; j<n; ++j)
				{
					FEFace* pf2 = FacePtr(pf->m_nbr[j]);

					// push unprocessed neighbour
					if (pf2 && (pf2->m_sid == -1) && (pf->m_fn*pf2->m_fn >= eps))
					{
						pf2->m_sid = -2;
						stack[ns++] = pf2;
					}
				}
			}
			++nsg;
		}
	}

	// update the normals
	UpdateNormals();
}

//-----------------------------------------------------------------------------
// Calculate normals of the mesh' faces based on smoothing groups
//
void FEMeshBase::UpdateNormals()
{
	int NN = Nodes();
	int NF = Faces();

	// calculate face normals
	for (int i = 0; i<NF; ++i)
	{
		FEFace* pf = FacePtr(i);

		// calculate the face normals
		vec3d& r0 = Node(pf->n[0]).r;
		vec3d& r1 = Node(pf->n[1]).r;
		vec3d& r2 = Node(pf->n[2]).r;

		pf->m_fn = (r1 - r0) ^ (r2 - r0);
		pf->m_fn.Normalize();

		int nf = pf->Nodes();
		for (int j = 0; j<nf; ++j) pf->m_nn[j] = pf->m_fn;
	}

	//calculate the node normals
	vector<vec3d> norm; norm.resize(NN);
	for (int i = 0; i<NN; ++i) norm[i] = vec3d(0, 0, 0);

	// reset smoothing id's
	FEFace* pf = FacePtr();
	for (int i = 0; i<NF; ++i, ++pf) pf->m_ntag = -1;

	// this array keeps track of all faces in a smoothing group
	vector<FEFace*> F(NF);
	int FC = 0;

	// this array is used as a stack when processing neighbors
	vector<FEFace*> stack(NF);
	int ns = 0;

	// loop over all faces
	int nsg = 0;
	for (int i = 0; i<NF; ++i)
	{
		FEFace* pf = FacePtr(i);
		if (pf->m_ntag == -1)
		{
			// find all connected faces
			stack[ns++] = pf;
			while (ns > 0)
			{
				// pop a face
				pf = stack[--ns];

				// mark as processed
				pf->m_ntag = nsg;
				F[FC++] = pf;

				// add face normal to node normal
				int n = pf->Nodes();
				for (int j = 0; j<n; ++j) norm[pf->n[j]] += pf->m_fn;

				// process neighbors
				n = pf->Edges();
				for (int j = 0; j<n; ++j)
				{
					FEFace* pf2 = FacePtr(pf->m_nbr[j]);
					// push unprocessed neighbor
					if (pf2 && (pf2->m_ntag == -1) && (pf->m_sid == pf2->m_sid))
					{
						pf2->m_ntag = -2;
						stack[ns++] = pf2;
					}
				}
			}

			// assign node normals
			for (int j = 0; j<FC; ++j)
			{
				pf = F[j];
				assert(pf->m_ntag == nsg);
				int nf = pf->Nodes();
				for (int k = 0; k<nf; ++k) pf->m_nn[k] = norm[pf->n[k]];
			}

			// clear normals
			for (int j = 0; j<FC; ++j)
			{
				pf = F[j];
				int nf = pf->Nodes();
				for (int k = 0; k<nf; ++k) norm[pf->n[k]] = vec3d(0, 0, 0);
			}
			++nsg;
			FC = 0;
		}
	}

	// normalize face normals
	pf = FacePtr();
	for (int i = 0; i<NF; ++i, ++pf)
	{
		int n = pf->Nodes();
		for (int j = 0; j<n; ++j) pf->m_nn[j].Normalize();
	}
}

//-----------------------------------------------------------------------------
// assign smoothing IDs based on surface partition
void FEMeshBase::SmoothByPartition()
{
	// assign group IDs to smoothing IDs
	for (int i=0; i<Faces(); ++i)
	{
		FEFace& face = Face(i);
		face.m_sid = face.m_gid;
	}	

	// update the normals
	UpdateNormals();
}

//-----------------------------------------------------------------------------
void FEMeshBase::UpdateMeshData()
{
	UpdateNormals();
	UpdateBox();
}