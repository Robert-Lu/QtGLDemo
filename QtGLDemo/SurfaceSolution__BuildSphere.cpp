#include "stdafx.h"
#include "SurfaceSolutionBase.h"
#include <cmath>


/**
 * \brief convert from Cartesian to spherical coordinate with r given
 * \param p Vec3f (x, y, z)
 * \param r sqrt(x^2 + y^2 + z^2)
 * \return Vec3f (r, theta, phi)
 */
inline Vec3f to_spherical_coord_with_const_r(Vec3f p, float r)
{
    float x = p[0];
    float y = p[1];
    float z = p[2];

    float theta = acosf(z / r);
    float phi = atan2f(y, x);

    return{ r, theta, phi };
}


/**
* \brief convert from spherical to Cartesian coordinate with r given
* \param p Vec3f (r, theta, phi)
* \param r sqrt(x^2 + y^2 + z^2)
* \return Vec3f (x, y, z)
*/
inline Vec3f to_cartesian_coord_with_const_r(Vec3f p, float r)
{
    float theta = p[1];
    float phi = p[2];

    float x = r * sinf(theta) * cosf(phi);
    float y = r * sinf(theta) * sinf(phi);
    float z = r * cosf(theta);

    return{ x, y, z };
}


/**
 * \brief calculate the mid point of p and q in spherical coordinates.
 * \param p first point
 * \param q second point
 * \param r radio
 * \return the mid point of p and q in spherical coordinates
 */
inline Vec3f mid_point_spherical_with_const_r(Vec3f p, Vec3f q, float r)
{
    Vec3f t = p + q;
    return t.normalized() * r;
}


/**
 * \brief Build an Octahedron.
 * 
 * What is octahedron: https://en.wikipedia.org/wiki/Octahedron
 * 
 * \param mesh      ref to mesh, will change
 * \param r         radio for vertices to central point
 * \param clear     whether clear the mesh
 */
void SurfaceSolutionBase::BuildOctahedron(TriMesh& mesh, float r, bool clear)
{
    if (clear)
        mesh.clear();

    // generate vertices
    VertexHandle vhandle[6];
    vhandle[0] = mesh.add_vertex(Point(0.0f, 0.0f,    r));  // up
    vhandle[1] = mesh.add_vertex(Point(   r, 0.0f, 0.0f));  // +x
    vhandle[2] = mesh.add_vertex(Point(0.0f,    r, 0.0f));  // +y
    vhandle[3] = mesh.add_vertex(Point(  -r, 0.0f, 0.0f));  // -x
    vhandle[4] = mesh.add_vertex(Point(0.0f,   -r, 0.0f));  // -y
    vhandle[5] = mesh.add_vertex(Point(0.0f, 0.0f,   -r));  // bottom

    // generate (quadrilateral) faces
    std::vector<VertexHandle>  face_vhandles;
    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[2]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[3]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[3]);
    face_vhandles.push_back(vhandle[4]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[4]);
    face_vhandles.push_back(vhandle[1]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[1]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[3]);
    face_vhandles.push_back(vhandle[2]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[4]);
    face_vhandles.push_back(vhandle[3]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[4]);
    mesh.add_face(face_vhandles);
}


/**
* \brief Build an Icosahedron.
*
* What is Icosahedron: https://en.wikipedia.org/wiki/Icosahedron
*
* \param mesh      ref to mesh, will change
* \param r         radio for vertices to central point
* \param clear     whether clear the mesh
*/
void SurfaceSolutionBase::BuildIcosahedron(TriMesh& mesh, float r, bool clear)
{
    if (clear)
        mesh.clear();

    // generate vertices
    VertexHandle vhandle[12];

    vhandle[0] = mesh.add_vertex(Point(0.000, 1.000, 0.000) * r);
    vhandle[1] = mesh.add_vertex(Point(0.894, 0.447, 0.000) * r);
    vhandle[2] = mesh.add_vertex(Point(0.276, 0.447, 0.851) * r);
    vhandle[3] = mesh.add_vertex(Point(-0.724, 0.447, 0.526) * r);
    vhandle[4] = mesh.add_vertex(Point(-0.724, 0.447, -0.526) * r);
    vhandle[5] = mesh.add_vertex(Point(0.276, 0.447, -0.851) * r);
    vhandle[6] = mesh.add_vertex(Point(0.724, -0.447, 0.526) * r);
    vhandle[7] = mesh.add_vertex(Point(-0.276, -0.447, 0.851) * r);
    vhandle[8] = mesh.add_vertex(Point(-0.894, -0.447, 0.000) * r);
    vhandle[9] = mesh.add_vertex(Point(-0.276, -0.447, -0.851) * r);
    vhandle[10] = mesh.add_vertex(Point(0.724, -0.447, -0.526) * r);
    vhandle[11] = mesh.add_vertex(Point(0.000, -1.000, 0.000) * r);

    int face_indices[20][3] =
    {
        { 11, 9, 10 },
        { 11, 8, 9 },
        { 11, 7, 8 },
        { 11, 6, 7 },
        { 11, 10, 6 },
        { 0, 5, 4 },
        { 0, 4, 3 },
        { 0, 3, 2 },
        { 0, 2, 1 },
        { 0, 1, 5 },
        { 10, 9, 5 },
        { 9, 8, 4 },
        { 8, 7, 3 },
        { 7, 6, 2 },
        { 6, 10, 1 },
        { 5, 9, 4 },
        { 4, 8, 3 },
        { 3, 7, 2 },
        { 2, 6, 1 },
        { 1, 10, 5 },
    };

    for (int i = 0; i < 20; i++)
        mesh.add_face(
            vhandle[face_indices[i][0]],
            vhandle[face_indices[i][1]],
            vhandle[face_indices[i][2]]
        );
}


