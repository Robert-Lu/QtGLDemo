#pragma once
class MeshRefineSolution
{
public:
    MeshRefineSolution(TriMesh &s, ConsoleMessageManager &m, TextConfigLoader &ac);
    ~MeshRefineSolution();

protected:
    TriMesh &mesh;
    ConsoleMessageManager &msg;
    TextConfigLoader &algorithm_config;
};

