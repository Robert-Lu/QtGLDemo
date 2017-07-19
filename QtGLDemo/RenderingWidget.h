#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

#include "ConsoleMessageManager.h"
#include "OpenGLCamera.h"
#include "Vertex.h"

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;

class RenderingWidget : public QOpenGLWidget,
                        protected QOpenGLFunctions
{
    Q_OBJECT

public:
    RenderingWidget(ConsoleMessageManager &_msg, QWidget *parent = Q_NULLPTR);
    ~RenderingWidget();

signals:
    void StatusInfo(const QString &);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    void wheelEvent(QWheelEvent *e);

    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

private:
    ConsoleMessageManager &msg;

    // OpenGL Staff
    QOpenGLBuffer buffer_main;
    QOpenGLVertexArrayObject vao_main;
    QOpenGLShaderProgram *shader_program_basic;

    // Camera
    OpenGLCamera camera;

    // Mesh Data
    TriMesh mesh;
    std::vector<Vertex> vertex_data;

    // Need Update Buffer
    bool buffer_need_update_main;

    // UI control temp
    QPoint current_position_;
};
