#include "stdafx.h"
#include "SurfaceSolutionEigen.h"

#define MAX_MAT_OUTPUT_SIZE 99

using OpenMesh::Vec3f;

inline std::string _mat_to_string(SpMat &m, const std::string &prompt = "")
{
    std::ostringstream oss;
    if (!prompt.empty())
        oss << "\n" << prompt << ":";

    if (m.cols() <= MAX_MAT_OUTPUT_SIZE && m.rows() <= MAX_MAT_OUTPUT_SIZE)
        oss << std::endl << m;
    else
        oss << "{ SpMat\t" << m.rows() << " * " << m.cols() << " }";
    return oss.str();
}

inline std::string _mat_to_string(Mat &m, const std::string &prompt = "")
{

    std::ostringstream oss;
    if (!prompt.empty())
        oss << "\n" << prompt << ":";

    if (m.cols() <= MAX_MAT_OUTPUT_SIZE && m.rows() <= MAX_MAT_OUTPUT_SIZE)
        oss << std::endl << m;
    else
        oss << "{ Mat\t" << m.rows() << " * " << m.cols() << " }";
    return oss.str();
}

inline std::string _mat_to_string(Vec &m, const std::string &prompt = "")
{

    std::ostringstream oss;
    if (!prompt.empty())
        oss << "\n" << prompt << ":";

    if (m.cols() <= MAX_MAT_OUTPUT_SIZE && m.rows() <= MAX_MAT_OUTPUT_SIZE)
        oss << std::endl << m;
    else
        oss << "{ Vec\t" << m.rows() << " * " << m.cols() << " }";
    return oss.str();
}

inline void _fill_sparse(SpMat &src, SpMat &dest, int rs, int cs, int r, int c)
{
    for (int c = 0; c<src.cols(); ++c)
    {
        for (SpMat::InnerIterator itin(src, c); itin; ++itin)
            dest.insertBack(itin.row(), c) = itin.value();
    }
}

void SurfaceSolutionEigen::update()
{
    /**
    *            LHS       V' ==               RHS
    *
    * [    w_L    * Lap]           [            O            ]
    * [ (w_P+w_F) *  I ] * V' ==   [ (w_P+w_F) * V + w_F * F ]
    */

    // Extract Constants;
    float w_L = algorithm_config.get_float("w_L", 1.0f);
    float w_P = algorithm_config.get_float("w_P", 1.0f);
    float w_F = algorithm_config.get_float("w_F", 1.0f);

    // Initialize
    SpMat LHS{ 2 * num_verts, num_verts };
    Mat V_prime = Mat::Zero(num_verts, 3);
    Mat RHS = Mat::Zero(2 * num_verts, 3);

    // Build LHS
    LHS.reserve(mLaplacian.nonZeros() + num_verts);
    for (int k = 0; k < mLaplacian.outerSize(); ++k)
    {
        LHS.insert(k + num_verts, k) = w_P + w_F;
        for (SpMat::InnerIterator it(mLaplacian, k); it; ++it)
        {
            LHS.insert(it.row(), it.col()) = it.value() * w_L;
        }
    }
    msg.log(_mat_to_string(LHS, "LHS"), MATRIX_MSG);
    msg.log(_mat_to_string(mLaplacian, "Laplacian"), MATRIX_MSG);

    // Build RHS
    for (int r = 0; r < num_verts; r++)
    {
        auto pos = mesh.point(verts[r]);
        auto dir = dis_field->get_dir(pos);
        for (int c = 0; c < 3; c++)
        {
            RHS(r + num_verts, c) = pos[c] * (w_P + w_F) + dir[c] * w_F;
        }
    }
    msg.log(_mat_to_string(RHS, "RHS"), MATRIX_MSG);

    // LU solve 
    SpMat temp = LHS.transpose() * LHS;
    Mat temp_b = LHS.transpose() * RHS;
    Eigen::SparseLU<SpMat, Eigen::COLAMDOrdering<int> > solver;
    // compute the ordering permutation vector from the structural pattern
    solver.analyzePattern(temp);
    // compute the numerical factorization
    solver.factorize(temp);
    Mat result = solver.solve(temp_b);
    msg.log(QString("matrix size %0 * %1").arg(temp.rows()).arg(temp.cols()), TRIVIAL_MSG);


    msg.log(_mat_to_string(temp, "LHS.transpose() * LHS"), MATRIX_MSG);
    msg.log(_mat_to_string(temp_b, "LHS.transpose() * RHS"), MATRIX_MSG);
    msg.log(_mat_to_string(result, "result"), MATRIX_MSG);

    for (int i = 0; i < num_verts; i++)
    {
        mesh.point(verts[i]) = Vec3f{ result(i, 0), result(i, 1), result(i, 2) };
    }
}

SurfaceSolutionEigen::SurfaceSolutionEigen(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac, std::map<VertexHandle, float> &mt)
    : SurfaceSolutionBase(s, d, m, ac, mt)
{
    BuildLaplacianMatrix();
}

SurfaceSolutionEigen::~SurfaceSolutionEigen()
{
}

void SurfaceSolutionEigen::BuildLaplacianMatrix()
{
    mLaplacian = SpMat(num_verts, num_verts);
    mLaplacian.reserve(builderLaplacian.size());
    for (int i = 0; i < num_verts; i++)
    {
        float weight_sum = 0.0f;
        for (int neib_vert_idx : neighbors[i])
        {
            float weight = 1.0f; // TODO
            mLaplacian.insert(i, neib_vert_idx) = -weight;
            weight_sum += weight;
        }
        mLaplacian.insert(i, i) = weight_sum;
    }
}
