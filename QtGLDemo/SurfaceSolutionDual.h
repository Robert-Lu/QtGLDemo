#pragma once
#include "SurfaceSolutionMatlab.h"
#include <engine.h>


class SurfaceSolutionNeo :
    public SurfaceSolutionMatlab
{
public:
    SurfaceSolutionNeo(TriMesh &si, TriMesh &so, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac);
    ~SurfaceSolutionNeo();

    virtual void update() override;

protected:
    TriMesh &mesh_inner;
    TriMesh &mesh_outer;
    MeshRefineSolution refine_inner;
    MeshRefineSolution refine_outer;

    // Basic Information
    int num_verts_inner;
    std::vector<VertexHandle> verts_inner;
    std::map<VertexHandle, int> vert_index_inner;
    std::vector<int> num_neighbors_inner;
    std::vector<std::vector<int>> neighbors_inner;
    bool surface_changed_inner;

    int num_verts_outer;
    std::vector<VertexHandle> verts_outer;
    std::map<VertexHandle, int> vert_index_outer;
    std::vector<int> num_neighbors_outer;
    std::vector<std::vector<int>> neighbors_outer;
    bool surface_changed_outer;

    void UpdateBasicMeshInformationDual();
    void BuildLaplacianMatrixBuilder(SpMatBuilder &, TriMesh &);
};

