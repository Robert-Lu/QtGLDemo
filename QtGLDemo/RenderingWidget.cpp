#include "stdafx.h"
#include "RenderingWidget.h"
#include "Vertex.h"
#include "QTiming.h"
#include "GlobalConfig.h"

#define TIC QTiming::Tic();
#define TOC(s) msg.log(tr(s " in %0 ms.").arg(QTiming::Toc()), INFO_MSG);

#define NORM3(p, q) (sqrtf(powf((p)[0] - (q)[0], 2) + powf((p)[1] - (q)[1], 2) + powf((p)[2] - (q)[2], 2)))
#define BOUND(x, l, h) ((x) > (h) ? (h) : (x) < (l) ? (l) : (x))

RenderingWidget::RenderingWidget(ConsoleMessageManager &_msg, QWidget *parent)
    : QOpenGLWidget(parent), msg(_msg), slicing_position(0.0f), dis_field(nullptr),
buffer_need_update_mesh(true), buffer_need_update_pc(true), 
buffer_need_update_base(true), buffer_need_update_slice(true),
render_config{ RENDER_CONFIG_FILENAME }, algorithm_config{ ALGORITHM_CONFIG_FILENAME }
{
    // Set the focus policy to Strong,
    // then the renderingWidget can accept keyboard input event.
    setFocusPolicy(Qt::StrongFocus);

    //ui.setupUi(this);
    
    camera = OpenGLCamera(
        { render_config.get_float("Default_Camera_X"),
          render_config.get_float("Default_Camera_Y"),
          render_config.get_float("Default_Camera_Z"), },
        { 0.0f, 0.0f, 0.0f });

    background_color = {
        render_config.get_float("Default_Background_Color_R"),
        render_config.get_float("Default_Background_Color_G"),
        render_config.get_float("Default_Background_Color_B"),
    };

    mesh_color = {
        render_config.get_float("Default_Mesh_Color_R"),
        render_config.get_float("Default_Mesh_Color_G"),
        render_config.get_float("Default_Mesh_Color_B"),
    };

    point_clout_color = {
        render_config.get_float("Default_Point_Cloud_Color_R"),
        render_config.get_float("Default_Point_Cloud_Color_G"),
        render_config.get_float("Default_Point_Cloud_Color_B"),
    };

    /*
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
        //// dispose the face normals, as we don't need them anymore
        mesh_.release_face_normals();
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
    */

    DrawCoord(vertex_data_base, render_config.get_float("Coordinate_Length"));

    msg.log("Rendering Widget constructor end.", TRIVIAL_MSG);
}

RenderingWidget::~RenderingWidget()
{
    // Actually destroy our OpenGL information
    vao_mesh.destroy();
    buffer_mesh.destroy();
    delete shader_program_basic_light;
}

void RenderingWidget::ReadMeshFromFile()
{
    // get file name.
    QString filename = QFileDialog::
        getOpenFileName(this, tr("Read Mesh"),
            "./mesh", tr("Mesh Files (*.obj)"));

    // check valid.
    if (filename.isEmpty())
    {
        emit(StatusInfo(tr("Read from file Canceled.")));
        msg.log(tr("Read from file Canceled."), INFO_MSG);
        return;
    }

    OpenMesh::IO::Options opt;
    mesh.request_vertex_normals();  // require vertex normal for mesh.

    TIC;
    if (!OpenMesh::IO::read_mesh(mesh, filename.toStdString()), opt)
    {
        msg.log(tr("Cannot read mesh."), ERROR_MSG);
        throw "up";
    }
    TOC("Read Mesh");

    if (filename.contains("coodtr"))
    {
        msg.log(tr("Coordinate System will be translated."), INFO_MSG);
        TranslateCoodinate(mesh);
    }

    // If the file did not provide vertex normals, then calculate them
    if (!opt.check(OpenMesh::IO::Options::VertexNormal))
    {
        TIC;
        // we need face normals to update the vertex normals
        mesh.request_face_normals();

        // let the mesh_ update the normals
        mesh.update_normals();

        // maybe face normal has future usage.
        //// dispose the face normals, as we don't need them anymore
        //mesh_.release_face_normals();
        TOC("Calculate Vertex Normals");
    }
    else
    {
        msg.log(tr("File provides Vertex Normals."), INFO_MSG);
    }

    // extract every vertex by face-vertex circulator.
    GenerateBufferFromMesh(mesh, vertex_data_mesh);

    buffer_need_update_mesh = true;
}

