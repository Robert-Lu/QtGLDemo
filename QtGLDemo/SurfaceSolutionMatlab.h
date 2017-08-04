#pragma once
#include "SurfaceSolutionBase.h"
#include <engine.h>


class SurfaceSolutionMatlab :
    public SurfaceSolutionBase
{
public:
    SurfaceSolutionMatlab(TriMesh &s, OcTreeField *d, ConsoleMessageManager &m, TextConfigLoader &ac);
    ~SurfaceSolutionMatlab();

    virtual void update() override;

protected:
    static void InputVariableToEngine(Engine* e, const std::string &name, float val);
    static void InputSparseMatrixToEngine(Engine* e, const std::string &name, 
        const SpMatBuilder &builder);
    static void InputDenseMatrixToEngine(Engine* e, const std::string &name,
        const std::vector<std::vector<float>> &data);
    static std::vector<std::vector<float>> RetieveDenseMatricFromEngine(
        Engine *e, const std::string &name, int n_row, int n_col);

    Engine *engine;
};

