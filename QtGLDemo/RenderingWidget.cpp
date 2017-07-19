#include "stdafx.h"
#include "RenderingWidget.h"
#include "Vertex.h"

// Create a colored triangle
static const Vertex sg_vertexes[] = {
    Vertex(QVector3D(0.00f,  0.75f, 1.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)),
    Vertex(QVector3D(0.75f, -0.75f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)),
    Vertex(QVector3D(-0.75f, -0.75f, 1.0f), QVector3D(0.0f, 0.0f, 1.0f), QVector3D(1.0f, 0.0f, 0.0f)),
};
RenderingWidget::RenderingWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    //ui.setupUi(this);
}

RenderingWidget::~RenderingWidget()
{
    // Actually destroy our OpenGL information
    m_object.destroy();
    m_vertex.destroy();
    delete m_program;
}

void RenderingWidget::initializeGL()
{
    // Initialize OpenGL Backend
    initializeOpenGLFunctions();

    // Set global information
    glClearColor(0.0f, 0.0f, 0.5f, 1.0f);

    // Application-specific initialization
    {
        // Create Shader (Do not release until VAO is created)
        m_program = new QOpenGLShaderProgram();
        m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "./shader/BasicPhongVertexShader.vertexshader");
        m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "./shader/BasicPhongFragmentShader.fragmentshader");
        m_program->link();
        m_program->bind();

        // Create Buffer (Do not release until VAO is created)
        m_vertex.create();
        m_vertex.bind();
        m_vertex.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_vertex.allocate(sg_vertexes, sizeof(sg_vertexes));

        // Create Vertex Array Object
        m_object.create();
        m_object.bind();
        m_program->enableAttributeArray(0);
        m_program->enableAttributeArray(1);
        m_program->enableAttributeArray(2);
        m_program->setAttributeBuffer(0, GL_FLOAT, Vertex::positionOffset(), Vertex::PositionTupleSize, Vertex::stride());
        m_program->setAttributeBuffer(1, GL_FLOAT, Vertex::colorOffset(), Vertex::ColorTupleSize, Vertex::stride());
        m_program->setAttributeBuffer(2, GL_FLOAT, Vertex::normalOffset(), Vertex::NormalTupleSize, Vertex::stride());

        // Release (unbind) all
        m_object.release();
        m_vertex.release();
        m_program->release();
    }

    emit(StatusInfo(tr("Ready")));
}

void RenderingWidget::resizeGL(int w, int h)
{
}

void RenderingWidget::paintGL()
{
    // Clear
    glClear(GL_COLOR_BUFFER_BIT);

    // Render using our shader
    m_program->bind();
    {
        m_object.bind();
        glDrawArrays(GL_TRIANGLES, 0, sizeof(sg_vertexes) / sizeof(sg_vertexes[0]));
        m_object.release();
    }
    m_program->release();
}