void RenderingWidget::ReadPointCloudFromFile()
{
    // get file name.
    QString filename = QFileDialog::
        getOpenFileName(this, tr("Read Point Cloud"),
            "./mesh", tr("Point Clouds (*.ply);;Mesh Files (*.obj)"));

    // check valid.
    if (filename.isEmpty())
    {
        emit(StatusInfo(tr("Read from file Canceled.")));
        msg.log(tr("Read from file Canceled."), INFO_MSG);
        return;
    }

    OpenMesh::IO::Options opt; 
    // do not require vertex normal for point cloud.

    TIC;
    if (!OpenMesh::IO::read_mesh(pc, filename.toStdString()), opt)
    {
        msg.log(tr("Cannot read point cloud."), ERROR_MSG);
        throw "up";
    }
    TOC("Read Mesh");

    if (filename.contains("coodtr"))
    {
        msg.log(tr("Coordinate System will be translated."), INFO_MSG);
        TranslateCoodinate(pc);
    }

    // extract every vertex by vertex iterator.
    GenerateBufferFromPointCloud(pc, vertex_data_pc);

    msg.log(tr("Point Count: %0.").arg(pc.n_vertices()), INFO_MSG);

    buffer_need_update_pc = true;
}

void RenderingWidget::SavePointCloudToFile()
{
    // get file name.
    QString filename = QFileDialog::
        getSaveFileName(this, tr("Save Point Cloud"),
            "./mesh", tr("Point Clouds (*.ply);;Mesh Files (*.obj)"));

    // check valid.
    if (filename.isEmpty())
    {
        emit(StatusInfo(tr("Save to file Canceled.")));
        msg.log(tr("Save to file Canceled."), INFO_MSG);
        return;
    }

    OpenMesh::IO::write_mesh(pc, filename.toStdString());
}

void RenderingWidget::ApplyUnifyForMesh()
{
    if (mesh.vertices_empty())
    {
        emit(StatusInfo(tr("Unify on Empty Space.")));
        msg.log(tr("Unify on Empty Space."), INFO_MSG);
        return;
    }

    ApplyUnify(mesh);

    GenerateBufferFromMesh(mesh, vertex_data_mesh);

    buffer_need_update_mesh = true;

    emit(StatusInfo(tr("Unify on Mesh.")));
    msg.log(tr("Unify on Mesh."), INFO_MSG);
}

void RenderingWidget::ApplyUnifyForPointCloud()
{
    if (pc.vertices_empty())
    {
        emit(StatusInfo(tr("Unify on Empty Space.")));
        msg.log(tr("Unify on Empty Space."), INFO_MSG);
        return;
    }

    ApplyUnify(pc);

    GenerateBufferFromPointCloud(pc, vertex_data_pc);

    buffer_need_update_pc = true;

    emit(StatusInfo(tr("Unify on Point Cloud.")));
    msg.log(tr("Unify on Point Cloud."), INFO_MSG);
}

void RenderingWidget::ApplyFlipForMesh(int i)
{
    if (mesh.vertices_empty())
    {
        emit(StatusInfo(tr("Flip on Empty Space.")));
        msg.log(tr("Flip on Empty Space."), INFO_MSG);
        return;
    }

    ApplyFlip(mesh, i);

    GenerateBufferFromMesh(mesh, vertex_data_mesh);

    buffer_need_update_mesh = true;

    char cood = i == 0 ? 'X' : i == 1 ? 'Y' : 'Z';

    emit(StatusInfo(tr("Flip %0 on Mesh.").arg(cood)));
    msg.log(tr("Flip %0 on Mesh.").arg(cood), INFO_MSG);
}

void RenderingWidget::ApplyFlipForPointCloud(int i)
{
    if (pc.vertices_empty())
    {
        emit(StatusInfo(tr("Flip on Empty Space.")));
        msg.log(tr("Flip on Empty Space."), INFO_MSG);
        return;
    }

    ApplyFlip(pc, i);

    GenerateBufferFromPointCloud(pc, vertex_data_pc);

    buffer_need_update_pc = true;

    char cood = i == 0 ? 'X' : i == 1 ? 'Y' : 'Z';

    emit(StatusInfo(tr("Flip %0 on Point Cloud.").arg(cood)));
    msg.log(tr("Flip %0 on Point Cloud.").arg(cood), INFO_MSG);
}

