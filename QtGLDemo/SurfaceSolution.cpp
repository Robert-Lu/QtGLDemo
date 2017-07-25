#include "stdafx.h"
#include "SurfaceSolution.h"

using OpenMesh::Vec3f;

SurfaceSolution::SurfaceSolution(TriMesh& s, OcTreeField *d, ConsoleMessageManager &m)
    : s_mesh(s), dis_field(d), msg(m)
{

}

void SurfaceSolution::update()
{
}

SurfaceSolution::~SurfaceSolution()
{
}

void SurfaceSolution::BuildSphere(TriMesh& m, OpenMesh::Vec3f ori, float r)
{
    m.clear();

}
