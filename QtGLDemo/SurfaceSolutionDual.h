#pragma once
#include "SurfaceSolutionMatlab.h"
#include "kdTreeSolution.h"
#include <engine.h>


class SurfaceSolutionNeo :
    public SurfaceSolutionMatlab
{
public:
    SurfaceSolutionNeo(TriMesh &si, TriMesh &so, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac);
    ~SurfaceSolutionNeo();

    void update() override;
    virtual void update_inner();
    virtual void update_outer();

protected:
    // Mesh
    TriMesh &mesh_inner;
    TriMesh &mesh_outer;
    MeshRefineSolution refine_inner;
    MeshRefineSolution refine_outer;
    SpMatBuilder builderLaplacianInner;
    SpMatBuilder builderLaplacianOuter;
    bool changed_inner {true};
    bool changed_outer {true};

    // Basic Information
    int num_verts_inner;
    std::vector<VertexHandle> verts_inner;
    std::map<VertexHandle, int> vert_index_inner;
    std::vector<int> num_neighbors_inner;
    std::vector<std::vector<int>> neighbors_inner;
    //bool surface_changed_inner;
    
    int num_verts_outer;
    std::vector<VertexHandle> verts_outer;
    std::map<VertexHandle, int> vert_index_outer;
    std::vector<int> num_neighbors_outer;
    std::vector<std::vector<int>> neighbors_outer;
    //bool surface_changed_outer;

    void UpdateBasicMeshInformationInner();
    void UpdateBasicMeshInformationOuter();
    void BuildLaplacianMatrixBuilderInner();
    void BuildLaplacianMatrixBuilderOuter();

    // kdTree
    kdt::kdTree *kdtree_inner{ nullptr };
    kdt::kdTree *kdtree_outer{ nullptr };
    std::vector<kdt::kdPoint> pts_inner;
    std::vector<kdt::kdPoint> pts_outer;

    void BuildKdtreeInner();
    void BuildKdtreeOuter();
};