void RenderingWidget::BuildDistanceFieldFromPointCloud()
{
    if (pc.vertices_empty())
    {
        emit(StatusInfo(tr("Build Distance Field Empty Space.")));
        msg.log(tr("Build Distance Field Empty Space."), INFO_MSG);
        return;
    }

    // Build kdTree
    pts.clear();
    for (auto vh : pc.vertices())
    {
        auto x = pc.point(vh)[0];
        auto y = pc.point(vh)[1];
        auto z = pc.point(vh)[2];
        pts.push_back({ x, y, z });
    }

    kdtree = new kdt::kdTree(pts);


    OpenMesh::Vec3f min_pos = {
        algorithm_config.get_float("Domain_Min_X"),
        algorithm_config.get_float("Domain_Min_Y"),
        algorithm_config.get_float("Domain_Min_Z"),
    };

    OpenMesh::Vec3f max_pos = {
        algorithm_config.get_float("Domain_Max_X"),
        algorithm_config.get_float("Domain_Max_Y"),
        algorithm_config.get_float("Domain_Max_Z"),
    };

    // the max time for octree to expand(split).
    int max_div = algorithm_config.get_int("Max_Division_OcTree");

    dis_field = new OcTreeField(min_pos, max_pos, kdtree, pts, max_div);

    auto stat = dis_field->stat();
    msg.log("Build Distance Field on OcTree.", INFO_MSG);
    msg.indent_more();
    msg.log(tr("Minimum size of grid: %0.").arg(stat.min_size), TRIVIAL_MSG);
    msg.log(tr("Total number of OcTree nodes: %0.").arg(stat.grid_cnt), TRIVIAL_MSG);
    msg.indent_more();
    int d = 0;
    for (auto num : stat.depth_cnt)
    {
        msg.log(tr("Number of nodes with depth %0: %1.").arg(d++).arg(num), TRIVIAL_MSG);
    }
    msg.reset_indent();

    UpdateSlicingPlane();
    buffer_need_update_slice = true;
}

void RenderingWidget::ChangeColorBackground()
{
    QColor color = QColorDialog::getColor(Qt::white, this, tr("background color"));
    if (!color.isValid())
        return;

    GLfloat r = (color.red()) / 255.0f;
    GLfloat g = (color.green()) / 255.0f;
    GLfloat b = (color.blue()) / 255.0f;
    background_color = { r, g, b };

    buffer_need_update_base = true;
    update();
}

void RenderingWidget::ChangeColorMesh()
{
    QColor color = QColorDialog::getColor(Qt::white, this, tr("background color"));
    if (!color.isValid())
        return;

    GLfloat r = (color.red()) / 255.0f;
    GLfloat g = (color.green()) / 255.0f;
    GLfloat b = (color.blue()) / 255.0f;
    mesh_color = { r, g, b };

    GenerateBufferFromMesh(mesh, vertex_data_mesh);
    buffer_need_update_mesh = true;
    update();
}

void RenderingWidget::ChangeColorPointCloud()
{
    QColor color = QColorDialog::getColor(Qt::white, this, tr("background color"));
    if (!color.isValid())
        return;

    GLfloat r = (color.red()) / 255.0f;
    GLfloat g = (color.green()) / 255.0f;
    GLfloat b = (color.blue()) / 255.0f;
    point_clout_color = { r, g, b };

    GenerateBufferFromPointCloud(pc, vertex_data_pc);
    buffer_need_update_pc = true;
    update();
}

void RenderingWidget::SyncConfigBundle(ConfigBundle &c)
{
    config_bundle = c;
    UpdateSlicingPlane();
}

