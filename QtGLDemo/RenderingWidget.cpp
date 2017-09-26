#include "stdafx.h"
#include "RenderingWidget.h"
#include "ScriptLoader.h"
#include "Vertex.h"
#include "QTiming.h"
#include "GlobalConfig.h"
//#include "SurfaceSolutionEigen.h"
//#include "SurfaceSolutionMatlab.h"
#include "SurfaceSolutionNeo.h"

#include <QFileInfo>
#include <QFontMetrics>

#define TIC QTiming::Tic();
#define TOC(s) {\
    QString prompt = tr(s " in %0 ms.").arg(QTiming::Toc());\
    msg.log(prompt, INFO_MSG);\
    emit(StatusInfo(prompt));\
}

#define NORM3(p, q) (sqrtf(powf((p)[0] - (q)[0], 2) + powf((p)[1] - (q)[1], 2) + powf((p)[2] - (q)[2], 2)))
#define BOUND(x, l, h) ((x) > (h) ? (h) : (x) < (l) ? (l) : (x))

RenderingWidget::RenderingWidget(TextConfigLoader &_gui_config, ConsoleMessageManager &_msg, ConfigBundle &cb, QWidget *parent)
    : QOpenGLWidget(parent), msg(_msg), slicing_position(0.0f), config_bundle(cb),
dis_field(nullptr), ss(nullptr), mode{ ViewMode }, gui_config(_gui_config),
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

    timer_auto_update = new QTimer(this);
    connect(timer_auto_update, &QTimer::timeout, this, [this]() {
        update();
    });
    timer_auto_update->start(50);
}

RenderingWidget::~RenderingWidget()
{
    // Actually destroy our OpenGL information
    vao_mesh.destroy();
    buffer_mesh.destroy();
    delete shader_program_basic_light;
}

inline bool fileExists(QString path) {
    QFileInfo check_file(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()) {
        return true;
    }
    else {
        return false;
    }
}

void RenderingWidget::ClearMesh()
{
    mesh.clear();

    GenerateBufferFromMesh(mesh, vertex_data_mesh);
    buffer_need_update_mesh = true;

    QString m{ "clear mesh." };
    msg.log(m, INFO_MSG);
    _InsertScriptHistory(m, InfoType);
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

    ReadMeshFromFile(filename);
}

void RenderingWidget::ReadMeshFromFile(const QString &filename)
{
    // check valid.
    if (filename.isEmpty() || !fileExists(filename))
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
    OpenMesh::IO::read_mesh(mesh, filename.toStdString());
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

    // Surface Solution unbind
    if (ss != nullptr)
    {
        delete ss;
        ss = nullptr;
    }

    _InsertScriptHistory(QString("read mesh from file, size=(V:%0, E:%1, F:%2)")
        .arg(mesh.n_vertices()).arg(mesh.n_edges()).arg(mesh.n_faces()),
        InfoType);
}

void RenderingWidget::GenerateSphereMesh()
{
    SurfaceSolutionBase::BuildSphere(mesh, 
        algorithm_config.get_float("Shpere_Radio", 1.0f),
        algorithm_config.get_int("Shpere_Max_Division", 5), true);
    // Update the vertex normals
    mesh.request_vertex_normals();  
    mesh.request_face_normals();
    mesh.update_normals();

    // Extract every vertex by face-vertex circulator.
    GenerateBufferFromMesh(mesh, vertex_data_mesh);

    buffer_need_update_mesh = true;

    // Surface Solution unbind
    if (ss != nullptr)
    {
        delete ss;
        ss = nullptr;
    }
}

void RenderingWidget::ClearPointCloud()
{
    pc.clear();

    GenerateBufferFromPointCloud(pc, vertex_data_pc);
    buffer_need_update_pc = true;

    QString m{ "clear point cloud." };
    msg.log(m, INFO_MSG);
    _InsertScriptHistory(m, InfoType);
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
    
    ReadPointCloudFromFile(filename);
}

