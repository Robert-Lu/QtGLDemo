#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

#include "ConsoleMessageManager.h"
#include "OpenGLCamera.h"
#include "Vertex.h"
#include "TextConfigLoader.h"
#include "kdTreeSolution.h"
#include "OcTreeFieldSolution.h"
#include "SurfaceSolutionBase.h"

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;

#include <deque>

#include "ConfigBundleStructure.h"

class RenderingWidget : public QOpenGLWidget,
                        protected QOpenGLFunctions
{
    Q_OBJECT

public:
    RenderingWidget(TextConfigLoader &_gui_config, ConsoleMessageManager &_msg, ConfigBundle &config_bundle, QWidget *parent = Q_NULLPTR);
    virtual ~RenderingWidget();

signals:
    void StatusInfo(const QString &);

public slots:
    void ClearMesh();
    void ReadMeshInnerFromFile();
    void ReadMeshOuterFromFile();
    //void ReadMeshFromFile(const QString &);
    //void GenerateSphereMesh();
    void GenerateSphereMeshInner();
    void GenerateSphereMeshOuter();
    void ClearPointCloud();
    void ReadPointCloudFromFile();
    void ReadPointCloudFromFile(const QString &);
    void ClearDistanceField();
    void ReadDistanceFieldFromFile();
    void ReadDistanceFieldFromFile(const QString &);
    void SaveMeshInnerToFile();
    void SaveMeshOuterToFile();
    void SavePointCloudToFile();
    void SaveDistanceFieldToFile();
    void ApplyUnifyForMesh();
    void ApplyUnifyForPointCloud();
    void ApplyFlipForMesh(int i);
    void ApplyFlipForPointCloud(int i);
    void BuildDistanceFieldFromPointCloud();
    void ChangeColorBackground();
    void ChangeColorMesh();
    void ChangeColorPointCloud();
    void SyncConfigBundle(ConfigBundle &);
    void UpdateSlicingPlane(int max_div = 0);
    void UpdateSurface(bool inner, bool outer, int iter = 1);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

    void wheelEvent(QWheelEvent *e) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

private:
    // utility
    ConsoleMessageManager &msg;
    TextConfigLoader &gui_config;
    TextConfigLoader render_config;
    TextConfigLoader algorithm_config;
    ConfigBundle &config_bundle;
    QTimer *timer_auto_update;

    // Mode & Script
    enum Mode { ViewMode, ScriptMode, SelectMode };
    Mode mode;
    QLineEdit *script_lineedit;
    std::deque<QString> script_history;
    std::deque<int> script_history_linecnt;
    enum ScriptHistoryType { ScriptType, InfoType, ErrorType, VoidType };
    std::deque<ScriptHistoryType> script_history_type;

    // OpenGL Staff
    QOpenGLShaderProgram *shader_program_basic_light;
    QOpenGLBuffer buffer_mesh;
    QOpenGLVertexArrayObject vao_mesh;
    QOpenGLBuffer buffer_pc;
    QOpenGLVertexArrayObject vao_pc;

    QOpenGLShaderProgram *shader_program_pure_color;
    QOpenGLBuffer buffer_base;
    QOpenGLVertexArrayObject vao_base;
    QOpenGLBuffer buffer_slice;
    QOpenGLVertexArrayObject vao_slice;

    // kdTree
    kdt::kdTree *kdtree;
    std::vector<kdt::kdPoint> pts;

    // Distance Field 
    OcTreeField *dis_field;
    
    // Surface Solution
    SurfaceSolutionBase *ss;

    // Vertex Data
    TriMesh mesh_inner;
    TriMesh mesh_outer;
    TriMesh pc;
    std::vector<Vertex3D> vertex_data_mesh;
    int vertex_data_mesh_inner_size;
    std::vector<Vertex3D> vertex_data_pc;
    std::vector<Vertex2D> vertex_data_base;
    std::vector<Vertex2D> vertex_data_slice;

    // Range
    std::set<VertexHandle> vertex_range_inner;
    std::set<FaceHandle> face_range_inner;
    std::set<VertexHandle> vertex_range_outer;
    std::set<FaceHandle> face_range_outer;

    // Need Update Buffer
    bool buffer_need_update_mesh;
    bool buffer_need_update_pc;
    bool buffer_need_update_base;
    bool buffer_need_update_slice;

    // Camera
    OpenGLCamera camera;

    // Slicing Position
    float slicing_position;

    // Colors
    OpenMesh::Vec3f background_color;
    OpenMesh::Vec3f mesh_color;
    OpenMesh::Vec3f point_clout_color;

    // UI control temp
    QPoint current_position_;
    bool tagSlicingChanged;

    // Helper Functions
    static void DrawCoord(std::vector<Vertex2D> &, float len = 1.0f);
    static void TranslateCoodinate(TriMesh &);
    void GenerateBufferFromMesh(TriMesh &, std::vector<Vertex3D> &);
    void GenerateBufferFromMesh(std::vector<Vertex3D> &);
    void GenerateBufferFromPointCloud(TriMesh &, std::vector<Vertex3D> &);
    static void ApplyUnify(TriMesh &);
    static void ApplyFlip(TriMesh &, int i);
    void _PopScriptHistory();
    void _InsertScriptHistory(const QString &str, ScriptHistoryType type);
    void _RunScriptLine(std::vector<QString> &script, int recursion_limit = 10);
    void RunScript();
    void ReadMeshFromFile(const QString &, TriMesh &);
    VertexHandle SelectVertexByScreenPositionInner(const QPoint&, float &);
    VertexHandle SelectVertexByScreenPositionOuter(const QPoint&, float &);
    void TopologyMergeInner();
    void TopologyMergeOuter();
};