void RenderingWidget::UpdateSlicingPlane()
{
    vertex_data_slice.clear();
    
    if (config_bundle.slice_config.use_slice)
    {
        int idmain, idr, idc;
        if (config_bundle.slice_config.slice_x)
        {
            idmain = 0;
            idr = 1;
            idc = 2;
        }
        else if (config_bundle.slice_config.slice_y)
        {
            idmain = 1;
            idr = 0;
            idc = 2;
        }
        else
        {
            idmain = 2;
            idr = 0;
            idc = 1;
        }

        OpenMesh::Vec3f min_pos = {
            algorithm_config.get_float("Domain_Min_X"),
            algorithm_config.get_float("Domain_Min_Y"),
            algorithm_config.get_float("Domain_Min_Z"),
        };

        OpenMesh::Vec3f max_pos = {
            algorithm_config.get_float("Domain_Max_X"),
            algorithm_config.get_float("Domain_Max_Y"),
            algorithm_config.get_float("Domain_Max_Z"),
        };

        float grid_step = algorithm_config.get_float("Grid_Size");

        OpenMesh::Vec3f color{ 1.0f, 0.7f, 0.7f };
        float rr, cc;
        for (float r = min_pos[idr]; r < max_pos[idr]; r += grid_step)
        {
            rr = r + grid_step;
            for (float c = min_pos[idc]; c < max_pos[idc]; c += grid_step)
            {
                cc = c + grid_step;
                OpenMesh::Vec3f posll, poslh, poshl, poshh;
                posll[idmain] = slicing_position;
                posll[idr] = r;
                posll[idc] = c;
                poslh[idmain] = slicing_position;
                poslh[idr] = r;
                poslh[idc] = cc;
                poshl[idmain] = slicing_position;
                poshl[idr] = rr;
                poshl[idc] = c;
                poshh[idmain] = slicing_position;
                poshh[idr] = rr;
                poshh[idc] = cc;

                if (kdtree != nullptr)
                {
                    auto mid = (posll + poshh) / 2.0f;
                    kdt::kdPoint p{ mid[0], mid[1], mid[2] };
                    auto idx = kdtree->nnSearch(p);
                    auto pos = pc.point(pc.vertex_handle(idx));
                    auto dis = NORM3(pos, p);
                    color[0] = BOUND(1.0 - powf(dis, 0.25f), 0.0f, 1.0f);
                }

                vertex_data_slice.push_back({ posll , color });
                vertex_data_slice.push_back({ poslh , color });
                vertex_data_slice.push_back({ poshl , color });
                vertex_data_slice.push_back({ poshh , color });
                vertex_data_slice.push_back({ poslh , color });
                vertex_data_slice.push_back({ poshl , color });
            }
        }
    }

    buffer_need_update_slice = true;
}