void RenderingWidget::ReadPointCloudFromFile(const QString &filename)
{
    // check valid.
    if (filename.isEmpty() || !fileExists(filename))
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
    TOC("Read Point Cloud");

    if (filename.contains("coodtr"))
    {
        msg.log(tr("Coordinate System will be translated."), INFO_MSG);
        TranslateCoodinate(pc);
    }

    // extract every vertex by vertex iterator.
    GenerateBufferFromPointCloud(pc, vertex_data_pc);
    buffer_need_update_pc = true;

    msg.log(tr("Point Count: %0.").arg(pc.n_vertices()), INFO_MSG);

    _InsertScriptHistory(QString("read point cloud from file, size=%0")
        .arg(pc.n_vertices()),
        InfoType);
}

void RenderingWidget::ClearDistanceField()
{
    if (dis_field != nullptr)
    {
        delete dis_field;
        dis_field = nullptr;
    }

    QString m{ "clear field." };
    msg.log(m, INFO_MSG);
    _InsertScriptHistory(m, InfoType);
}

void RenderingWidget::ReadDistanceFieldFromFile()
{
    // get file name.
    QString filename = QFileDialog::
        getOpenFileName(this, tr("Open Distance Field (as OcTree)"),
            "./mesh", tr("OcTree Field (*.octree)"));

    // check valid.
    if (filename.isEmpty())
    {
        emit(StatusInfo(tr("Read from file Canceled.")));
        msg.log(tr("Read from file Canceled."), INFO_MSG);
        return;
    }

    ReadDistanceFieldFromFile(filename);
}

void RenderingWidget::ReadDistanceFieldFromFile(const QString &filename)
{
    if (filename.isEmpty() || !fileExists(filename))
    {
        emit(StatusInfo(tr("Read from file Canceled.")));
        msg.log(tr("Read from file Canceled."), INFO_MSG);
        return;
    }

    if (dis_field != nullptr)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Test",
            "Distance Field already exist, confirm to overwrite?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
        {
            emit(StatusInfo(tr("Read from file Canceled.")));
            msg.log(tr("Read from file Canceled."), INFO_MSG);
            return;
        }
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        emit(StatusInfo(tr("Read from file Failed.")));
        msg.log(tr("Read from file Failed."), INFO_MSG);
        return;
    }

    TIC;
    QDataStream in(&file);
    dis_field = new OcTreeField(in);
    file.close();
    TOC("Read Distance Field from file");

    auto stat = dis_field->stat();
    msg.log("Build Distance Field on OcTree.", INFO_MSG);
    _InsertScriptHistory("read field from file.", InfoType);
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
}

