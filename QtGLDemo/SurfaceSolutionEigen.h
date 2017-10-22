#pragma once
#include "SurfaceSolutionBase.h"

#include <Eigen/Sparse>
#include <Eigen/Dense>
using SpMat = Eigen::SparseMatrix<float>;
using Mat = Eigen::MatrixXf;
using Vec = Eigen::VectorXf;
using T = Eigen::Triplet<float>;

class SurfaceSolutionEigen :
    public SurfaceSolutionBase
{
public:
    SurfaceSolutionEigen(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac, std::map<VertexHandle, float> &);
    virtual void update() override;
    ~SurfaceSolutionEigen();

protected:
    SpMat mLaplacian;
  
    void BuildLaplacianMatrix();
};