void RenderingWidget::initializeGL()
{
    // Initialize OpenGL Back-End
    initializeOpenGLFunctions();

    // Binding Shader and Buffer
    // Basic Light Shader, use Vertex3D
    shader_program_basic_light = new QOpenGLShaderProgram();
    shader_program_basic_light->addShaderFromSourceFile(QOpenGLShader::Vertex, render_config.get_string("Main_Vertex_Shader"));
    shader_program_basic_light->addShaderFromSourceFile(QOpenGLShader::Fragment, render_config.get_string("Main_Freagment_Shader"));
    shader_program_basic_light->link();
    shader_program_basic_light->bind();
    {
        // Buffer for Mesh, draw as GL_TRIANGLES
        buffer_mesh.create();
        buffer_mesh.bind();
        buffer_mesh.setUsagePattern(QOpenGLBuffer::StaticDraw);
        {
            vao_mesh.create();
            vao_mesh.bind();
            {
                shader_program_basic_light->enableAttributeArray(0);
                shader_program_basic_light->enableAttributeArray(1);
                shader_program_basic_light->enableAttributeArray(2);
                shader_program_basic_light->setAttributeBuffer(0, GL_FLOAT, Vertex3D::positionOffset(), Vertex3D::PositionTupleSize, Vertex3D::stride());
                shader_program_basic_light->setAttributeBuffer(1, GL_FLOAT, Vertex3D::colorOffset(), Vertex3D::ColorTupleSize, Vertex3D::stride());
                shader_program_basic_light->setAttributeBuffer(2, GL_FLOAT, Vertex3D::normalOffset(), Vertex3D::NormalTupleSize, Vertex3D::stride());
            }
            vao_mesh.release();
        }
        buffer_mesh.release();

        // Buffer for Point Cloud, draw as GL_POINTS
        buffer_pc.create();
        buffer_pc.bind();
        buffer_pc.setUsagePattern(QOpenGLBuffer::StaticDraw);
        {
            vao_pc.create();
            vao_pc.bind();
            {
                shader_program_basic_light->enableAttributeArray(0);
                shader_program_basic_light->enableAttributeArray(1);
                shader_program_basic_light->enableAttributeArray(2);
                shader_program_basic_light->setAttributeBuffer(0, GL_FLOAT, Vertex3D::positionOffset(), Vertex3D::PositionTupleSize, Vertex3D::stride());
                shader_program_basic_light->setAttributeBuffer(1, GL_FLOAT, Vertex3D::colorOffset(), Vertex3D::ColorTupleSize, Vertex3D::stride());
                shader_program_basic_light->setAttributeBuffer(2, GL_FLOAT, Vertex3D::normalOffset(), Vertex3D::NormalTupleSize, Vertex3D::stride());
            }
            vao_pc.release();
        }
        buffer_pc.release();
    }
    shader_program_basic_light->release();

    // Pure Color Shader, use Vertex2D
    shader_program_pure_color = new QOpenGLShaderProgram();
    shader_program_pure_color->addShaderFromSourceFile(QOpenGLShader::Vertex, render_config.get_string("Pure_Color_Vertex_Shader"));
    shader_program_pure_color->addShaderFromSourceFile(QOpenGLShader::Fragment, render_config.get_string("Pure_Color_Freagment_Shader"));
    shader_program_pure_color->link();
    shader_program_pure_color->bind();
    {
        // Buffer for Base Component, draw as GL_LINES
        buffer_base.create();
        buffer_base.bind();
        buffer_base.setUsagePattern(QOpenGLBuffer::StaticDraw);
        {
            vao_base.create();
            vao_base.bind();
            {
                shader_program_pure_color->enableAttributeArray(0);
                shader_program_pure_color->enableAttributeArray(1);
                shader_program_pure_color->setAttributeBuffer(0, GL_FLOAT, Vertex2D::positionOffset(), Vertex2D::PositionTupleSize, Vertex2D::stride());
                shader_program_pure_color->setAttributeBuffer(1, GL_FLOAT, Vertex2D::colorOffset(), Vertex2D::ColorTupleSize, Vertex2D::stride());
            }
            vao_base.release();
        }
        buffer_base.release();

        // Buffer for Slicing Plane, draw as GL_TRIANGLES
        buffer_slice.create();
        buffer_slice.bind();
        buffer_slice.setUsagePattern((QOpenGLBuffer::StaticDraw));
        {
            vao_slice.create();
            vao_slice.bind();
            {
                shader_program_pure_color->enableAttributeArray(0);
                shader_program_pure_color->enableAttributeArray(1);
                shader_program_pure_color->setAttributeBuffer(0, GL_FLOAT, Vertex2D::positionOffset(), Vertex2D::PositionTupleSize, Vertex2D::stride());
                shader_program_pure_color->setAttributeBuffer(1, GL_FLOAT, Vertex2D::colorOffset(), Vertex2D::ColorTupleSize, Vertex2D::stride());
            }
            vao_slice.release();
        }
        buffer_slice.release();
    }
    shader_program_basic_light->release();


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
    glClearColor(background_color[0], 
        background_color[1],
        background_color[2],
        1.0f);

    QMatrix4x4 mat_model;

    QMatrix4x4 mat_projection;
    mat_projection.perspective(45.0f,
        float(this->width()) / float(this->height()),
        0.1f, 100.f);

    // update buffer for mesh if necessary.
    if (buffer_need_update_mesh)
    {
        vao_mesh.bind();
        {
            buffer_mesh.bind();
            {
                buffer_mesh.allocate(vertex_data_mesh.data(), vertex_data_mesh.size() * Vertex3D::stride());
            }
            buffer_mesh.release();
        }
        vao_mesh.release();

        buffer_need_update_mesh = false;
    }

    // update buffer for point cloud if necessary.
    if (buffer_need_update_pc)
    {
        vao_pc.bind();
        {
            buffer_pc.bind();
            {
                buffer_pc.allocate(vertex_data_pc.data(), vertex_data_pc.size() * Vertex3D::stride());
            }
            buffer_pc.release();
        }
        vao_pc.release();

        buffer_need_update_pc = false;
    }

    // update buffer for base if necessary.
    if (buffer_need_update_base)
    {
        vao_base.bind();
        {
            buffer_base.bind();
            {
                buffer_base.allocate(vertex_data_base.data(), vertex_data_base.size() * Vertex2D::stride());
            }
            buffer_base.release();
        }
        vao_base.release();

        buffer_need_update_base = false;
    }

    // update buffer for slicing component if necessary.
    if (buffer_need_update_slice)
    {
        vao_slice.bind();
        {
            buffer_slice.bind();
            {
                buffer_slice.allocate(vertex_data_slice.data(), vertex_data_slice.size() * Vertex2D::stride());
            }
            buffer_slice.release();
        }
        vao_slice.release();

        buffer_need_update_slice = false;
    }

    shader_program_basic_light->bind();
    {
        vao_mesh.bind();
        {
            shader_program_basic_light->setUniformValue("lightDirFrom", 1.0f, 1.0f, 1.0f);
            shader_program_basic_light->setUniformValue("viewPos", camera.position());
            shader_program_basic_light->setUniformValue("model", mat_model);
            shader_program_basic_light->setUniformValue("view", camera.view_mat());
            shader_program_basic_light->setUniformValue("projection", mat_projection);
            glDrawArrays(GL_TRIANGLES, 0, vertex_data_mesh.size());
        }
        vao_mesh.release();

        vao_pc.bind();
        {
            shader_program_basic_light->setUniformValue("lightDirFrom", 1.0f, 1.0f, 1.0f);
            shader_program_basic_light->setUniformValue("viewPos", camera.position());
            shader_program_basic_light->setUniformValue("model", mat_model);
            shader_program_basic_light->setUniformValue("view", camera.view_mat());
            shader_program_basic_light->setUniformValue("projection", mat_projection);
            glDrawArrays(GL_POINTS, 0, vertex_data_pc.size());
        }
        vao_pc.release();
    }
    shader_program_basic_light->release();

    glDisable(GL_CULL_FACE);

    shader_program_pure_color->bind();
    {
        vao_base.bind();
        {
            shader_program_pure_color->setUniformValue("alpha", 1.0f);
            shader_program_pure_color->setUniformValue("model", mat_model);
            shader_program_pure_color->setUniformValue("view", camera.view_mat());
            shader_program_pure_color->setUniformValue("projection", mat_projection);
            glDrawArrays(GL_LINES, 0, vertex_data_base.size());
        }
        vao_base.release();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        vao_slice.bind();
        {
            auto alpha = render_config.get_float("Slicing_Plane_Alpha");
            shader_program_pure_color->setUniformValue("alpha", alpha);
            shader_program_pure_color->setUniformValue("model", mat_model);
            shader_program_pure_color->setUniformValue("view", camera.view_mat());
            shader_program_pure_color->setUniformValue("projection", mat_projection);
            glDrawArrays(GL_TRIANGLES, 0, vertex_data_slice.size());
        }
        vao_slice.release();
    }
    shader_program_pure_color->release();
}