void RenderingWidget::SaveMeshToFile()
{
    // get file name.
    QString filename = QFileDialog::
        getSaveFileName(this, tr("Save Mesh"),
            "./mesh", tr("Mesh Files (*.obj)"));

    // check valid.
    if (filename.isEmpty())
    {
        emit(StatusInfo(tr("Save to file Canceled.")));
        msg.log(tr("Save to file Canceled."), INFO_MSG);
        return;
    }

    OpenMesh::IO::write_mesh(mesh, filename.toStdString());
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

void RenderingWidget::SaveDistanceFieldToFile()
{    
    // get file name.
    QString filename = QFileDialog::
        getSaveFileName(this, tr("Save Distance Field (as OcTree)"),
            "./mesh", tr("OcTree Field (*.octree)"));

    // check valid.
    if (filename.isEmpty())
    {
        emit(StatusInfo(tr("Save to file Canceled.")));
        msg.log(tr("Save to file Canceled."), INFO_MSG);
        return;
    }
    if (dis_field == nullptr)
    {
        emit(StatusInfo(tr("Dis Field is Empty.")));
        msg.log(tr("Dis Field is Empty."), INFO_MSG);
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        emit(StatusInfo(tr("Save to file Failed.")));
        msg.log(tr("Save to file Failed."), INFO_MSG);
        return;
    }

    TIC;
    QDataStream out(&file);
    dis_field->save_to_file(out);
    file.close();
    TOC("Save Distance Field to file");
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

    TIC;
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

    if (dis_field != nullptr)
    {
        delete dis_field;
        dis_field = nullptr;
    }
    dis_field = new OcTreeField(min_pos, max_pos, kdtree, &pts, max_div);

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
    TOC("Build Distance Field from Point Cloud");

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

void RenderingWidget::UpdateSlicingPlane(int max_div_set)
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
        OpenMesh::Vec3f color{ 0.6f, 0.7f, 0.9f };

        if (dis_field == nullptr)
        {
            OpenMesh::Vec3f posll, poslh, poshl, poshh;
            posll[idmain] = slicing_position;
            posll[idr] = min_pos[idr];
            posll[idc] = min_pos[idc];
            poslh[idmain] = slicing_position;
            poslh[idr] = min_pos[idr];
            poslh[idc] = max_pos[idc];
            poshl[idmain] = slicing_position;
            poshl[idr] = max_pos[idr];
            poshl[idc] = min_pos[idc];
            poshh[idmain] = slicing_position;
            poshh[idr] = max_pos[idr];
            poshh[idc] = max_pos[idc];
            vertex_data_slice.push_back({ posll , color });
            vertex_data_slice.push_back({ poslh , color });
            vertex_data_slice.push_back({ poshl , color });
            vertex_data_slice.push_back({ poshh , color });
            vertex_data_slice.push_back({ poslh , color });
            vertex_data_slice.push_back({ poshl , color });
        }
        else
        {
            int max_div;
            bool use_mid = false;
            if (max_div_set == 0)
                max_div = render_config.get_int("Max_Plane_Division");
            else
            {
                max_div = max_div_set;
                use_mid = true;
            }
            float grid_step_r = (max_pos[idr] - min_pos[idr]) / powf(2.0, max_div);
            float grid_step_c = (max_pos[idc] - min_pos[idc]) / powf(2.0, max_div);

            OpenMesh::Vec3f color{ 1.0f, 0.7f, 0.7f };
            float rr, cc;
            for (float r = min_pos[idr]; r < max_pos[idr]; r += grid_step_r)
            {
                rr = r + grid_step_r;
                for (float c = min_pos[idc]; c < max_pos[idc]; c += grid_step_c)
                {
                    cc = c + grid_step_c;
                    OpenMesh::Vec3f posll, poslh, poshl, poshh;
                    OpenMesh::Vec3f colorll, colorlh, colorhl, colorhh;
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

                    if (dis_field != nullptr)
                    {
                        QColor qcolor;

                        if (!use_mid)
                        {
                            auto disll = dis_field->get_value(posll);
                            auto color_phasell = BOUND(powf(disll, 0.5f), 0.0f, 0.5f);
                            qcolor = QColor::fromHsvF(color_phasell, 1.0f, 1.0f);
                            colorll[0] = qcolor.redF();
                            colorll[1] = qcolor.greenF();
                            colorll[2] = qcolor.blueF();

                            auto dislh = dis_field->get_value(poslh);
                            auto color_phaselh = BOUND(powf(dislh, 0.5f), 0.0f, 0.5f);
                            qcolor = QColor::fromHsvF(color_phaselh, 1.0f, 1.0f);
                            colorlh[0] = qcolor.redF();
                            colorlh[1] = qcolor.greenF();
                            colorlh[2] = qcolor.blueF();

                            auto dishl = dis_field->get_value(poshl);
                            auto color_phasehl = BOUND(powf(dishl, 0.5f), 0.0f, 0.5f);
                            qcolor = QColor::fromHsvF(color_phasehl, 1.0f, 1.0f);
                            colorhl[0] = qcolor.redF();
                            colorhl[1] = qcolor.greenF();
                            colorhl[2] = qcolor.blueF();

                            auto dishh = dis_field->get_value(poshh);
                            auto color_phasehh = BOUND(powf(dishh, 0.5f), 0.0f, 0.5f);
                            qcolor = QColor::fromHsvF(color_phasehh, 1.0f, 1.0f);
                            colorhh[0] = qcolor.redF();
                            colorhh[1] = qcolor.greenF();
                            colorhh[2] = qcolor.blueF();
                        }
                        else
                        {
                            auto mid_pos = (posll + poshh) / 2;
                            auto dis = dis_field->get_value(mid_pos);
                            auto color_phase = BOUND(powf(dis, 0.5f), 0.0f, 1.0f);
                            qcolor = QColor::fromHsvF(color_phase, 1.0f, 1.0f);
                            colorll[0] = qcolor.redF();
                            colorll[1] = qcolor.greenF();
                            colorll[2] = qcolor.blueF();
                            colorlh[0] = qcolor.redF();
                            colorlh[1] = qcolor.greenF();
                            colorlh[2] = qcolor.blueF();
                            colorhl[0] = qcolor.redF();
                            colorhl[1] = qcolor.greenF();
                            colorhl[2] = qcolor.blueF();
                            colorhh[0] = qcolor.redF();
                            colorhh[1] = qcolor.greenF();
                            colorhh[2] = qcolor.blueF();
                        }
                    }

                    vertex_data_slice.push_back({ posll , colorll });
                    vertex_data_slice.push_back({ poslh , colorlh });
                    vertex_data_slice.push_back({ poshl , colorhl });
                    vertex_data_slice.push_back({ poshh , colorhh });
                    vertex_data_slice.push_back({ poslh , colorlh });
                    vertex_data_slice.push_back({ poshl , colorhl });
                }
            }
        }
    }

    buffer_need_update_slice = true;
}

