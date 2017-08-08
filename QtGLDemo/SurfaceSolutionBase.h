#pragma once

#include "ConsoleMessageManager.h"
#include "OcTreeFieldSolution.h"

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/Status.hh>
using TriMesh = OpenMesh::TriMesh_ArrayKernelT<>;
using VertexHandle = TriMesh::VertexHandle;
using FaceHandle = TriMesh::FaceHandle;
using HalfEdgeHandle = TriMesh::HalfedgeHandle;
using Point = TriMesh::Point;
using OpenMesh::Vec3f;
using SpMatBuilder = std::vector<std::tuple<int, int, float>>;
using SpMatTriple = std::tuple<int, int, float>;

class SurfaceSolutionBase
{
public:
    SurfaceSolutionBase(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac);
    virtual void update() = 0;
    int tagged(VertexHandle vh);
    ~SurfaceSolutionBase();

    static void BuildOctahedron(TriMesh& mesh, float r, bool clear = false);
    static void BuildIcosahedron(TriMesh& mesh, float r, bool clear = false);
    static void BuildSphere(TriMesh &m, float r, int max_div = 2, bool clear = false);

protected:
    TriMesh &mesh;
    ConsoleMessageManager &msg;
    TextConfigLoader &algorithm_config;
    OcTreeField *dis_field;
    
    // Basic Information
    int num_verts;
    std::vector<VertexHandle> verts;
    std::map<VertexHandle, int> vert_index;
    std::vector<int> num_neighbors;
    std::vector<std::vector<int>> neighbors;

    // Tag Vertex, DEBUG USE
    std::map<VertexHandle, int> verts_tag;

    // Matrix Builder
    SpMatBuilder builderLaplacian;

    void UpdateBasicMeshInformation();
    void BuildLaplacianMatrixBuilder();
    void RefineSurface();

private:
    float _cotangent_for_angle_AOB(int, int, int);
};

