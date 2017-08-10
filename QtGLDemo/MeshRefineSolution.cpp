#include "stdafx.h"
#include "MeshRefineSolution.h"


MeshRefineSolution::MeshRefineSolution(TriMesh &s, ConsoleMessageManager &m, TextConfigLoader &ac)
    : mesh(s), msg(m), algorithm_config(ac)
{
}


MeshRefineSolution::~MeshRefineSolution()
{
}