void RenderingWidget::UpdateSurface(int iter) // default = 1
{
    if (mesh.vertices_empty())
    {
        emit(StatusInfo(tr("Update Canceled, Surface is Empty.")));
        msg.log(tr("Update Canceled, Surface is Empty."), INFO_MSG);
        return;
    }

    if (dis_field == nullptr)
    {
        emit(StatusInfo(tr("Update Canceled, dis_field is null.")));
        msg.log(tr("Update Canceled, dis_field is null."), INFO_MSG);
        return;
    }

    if (ss == nullptr)
    {
        // initialize the Surface Solution.
        QString method = algorithm_config.get_string("SurfaceUpdateMethod", "Neo");

        if (method == "Matlab")
            ss = new SurfaceSolutionMatlab(mesh, dis_field, msg, algorithm_config);
        else if (method == "Neo")
            ss = new SurfaceSolutionNeo(mesh, dis_field, msg, algorithm_config);
    }

    algorithm_config.reload();

    for (int i = 0; i < iter; i++)
        ss->update();

    mesh.update_normals();

    GenerateBufferFromMesh(mesh, vertex_data_mesh);
    buffer_need_update_mesh = true;
    update();
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
    glClearColor(background_color[0], 
        background_color[1],
        background_color[2],
        1.0f);

    // Wire-frame mode.
    // Any subsequent drawing calls will render the triangles in
    // wire-frame mode until we set it back to its default using
    // `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)`.
    if (!config_bundle.render_config.draw_face)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (config_bundle.render_config.cull_face)
        glEnable(GL_CULL_FACE);

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

    // Render
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_program_basic_light->bind();
    {
        vao_pc.bind();
        {
            if (config_bundle.render_config.point_cloud_visible)
            {
                shader_program_basic_light->setUniformValue("lightDirFrom", 1.0f, 1.0f, 1.0f);
                shader_program_basic_light->setUniformValue("viewPos", camera.position());
                shader_program_basic_light->setUniformValue("model", mat_model);
                shader_program_basic_light->setUniformValue("view", camera.view_mat());
                shader_program_basic_light->setUniformValue("projection", mat_projection);
                shader_program_basic_light->setUniformValue("alpha", config_bundle.render_config.point_cloud_alpha);

                shader_program_basic_light->setUniformValue("material.ambient", 1.0f);
                shader_program_basic_light->setUniformValue("material.diffuse", 0.0f);
                shader_program_basic_light->setUniformValue("material.specular", 0.0f);
                shader_program_basic_light->setUniformValue("material.shininess", 32.0f);
                glDrawArrays(GL_POINTS, 0, vertex_data_pc.size());
            }
        }
        vao_pc.release();

        vao_mesh.bind();
        {
            if (config_bundle.render_config.mesh_visible)
            {
                shader_program_basic_light->setUniformValue("lightDirFrom", camera.direction());
                shader_program_basic_light->setUniformValue("viewPos", camera.position());
                shader_program_basic_light->setUniformValue("model", mat_model);
                shader_program_basic_light->setUniformValue("view", camera.view_mat());
                shader_program_basic_light->setUniformValue("projection", mat_projection);
                shader_program_basic_light->setUniformValue("alpha", config_bundle.render_config.mesh_alpha);
                const auto &material = config_bundle.render_config.material;
                shader_program_basic_light->setUniformValue("material.ambient", material.ambient);
                shader_program_basic_light->setUniformValue("material.diffuse", material.diffuse);
                shader_program_basic_light->setUniformValue("material.specular", material.specular);
                shader_program_basic_light->setUniformValue("material.shininess", material.shininess);
                glDrawArrays(GL_TRIANGLES, 0, vertex_data_mesh.size());
            }
        }
        vao_mesh.release();
    }
    shader_program_basic_light->release();

    // When draw base(coordinates) and slice(slicing plane):
    //     disable face cull and adjust polygon mode to fill
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

        if (config_bundle.render_config.slice_visible)
        {
            vao_slice.bind();
            {
                auto alpha = config_bundle.render_config.slice_alpha;
                shader_program_pure_color->setUniformValue("alpha", alpha);
                shader_program_pure_color->setUniformValue("model", mat_model);
                shader_program_pure_color->setUniformValue("view", camera.view_mat());
                shader_program_pure_color->setUniformValue("projection", mat_projection);
                glDrawArrays(GL_TRIANGLES, 0, vertex_data_slice.size());
            }
            vao_slice.release();
        }
    }
    shader_program_pure_color->release();
    
    // END OF GL

    // Restore Polygon Mode to ensure the correctness of native painter
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE | GL_DEPTH_TEST);

    // Native Painter work.
    QPainter painter(this);

    // Configure Font
    painter.setPen(Qt::white);
    QFont font;
    QString font_family = gui_config.get_string("Script_Font_Family", "");
    int font_size = gui_config.get_int("Script_Font_Size", 12);
    font.setFamily(font_family);
    font.setPointSize(font_size);
    painter.setFont(font);
    QFont font_bold = font;
    font_bold.setBold(true);

    double dpi = QGuiApplication::primaryScreen()->physicalDotsPerInch();
    int draw_height = this->height() * 14 / 15;
    int draw_width = this->height() / 15;
    
    painter.setPen(Qt::yellow);
    //if (mode == ScriptMode && script_lineedit != nullptr)
    //{
    //    // draw prompt
    //    painter.drawText(draw_width - font_size / 72.0f * dpi, draw_height,
    //        ">>");

    //    // draw line
    //    painter.drawText(draw_width, draw_height,
    //        script_lineedit->text());

    //    QFontMetrics fm(painter.font());
    //    int cursor_pos = script_lineedit->cursorPosition();
    //    int cursor_width = fm.width(script_lineedit->text().left(cursor_pos));
    //    int font_width = fm.averageCharWidth();

    //    // draw cursor
    //    int rate = 255.0f * std::abs(QDateTime::currentMSecsSinceEpoch() % 1000 - 500) / 500.0f;
    //    painter.setPen(QColor{ 255, 255, 0, rate });
    //    painter.setFont(font_bold);
    //    painter.drawText(draw_width + cursor_width - 0.5f * font_width, draw_height,
    //        "|");
    //    painter.setFont(font);
    //}
    //if (!script_history.empty())
    //{
    //    for (int i = 0; i < script_history.size(); i++)
    //    {
    //        draw_height -= font_size / 72.0f * dpi;

    //        // set color and draw prompt
    //        if (script_history_type[i] == ErrorType)
    //        {
    //            painter.setPen(Qt::red);
    //            painter.drawText(draw_width - font_size / 72.0f * dpi, draw_height,
    //                "X");
    //        }
    //        else if (script_history_type[i] == ScriptType)
    //        {
    //            painter.setPen(Qt::darkYellow);
    //            painter.drawText(draw_width - font_size / 72.0f * dpi, draw_height,
    //                ">>");
    //        }
    //        else if (script_history_type[i] == InfoType)
    //        {
    //            painter.setPen(Qt::white);
    //            painter.drawText(draw_width - font_size / 72.0f * dpi, draw_height,
    //                "*");
    //        }
    //        painter.drawText(draw_width, draw_height,
    //            script_history[i]);
    //    }
    //}
    painter.end();
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

    // Priority: prime
    switch (e->key())
    {
    //case Qt::Key_Escape:
    //    mode = ViewMode;
    //    this->grabKeyboard();
    //    break;
    case Qt::Key_Q:
        if (has_ctrl)
            exit(0);
    }

    if (mode != ViewMode)
        return;

    // Priority: normal
    // following key responses will not activate under ScriptMode or SelectMode. 
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

    case Qt::Key_R:
        emit(StatusInfo(QString("reset camera")));
        camera = OpenGLCamera({ 
                render_config.get_float("Default_Camera_X"),
                render_config.get_float("Default_Camera_Y"),
                render_config.get_float("Default_Camera_Z") 
            },
            { 0.0f, 0.0f, 0.0f }
        );
        break;
    case Qt::Key_F:
        config_bundle.render_config.draw_face = !config_bundle.render_config.draw_face;
        emit(StatusInfo(QString("render_show_face set to %0").arg(config_bundle.render_config.draw_face)));
        break;
    case Qt::Key_C:
        config_bundle.render_config.cull_face = !config_bundle.render_config.cull_face;
        emit(StatusInfo(QString("render_cull_face set to %0").arg(config_bundle.render_config.draw_face)));
        break;
    case Qt::Key_T:
        this->GenerateSphereMesh();
        break;
    case Qt::Key_U:
        this->UpdateSurface();
        break;
    // MODE
    //case Qt::Key_Colon:
    //    mode = ScriptMode;
    //    {
    //        script_lineedit = new QLineEdit(this);
    //        script_lineedit->grabKeyboard();
    //        connect(script_lineedit, &QLineEdit::textChanged, this, [this]() {
    //            update();
    //        });
    //        connect(script_lineedit, &QLineEdit::cursorPositionChanged, this, [this]() {
    //            update();
    //        });
    //        connect(script_lineedit, &QLineEdit::returnPressed, this, [this]() {
    //            RunScript();
    //            update();
    //        });
    //    }
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
    OpenMesh::Vec3f red{ 1.0f, 0.0f, 0.0f };
    OpenMesh::Vec3f orange{ 1.0f, 0.5f, 0.0f };
    OpenMesh::Vec3f yellow{ 1.0f, 1.0f, 0.0f };
    OpenMesh::Vec3f green{ 0.0f, 1.0f, 0.0f };
    OpenMesh::Vec3f aoi{ 0.0f, 1.0f, 1.0f };
    OpenMesh::Vec3f blue{ 0.0f, 0.0f, 1.0f };

    D.clear();
    for (auto f_it : M.faces())
    {
        auto fv_it = M.fv_iter(f_it);

        for (; fv_it; ++fv_it)
        {
            auto vh = *fv_it;
            auto pos = M.point(vh);
            auto nor = M.normal(vh);
            int tag = -1;
            if (ss != nullptr)
                tag = ss->tagged(vh);

            if (tag == 1)
                D.push_back({ pos, red, nor });
            else if (tag == 2)
                D.push_back({ pos, orange, nor });
            else if (tag == 3)
                D.push_back({ pos, yellow, nor });
            else if (tag == 4)
                D.push_back({ pos, green, nor });
            else if (tag == 5)
                D.push_back({ pos, aoi, nor });
            else if (tag == 6)
                D.push_back({ pos, blue, nor });
            else
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
