#pragma once

#include "ConsoleMessageManager.h"
#include "OcTreeFieldSolution.h"
#include "MeshRefineSolution.h"

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
    SurfaceSolutionBase(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac, std::map<VertexHandle, float> &);
    virtual void update() = 0;
    int tagged(VertexHandle vh);
    ~SurfaceSolutionBase();

    static void BuildOctahedron(TriMesh& mesh, float r, bool clear = false);
    static void BuildIcosahedron(TriMesh& mesh, float r, bool clear = false);
    static void BuildSphere(TriMesh &m, float r, int max_div = 2, bool clear = false, float x = 0, float y = 0, float z = 0);

protected:
    ConsoleMessageManager &msg;
    TextConfigLoader &algorithm_config;
    OcTreeField *dis_field;
//private:
    TriMesh &mesh;
    MeshRefineSolution refine;
    
    // Basic Information
    int num_verts;
    std::vector<VertexHandle> verts;
    std::map<VertexHandle, int> vert_index;
    std::vector<int> num_neighbors;
    std::vector<std::vector<int>> neighbors;
    bool surface_changed;

    // Tension 
    std::map<VertexHandle, float> &map_tension_inner;

    // Tag Vertex, DEBUG USE
    std::map<VertexHandle, int> verts_tag;

    // Matrix Builder
    SpMatBuilder builderLaplacian;

    void UpdateBasicMeshInformation();
    void BuildLaplacianMatrixBuilder();
    void RefineSurface();
    float _cotangent_for_angle_AOB(int, int, int);
};

