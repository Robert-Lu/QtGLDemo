#pragma once

#include "ConsoleMessageManager.h"
#include "TextConfigLoader.h"
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
    MeshRefineSolution(TriMesh &s, ConsoleMessageManager &m, TextConfigLoader &ac, std::map<VertexHandle, float> &);
    ~MeshRefineSolution(); 
    bool refine();
    bool refine(const QString &pre);

protected:  
    TriMesh &mesh;
    ConsoleMessageManager &msg;
    TextConfigLoader &algorithm_config;

    // Tension
    std::map<VertexHandle, float> &map_tension_inner;


    int _Split(float edge_split_tolerance, bool& changed);
    int _Flip(float edge_flip_tolerance, bool& changed);
    int _Collapse(float edge_collapse_tolerance, bool& changed);
};