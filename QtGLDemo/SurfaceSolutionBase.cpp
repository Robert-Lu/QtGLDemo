#include "stdafx.h"
#include "SurfaceSolutionBase.h"
#include <sstream>

#define MOD(x, m) (((x) + (m)) % (m))

/**
 * \brief record basic information from mesh.
 * 
 * num_vert, verts, num_neighbors, neighbors.
 */
void SurfaceSolutionBase::UpdateBasicMeshInformation()
{
    num_verts = mesh.n_vertices();
    verts.clear();
    num_neighbors.clear();
    neighbors.clear();
    vert_index.clear();
    for (auto vh : mesh.vertices())
    {
        vert_index[vh] = verts.size();
        verts.push_back(vh);
    }
    for (auto vh : mesh.vertices())
    {
        neighbors.push_back(std::vector<int>());
        int neighbor_count = 0;
        auto vv_iter = mesh.vv_begin(vh);
        for (; vv_iter != mesh.vv_end(vh); vv_iter++)
        {
            neighbor_count++;
            neighbors.back().push_back(vert_index[*vv_iter]);
        }
        num_neighbors.push_back(neighbor_count);
    }
}


// return cot(angle AOB)
float SurfaceSolutionBase::_cotangent_for_angle_AOB(int A, int O, int B)
{
    Vec3f a = mesh.point(verts[A]);
    Vec3f o = mesh.point(verts[O]);
    Vec3f b = mesh.point(verts[B]);

    Vec3f oa = a - o;
    Vec3f ob = b - o;

    auto cot = OpenMesh::dot(oa, ob) / OpenMesh::cross(oa, ob).norm();

    return cot;
}


/**
 * \brief build Sparse Laplacian Matrix.
 */
