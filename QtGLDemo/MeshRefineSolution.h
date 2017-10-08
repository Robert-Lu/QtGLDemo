#pragma once

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/Status.hh>
using TriMesh = OpenMesh::TriMesh_ArrayKernelT<>;
using VertexHandle = TriMesh::VertexHandle;
using FaceHandle = TriMesh::FaceHandle;
using HalfEdgeHandle = TriMesh::HalfedgeHandle;
using Point = TriMesh::Point;
using OpenMesh::Vec3f;

class MeshRefineSolution
{
public:
    MeshRefineSolution(TriMesh &s, ConsoleMessageManager &m, TextConfigLoader &ac);
    ~MeshRefineSolution();
    bool refine();

protected:
    TriMesh &mesh;
    ConsoleMessageManager &msg;
    TextConfigLoader &algorithm_config;

    int _Split(float edge_split_tolerance, bool& changed);
    int _Flip(float edge_flip_tolerance, bool& changed);
    int _Collapse(float edge_collapse_tolerance, bool& changed);
};