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

    // OpenGL Staff
    QOpenGLBuffer buffer_main;
    QOpenGLVertexArrayObject vao_main;
    QOpenGLShaderProgram *shader_program_basic;
    QOpenGLBuffer buffer_pc;
    QOpenGLVertexArrayObject vao_pc;

    // Camera
    OpenGLCamera camera;

    // Mesh Data
    TriMesh mesh;
    TriMesh pc;
    std::vector<Vertex> vertex_data_main;
    std::vector<Vertex> vertex_data_pc;

    // Need Update Buffer
    bool buffer_need_update_main;
    bool buffer_need_update_pc;

    // UI control temp
    QPoint current_position_;

    // Helper Functions
    static void TranslateCoodinate(TriMesh &);
    static void GenerateBufferFromMesh(TriMesh &, std::vector<Vertex> &);
    static void GenerateBufferFromPointCloud(TriMesh &, std::vector<Vertex> &);
    static void ApplyUnify(TriMesh &);
};