void SurfaceSolutionBase::BuildLaplacianMatrixBuilder()
{
    builderLaplacian.clear();

    auto weight_policy = algorithm_config.get_string("LaplacianWeight", "uniform");

    for (int i = 0; i < num_verts; i++)
    {
        float weight_sum = 0.0f;

        if (weight_policy == "uniform")
        {
            for (int neib_vert_idx : neighbors[i])
            {
                float weight = 1.0f; // uniform
                builderLaplacian.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
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
                builderLaplacian.push_back(SpMatTriple{ i, vj, -weight });
            }
        }
        else
        {
            msg.log("Unknown Laplacian policy ", weight_policy, ERROR_MSG);
            // still use uniform
            for (int neib_vert_idx : neighbors[i])
            {
                float weight = 1.0f; // uniform
                builderLaplacian.push_back(SpMatTriple{ i, neib_vert_idx, -weight });
                weight_sum += weight;
            }
        }

        builderLaplacian.push_back(SpMatTriple{ i, i, weight_sum });
    }
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


inline float _cos_max_anglle(TriMesh &mesh, FaceHandle fh)
{
    auto fv_iter = mesh.fv_begin(fh);
    Vec3f pos[3];
    int i = 0;
    for (; fv_iter != mesh.fv_end(fh); fv_iter++)
    {
        auto v = *fv_iter;
        pos[i++] = mesh.point(fv_iter);
    }
    float a = (pos[1] - pos[0]).norm();
    float b = (pos[2] - pos[0]).norm();
    float c = (pos[2] - pos[1]).norm();
    if (a < b)
        std::swap(a, b);
    if (a < c)
        std::swap(a, c);
    if (b < c)
        std::swap(b, c);
    return (b * b + c * c - a * a) / b / c / 2.0f;
}


inline float _cos_min_anglle(TriMesh &mesh, FaceHandle fh)
{
    auto fv_iter = mesh.fv_begin(fh);
    Vec3f pos[3];
    int i = 0;
    for (; fv_iter != mesh.fv_end(fh); fv_iter++)
    {
        auto v = *fv_iter;
        pos[i++] = mesh.point(fv_iter);
    }
    float a = (pos[1] - pos[0]).norm();
    float b = (pos[2] - pos[0]).norm();
    float c = (pos[2] - pos[1]).norm();
    if (a < b)
        std::swap(a, b);
    if (a < c)
        std::swap(a, c);
    if (b < c)
        std::swap(b, c);
    return (b * b + a * a - c * c) / b / a / 2.0f;
}


void SurfaceSolutionBase::RefineSurface()
{
    // Calculate average edge length and triangle area.
    float ave_edge_len = 0.0f;
    float ave_face_area = 0.0f;
    for (auto eh : mesh.edges())
    {
        ave_edge_len += mesh.calc_edge_length(eh);
    }
    for (auto fh : mesh.faces())
    {
        ave_face_area += _area(mesh, fh);
    }
    ave_edge_len /= mesh.n_edges();
    ave_face_area /= mesh.n_faces();

    // Record the faces to expand.
    verts_tag.clear();
    std::set<FaceHandle> set_faces_to_expand;
    std::vector<HalfEdgeHandle> edges_to_expand;

    // For each face, check for expand.
    float area_expand_threshold = algorithm_config.get_float("Area_Expand_Threshold", 1.5f);
    float big_angle_expand_threshold = algorithm_config.get_float("Big_Angle_Expand_Threshold", 135.0f);
    float small_angle_expand_threshold = algorithm_config.get_float("Small_Angle_Expand_Threshold", 30.0f);
    float cos_big_angle_expand_threshold = cos(big_angle_expand_threshold / 180.0f * 3.1415926f);
    float cos_small_angle_expand_threshold = cos(small_angle_expand_threshold / 180.0f * 3.1415926f);
    for (auto fh : mesh.faces())
    {
        // Skip if already in the set of faces to expand.
        if (set_faces_to_expand.find(fh) != set_faces_to_expand.end())
            continue;

        // Expand Case 1: face area too big:
        auto face_area = _area(mesh, fh);
        if (face_area > area_expand_threshold * ave_face_area)
        {
            // Find the largest edge.
            HalfEdgeHandle eh, eh_first, eh_max;
            eh = eh_first = eh_max = mesh.halfedge_handle(fh);

            float max_len = 0.0f;
            do
            {
                if (mesh.calc_edge_length(eh) > max_len)
                {
                    eh_max = eh;
                    max_len = mesh.calc_edge_length(eh);
                }
                eh = mesh.next_halfedge_handle(eh);
            }
            while (eh != eh_first);

            FaceHandle adjcent_face = mesh.opposite_face_handle(eh_max);
            set_faces_to_expand.insert(fh);
            set_faces_to_expand.insert(adjcent_face);
            edges_to_expand.push_back(eh_max);

            verts_tag[mesh.from_vertex_handle(eh_max)] = 1;
            verts_tag[mesh.to_vertex_handle(eh_max)] = 1;

            continue;
        }

        // Expand Case 2: angle too big:
        auto cos_max_angle = _cos_max_anglle(mesh, fh);
        if (cos_max_angle < cos_big_angle_expand_threshold)
        {
            // Find the largest edge.
            HalfEdgeHandle eh, eh_first, eh_max;
            eh = eh_first = eh_max = mesh.halfedge_handle(fh);

            float max_len = 0.0f;
            do
            {
                if (mesh.calc_edge_length(eh) > max_len)
                {
                    eh_max = eh;
                    max_len = mesh.calc_edge_length(eh);
                }
                eh = mesh.next_halfedge_handle(eh);
            } while (eh != eh_first);

            FaceHandle adjcent_face = mesh.opposite_face_handle(eh_max);
            set_faces_to_expand.insert(fh);
            set_faces_to_expand.insert(adjcent_face);
            edges_to_expand.push_back(eh_max);

            verts_tag[mesh.from_vertex_handle(eh_max)] = 2;
            verts_tag[mesh.to_vertex_handle(eh_max)] = 2;

            continue;
        }

        // Expand Case 3: angle too small:
        auto cos_min_angle = _cos_min_anglle(mesh, fh);
        if (cos_min_angle > cos_small_angle_expand_threshold)
        {
            // Find the largest edge.
            HalfEdgeHandle eh, eh_first, eh_max;
            eh = eh_first = eh_max = mesh.halfedge_handle(fh);

            float max_len = 0.0f;
            do
            {
                if (mesh.calc_edge_length(eh) > max_len)
                {
                    eh_max = eh;
                    max_len = mesh.calc_edge_length(eh);
                }
                eh = mesh.next_halfedge_handle(eh);
            } while (eh != eh_first);

            FaceHandle adjcent_face = mesh.opposite_face_handle(eh_max);
            set_faces_to_expand.insert(fh);
            set_faces_to_expand.insert(adjcent_face);
            edges_to_expand.push_back(eh_max);

            verts_tag[mesh.from_vertex_handle(eh_max)] = 3;
            verts_tag[mesh.to_vertex_handle(eh_max)] = 3;

            continue;
        }

        // Expand Case
    }

    // Apply the Expand.


    // Record the faces to shrink.
    //std::set<FaceHandle> set_faces_to_shrink;
    //std::vector<HalfEdgeHandle> edges_to_shrink;

    // For each face, check for shrink.
    /*
    float area_shrink_threshold = algorithm_config.get_float("Area_Shrink_Threshold", 1.5f);
    for (auto fh : mesh.faces())
    {
        auto face_area = _area(mesh, fh);
        if (face_area < area_shrink_threshold * ave_face_area)
        {
            // Find the largest edge.
            HalfEdgeHandle eh_first = mesh.halfedge_handle(fh);
            HalfEdgeHandle eh = eh_first, eh_max = eh_first;
            float max_len = 0.0f;
            do
            {
                if (mesh.calc_edge_length(eh) > max_len)
                {
                    eh_max = eh;
                    max_len = mesh.calc_edge_length(eh);
                }
                eh = mesh.next_halfedge_handle(eh);
            } while (eh != eh_first);

            verts_tag[mesh.from_vertex_handle(eh_max)] = 4;
            verts_tag[mesh.to_vertex_handle(eh_max)] = 4;

            continue;
        }
    }
    */

    // Apply the Shrink.
}


SurfaceSolutionBase::SurfaceSolutionBase(TriMesh& s, OcTreeField *d, 
    ConsoleMessageManager &m, TextConfigLoader &ac)
    : mesh(s), dis_field(d), msg(m), algorithm_config(ac)
{
    // Extract basic information from the mesh.
    UpdateBasicMeshInformation();

    // Build Laplacian Matrix.
    BuildLaplacianMatrixBuilder();
}


int SurfaceSolutionBase::tagged(VertexHandle vh)
{
    if (verts_tag.find(vh) != verts_tag.end())
        return verts_tag[vh];
    else
        return -1;
}

SurfaceSolutionBase::~SurfaceSolutionBase()
{
}

