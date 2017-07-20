#include "stdafx.h"
#include "QtGLDemo.h"

QtGLDemo::QtGLDemo(ConsoleMessageManager &_msg, TextConfigLoader &_gui_config, QWidget *parent)
    : QMainWindow(parent), msg(_msg), gui_config(_gui_config)
{
    // ui.setupUi(this);
    _msg.log("Main Window INIT begin", TRIVIAL_MSG);

    auto zoom = gui_config.get_float("Global_Zoom");

    auto height = gui_config.get_int("Window_Height");
    auto width  = gui_config.get_int("Window_Width");
    auto enable_resize = gui_config.get_bool("Resize_Enable");
    if (enable_resize)
    {
        this->setMinimumSize(width * zoom, height * zoom);
        this->resize(0, 0);
    }
    else
    {
        this->setFixedSize(width * zoom, height * zoom);
    }

    rendering_widget = new RenderingWidget(msg, this);
    setCentralWidget(rendering_widget);

    CreateAction();
    CreateMenu();
    CreateStatusBar();

    msg.log("Main Window INIT end", TRIVIAL_MSG);
}

void QtGLDemo::File_Open_Mesh()
{
    this->rendering_widget->ReadMeshFromFile();
    msg.log("Main Window File_Open_Mesh()", TRIVIAL_MSG);
}

void QtGLDemo::File_Open_PointCloud()
{
    this->rendering_widget->ReadPointCloudFromFile();
    msg.log("Main Window File_Open_PointCloud()", TRIVIAL_MSG);
}

void QtGLDemo::File_Save()
{
    msg.log("Main Window Save()", TRIVIAL_MSG);
}

void QtGLDemo::Edit_Unify_Mesh()
{
    this->rendering_widget->ApplyUnifyForMesh();
    msg.log("Main Window Unify()", TRIVIAL_MSG);
}

void QtGLDemo::Edit_Unify_PointCloud()
{
    this->rendering_widget->ApplyUnifyForPointCloud();
    msg.log("Main Window Unify()", TRIVIAL_MSG);
}

void QtGLDemo::SetStatusInfo(const QString& t)
{
    this->statusInfoLabel->setText(t);
}

void QtGLDemo::CreateAction()
{
    actFileOpenMesh = new QAction(tr("&Mesh"));
    connect(actFileOpenMesh, SIGNAL(triggered()), this, SLOT(File_Open_Mesh()));

    actFileOpenPointCloud = new QAction(tr("&Point Cloud"));
    connect(actFileOpenPointCloud, SIGNAL(triggered()), this, SLOT(File_Open_PointCloud()));

    actFileSave = new QAction(tr("&Save"));
    connect(actFileSave, SIGNAL(triggered()), this, SLOT(File_Save()));

    actEditUnifyMesh = new QAction(tr("&Mesh"));
    connect(actEditUnifyMesh, SIGNAL(triggered()), this, SLOT(Edit_Unify_Mesh()));

    actEditUnifyPointCloud = new QAction(tr("&Point Cloud"));
    connect(actEditUnifyPointCloud, SIGNAL(triggered()), this, SLOT(Edit_Unify_PointCloud()));

    actEditTranslateMeshX = new QAction(tr("flap &X"));
    connect(actEditTranslateMeshX, &QAction::triggered, this, [this]() {
        this->rendering_widget->ApplyFlipForMesh(0);
    });

    actEditTranslateMeshY = new QAction(tr("flap &Y"));
    connect(actEditTranslateMeshY, &QAction::triggered, this, [this]() {
        this->rendering_widget->ApplyFlipForMesh(1);
    });

    actEditTranslateMeshZ = new QAction(tr("flap &Z"));
    connect(actEditTranslateMeshZ, &QAction::triggered, this, [this]() {
        this->rendering_widget->ApplyFlipForMesh(2);
    });

    actEditTranslatePointCloudX = new QAction(tr("flap &X"));
    connect(actEditTranslatePointCloudX, &QAction::triggered, this, [this]() {
        this->rendering_widget->ApplyFlipForPointCloud(0);
    });

    actEditTranslatePointCloudY = new QAction(tr("flap &Y"));
    connect(actEditTranslatePointCloudY, &QAction::triggered, this, [this]() {
        this->rendering_widget->ApplyFlipForPointCloud(1);
    });

    actEditTranslatePointCloudZ = new QAction(tr("flap &Z"));
    connect(actEditTranslatePointCloudZ, &QAction::triggered, this, [this]() {
        this->rendering_widget->ApplyFlipForPointCloud(2);
    });
}

void QtGLDemo::CreateMenu()
{
    // File
    fileMenu = this->menuBar()->addMenu(tr("&File"));
    // File/Open
    auto openMenu = fileMenu->addMenu(tr("&Open"));
    // File/Open/Mesh
    openMenu->addAction(actFileOpenMesh);
    // File/Open/Point Cloud
    openMenu->addAction(actFileOpenPointCloud);
    // File/Save
    fileMenu->addAction(actFileSave);

    // Edit
    editMenu = this->menuBar()->addMenu(tr("&Edit"));
    // Edit/Unify
    auto unifyMenu = editMenu->addMenu(tr("&Unify"));
    // Edit/Unify/Mesh
    unifyMenu->addAction(actEditUnifyMesh);
    // Edit/Unify/Point Cloud
    unifyMenu->addAction(actEditUnifyPointCloud);
    // Edit/Flip
    auto flipMenu = editMenu->addMenu(tr("&Flip"));
    // Edit/Flip/Mesh
    auto flipMeshMenu = flipMenu->addMenu(tr("&Mesh"));
    // Edit/Flip/Mesh/X|Y|Z
    flipMeshMenu->addAction(actEditTranslateMeshX);
    flipMeshMenu->addAction(actEditTranslateMeshY);
    flipMeshMenu->addAction(actEditTranslateMeshZ);
    // Edit/Flip/Point Cloud
    auto flipPointCloudMenu = flipMenu->addMenu(tr("&Point Cloud"));
    // Edit/Flip/Point Cloud/X|Y|Z
    flipPointCloudMenu->addAction(actEditTranslatePointCloudX);
    flipPointCloudMenu->addAction(actEditTranslatePointCloudY);
    flipPointCloudMenu->addAction(actEditTranslatePointCloudZ);
}

void QtGLDemo::CreateStatusBar()
{
    statusInfoLabel = new QLabel(tr("status"));
    statusInfoLabel->setAlignment(Qt::AlignVCenter);
    statusBar()->addWidget(statusInfoLabel);
    connect(rendering_widget, SIGNAL(StatusInfo(const QString &)), this, SLOT(SetStatusInfo(const QString&)));
}
