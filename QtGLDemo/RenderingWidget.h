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

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;

class RenderingWidget : public QOpenGLWidget,
                        protected QOpenGLFunctions
{
    Q_OBJECT

public:
    RenderingWidget(ConsoleMessageManager &_msg, QWidget *parent = Q_NULLPTR);
    virtual ~RenderingWidget();

signals:
    void StatusInfo(const QString &);

public slots:
    void ReadMeshFromFile();
    void ReadPointCloudFromFile();
    void ApplyUnifyForMesh();
    void ApplyUnifyForPointCloud();
    void ApplyFlipForMesh(int i);
    void ApplyFlipForPointCloud(int i);
    void BuildDistanceFieldFromPointCloud();

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
    ConsoleMessageManager &msg;
    TextConfigLoader render_config;
    TextConfigLoader algorithm_config;

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

    // Vertex Data
    TriMesh mesh;
    TriMesh pc;
    std::vector<Vertex3D> vertex_data_main;
    std::vector<Vertex3D> vertex_data_pc;
    std::vector<Vertex2D> vertex_data_base;
    std::vector<Vertex2D> vertex_data_slice;

    // Need Update Buffer
    bool buffer_need_update_mesh;
    bool buffer_need_update_pc;
    bool buffer_need_update_base;
    bool buffer_need_update_slice;

    // Camera
    OpenGLCamera camera;

    // Background Color
    OpenMesh::Vec3f background_color;

    // UI control temp
    QPoint current_position_;

    // Helper Functions
    static void DrawCoord(std::vector<Vertex2D> &, float len = 1.0f);
    static void TranslateCoodinate(TriMesh &);
    void GenerateBufferFromMesh(TriMesh &, std::vector<Vertex3D> &);
    void GenerateBufferFromPointCloud(TriMesh &, std::vector<Vertex3D> &);
    static void ApplyUnify(TriMesh &);
    static void ApplyFlip(TriMesh &, int i);
};
