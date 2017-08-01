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

#include <Eigen/Sparse>
#include <Eigen/Dense>
using SpMat = Eigen::SparseMatrix<float>;
using Mat = Eigen::MatrixXf;
using Vec = Eigen::VectorXf;
using T = Eigen::Triplet<float>;

class SurfaceSolution
{
public:
    SurfaceSolution(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac);
    void update();
    ~SurfaceSolution();

    static void BuildOctahedron(TriMesh& mesh, float r, bool clear = false);
    static void BuildIcosahedron(TriMesh& mesh, float r, bool clear = false);
    static void BuildSphere(TriMesh &m, float r, int max_div = 2, bool clear = false);

protected:
    TriMesh &mesh;
    ConsoleMessageManager &msg;
    TextConfigLoader &algorithm_config;
    OcTreeField *dis_field;

    int num_verts;
    std::vector<VertexHandle> verts;
    std::map<VertexHandle, int> vert_index;
    std::vector<int> num_neighbors;
    std::vector<std::vector<int>> neighbors;
    SpMat mLaplacian;

    void UpdateBasicMeshInformation();
    void BuildLaplacianMatrix();
};

