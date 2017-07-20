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

void QtGLDemo::Edit_UnifyMesh()
{
    this->rendering_widget->ApplyUnifyForMesh();
    msg.log("Main Window Unify()", TRIVIAL_MSG);
}

void QtGLDemo::Edit_UnifyPointCloud()
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
    actFileOpenMesh = new QAction(tr("Open &Mesh"));
    connect(actFileOpenMesh, SIGNAL(triggered()), this, SLOT(File_Open_Mesh()));
    actFileOpenPointCloud = new QAction(tr("Open &Point Cloud"));
    connect(actFileOpenPointCloud, SIGNAL(triggered()), this, SLOT(File_Open_PointCloud()));
    actFileSave = new QAction(tr("&Save"));
    connect(actFileSave, SIGNAL(triggered()), this, SLOT(File_Save()));
    actEditUnifyMesh = new QAction(tr("Unify &Mesh"));
    connect(actEditUnifyMesh, SIGNAL(triggered()), this, SLOT(Edit_UnifyMesh()));
    actEditUnifyPointCloud = new QAction(tr("Unify &Point Cloud"));
    connect(actEditUnifyPointCloud, SIGNAL(triggered()), this, SLOT(Edit_UnifyPointCloud()));
}

void QtGLDemo::CreateMenu()
{
    fileMenu = this->menuBar()->addMenu(tr("&File"));
    auto openMenu = fileMenu->addMenu(tr("&Open"));
    openMenu->addAction(actFileOpenMesh);
    openMenu->addAction(actFileOpenPointCloud);
    fileMenu->addAction(actFileSave);
    editMenu = this->menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(actEditUnifyMesh);
    editMenu->addAction(actEditUnifyPointCloud);
}

void QtGLDemo::CreateStatusBar()
{
    statusInfoLabel = new QLabel(tr("status"));
    statusInfoLabel->setAlignment(Qt::AlignVCenter);
    statusBar()->addWidget(statusInfoLabel);
    connect(rendering_widget, SIGNAL(StatusInfo(const QString &)), this, SLOT(SetStatusInfo(const QString&)));
}
