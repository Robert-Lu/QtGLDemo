#include "stdafx.h"
#include "QTiming.h"
#include "SurfaceSolutionDual.h"

#define TIC QTiming::Tic();
#define TOC(s) {\
    QString prompt = QString(s " in %0 ms.").arg(QTiming::Toc());\
    msg.log(prompt, INFO_MSG);\
}

#define MOD(x, m) (((x) + (m)) % (m))
#define NORM3(p, q) (sqrtf(powf((p)[0] - (q)[0], 2) + powf((p)[1] - (q)[1], 2) + powf((p)[2] - (q)[2], 2)))

inline float _pinch(float min, float val, float max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

inline float _area(TriMesh &mesh, FaceHandle fh)
{
    float a = 0.0f;
    auto fv_iter = mesh.fv_begin(fh);
    Vec3f pos[3];
    int i = 0;
    for (; fv_iter != mesh.fv_end(fh); fv_iter++)
    {
        auto v = *fv_iter;
        pos[i++] = mesh.point(fv_iter);
    }
    Vec3f AB = pos[1] - pos[0];
    Vec3f AC = pos[2] - pos[0];
    return OpenMesh::cross(AB, AC).norm() / 2;
}

//inline Vec3f kdpoint_to_vec3f(kdt::kdPoint p)
//{
//    return{ p[0], p[1], p[2] };
//}

SurfaceSolutionNeo::SurfaceSolutionNeo(TriMesh& si, TriMesh& so, OcTreeField* d, ConsoleMessageManager& m, TextConfigLoader& ac) :
    SurfaceSolutionMatlab(si, d, m, ac),
    mesh_inner(si), mesh_outer(so), refine_inner(si, m, ac), refine_outer(so, m, ac)
{
    msg.log("Neo Test Method Iteration.");
}


SurfaceSolutionNeo::~SurfaceSolutionNeo()
{
}

void SurfaceSolutionNeo::update()
{
    InputVariableToEngine(engine, "update_Inner", 1.0f);
    InputVariableToEngine(engine, "update_Outer", 1.0f);

    // Extract Constants.
    float w_L = algorithm_config.get_float("w_L", 1.0f);
    float w_P = algorithm_config.get_float("w_P", 1.0f);
    float w_F = algorithm_config.get_float("w_F", 1.0f);
    float epsilon = algorithm_config.get_float("epsilon", 1.0e-3f);

    float grav_acc = algorithm_config.get_float("grav_acc", 10.0f);
    float area_press = algorithm_config.get_float("area_press", 10.0f);
    float area_mass = algorithm_config.get_float("area_mass", 100.0f);

    InputVariableToEngine(engine, "grav_acc", grav_acc);
    InputVariableToEngine(engine, "area_mass", area_mass);
    InputVariableToEngine(engine, "area_press", area_press);

    InputVariableToEngine(engine, "w_L", w_L);
    InputVariableToEngine(engine, "w_P", w_P);
    InputVariableToEngine(engine, "w_F", w_F);
    InputVariableToEngine(engine, "epsilon", epsilon);

    InputVariableToEngine(engine, "size_Inner", mesh_inner.n_vertices());
    InputVariableToEngine(engine, "size_Outer", mesh_outer.n_vertices());

    BuildLaplacianMatrixBuilderInner();
    InputSparseMatrixToEngine(engine, "Lap_Inner", builderLaplacianInner);

    // Extract Position.
    std::vector<std::vector<float>> data_position(num_verts_inner, std::vector<float>(3, 0.0f));
    SpMatBuilder builderDistance;
    SpMatBuilder builderIntensity;
    std::vector<std::vector<float>> data_normal(num_verts_inner, std::vector<float>(3, 0.0f));
    std::vector<std::vector<float>> data_grad_potential(num_verts_inner, std::vector<float>(3, 0.0f));
    for (int r = 0; r < num_verts_inner; r++)
    {
        auto pos = mesh_inner.point(verts_inner[r]);
        auto nor = mesh_inner.normal(verts_inner[r]);
        auto dir = dis_field->get_dir(pos);
        auto dis = dis_field->get_value(pos);
        builderDistance.push_back(SpMatTriple{ r, r, dis });
        builderIntensity.push_back(SpMatTriple{ r, r,
            _pinch(0.0f, dis - epsilon, 2 * epsilon) });
        for (int c = 0; c < 3; c++)
        {
            data_position[r][c] = pos[c];
            data_normal[r][c] = nor[c];
            data_grad_potential[r][c] = dir[c];
        }
    }
    InputDenseMatrixToEngine(engine, "V_Inner", data_position);
    InputSparseMatrixToEngine(engine, "Dis_Inner", builderDistance);
    InputSparseMatrixToEngine(engine, "Its_Inner", builderIntensity);
    InputDenseMatrixToEngine(engine, "N_Inner", data_normal);

    BuildLaplacianMatrixBuilderOuter();
    InputSparseMatrixToEngine(engine, "Lap_Outer", builderLaplacianOuter);

    // Extract Position.
    std::vector<std::vector<float>> data_position_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    SpMatBuilder builderDistance_outer;
    SpMatBuilder builderIntensity_outer;
    std::vector<std::vector<float>> data_normal_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    std::vector<std::vector<float>> data_grad_potential_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    for (int r = 0; r < num_verts_outer; r++)
    {
        auto pos = mesh_outer.point(verts_outer[r]);
        auto nor = mesh_outer.normal(verts_outer[r]);
        auto dir = dis_field->get_dir(pos);
        auto dis = dis_field->get_value(pos);
        builderDistance_outer.push_back(SpMatTriple{ r, r, dis });
        builderIntensity_outer.push_back(SpMatTriple{ r, r,
            _pinch(0.0f, dis - epsilon, 2 * epsilon) });
        for (int c = 0; c < 3; c++)
        {
            data_position_outer[r][c] = pos[c];
            data_normal_outer[r][c] = nor[c];
            data_grad_potential_outer[r][c] = dir[c];
        }
    }
    InputDenseMatrixToEngine(engine, "V_Outer", data_position_outer);
    InputSparseMatrixToEngine(engine, "Dis_Outer", builderDistance_outer);
    InputSparseMatrixToEngine(engine, "Its_Outer", builderIntensity_outer);
    InputDenseMatrixToEngine(engine, "N_Outer", data_normal_outer);

    // Build Mass Matrix and Area Matrix.
    SpMatBuilder builderMass, builderArea;
    for (auto f : mesh_inner.faces())
    {
        auto area = _area(mesh_inner, f);
        auto fv_iter = mesh_inner.fv_begin(f);
        for (; fv_iter != mesh_inner.fv_end(f); fv_iter++)
        {
            auto index = vert_index_inner[*fv_iter];
            // apply 1/3 the mass of face to each 3 face vertices.
            builderMass.push_back(SpMatTriple{ index, index,
                area_mass * area / 3.0f });
            builderArea.push_back(SpMatTriple{ index, index,
                area / 3.0f });
        }
    }
    InputSparseMatrixToEngine(engine, "Mass_Inner", builderMass);
    InputSparseMatrixToEngine(engine, "Area_Inner", builderArea);
    for (auto f : mesh_outer.faces())
    {
        auto area = _area(mesh_outer, f);
        auto fv_iter = mesh_outer.fv_begin(f);
        for (; fv_iter != mesh_outer.fv_end(f); fv_iter++)
        {
            auto index = vert_index_outer[*fv_iter];
            // apply 1/3 the mass of face to each 3 face vertices.
            builderMass.push_back(SpMatTriple{ index, index,
                area_mass * area / 3.0f });
            builderArea.push_back(SpMatTriple{ index, index,
                area / 3.0f });
        }
    }
    InputSparseMatrixToEngine(engine, "Mass_Outer", builderMass);
    InputSparseMatrixToEngine(engine, "Area_Outer", builderArea);


    // Main.
    auto script_filename = algorithm_config.get_string("MatlabScriptFileNeo");
    QFile f{ script_filename };
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        msg.log("cannot read Matlab script.", ERROR_MSG);
        return;
    }

    // Matlab Set Output
    char p[1000] = "";
    engOutputBuffer(engine, p, 1000);
    engEvalString(engine, f.readAll().toStdString().c_str());

    if (strlen(p) > 0)
        msg.log(p, INFO_MSG);


    // Retrieve Data
    auto data = RetieveDenseMatricFromEngine(engine, "V_prime_Inner", mesh_inner.n_vertices(), 3);
    for (int i = 0; i < mesh_inner.n_vertices(); i++)
    {
        mesh_inner.point(verts_inner[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    }
    data = RetieveDenseMatricFromEngine(engine, "V_prime_Outer", mesh_outer.n_vertices(), 3);
    for (int i = 0; i < mesh_outer.n_vertices(); i++)
    {
        mesh_outer.point(verts_outer[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    }

    changed_inner = refine_inner.refine("Inner_");
    changed_outer = refine_outer.refine("Outer_");
}

void SurfaceSolutionNeo::update_inner()
{
    InputVariableToEngine(engine, "update_Inner", 1.0f);
    InputVariableToEngine(engine, "update_Outer", 0.0f);

    // Extract Constants.
    float w_L = algorithm_config.get_float("w_L", 1.0f);
    float w_P = algorithm_config.get_float("w_P", 1.0f);
    float w_F = algorithm_config.get_float("w_F", 1.0f);
    float epsilon = algorithm_config.get_float("epsilon", 1.0e-3f);

    float grav_acc = algorithm_config.get_float("grav_acc", 10.0f);
    float area_press = algorithm_config.get_float("area_press", 10.0f);
    float area_mass = algorithm_config.get_float("area_mass", 100.0f);

    InputVariableToEngine(engine, "grav_acc", grav_acc);
    InputVariableToEngine(engine, "area_mass", area_mass);
    InputVariableToEngine(engine, "area_press", area_press);

    InputVariableToEngine(engine, "w_L", w_L);
    InputVariableToEngine(engine, "w_P", w_P);
    InputVariableToEngine(engine, "w_F", w_F);
    InputVariableToEngine(engine, "epsilon", epsilon);

    InputVariableToEngine(engine, "size_Inner", mesh_inner.n_vertices());
    // InputVariableToEngine(engine, "size_Outer", mesh_outer.n_vertices());

    BuildLaplacianMatrixBuilderInner();
    InputSparseMatrixToEngine(engine, "Lap_Inner", builderLaplacianInner);

    // Extract Position.
    std::vector<std::vector<float>> data_position(num_verts_inner, std::vector<float>(3, 0.0f));
    SpMatBuilder builderDistance;
    SpMatBuilder builderIntensity;
    std::vector<std::vector<float>> data_normal(num_verts_inner, std::vector<float>(3, 0.0f));
    std::vector<std::vector<float>> data_grad_potential(num_verts_inner, std::vector<float>(3, 0.0f));
    for (int r = 0; r < num_verts_inner; r++)
    {
        auto pos = mesh_inner.point(verts_inner[r]);
        auto nor = mesh_inner.normal(verts_inner[r]);
        auto dir = dis_field->get_dir(pos);
        auto dis = dis_field->get_value(pos);
        builderDistance.push_back(SpMatTriple{ r, r, dis });
        builderIntensity.push_back(SpMatTriple{ r, r,
            _pinch(0.0f, dis - epsilon, 2 * epsilon) });
        for (int c = 0; c < 3; c++)
        {
            data_position[r][c] = pos[c];
            data_normal[r][c] = nor[c];
            data_grad_potential[r][c] = dir[c];
        }
    }
    InputDenseMatrixToEngine(engine, "V_Inner", data_position);
    InputSparseMatrixToEngine(engine, "Dis_Inner", builderDistance);
    InputSparseMatrixToEngine(engine, "Its_Inner", builderIntensity);
    InputDenseMatrixToEngine(engine, "N_Inner", data_normal);

    // BuildLaplacianMatrixBuilderOuter();
    // InputSparseMatrixToEngine(engine, "Lap_Outer", builderLaplacianOuter);
    //
    // // Extract Position.
    // std::vector<std::vector<float>> data_position_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    // SpMatBuilder builderDistance_outer;
    // SpMatBuilder builderIntensity_outer;
    // std::vector<std::vector<float>> data_normal_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    // std::vector<std::vector<float>> data_grad_potential_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    // for (int r = 0; r < num_verts_outer; r++)
    // {
    //     auto pos = mesh_outer.point(verts_outer[r]);
    //     auto nor = mesh_outer.normal(verts_outer[r]);
    //     auto dir = dis_field->get_dir(pos);
    //     auto dis = dis_field->get_value(pos);
    //     builderDistance_outer.push_back(SpMatTriple{ r, r, dis });
    //     builderIntensity_outer.push_back(SpMatTriple{ r, r,
    //         _pinch(0.0f, dis - epsilon, 2 * epsilon) });
    //     for (int c = 0; c < 3; c++)
    //     {
    //         data_position_outer[r][c] = pos[c];
    //         data_normal_outer[r][c] = nor[c];
    //         data_grad_potential_outer[r][c] = dir[c];
    //     }
    // }
    // InputDenseMatrixToEngine(engine, "V_Outer", data_position_outer);
    // InputSparseMatrixToEngine(engine, "Dis_Outer", builderDistance_outer);
    // InputSparseMatrixToEngine(engine, "Its_Outer", builderIntensity_outer);
    // InputDenseMatrixToEngine(engine, "N_Outer", data_normal_outer);

    // Build Mass Matrix and Area Matrix.
    SpMatBuilder builderMass, builderArea;
    for (auto f : mesh_inner.faces())
    {
        auto area = _area(mesh_inner, f);
        auto fv_iter = mesh_inner.fv_begin(f);
        for (; fv_iter != mesh_inner.fv_end(f); fv_iter++)
        {
            auto index = vert_index_inner[*fv_iter];
            // apply 1/3 the mass of face to each 3 face vertices.
            builderMass.push_back(SpMatTriple{ index, index,
                area_mass * area / 3.0f });
            builderArea.push_back(SpMatTriple{ index, index,
                area / 3.0f });
        }
    }
    InputSparseMatrixToEngine(engine, "Mass_Inner", builderMass);
    InputSparseMatrixToEngine(engine, "Area_Inner", builderArea);
    // for (auto f : mesh_outer.faces())
    // {
    //     auto area = _area(mesh_outer, f);
    //     auto fv_iter = mesh_outer.fv_begin(f);
    //     for (; fv_iter != mesh_outer.fv_end(f); fv_iter++)
    //     {
    //         auto index = vert_index_outer[*fv_iter];
    //         // apply 1/3 the mass of face to each 3 face vertices.
    //         builderMass.push_back(SpMatTriple{ index, index,
    //             area_mass * area / 3.0f });
    //         builderArea.push_back(SpMatTriple{ index, index,
    //             area / 3.0f });
    //     }
    // }
    // InputSparseMatrixToEngine(engine, "Mass_Outer", builderMass);
    // InputSparseMatrixToEngine(engine, "Area_Outer", builderArea);

    // Extract Repulsion from kdtree
    SpMatBuilder builderRepulaionInner;
    bool enable_repulsion = algorithm_config.get_bool("EnableRepulsion", true);
    if (enable_repulsion)
    {
        // build outer kdtree
        BuildKdtreeOuter();

        for (int r = 0; r < num_verts_inner; r++)
        {
            auto pos = mesh_inner.point(verts_inner[r]);
            kdt::kdPoint p{ pos[0], pos[1], pos[2] };
            auto idx = kdtree_outer->nnSearch(p);
            auto nearest = pts_outer[idx];
            auto dis = NORM3(p, nearest);
            float factor = _pinch(0.0f, (dis - 1.5f * epsilon) / epsilon, 1.0f);
            builderRepulaionInner.push_back(SpMatTriple{ r, r, factor });
        }
    }
    else
    {
        for (int r = 0; r < num_verts_inner; r++)
        {
            builderRepulaionInner.push_back(SpMatTriple{ r, r, 1.0f });
        }
    }
    InputSparseMatrixToEngine(engine, "Rep_Inner", builderRepulaionInner);

    // Main.
    auto script_filename = algorithm_config.get_string("MatlabScriptFileNeo");
    QFile f{ script_filename };
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        msg.log("cannot read Matlab script.", ERROR_MSG);
        return;
    }

    TIC
    // Matlab Set Output
    char p[1000] = "";
    engOutputBuffer(engine, p, 1000);
    engEvalString(engine, f.readAll().toStdString().c_str());

    if (strlen(p) > 0)
        msg.log(p, INFO_MSG);
    TOC("Matlab work")

    // Retrieve Data
    auto data = RetieveDenseMatricFromEngine(engine, "V_prime_Inner", mesh_inner.n_vertices(), 3);
    for (int i = 0; i < mesh_inner.n_vertices(); i++)
    {
        mesh_inner.point(verts_inner[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    }
    // data = RetieveDenseMatricFromEngine(engine, "V_prime_Outer", mesh_outer.n_vertices(), 3);
    // for (int i = 0; i < mesh_outer.n_vertices(); i++)
    // {
    //     mesh_outer.point(verts_outer[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    // }

    TIC
    changed_inner = refine_inner.refine("Inner_");
    // changed_outer = refine_outer.refine();
    TOC("refine")
}

void SurfaceSolutionNeo::update_outer()
{
    InputVariableToEngine(engine, "update_Inner", 0.0f);
    InputVariableToEngine(engine, "update_Outer", 1.0f);

    // Extract Constants.
    float w_L = algorithm_config.get_float("w_L", 1.0f);
    float w_P = algorithm_config.get_float("w_P", 1.0f);
    float w_F = algorithm_config.get_float("w_F", 1.0f);
    float epsilon = algorithm_config.get_float("epsilon", 1.0e-3f);

    float grav_acc = algorithm_config.get_float("grav_acc", 10.0f);
    float area_press = algorithm_config.get_float("area_press", 10.0f);
    float area_mass = algorithm_config.get_float("area_mass", 100.0f);

    InputVariableToEngine(engine, "grav_acc", grav_acc);
    InputVariableToEngine(engine, "area_mass", area_mass);
    InputVariableToEngine(engine, "area_press", area_press);

    InputVariableToEngine(engine, "w_L", w_L);
    InputVariableToEngine(engine, "w_P", w_P);
    InputVariableToEngine(engine, "w_F", w_F);
    InputVariableToEngine(engine, "epsilon", epsilon);

    // InputVariableToEngine(engine, "size_Inner", mesh_inner.n_vertices());
     InputVariableToEngine(engine, "size_Outer", mesh_outer.n_vertices());

    // BuildLaplacianMatrixBuilderInner();
    // InputSparseMatrixToEngine(engine, "Lap_Inner", builderLaplacianInner);

    // Extract Position.
    BuildLaplacianMatrixBuilderOuter();
    InputSparseMatrixToEngine(engine, "Lap_Outer", builderLaplacianOuter);

    std::vector<std::vector<float>> data_position(num_verts_outer, std::vector<float>(3, 0.0f));
    SpMatBuilder builderDistance;
    SpMatBuilder builderIntensity;
    std::vector<std::vector<float>> data_normal(num_verts_outer, std::vector<float>(3, 0.0f));
    std::vector<std::vector<float>> data_grad_potential(num_verts_outer, std::vector<float>(3, 0.0f));
    // for (int r = 0; r < num_verts_inner; r++)
    // {
    //     auto pos = mesh_inner.point(verts_inner[r]);
    //     auto nor = mesh_inner.normal(verts_inner[r]);
    //     auto dir = dis_field->get_dir(pos);
    //     auto dis = dis_field->get_value(pos);
    //     builderDistance.push_back(SpMatTriple{ r, r, dis });
    //     builderIntensity.push_back(SpMatTriple{ r, r,
    //         _pinch(0.0f, dis - epsilon, 2 * epsilon) });
    //     for (int c = 0; c < 3; c++)
    //     {
    //         data_position[r][c] = pos[c];
    //         data_normal[r][c] = nor[c];
    //         data_grad_potential[r][c] = dir[c];
    //     }
    // }
    // InputDenseMatrixToEngine(engine, "V_Inner", data_position);
    // InputSparseMatrixToEngine(engine, "Dis_Inner", builderDistance);
    // InputSparseMatrixToEngine(engine, "Its_Inner", builderIntensity);
    // InputDenseMatrixToEngine(engine, "N_Inner", data_normal);

    // Extract Position.
    std::vector<std::vector<float>> data_position_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    SpMatBuilder builderDistance_outer;
    SpMatBuilder builderIntensity_outer;
    std::vector<std::vector<float>> data_normal_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    std::vector<std::vector<float>> data_grad_potential_outer(num_verts_outer, std::vector<float>(3, 0.0f));
    for (int r = 0; r < num_verts_outer; r++)
    {
        auto pos = mesh_outer.point(verts_outer[r]);
        auto nor = mesh_outer.normal(verts_outer[r]);
        auto dir = dis_field->get_dir(pos);
        auto dis = dis_field->get_value(pos);
        builderDistance_outer.push_back(SpMatTriple{ r, r, dis });
        builderIntensity_outer.push_back(SpMatTriple{ r, r,
            _pinch(0.0f, dis - epsilon, 2 * epsilon) });
        for (int c = 0; c < 3; c++)
        {
            data_position_outer[r][c] = pos[c];
            data_normal_outer[r][c] = nor[c];
            data_grad_potential_outer[r][c] = dir[c];
        }
    }
    InputDenseMatrixToEngine(engine, "V_Outer", data_position_outer);
    InputSparseMatrixToEngine(engine, "Dis_Outer", builderDistance_outer);
    InputSparseMatrixToEngine(engine, "Its_Outer", builderIntensity_outer);
    InputDenseMatrixToEngine(engine, "N_Outer", data_normal_outer);

    // Build Mass Matrix and Area Matrix.
    SpMatBuilder builderMass, builderArea;
    // for (auto f : mesh_inner.faces())
    // {
    //     auto area = _area(mesh_inner, f);
    //     auto fv_iter = mesh_inner.fv_begin(f);
    //     for (; fv_iter != mesh_inner.fv_end(f); fv_iter++)
    //     {
    //         auto index = vert_index_inner[*fv_iter];
    //         // apply 1/3 the mass of face to each 3 face vertices.
    //         builderMass.push_back(SpMatTriple{ index, index,
    //             area_mass * area / 3.0f });
    //         builderArea.push_back(SpMatTriple{ index, index,
    //             area / 3.0f });
    //     }
    // }
    // InputSparseMatrixToEngine(engine, "Mass_Inner", builderMass);
    // InputSparseMatrixToEngine(engine, "Area_Inner", builderArea);
    for (auto f : mesh_outer.faces())
    {
        auto area = _area(mesh_outer, f);
        auto fv_iter = mesh_outer.fv_begin(f);
        for (; fv_iter != mesh_outer.fv_end(f); fv_iter++)
        {
            auto index = vert_index_outer[*fv_iter];
            // apply 1/3 the mass of face to each 3 face vertices.
            builderMass.push_back(SpMatTriple{ index, index,
                area_mass * area / 3.0f });
            builderArea.push_back(SpMatTriple{ index, index,
                area / 3.0f });
        }
    }
    InputSparseMatrixToEngine(engine, "Mass_Outer", builderMass);
    InputSparseMatrixToEngine(engine, "Area_Outer", builderArea);

    // Extract Repulsion from kdtree
    SpMatBuilder builderRepulaionOuter;
    bool enable_repulsion = algorithm_config.get_bool("EnableRepulsion", true);
    if (enable_repulsion)
    {
        // build outer kdtree
        BuildKdtreeInner();

        for (int r = 0; r < num_verts_outer; r++)
        {
            auto pos = mesh_outer.point(verts_outer[r]);
            kdt::kdPoint p{ pos[0], pos[1], pos[2] };
            auto idx = kdtree_inner->nnSearch(p);
            auto nearest = pts_inner[idx];
            auto dis = NORM3(p, nearest);
            float factor = _pinch(0.0f, (dis - 1.5f * epsilon) / epsilon, 1.0f);
            builderRepulaionOuter.push_back(SpMatTriple{ r, r, factor });
        }
    }
    else
    {
        for (int r = 0; r < num_verts_outer; r++)
        {
            builderRepulaionOuter.push_back(SpMatTriple{ r, r, 1.0f });
        }
    }
    InputSparseMatrixToEngine(engine, "Rep_Outer", builderRepulaionOuter);

    // Main.
    auto script_filename = algorithm_config.get_string("MatlabScriptFileNeo");
    QFile f{ script_filename };
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        msg.log("cannot read Matlab script.", ERROR_MSG);
        return;
    }

    TIC
    // Matlab Set Output
    char p[1000] = "";
    engOutputBuffer(engine, p, 1000);
    engEvalString(engine, f.readAll().toStdString().c_str());

    if (strlen(p) > 0)
        msg.log(p, INFO_MSG);
    TOC("Matlab work")

    // Retrieve Data
    // auto data = RetieveDenseMatricFromEngine(engine, "V_prime_Inner", mesh_inner.n_vertices(), 3);
    // for (int i = 0; i < mesh_inner.n_vertices(); i++)
    // {
    //     mesh_inner.point(verts_inner[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    // }
    auto data = RetieveDenseMatricFromEngine(engine, "V_prime_Outer", mesh_outer.n_vertices(), 3);
    for (int i = 0; i < mesh_outer.n_vertices(); i++)
    {
        mesh_outer.point(verts_outer[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    }

    // changed_inner = refine_inner.refine();
    TIC
    changed_outer = refine_outer.refine("Outer_");
    TOC("refine")
}

void SurfaceSolutionNeo::UpdateBasicMeshInformationInner()
{
    TIC
    num_verts_inner = mesh_inner.n_vertices();
    verts_inner.clear();
    num_neighbors_inner.clear();
    neighbors_inner.clear();
    vert_index_inner.clear();
    for (auto vh : mesh_inner.vertices())
    {
        vert_index_inner[vh] = verts_inner.size();
        verts_inner.push_back(vh);
    }
    for (auto vh : mesh_inner.vertices())
    {
        neighbors_inner.push_back(std::vector<int>());
        int neighbor_count = 0;
        auto vv_iter = mesh_inner.vv_begin(vh);
        for (; vv_iter != mesh_inner.vv_end(vh); vv_iter++)
        {
            neighbor_count++;
            neighbors_inner.back().push_back(vert_index_inner[*vv_iter]);
        }
        num_neighbors_inner.push_back(neighbor_count);
    }

    changed_inner = false;
    TOC("basic info inner")
}

void SurfaceSolutionNeo::UpdateBasicMeshInformationOuter()
{
    TIC
    num_verts_outer = mesh_outer.n_vertices();
    verts_outer.clear();
    num_neighbors_outer.clear();
    neighbors_outer.clear();
    vert_index_outer.clear();
    for (auto vh : mesh_outer.vertices())
    {
        vert_index_outer[vh] = verts_outer.size();
        verts_outer.push_back(vh);
    }
    for (auto vh : mesh_outer.vertices())
    {
        neighbors_outer.push_back(std::vector<int>());
        int neighbor_count = 0;
        auto vv_iter = mesh_outer.vv_begin(vh);
        for (; vv_iter != mesh_outer.vv_end(vh); vv_iter++)
        {
            neighbor_count++;
            neighbors_outer.back().push_back(vert_index_outer[*vv_iter]);
        }
        num_neighbors_outer.push_back(neighbor_count);
    }

    changed_outer = false;
    TOC("basic info outer")
}

void SurfaceSolutionNeo::BuildLaplacianMatrixBuilderInner()
{
    if (changed_inner || builderLaplacianInner.empty())
        UpdateBasicMeshInformationInner();
    else
        return;

    TIC
    builderLaplacianInner.clear();

    auto weight_policy = algorithm_config.get_string("LaplacianWeight", "uniform");

    for (int i = 0; i < num_verts_inner; i++)
    {
        float weight_sum = 0.0f;

        if (weight_policy == "uniform")
        {
            for (int neib_vert_idx : neighbors_inner[i])
            {
                float weight = 1.0f; // uniform
                builderLaplacianInner.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }
        else if (weight_policy == "cotangent")
        {
            int degree = neighbors_inner[i].size();
            for (int j = 0; j < degree; ++j)
            {
                int vj = neighbors_inner[i][j];
                int vleft = neighbors_inner[i][MOD(j + 1, degree)];
                int vright = neighbors_inner[i][MOD(j - 1, degree)];

                auto weight = _cotangent_for_angle_AOB(i, vleft, vj) +
                    _cotangent_for_angle_AOB(i, vright, vj);
                weight_sum += weight;
                builderLaplacianInner.push_back(SpMatTriple{ i, vj, -weight });
            }
        }
        else
        {
            msg.log("Unknown Laplacian policy ", weight_policy, ERROR_MSG);
            // still use uniform
            for (int neib_vert_idx : neighbors_inner[i])
            {
                float weight = 1.0f; // uniform
                builderLaplacianInner.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }

        builderLaplacianInner.push_back(SpMatTriple{ i, i, weight_sum });
    }
    TOC("update Lap inner")
}

void SurfaceSolutionNeo::BuildLaplacianMatrixBuilderOuter()
{
    if (changed_outer || builderLaplacianOuter.empty())
        UpdateBasicMeshInformationOuter();
    else
        return;

    TIC
    builderLaplacianOuter.clear();

    auto weight_policy = algorithm_config.get_string("LaplacianWeight", "uniform");

    for (int i = 0; i < num_verts_outer; i++)
    {
        float weight_sum = 0.0f;

        if (weight_policy == "uniform")
        {
            for (int neib_vert_idx : neighbors_outer[i])
            {
                float weight = 1.0f; // uniform
                builderLaplacianOuter.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }
        else if (weight_policy == "cotangent")
        {
            int degree = neighbors_outer[i].size();
            for (int j = 0; j < degree; ++j)
            {
                int vj = neighbors_outer[i][j];
                int vleft = neighbors_outer[i][MOD(j + 1, degree)];
                int vright = neighbors_outer[i][MOD(j - 1, degree)];

                auto weight = _cotangent_for_angle_AOB(i, vleft, vj) +
                    _cotangent_for_angle_AOB(i, vright, vj);
                weight_sum += weight;
                builderLaplacianOuter.push_back(SpMatTriple{ i, vj, -weight });
            }
        }
        else
        {
            msg.log("Unknown Laplacian policy ", weight_policy, ERROR_MSG);
            // still use uniform
            for (int neib_vert_idx : neighbors_outer[i])
            {
                float weight = 1.0f; // uniform
                builderLaplacianOuter.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }

        builderLaplacianOuter.push_back(SpMatTriple{ i, i, weight_sum });
    }
    TOC("update Lap outer")
}

void SurfaceSolutionNeo::BuildKdtreeInner()
{
    if (!changed_inner && kdtree_inner != nullptr)
        return;

    if (changed_inner)
        UpdateBasicMeshInformationInner();

    TIC
    // Build kdTree
    pts_inner.clear();
    for (auto vh : mesh_inner.vertices())
    {
        auto x = mesh_inner.point(vh)[0];
        auto y = mesh_inner.point(vh)[1];
        auto z = mesh_inner.point(vh)[2];
        pts_inner.push_back({ x, y, z });
    }

    kdtree_inner = new kdt::kdTree(pts_inner);
    TOC("build kdtree inner")
}

void SurfaceSolutionNeo::BuildKdtreeOuter()
{
    if (!changed_outer && kdtree_outer != nullptr)
        return;

    if (changed_outer)
        UpdateBasicMeshInformationOuter();

    TIC
    // Build kdTree
    pts_outer.clear();
    for (auto vh : mesh_outer.vertices())
    {
        auto x = mesh_outer.point(vh)[0];
        auto y = mesh_outer.point(vh)[1];
        auto z = mesh_outer.point(vh)[2];
        pts_outer.push_back({ x, y, z });
    }

    kdtree_outer = new kdt::kdTree(pts_outer);
    TOC("build kdtree outer")
}

