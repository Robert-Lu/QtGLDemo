#include "stdafx.h"
#include "RenderingWidget.h"
#include "Vertex.h"
#include "QTiming.h"

#define TIC QTiming::Tic();
#define TOC(s) msg.log(tr(s " in %0 ms.").arg(QTiming::Toc()), INFO_MSG);

RenderingWidget::RenderingWidget(ConsoleMessageManager &_msg, QWidget *parent)
    : QOpenGLWidget(parent), msg(_msg), buffer_need_update_main(true)
{
    // Set the focus policy to Strong,
    // then the renderingWidget can accept keyboard input event and response.
    setFocusPolicy(Qt::StrongFocus);

    //ui.setupUi(this);
    camera = OpenGLCamera({ 3.0f, 3.0f, 3.0f }, { 0.0f, 0.0f, 0.0f });

    // read mesh from stdin
    OpenMesh::IO::Options opt;
    mesh.request_vertex_normals();

    TIC;
    if (!OpenMesh::IO::read_mesh(mesh, "./mesh/horse.normalized.obj"), opt)
        //if (!OpenMesh::IO::read_mesh(mesh, "./mesh/contraction/coca3_21_cont_o.coodtr.obj"), opt)
    {
        std::cerr << "Error: Cannot read mesh " << std::endl;
        throw "up";
    }
    TOC("Read Mesh");

    // If the file did not provide vertex normals, then calculate them
    if (!opt.check(OpenMesh::IO::Options::VertexNormal))
    {
        TIC;
        // we need face normals to update the vertex normals
        mesh.request_face_normals();

        // let the mesh_ update the normals
        mesh.update_normals();

        // maybe face normal has future usage.
        ////// dispose the face normals, as we don't need them anymore
        ////mesh_.release_face_normals();
        TOC("Calculate Vertex Normals");
    }
    else
    {
        msg.log(tr("File provides Vertex Normals."), INFO_MSG);
    }

    for (auto f_it : mesh.faces())
    {
        auto fv_it = mesh.fv_iter(f_it);

        for (; fv_it; ++fv_it)
        {
            auto vh = *fv_it;
            auto pos = mesh.point(vh);
            auto nor = mesh.normal(vh);
            vertex_data.push_back({ pos, {1.0f, 1.0f, 1.0f}, nor });
        }
    }

    msg.log("Rendering Widget constructor end.", TRIVIAL_MSG);
}

RenderingWidget::~RenderingWidget()
{
    // Actually destroy our OpenGL information
    vao_main.destroy();
    buffer_main.destroy();
    delete shader_program_basic;
}

void RenderingWidget::initializeGL()
{
    // Initialize OpenGL Back-end
    initializeOpenGLFunctions();

    // Application-specific initialization
    {
        // Create Shader (Do not release until VAO is created)
        shader_program_basic = new QOpenGLShaderProgram();
        shader_program_basic->addShaderFromSourceFile(QOpenGLShader::Vertex, "./shader/BasicPhongVertexShader.vertexshader");
        shader_program_basic->addShaderFromSourceFile(QOpenGLShader::Fragment, "./shader/BasicPhongFragmentShader.fragmentshader");
        shader_program_basic->link();
        shader_program_basic->bind();

        // Create Buffer (Do not release until VAO is created)
        buffer_main.create();
        buffer_main.bind();
        buffer_main.setUsagePattern(QOpenGLBuffer::StaticDraw);

        // Create Vertex Array Object
        vao_main.create();
        vao_main.bind();
        shader_program_basic->enableAttributeArray(0);
        shader_program_basic->enableAttributeArray(1);
        shader_program_basic->enableAttributeArray(2);
        shader_program_basic->setAttributeBuffer(0, GL_FLOAT, Vertex::positionOffset(), Vertex::PositionTupleSize, Vertex::stride());
        shader_program_basic->setAttributeBuffer(1, GL_FLOAT, Vertex::colorOffset(), Vertex::ColorTupleSize, Vertex::stride());
        shader_program_basic->setAttributeBuffer(2, GL_FLOAT, Vertex::normalOffset(), Vertex::NormalTupleSize, Vertex::stride());

        // Release (unbind) all
        vao_main.release();
        buffer_main.release();
        shader_program_basic->release();
    }

    emit(StatusInfo(tr("Ready")));
    msg.log("initializeGL() end.", TRIVIAL_MSG);
}

void RenderingWidget::resizeGL(int w, int h)
{
    update();
    msg.log(tr("resizeGL(%0, %1) end.").arg(w).arg(h), TRIVIAL_MSG);
}

void RenderingWidget::paintGL()
{
    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.5f, 1.0f);

    QMatrix4x4 mat_model;

    QMatrix4x4 mat_projection;
    mat_projection.perspective(45.0f,
        float(this->width()) / float(this->height()),
        0.1f, 100.f);

    if (buffer_need_update_main)
    {
        vao_main.bind();
        {
            buffer_main.bind();
            {
                buffer_main.allocate(vertex_data.data(), vertex_data.size() * Vertex::stride());
            }
            buffer_main.release();
        }
        vao_main.release();

        buffer_need_update_main = false;
    }

    // Render using our shader
    shader_program_basic->bind();
    {
        vao_main.bind();

        shader_program_basic->setUniformValue("lightDirFrom", 1.0f, 1.0f, 1.0f);
        shader_program_basic->setUniformValue("viewPos", camera.position());
        shader_program_basic->setUniformValue("model", mat_model);
        shader_program_basic->setUniformValue("view", camera.view_mat());
        shader_program_basic->setUniformValue("projection", mat_projection);
        glDrawArrays(GL_TRIANGLES, 0, vertex_data.size());

        vao_main.release();
    }
    shader_program_basic->release();
}

