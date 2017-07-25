#pragma once

#include "kdTreeSolution.h"
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#define NORM3(p, q) (sqrtf(powf((p)[0] - (q)[0], 2) + powf((p)[1] - (q)[1], 2) + powf((p)[2] - (q)[2], 2)))
#define BOUND(x, l, h) ((x) > (h) ? (h) : (x) < (l) ? (l) : (x))
#define SPVEC3(v) (v)[0], (v)[1], (v)[2]
typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;

struct OcNode
{
    // data
    float value;
    OpenMesh::Vec3f dir;
    // meta
    OpenMesh::Vec3f min_pos; // mid position of the grid
    OpenMesh::Vec3f max_pos; // mid position of the grid
    float size;
    // children
    OcNode **children;

    OcNode() = default;
    OcNode(OpenMesh::Vec3f &min, OpenMesh::Vec3f &max, float s)
        : min_pos(min), max_pos(max), size(s), children(nullptr) { }
};

class OcTreeField
{
public:
    OcTreeField(OpenMesh::Vec3f min_pos, OpenMesh::Vec3f max_pos, 
        kdt::kdTree* kdt, std::vector<kdt::kdPoint> *_pts, 
            int _max_div = 8);
    OcTreeField(QDataStream &in);
    ~OcTreeField();
    OcNode find(OpenMesh::Vec3f);
    bool save_to_file(QDataStream &out);

    struct StatBundle
    {
        int grid_cnt;
        float min_size;
        std::vector<int> depth_cnt;
    } stat_bundle;
    StatBundle stat() { return stat_bundle; }

    OcNode *tree;
    kdt::kdTree *kdtree;
    std::vector<kdt::kdPoint> *pts;
    int max_div;
    // stats

private:
    void _expand(OcNode *root, int limit);
    OcNode _find(OcNode *root, OpenMesh::Vec3f pos);
    OcNode *_rebuild(QDataStream& in);
    void _save(QDataStream& out, OcNode *root);

};

/* header only implementation */

inline OcTreeField::OcTreeField(OpenMesh::Vec3f min_pos, OpenMesh::Vec3f max_pos, kdt::kdTree* kdt, std::vector<kdt::kdPoint> *_pts, int _max_div)
    : max_div(_max_div), kdtree(kdt), pts(_pts)
{
    float size = NORM3(max_pos, min_pos);
    stat_bundle.grid_cnt = 1;
    stat_bundle.min_size = size;
    stat_bundle.depth_cnt = std::vector<int>(_max_div + 1, 0);

    tree = new OcNode{ min_pos, max_pos, size };
    stat_bundle.depth_cnt[0] += 1;
    _expand(tree, _max_div);
}

inline OcNode *OcTreeField::_rebuild(QDataStream& in)
{
    OcNode *root = new OcNode();
    in >> root->min_pos[0] >> root->min_pos[1] >> root->min_pos[2];
    in >> root->max_pos[0] >> root->max_pos[1] >> root->max_pos[2];
    in >> root->size;
    in >> root->value;
    in >> root->dir[0] >> root->dir[1] >> root->dir[2];

    bool has_children;
    in >> has_children;
    if (has_children)
    {
        root->children = new OcNode*[8];
        for (int i = 0; i < 8; i++)
        {
            root->children[i] = _rebuild(in);
        }
    }

    return root;
}

inline OcTreeField::OcTreeField(QDataStream& in)
{
    // rebuild stat
    qint32 temp;
    in >> temp;
    stat_bundle.grid_cnt = temp;
    // (float)
    in >> stat_bundle.min_size;
    int size;
    in >> temp;
    size = temp;
    stat_bundle.depth_cnt = std::vector<int>(size, 0);
    for (int i = 0; i < size; i++)
    {
        in >> temp;
        stat_bundle.depth_cnt[i] = temp;
    }

    // rebuild main
    tree = _rebuild(in);
}

inline OcTreeField::~OcTreeField()
{
}

inline OcNode OcTreeField::find(OpenMesh::Vec3f pos)
{
    return _find(tree, pos);
}

