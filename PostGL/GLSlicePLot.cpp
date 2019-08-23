#include "stdafx.h"
#include "GLSlicePLot.h"
#include "GLWLib/GLWidgetManager.h"
#include "PostLib/constants.h"
#include "GLModel.h"
using namespace Post;

extern int LUT[256][15];
extern int ET_HEX[12][2];

CGLSlicePlot::CGLSlicePlot(CGLModel* po) : CGLPlot(po)
{
	static int n = 1;
	char szname[128] = {0};
	sprintf(szname, "Slice.%02d", n++);
	SetName(szname);

	m_norm = vec3f(1,0,0);

	AddIntParam(0, "Data field")->SetEnumNames("@data_scalar");
	AddIntParam(0, "Color map")->SetEnumNames("@color_map");
	AddBoolParam(true, "Allow clipping");
	AddBoolParam(true, "Show legend"   );
	AddIntParam(0, "Slices");
	AddDoubleParam(0, "Slice offset")->SetFloatRange(0.0, 1.0);
	AddIntParam(0, "Range")->SetEnumNames("dynamic\0user\0");
	AddDoubleParam(0, "Range max");
	AddDoubleParam(0, "Range min");
	AddDoubleParam(0, "X-normal" );
	AddDoubleParam(0, "Y-normal" );
	AddDoubleParam(0, "Z-normal" );

	m_nslices = 10;
	m_nfield = 0;
	m_offset = 0.5f;

	m_Col.SetDivisions(m_nslices);
	m_Col.SetSmooth(false);

	m_nrange = 0;
	m_fmin = 0.f;
	m_fmax = 0.f;

	m_pbar = new GLLegendBar(&m_Col, 0, 0, 120, 500);
	m_pbar->align(GLW_ALIGN_LEFT| GLW_ALIGN_VCENTER);
	m_pbar->copy_label(szname);
	CGLWidgetManager::GetInstance()->AddWidget(m_pbar);

	UpdateData(false);
}

CGLSlicePlot::~CGLSlicePlot()
{
	CGLWidgetManager::GetInstance()->RemoveWidget(m_pbar);
	delete m_pbar;	
}

int CGLSlicePlot::GetSlices() { return m_nslices; }
void CGLSlicePlot::SetSlices(int nslices) { m_nslices = nslices; m_Col.SetDivisions(nslices); }

void CGLSlicePlot::UpdateData(bool bsave)
{
	if (bsave)
	{
		m_nfield = GetIntValue(DATA_FIELD);
		m_Col.SetColorMap(GetIntValue(COLOR_MAP));
		AllowClipping(GetBoolValue(CLIP));
		// TODO: show legend
		m_nslices = GetIntValue(SLICES);
		m_offset = GetFloatValue(SLICE_OFFSET);
		m_nrange = GetIntValue(RANGE);
		m_fmax = GetFloatValue(RANGE_MAX);
		m_fmin = GetFloatValue(RANGE_MIN);
		m_norm.x = GetFloatValue(NORMAL_X);
		m_norm.y = GetFloatValue(NORMAL_Y);
		m_norm.z = GetFloatValue(NORMAL_Z);
	}
	else
	{
		SetIntValue(DATA_FIELD, m_nfield);
		SetIntValue(COLOR_MAP, m_Col.GetColorMap());
		SetBoolValue(CLIP, AllowClipping());
		SetIntValue(SLICES, m_nslices);
		SetFloatValue(SLICE_OFFSET, m_offset);
		SetIntValue(RANGE, m_nrange);
		SetFloatValue(RANGE_MAX, m_fmax);
		SetFloatValue(RANGE_MIN, m_fmin);
		SetFloatValue(NORMAL_X, m_norm.x);
		SetFloatValue(NORMAL_Y, m_norm.y);
		SetFloatValue(NORMAL_Z, m_norm.z);
	}
}

void CGLSlicePlot::SetSliceOffset(float f) 
{ 
	m_offset = f; 
	if (m_offset < 0.f) m_offset = 0.f;
	if (m_offset > 1.f) m_offset = 1.f;
}

void CGLSlicePlot::Render(CGLContext& rc)
{
	if (m_nfield == 0) return;
	m_box = GetModel()->GetFEModel()->GetBoundingBox();

	GLTexture1D& tex = m_Col.GetTexture();

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_1D);
	glDisable(GL_LIGHTING);
	tex.MakeCurrent();
	double fmin, fmax;
	vec3d n = m_norm;
	n.Normalize();
	m_box.Range(n, fmin, fmax);
	float Df = fabs(fmax - fmin);
	if (Df != 0.f)
	{
		fmin += 1e-3*Df;
		fmax -= 1e-3*Df;
	}
	glColor3ub(255, 255, 255);
	if (m_nslices == 1)
	{
		float ref = fmin + m_offset*(fmax - fmin);
		RenderSlice(ref);
	}
	else
	{
		float df = m_offset / m_nslices;
		fmin += df;
		fmax -= df;
		for (int i = 0; i < m_nslices; ++i)
		{
			float f = (float)i / (float)(m_nslices - 1);
			float ref = fmin + f*(fmax - fmin);
			RenderSlice(ref);
		}
	}
	glDisable(GL_TEXTURE_1D);
	glPopAttrib();
}

///////////////////////////////////////////////////////////////////////////////

