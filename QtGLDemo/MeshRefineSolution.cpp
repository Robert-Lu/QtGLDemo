#include "stdafx.h"
#include "MeshRefineSolution.h"


MeshRefineSolution::MeshRefineSolution(TriMesh &s, ConsoleMessageManager &m, TextConfigLoader &ac)
    : mesh(s), msg(m), algorithm_config(ac)
{
}


MeshRefineSolution::~MeshRefineSolution()
{
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


// get the longest edge.
inline HalfEdgeHandle _get_max_edge_in_face(TriMesh &mesh, FaceHandle fh, float &l)
{
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

    l = max_len;
    return eh_max;
}


// get the shortest edge.
inline HalfEdgeHandle _get_min_edge_in_face(TriMesh &mesh, FaceHandle fh, float &l)
{
    HalfEdgeHandle eh, eh_first, eh_min;
    eh = eh_first = eh_min = mesh.halfedge_handle(fh);
    float min_len = INFINITY;
    do
    {
        if (mesh.calc_edge_length(eh) < min_len)
        {
            eh_min = eh;
            min_len = mesh.calc_edge_length(eh);
        }
        eh = mesh.next_halfedge_handle(eh);
    } while (eh != eh_first);

    l = min_len;
    return eh_min;
}


// get the ratio of length of this edge to new edge if flipped.
inline float _get_flip_ratio(TriMesh &mesh, HalfEdgeHandle heh)
{
    VertexHandle v[4];
    v[0] = mesh.to_vertex_handle(heh);
    v[1] = mesh.to_vertex_handle(mesh.next_halfedge_handle(heh));
    auto oheh = mesh.opposite_halfedge_handle(heh);
    v[2] = mesh.to_vertex_handle(oheh);
    v[3] = mesh.to_vertex_handle(mesh.next_halfedge_handle(oheh));

    float len_edge = mesh.calc_edge_length(heh);
    float len_flip = (mesh.point(v[1]) - mesh.point(v[2])).norm();
    return len_edge / len_flip;
}


int MeshRefineSolution::_Split(float edge_split_tolerance, bool& changed)
{
    // Split
    std::set<FaceHandle> face_ignore;
    std::vector<HalfEdgeHandle> edge_to_split;
    for (auto fh : mesh.faces())
    {
        if (face_ignore.find(fh) != face_ignore.end())
            continue;
        // For each face, get the longest edge.
        float max_len, max_len_opposite;
        auto eh_max = _get_max_edge_in_face(mesh, fh, max_len);
        // Get the opposite of the longest edge.
        auto opposite_fh = mesh.opposite_face_handle(eh_max);

        if (max_len > edge_split_tolerance)
        {
            // Apply Split.
            changed = true;

            face_ignore.insert(fh);
            face_ignore.insert(opposite_fh);
            edge_to_split.push_back(eh_max);
        }
    }

    mesh.request_face_status();
    for (auto eh : edge_to_split)
    {
        VertexHandle vh[4];
        vh[0] = mesh.to_vertex_handle(eh);
        vh[1] = mesh.to_vertex_handle(mesh.next_halfedge_handle(eh));
        vh[2] = mesh.to_vertex_handle(mesh.opposite_halfedge_handle(eh));
        vh[3] = mesh.to_vertex_handle(mesh.next_halfedge_handle(
            mesh.opposite_halfedge_handle(eh)));

        auto mid_pos = (mesh.point(vh[0]) + mesh.point(vh[2])) / 2.0f;
        auto mid_vh = mesh.add_vertex(mid_pos);

        mesh.delete_face(mesh.face_handle(eh), false);
        mesh.delete_face(mesh.opposite_face_handle(eh), false);

        mesh.add_face(vh[0], vh[1], mid_vh);
        mesh.add_face(vh[1], vh[2], mid_vh);
        mesh.add_face(vh[2], vh[3], mid_vh);
        mesh.add_face(vh[3], vh[0], mid_vh);
    }
    mesh.garbage_collection();

    return edge_to_split.size();
}

int MeshRefineSolution::_Flip(float edge_flip_tolerance, bool& changed)
{
    // Flip
    std::set<FaceHandle> face_ignore;
    std::vector<HalfEdgeHandle> edge_to_flip;
    for (auto fh : mesh.faces())
    {
        if (face_ignore.find(fh) != face_ignore.end())
            continue;
        // For each face, get the longest edge.
        float max_len, max_len_opposite;
        auto eh_max = _get_max_edge_in_face(mesh, fh, max_len);
        // Get the opposite of the longest edge.
        auto opposite_fh = mesh.opposite_face_handle(eh_max);
        // Get the ratio of length of the edge to the flipped edge.
        auto ratio = _get_flip_ratio(mesh, eh_max);

        if (ratio > edge_flip_tolerance)
        {
            // Apply Flip.
            changed = true;

            face_ignore.insert(fh);
            face_ignore.insert(opposite_fh);
            edge_to_flip.push_back(eh_max);
        }
    }

    mesh.request_edge_status();
    mesh.request_vertex_status();
    mesh.request_face_status();
    int count = 0;
    for (auto eh : edge_to_flip)
    {
        if (mesh.is_flip_ok(mesh.edge_handle(eh)))
        {
            mesh.flip(mesh.edge_handle(eh));
            count++;
        }
    }
    mesh.garbage_collection();

    return count;
}

int MeshRefineSolution::_Collapse(float edge_collapse_tolerance, bool& changed)
{
    std::cout << edge_collapse_tolerance << "\n";
    int output_count = 50;

    // Collapse
    std::set<FaceHandle> face_ignore;
    std::vector<HalfEdgeHandle> edge_to_collapse;
    for (auto fh : mesh.faces())
    {
        if (face_ignore.find(fh) != face_ignore.end())
            continue;
        // For each face, get the shortest edge.
        float min_len;
        auto eh_min = _get_min_edge_in_face(mesh, fh, min_len);
        // Get the opposite of the longest edge.
        auto opposite_fh = mesh.opposite_face_handle(eh_min);
        if (face_ignore.find(opposite_fh) != face_ignore.end())
            continue;
        if (min_len < edge_collapse_tolerance)
        {
            if (output_count--)
                std::cout << "Collapse at\t" << min_len << std::endl;
            // Apply Collapse.
            changed = true;

            face_ignore.insert(fh);
            face_ignore.insert(opposite_fh);
            edge_to_collapse.push_back(eh_min);
        }
    }

    mesh.request_edge_status();
    mesh.request_vertex_status();
    mesh.request_face_status();
    int count = 0;
    for (auto eh : edge_to_collapse)
    {
        if (mesh.is_valid_handle(eh) && mesh.is_collapse_ok(eh))
        {
            mesh.collapse(eh);
            count++;
        }
    }
    mesh.garbage_collection();

    return count;
}

bool MeshRefineSolution::refine()
{
    algorithm_config.reload();
    bool changed = false;

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

    /**
    *    Split
    *      |
    *  Flip (loop)<--.
    *      |         |
    * <Flip again?>--^
    *      |
    *   Collapse
    *      |
    *    [done]
    */

    // Configure
    float edge_split_tolerance = INFINITY;
    float edge_collapse_tolerance = 0.0f;
    float edge_flip_tolerance = algorithm_config.get_float("Edge_Flip_Tolerance", 2.5f);
  
    if (algorithm_config.get_bool("Reletive_Tolerance", true))
    {
        edge_split_tolerance = ave_edge_len *
            algorithm_config.get_float("Edge_Split_Tolerance", 2.5f);
        edge_collapse_tolerance = ave_edge_len *
            algorithm_config.get_float("Edge_Collapse_Tolerance", 0.2f);
    }
    else
    {
        edge_split_tolerance = 1.0f *
            algorithm_config.get_float("Edge_Split_Tolerance", 2.5f);
        edge_collapse_tolerance = 1.0f *
            algorithm_config.get_float("Edge_Collapse_Tolerance", 0.2f);
    }
    
    int iter_limit_split = algorithm_config.get_int("Edge_Split_Iteration_limit", 3);
    int iter_limit_flip = algorithm_config.get_int("Edge_Flip_Iteration_limit", 5);
    int iter_limit_collapse = algorithm_config.get_int("Edge_Collapse_Iteration_limit", 3);

    // Split
    for (int i = 0; i < iter_limit_split; i++)
    {
        int count = _Split(edge_split_tolerance, changed);
        if (count > 0)
            msg.log(QString("%0 edges split at iteration %1.").arg(count).arg(i + 1), INFO_MSG);
        else
        {
            msg.log(QString("no edge split at iteration %0.").arg(i + 1), TRIVIAL_MSG);
            break;
        }
    }

    // Flip
    for (int i = 0; i < iter_limit_flip; i++)
    {
        int count = _Flip(edge_flip_tolerance, changed);
        if (count > 0)
            msg.log(QString("%0 edges flip at iteration %1.").arg(count).arg(i + 1), INFO_MSG);
        else
        {
            msg.log(QString("no edge flip at iteration %0.").arg(i + 1), TRIVIAL_MSG);
            break;
        }
    }

    // Collapse
    for (int i = 0; i < iter_limit_collapse; i++)
    {
        int count = _Collapse(edge_collapse_tolerance, changed);
        if (count > 0)
            msg.log(QString("%0 edges collapse at iteration %1.").arg(count).arg(i + 1), INFO_MSG);
        else
        {
            msg.log(QString("no edge collapse at iteration %0.").arg(i + 1), TRIVIAL_MSG);
            break;
        }
    }

    return changed;
}