inline void OcTreeField::_save(QDataStream& out, OcNode *root)
{
    out << root->min_pos[0] << root->min_pos[1] << root->min_pos[2];
    out << root->max_pos[0] << root->max_pos[1] << root->max_pos[2];
    out << root->size;
    out << root->value;
    out << root->dir[0] << root->dir[1] << root->dir[2];
    bool has_children = (root->children != nullptr);
    out << has_children;
    if (has_children)
    {
        for (int i = 0; i < 8; i++)
        {
            _save(out, root->children[i]);
        }
    }
}

inline bool OcTreeField::save_to_file(QDataStream& out)
{
    if (tree == nullptr)
        return false;

    // save stat
    out << (qint32)stat_bundle.grid_cnt;
    out << stat_bundle.min_size;
    out << (qint32)stat_bundle.depth_cnt.size();
    for (int i = 0; i < stat_bundle.depth_cnt.size(); i++)
        out << (qint32)stat_bundle.depth_cnt[i];

    // save main
    _save(out, tree);
    return true;
}

inline void OcTreeField::_expand(OcNode* root, int limit)
{
    const auto &min_pos = root->min_pos;
    const auto &max_pos = root->max_pos;
    const OpenMesh::Vec3f mid_pos = 0.5f * (min_pos + max_pos);

    kdt::kdPoint p{ SPVEC3(mid_pos) };
    auto idx = kdtree->nnSearch(p);
    auto pos = (*pts)[idx];
    auto dis = NORM3(pos, p);

    root->value = dis;

    // when distance less than grid size, expand
    if (dis < root->size / 2.0f && limit > 0)
    {
        stat_bundle.grid_cnt += 7;
        stat_bundle.depth_cnt[max_div - limit + 1] += 8;
        stat_bundle.depth_cnt[max_div - limit] -= 1;
        if (root->size / 2 < stat_bundle.min_size)
            stat_bundle.min_size = root->size / 2;

        root->children = new OcNode*[8];
        auto minx = min_pos[0];
        auto miny = min_pos[1];
        auto minz = min_pos[2];
        auto maxx = max_pos[0];
        auto maxy = max_pos[1];
        auto maxz = max_pos[2];
        auto midx = mid_pos[0];
        auto midy = mid_pos[1];
        auto midz = mid_pos[2];
        root->children[0] = new OcNode(
            OpenMesh::Vec3f{ midx, midy, midz }, OpenMesh::Vec3f{ maxx, maxy, maxz },
            root->size / 2);
        root->children[1] = new OcNode(
            OpenMesh::Vec3f{ minx, midy, midz }, OpenMesh::Vec3f{ midx, maxy, maxz },
            root->size / 2);
        root->children[2] = new OcNode(
            OpenMesh::Vec3f{ midx, miny, midz }, OpenMesh::Vec3f{ maxx, midy, maxz },
            root->size / 2);
        root->children[3] = new OcNode(
            OpenMesh::Vec3f{ minx, miny, midz }, OpenMesh::Vec3f{ midx, midy, maxz },
            root->size / 2);
        root->children[4] = new OcNode(
            OpenMesh::Vec3f{ midx, midy, minz }, OpenMesh::Vec3f{ maxx, maxy, midz },
            root->size / 2);
        root->children[5] = new OcNode(
            OpenMesh::Vec3f{ minx, midy, minz }, OpenMesh::Vec3f{ midx, maxy, midz },
            root->size / 2);
        root->children[6] = new OcNode(
            OpenMesh::Vec3f{ midx, miny, minz }, OpenMesh::Vec3f{ maxx, midy, midz },
            root->size / 2);
        root->children[7] = new OcNode(
            OpenMesh::Vec3f{ minx, miny, minz }, OpenMesh::Vec3f{ midx, midy, midz },
            root->size / 2);
        for (int i = 0; i < 8; i++)
            _expand(root->children[i], limit - 1);
    }
}

inline OcNode OcTreeField::_find(OcNode* root, OpenMesh::Vec3f pos)
{
    if (root->children == nullptr)
    {
        return *root;
    }

    auto x = pos[0];
    auto y = pos[1];
    auto z = pos[2];

    auto mid_pos = (root->max_pos + root->min_pos) / 2;
    auto mx = mid_pos[0];
    auto my = mid_pos[1];
    auto mz = mid_pos[2];

    int index = 0;
    if (z < mz)
        index += 4;
    if (y < my)
        index += 2;
    if (x < mx)
        index += 1;

    return _find(root->children[index], pos);
}