void RenderingWidget::mousePressEvent(QMouseEvent* e)
{
    setCursor(Qt::ClosedHandCursor);
    current_position_ = e->pos();

    //switch (e->button())
    //{
    //case Qt::LeftButton:
    //    //ptr_arcball_->MouseDown(e->pos());
    //    current_position_ = e->pos();
    //    setCursor(Qt::ClosedHandCursor);
    //    break;
    //case Qt::MidButton:
    //    current_position_ = e->pos();
    //    break;
    //default:
    //    break;
    //}

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
        break;
    case Qt::MidButton:
        camera.move_right_target(-4.0*GLfloat(e->x() - current_position_.x()) / GLfloat(width()) * multiplier);
        camera.move_up_target(+4.0*GLfloat(e->y() - current_position_.y()) / GLfloat(height()) * multiplier);
        break;
    case Qt::RightButton:
        slicing_position += (-2.5*GLfloat(e->x() - current_position_.x()) / GLfloat(width())) * multiplier;
        slicing_position += (+2.5*GLfloat(e->y() - current_position_.y()) / GLfloat(width())) * multiplier;
        UpdateSlicingPlane();
    default:
        break;
    }

    current_position_ = e->pos();
    update();
}

void RenderingWidget::mouseReleaseEvent(QMouseEvent* e)
{
    setCursor(Qt::ArrowCursor);
    current_position_ = e->pos();
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
    bool has_alt = e->modifiers() & Qt::AltModifier;
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
        if (has_alt)
        {
            emit(StatusInfo(QString("target move front")));
            camera.move_back_target(-dis);
        }
        else
        {
            emit(StatusInfo(QString("camera move front")));
            camera.move_back(-dis);
        }
        break;
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        if (has_alt)
        {
            emit(StatusInfo(QString("target move back ")));
            camera.move_back_target(+dis);
        }
        else
        {
            emit(StatusInfo(QString("camera move back ")));
            camera.move_back(+dis);
        }
        break;
    case Qt::Key_Escape:
        exit(0);
        break;
    case Qt::Key_R:
        emit(StatusInfo(QString("reset camera")));
        camera = OpenGLCamera({ render_config.get_float("Default_Camera_X"),
            render_config.get_float("Default_Camera_Y"),
            render_config.get_float("Default_Camera_Z") },
            { 0.0f, 0.0f, 0.0f });
        break;
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

void RenderingWidget::DrawCoord(std::vector<Vertex2D>& D, float len)
{
    OpenMesh::Vec3f origin{ 0.0f, 0.0f, 0.0f };
    OpenMesh::Vec3f x{ len, 0.0f, 0.0f };
    OpenMesh::Vec3f y{ 0.0f, len, 0.0f };
    OpenMesh::Vec3f z{ 0.0f, 0.0f, len };
    OpenMesh::Vec3f red{ 1.0f, 0.0f, 0.0f };
    OpenMesh::Vec3f green{ 0.0f, 1.0f, 0.0f };
    OpenMesh::Vec3f blue{ 0.0f, 0.0f, 1.0f };
    
    D.push_back({ origin, red });
    D.push_back({ x, red });
    D.push_back({ origin, green });
    D.push_back({ y, green });
    D.push_back({ origin, blue });
    D.push_back({ z, blue });
}

void RenderingWidget::TranslateCoodinate(TriMesh& M)
{
    for (auto v : M.vertices())
    {
        auto point = M.point(v);
        decltype(point) point_t{ point[2], point[0], point[1] };
        M.set_point(v, point_t);
    }
}

void RenderingWidget::GenerateBufferFromMesh(TriMesh& M, std::vector<Vertex3D>& D)
{
    OpenMesh::Vec3f color = mesh_color;

    D.clear();
    for (auto f_it : M.faces())
    {
        auto fv_it = M.fv_iter(f_it);

        for (; fv_it; ++fv_it)
        {
            auto vh = *fv_it;
            auto pos = M.point(vh);
            auto nor = M.normal(vh);
            D.push_back({ pos, color, nor });
        }
    }
}

void RenderingWidget::GenerateBufferFromPointCloud(TriMesh& M, std::vector<Vertex3D>& D)
{
    OpenMesh::Vec3f color = point_clout_color;

    D.clear();
    for (auto vh : M.vertices())
    {
        auto pos = M.point(vh);
//        auto nor = M.normal(vh);
        D.push_back({ pos, color, { 0.0f, 0.0f, 1.0f } });
    }
}

void RenderingWidget::ApplyUnify(TriMesh &M)
{
    using OpenMesh::Vec3f;

    Vec3f max_pos(-INFINITY, -INFINITY, -INFINITY);
    Vec3f min_pos(+INFINITY, +INFINITY, +INFINITY);

    for (auto v : M.vertices())
    {
        auto point = M.point(v);
        for (int i = 0; i < 3; i++)
        {
            float t = point[i];
            if (t > max_pos[i])
                max_pos[i] = t;
            if (t < min_pos[i])
                min_pos[i] = t;
        }
    }

    float xmax = max_pos[0], ymax = max_pos[1], zmax = max_pos[2];
    float xmin = min_pos[0], ymin = min_pos[1], zmin = min_pos[2];

    float diffX = xmax - xmin;
    float diffY = ymax - ymin;
    float diffZ = zmax - zmin;
    float diffMax;

    if (diffX < diffY)
        diffMax = diffY;
    else
        diffMax = diffX;
    if (diffMax < diffZ)
        diffMax = diffZ;

    float scale = 1.0f / diffMax;
    Vec3f center((xmin + xmax) / 2.f, (ymin + ymax) / 2.f, (zmin + zmax) / 2.f);
    for (auto v : M.vertices())
    {
        Vec3f pt = M.point(v);
        Vec3f res = (pt - center) * scale;

        M.set_point(v, res);
    }
}

void RenderingWidget::ApplyFlip(TriMesh& M, int i)
{
    using OpenMesh::Vec3f;
    for (auto v : M.vertices())
    {
        auto point = M.point(v);
        point[i] = -point[i];
        M.set_point(v, point);
    }
}