/**
 * \brief Build a sphere by dividing an octahedron.
 * \param mesh      ref to mesh, will change
 * \param r         radio for vertices to central point
 * \param clear     whether clear the mesh
 */
void SurfaceSolutionBase::BuildSphere(TriMesh& mesh, float r, int max_div, bool clear)
{
    if (clear)
        mesh.clear();

    BuildIcosahedron(mesh, r);

    for (int iter = 0; iter < max_div; iter++)
    {
        // Here we will record all the faces we want to add
        // because we cannot alter faces when iterating the faces.
        using FaceToAdd = std::array<VertexHandle, 3>;
        std::vector<FaceToAdd> face_to_add;
        // Set of Face Handles that have already expanded.
        std::set<FaceHandle> face_expanded;
        // Map to check Vertex Handle by its related Half Edge Handle.
        std::map<HalfEdgeHandle, VertexHandle> map_heh_vh;

        for (auto fh : mesh.faces())
        {
            HalfEdgeHandle he[3];
            FaceHandle face[3];

            he[0] = mesh.halfedge_handle(fh);
            face[0] = mesh.opposite_face_handle(he[0]);
            he[1] = mesh.next_halfedge_handle(he[0]);
            face[1] = mesh.opposite_face_handle(he[1]);
            he[2] = mesh.next_halfedge_handle(he[1]);
            face[2] = mesh.opposite_face_handle(he[2]);

            // The original vertices.
            VertexHandle ori_vex[3];
            // Add new vertices to each edge and record their handle,
            // the vertices may have already calculated.
            VertexHandle mid_vex[3];
            for (int i = 0; i < 3; i++)
            {
                ori_vex[i] = mesh.from_vertex_handle(he[i]);
                if (face_expanded.find(face[i]) == face_expanded.end())
                {
                    // if the opposite face has not expanded,
                    // get mid position of the edge.
                    auto mid_pos = mid_point_spherical_with_const_r(
                        mesh.point(mesh.to_vertex_handle(he[i])),
                        mesh.point(mesh.from_vertex_handle(he[i])),
                        r
                        );
                    mid_vex[i] = mesh.add_vertex(mid_pos);
                    map_heh_vh[he[i]] = mid_vex[i];
                }
                else
                {
                    // else, the opposite face has expanded,
                    // use the previous vertex.
                    mid_vex[i] = map_heh_vh[mesh.opposite_halfedge_handle(he[i])];
                }
            }

            // Record faces to add.
            face_to_add.push_back({ ori_vex[0], mid_vex[0], mid_vex[2] });
            face_to_add.push_back({ ori_vex[1], mid_vex[1], mid_vex[0] });
            face_to_add.push_back({ ori_vex[2], mid_vex[2], mid_vex[1] });
            face_to_add.push_back({ mid_vex[0], mid_vex[1], mid_vex[2] });

            // tag this face as expanded
            face_expanded.insert(fh);
        }

        // Delete all the faces and keep all the vertices as isolated.
        mesh.request_face_status();
        for (auto fh : face_expanded)
            mesh.delete_face(fh, false);
        mesh.garbage_collection();

        for (auto farr : face_to_add)
            mesh.add_face(farr[0], farr[1], farr[2]);
    }
}