void RenderingWidget::mousePressEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::LeftButton:
        //ptr_arcball_->MouseDown(e->pos());
        current_position_ = e->pos();
        setCursor(Qt::ClosedHandCursor);
        break;
    case Qt::MidButton:
        current_position_ = e->pos();
        break;
    default:
        break;
    }

    update();
}

void RenderingWidget::mouseMoveEvent(QMouseEvent* e)
{
    bool has_shift = e->modifiers() & Qt::ShiftModifier;
    bool has_ctrl = e->modifiers() & Qt::ControlModifier;
    bool has_alt = e->modifiers() & Qt::AltModifier;
    float multiplier = 1.0;
    multiplier *= has_shift ? 0.25f : 1.0f;
    multiplier *= has_ctrl ? 3.0f : 1.0f;
    switch (e->buttons())
    {
    case Qt::LeftButton:
        //ptr_arcball_->MouseMove(e->pos());
        if (has_alt)
        {
            camera.move_right_target(-4.0*GLfloat(e->x() - current_position_.x()) / GLfloat(width()) * multiplier);
            camera.move_up_target(+4.0*GLfloat(e->y() - current_position_.y()) / GLfloat(height()) * multiplier);
        }
        else
        {
            camera.move_around_right(-180.0*GLfloat(e->x() - current_position_.x()) / GLfloat(width()) * multiplier);
            camera.move_around_up(+180.0*GLfloat(e->y() - current_position_.y()) / GLfloat(height()) * multiplier);
        }
        current_position_ = e->pos();
        break;
    case Qt::MidButton:
        camera.move_right_target(-4.0*GLfloat(e->x() - current_position_.x()) / GLfloat(width()) * multiplier);
        camera.move_up_target(+4.0*GLfloat(e->y() - current_position_.y()) / GLfloat(height()) * multiplier);
        current_position_ = e->pos();
        break;
    default:
        break;
    }
    update();
}

void RenderingWidget::mouseReleaseEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::LeftButton:
        setCursor(Qt::ArrowCursor);
        break;

    case Qt::RightButton:
        break;
    default:
        break;
    }
}

void RenderingWidget::wheelEvent(QWheelEvent* e)
{
    bool has_shift = e->modifiers() & Qt::ShiftModifier;
    bool has_ctrl = e->modifiers() & Qt::ControlModifier;
    float multiplier = 1.0;
    multiplier *= has_shift ? 0.25f : 1.0f;
    multiplier *= has_ctrl ? 3.0f : 1.0f;
    camera.move_back(-e->delta()*0.0015 * multiplier);

    update();
}

void RenderingWidget::keyPressEvent(QKeyEvent* e)
{
    bool has_shift = e->modifiers() & Qt::ShiftModifier;
    bool has_ctrl = e->modifiers() & Qt::ControlModifier;
    float multiplier = 1.0;
    multiplier *= has_shift ? 0.25f : 1.0f;
    multiplier *= has_ctrl ? 3.0f : 1.0f;

    float angle = 10.0f * multiplier;
    float dis = 0.2f * multiplier;

    switch (e->key())
    {
    case Qt::Key_Enter:
        // Fail to catch Enter use these code.
        emit(StatusInfo(QString("pressed Enter")));
        break;
    case Qt::Key_Space:
        emit(StatusInfo(QString("pressed Space")));
        break;

    case Qt::Key_Left:
        emit(StatusInfo(QString("camera move_around_left  %0 degrees").arg(angle)));
        camera.move_around_right(-angle);
        break;
    case Qt::Key_Right:
        emit(StatusInfo(QString("camera move_around_right %0 degrees").arg(angle)));
        camera.move_around_right(+angle);
        break;
    case Qt::Key_Up:
        emit(StatusInfo(QString("camera move_around_up    %0 degrees").arg(angle)));
        camera.move_around_up(+angle);
        break;
    case Qt::Key_Down:
        emit(StatusInfo(QString("camera move_around_down  %0 degrees").arg(angle)));
        camera.move_around_up(-angle);
        break;

    case Qt::Key_W:
        emit(StatusInfo(QString("target move_around_up    %0 degrees").arg(angle)));
        camera.move_around_up_target(+angle);
        break;
    case Qt::Key_A:
        emit(StatusInfo(QString("target move_around_left  %0 degrees").arg(angle)));
        camera.move_around_right_target(-angle);
        break;
    case Qt::Key_S:
        emit(StatusInfo(QString("target move_around_down  %0 degrees").arg(angle)));
        camera.move_around_up_target(-angle);
        break;
    case Qt::Key_D:
        emit(StatusInfo(QString("target move_around_right %0 degrees").arg(angle)));
        camera.move_around_right_target(+angle);
        break;
    case Qt::Key_Equal:
    case Qt::Key_Plus:
        emit(StatusInfo(QString("move front")));
        camera.move_back(-dis);
        break;
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        emit(StatusInfo(QString("move back ")));
        camera.move_back(+dis);
        break;
    case Qt::Key_Escape:
        exit(0);
        break;
    //case Qt::Key_R:
    //    emit(StatusInfo(QString("reset camera")));
    //    camera = OpenGLCamera(DEFAULT_CAMERA_POSITION, { 0, 0, 0 });
    //    break;
    //case Qt::Key_L:
    //    emit(StatusInfo(QString("light fix switch")));
    //    light_dir_fix_ = !light_dir_fix_;
    //    break;

    default:
        emit(StatusInfo(QString("pressed ") + e->text() +
            QString("(%0)").arg(e->key())));
        break;
    }

    update();
}

void RenderingWidget::keyReleaseEvent(QKeyEvent* e)
{
}
