#pragma once

#include "ConsoleMessageManager.h"
#include "OcTreeFieldSolution.h"

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;
typedef OpenMesh::Vec3f     Vec3f;

#include <Eigen/Sparse>
#include <Eigen/Dense>
typedef Eigen::SparseMatrix<float>  SpMat;
typedef Eigen::MatrixXf             Mat;
typedef Eigen::VectorXf             Vec;
typedef Eigen::Triplet<float>       T;

class SurfaceSolution
{
public:
    SurfaceSolution(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m);
    void update();
    ~SurfaceSolution();

    static void BuildSphere(TriMesh &m, Vec3f ori, float r);

private:
    TriMesh &s_mesh;
    ConsoleMessageManager &msg;
    OcTreeField *dis_field;
};