void CGLSlicePlot::RenderSlice(float ref)
{
	int i, k, l;
	int ncase, *pf;
	float w;

	float ev[8];	// element nodal values
	float ex[8];	// element nodal distances
	vec3f er[8];

	const int HEX_NT[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	const int PEN_NT[8] = {0, 1, 2, 2, 3, 4, 5, 5};
	const int TET_NT[8] = {0, 1, 2, 2, 3, 3, 3, 3};
	const int* nt;

	// get the mesh
	CGLModel* mdl = GetModel();
	FEModel* ps = mdl->GetFEModel();
	FEMeshBase* pm = mdl->GetActiveMesh();

	vec3f norm = m_norm;
	norm.Normalize();

	vec2f rng = m_crng;
	if (rng.x == rng.y) rng.y++;
	float f;

	// loop over all elements
	for (i=0; i<pm->Elements(); ++i)
	{
		// render only if the element is visible and
		// its material is enabled
		FEElement& el = pm->Element(i);
		FEMaterial* pmat = ps->GetMaterial(el.m_MatID);
		if (pmat->benable && el.IsVisible() && el.IsSolid())
		{
			switch (el.Type())
			{
			case FE_HEX8   : nt = HEX_NT; break;
			case FE_HEX20  : nt = HEX_NT; break;
			case FE_HEX27  : nt = HEX_NT; break;
			case FE_PENTA6 : nt = PEN_NT; break;
            case FE_PENTA15: nt = PEN_NT; break;
            case FE_TET4   : nt = TET_NT; break;
            case FE_TET5   : nt = TET_NT; break;
			default:
				assert(false);
				return;
			}

			// get the nodal values
			for (k=0; k<8; ++k)
			{
				FENode& node = pm->Node(el.m_node[nt[k]]);

				f = m_val[el.m_node[nt[k]]];
				f = (f - rng.x) / (rng.y - rng.x);

				ev[k] = f;
				er[k] = node.m_rt;
				ex[k] = node.m_rt*norm;
			}

			// calculate the case of the element
			ncase = 0;
			for (k=0; k<8; ++k) 
				if (ex[k] <= ref) ncase |= (1 << k);

			// loop over faces
			pf = LUT[ncase];
			for (l=0; l<5; l++)
			{
				if (*pf == -1) break;

				// calculate nodal positions
				vec3f r[3];
				float tex[3];
				for (k=0; k<3; k++)
				{
					int n1 = ET_HEX[pf[k]][0];
					int n2 = ET_HEX[pf[k]][1];

					if (ex[n2] != ex[n1])
						w = (ref - ex[n1]) / (ex[n2] - ex[n1]);
					else w = 0.5;

					r[k] = er[n1]*(1-w) + er[n2]*w;
					tex[k] = ev[n1]*(1-w) + ev[n2]*w;
				}

				glNormal3f(0,0,-1);

				// render the face
				glBegin(GL_TRIANGLES);
				{
					glTexCoord1f(tex[0]); glVertex3f(r[0].x, r[0].y, r[0].z);
					glTexCoord1f(tex[1]); glVertex3f(r[1].x, r[1].y, r[1].z);
					glTexCoord1f(tex[2]); glVertex3f(r[2].x, r[2].y, r[2].z);
				}
				glEnd();

				pf+=3;
			}
		}
	}	
}

//-----------------------------------------------------------------------------
void CGLSlicePlot::SetEvalField(int n) 
{ 
	m_nfield = n; 
	Update(GetModel()->currentTimeIndex(), 0.0, false);
}

//-----------------------------------------------------------------------------
void CGLSlicePlot::Update(int ntime, float dt, bool breset)
{
	CGLModel* mdl = GetModel();

	FEMeshBase* pm = mdl->GetActiveMesh();
	FEModel* pfem = mdl->GetFEModel();

	int NN = pm->Nodes();
	int NS = pfem->GetStates();

	if (breset) { m_map.Clear(); m_rng.clear(); m_val.clear(); }

	if (m_map.States() != pfem->GetStates())
	{
		m_map.Create(NS, NN, 0.f, -1);
		m_rng.resize(NS);
		m_val.resize(NN);
	}
	if (m_nfield == 0) return;

	// see if we need to update this state
	if (breset ||(m_map.GetTag(ntime) != m_nfield))
	{
		m_map.SetTag(ntime, m_nfield);
		vector<float>& val = m_map.State(ntime);

		NODEDATA nd;
		for (int i=0; i<NN; ++i)
		{
			pfem->EvaluateNode(i, ntime, m_nfield, nd);
			val[i] = nd.m_val;
		}

		// evaluate the range
		float fmin, fmax;
		if (m_nrange == 0)
		{
			fmin = fmax = val[0];
			for (int i=0;i<NN; ++i)
			{
				if (val[i] < fmin) fmin = val[i];
				if (val[i] > fmax) fmax = val[i];
			}
			if (fmin == fmax) fmax++;

			m_fmin = fmin;
			m_fmax = fmax;
		}
		else
		{
			fmin = m_fmin;
			fmax = m_fmax;
			if (fmin == fmax) fmax++;
		}

		m_rng[ntime] = vec2f(fmin, fmax);
	}

	// copy current nodal values
	m_val = m_map.State(ntime);

	// update range
	vec2f r = m_rng[ntime];
//	m_Col.SetRange(r.x, r.y, false);

	m_crng = m_rng[ntime];
}