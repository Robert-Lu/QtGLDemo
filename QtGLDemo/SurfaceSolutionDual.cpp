#include "stdafx.h"
#include "SurfaceSolutionDual.h"

#define MOD(x, m) (((x) + (m)) % (m))

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
    UpdateBasicMeshInformationDual();

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

    BuildLaplacianMatrixBuilder(builderLaplacian, mesh_inner);
    InputSparseMatrixToEngine(engine, "Lap_Inner", builderLaplacian);

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
            dis - epsilon < 0 ? 0 : dis - epsilon });
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

    BuildLaplacianMatrixBuilder(builderLaplacian, mesh_outer);
    InputSparseMatrixToEngine(engine, "Lap_Outer", builderLaplacian);

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
            dis - epsilon < 0 ? 0 : dis - epsilon });
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
            auto index = vert_index[*fv_iter];
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
            auto index = vert_index[*fv_iter];
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
    auto data = RetieveDenseMatricFromEngine(engine, "V_prime", mesh_inner.n_vertices(), 3);
    for (int i = 0; i < mesh_inner.n_vertices(); i++)
    {
        mesh_inner.point(verts[i]) = Vec3f{ data[i][0], data[i][1], data[i][2] };
    }

    refine_inner.refine();
}

void SurfaceSolutionNeo::UpdateBasicMeshInformationDual()
{
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


}

void SurfaceSolutionNeo::BuildLaplacianMatrixBuilder(SpMatBuilder& builder, TriMesh &M)
{
    num_verts = M.n_vertices();
    verts.clear();
    num_neighbors.clear();
    neighbors.clear();
    vert_index.clear();
    for (auto vh : M.vertices())
    {
        vert_index[vh] = verts.size();
        verts.push_back(vh);
    }
    for (auto vh : M.vertices())
    {
        neighbors.push_back(std::vector<int>());
        int neighbor_count = 0;
        auto vv_iter = M.vv_begin(vh);
        for (; vv_iter != M.vv_end(vh); vv_iter++)
        {
            neighbor_count++;
            neighbors.back().push_back(vert_index[*vv_iter]);
        }
        num_neighbors.push_back(neighbor_count);
    }

    builder.clear();

    auto weight_policy = algorithm_config.get_string("LaplacianWeight", "uniform");

    for (int i = 0; i < num_verts; i++)
    {
        float weight_sum = 0.0f;

        if (weight_policy == "uniform")
        {
            for (int neib_vert_idx : neighbors[i])
            {
                float weight = 1.0f; // uniform
                builder.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }
        else if (weight_policy == "cotangent")
        {
            int degree = neighbors[i].size();
            for (int j = 0; j < degree; ++j)
            {
                int vj = neighbors[i][j];
                int vleft = neighbors[i][MOD(j + 1, degree)];
                int vright = neighbors[i][MOD(j - 1, degree)];

                auto weight = _cotangent_for_angle_AOB(i, vleft, vj) +
                    _cotangent_for_angle_AOB(i, vright, vj);
                weight_sum += weight;
                builder.push_back(SpMatTriple{ i, vj, -weight });
            }
        }
        else
        {
            msg.log("Unknown Laplacian policy ", weight_policy, ERROR_MSG);
            // still use uniform
            for (int neib_vert_idx : neighbors[i])
            {
                float weight = 1.0f; // uniform
                builder.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }

        builder.push_back(SpMatTriple{ i, i, weight_sum });
    }
}
