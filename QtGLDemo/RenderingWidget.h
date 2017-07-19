#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
//#include "ui_RenderingWidget.h"

class RenderingWidget : public QOpenGLWidget,
                        protected QOpenGLFunctions
{
    Q_OBJECT

public:
    RenderingWidget(QWidget *parent = Q_NULLPTR);
    ~RenderingWidget();

signals:
    void StatusInfo(const QString &);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

private:
    //Ui::RenderingWidget ui

    // OpenGL State Information
    QOpenGLBuffer m_vertex;
    QOpenGLVertexArrayObject m_object;
    QOpenGLShaderProgram *m_program;
};
