#include "stdafx.h"
#include "SurfaceSolutionMatlab.h"
#include <engine.h>


void SurfaceSolutionMatlab::InputVariableToEngine(Engine* ep, const std::string &name, float val)
{
    mxArray *scalar = mxCreateDoubleMatrix(1, 1, mxREAL);
    mxGetPr(scalar)[0] = val;
    // name.data(): return const char *.
    engPutVariable(ep, name.data(), scalar);
    mxDestroyArray(scalar);
}


void SurfaceSolutionMatlab::InputSparseMatrixToEngine(Engine* ep, const std::string &name, const SpMatBuilder &builder)
{
    int builder_length = builder.size();
    mxArray
        *_Row = mxCreateDoubleMatrix(builder_length, 1, mxREAL),
        *_Col = mxCreateDoubleMatrix(builder_length, 1, mxREAL),
        *_Val = mxCreateDoubleMatrix(builder_length, 1, mxREAL);

    for (int i = 0; i < builder_length; i++)
    {
        auto t = builder[i];
        mxGetPr(_Row)[i] = std::get<0>(t) + 1;     // MATLAB index starts
        mxGetPr(_Col)[i] = std::get<1>(t) + 1;     // from 1.
        mxGetPr(_Val)[i] = std::get<2>(t);
    }
    engPutVariable(ep, "tmp_Row", _Row);
    engPutVariable(ep, "tmp_Col", _Col);
    engPutVariable(ep, "tmp_Val", _Val);
    auto expression = name + " = sparse(tmp_Row, tmp_Col, tmp_Val);";
    engEvalString(ep, expression.data());
    mxDestroyArray(_Row);
    mxDestroyArray(_Col);
    mxDestroyArray(_Val);
}


void SurfaceSolutionMatlab::InputDenseMatrixToEngine(Engine* ep, const std::string &name,
    const std::vector<std::vector<float>> &data)
{
    int num_row = data.size();
    if (num_row == 0)
        return;
    int num_col = data[0].size();
    if (num_col == 0)
        return;

    mxArray *matrix = mxCreateDoubleMatrix(num_row, num_col, mxREAL);
    for (int i = 0; i < num_row; i++)
    {
        for (int j = 0; j < num_col; j++)
        {
            mxGetPr(matrix)[i + j * num_row] = data[i][j];
        }
    }
    engPutVariable(ep, name.data(), matrix);
    mxDestroyArray(matrix);
}


std::vector<std::vector<float>> SurfaceSolutionMatlab::RetieveDenseMatricFromEngine(Engine* ep, const std::string& name,
    int n_row, int n_col)
{
    mxArray *return_arr = engGetVariable(ep, name.data());
    std::vector<std::vector<float>> return_data(n_row, std::vector<float>(n_col));
    for (int r = 0; r < n_row; r++)
    {
        for (int c = 0; c < n_col; c++)
        {
            return_data[r][c] = mxGetPr(return_arr)[r + n_row * c];
        }
    }

    return return_data;
}


SurfaceSolutionMatlab::SurfaceSolutionMatlab(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac)
    : SurfaceSolutionBase(s, d, m, ac)
{
    // start MATLAB engine
    if ((engine = engOpen("")) == nullptr)
    {
        msg.log("MATLAB engine fail to start.", ERROR_MSG);
        return;
    }

    InputSparseMatrixToEngine(engine, "Lap", builderLaplacian);
}


SurfaceSolutionMatlab::~SurfaceSolutionMatlab()
{
}


void SurfaceSolutionMatlab::update()
{
    // Extract Constants.
    float w_L = algorithm_config.get_float("w_L", 1.0f);
    float w_P = algorithm_config.get_float("w_P", 1.0f);
    float w_F = algorithm_config.get_float("w_F", 1.0f);

    InputVariableToEngine(engine, "w_L", w_L);
    InputVariableToEngine(engine, "w_P", w_P);
    InputVariableToEngine(engine, "w_F", w_F);
    InputVariableToEngine(engine, "num_verts", num_verts);

    // Extract Position and \grad Potential.
    std::vector<std::vector<float>> data_position(num_verts, std::vector<float>(3, 0.0f));
    std::vector<std::vector<float>> data_grad_potential(num_verts, std::vector<float>(3, 0.0f));
    for (int r = 0; r < num_verts; r++)
    {
        auto pos = mesh.point(verts[r]);
        auto dir = dis_field->get_dir(pos);
        for (int c = 0; c < 3; c++)
        {
            data_position[r][c] = pos[c];
            data_grad_potential[r][c] = dir[c];
        }
    }

    InputDenseMatrixToEngine(engine, "V", data_position);
    InputDenseMatrixToEngine(engine, "F", data_grad_potential);

    // Main.
    /**
    *            LHS       V' ==               RHS
    *
    * [    w_L    * Lap]           [            O            ]
    * [ (w_P+w_F) *  I ] * V' ==   [ (w_P+w_F) * V + w_F * F ]
    */
    engEvalString(engine,
        "LHS = [w_L * Lap; (w_P+w_F) * eye(num_verts)];             \n"
        "RHS = [zeros(num_verts, 3); (w_P+w_F) * V + w_F * F];      \n"
        "V_prime = LHS \\ RHS;                                      \n"
        );

    // Retrieve Data
    auto data = RetieveDenseMatricFromEngine(engine, "V_prime", num_verts, 3);
    for (int i = 0; i < num_verts; i++)
    {
        mesh.point(verts[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    }

    RefineSurface();
}
