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

void QtGLDemo::Open()
{   
    msg.log("Main Window Open()", TRIVIAL_MSG);
}

void QtGLDemo::Save()
{
    msg.log("Main Window Save()", TRIVIAL_MSG);
}

void QtGLDemo::SetStatusInfo(const QString& t)
{
    this->statusInfoLabel->setText(t);
}

void QtGLDemo::CreateAction()
{
    actOpen = new QAction(tr("&Open"));
    connect(actOpen, SIGNAL(triggered()), this, SLOT(Open()));
    actSave = new QAction(tr("&Save"));
    connect(actSave, SIGNAL(triggered()), this, SLOT(Save()));
}

void QtGLDemo::CreateMenu()
{
    fileMenu = this->menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actOpen);
    fileMenu->addAction(actSave);
    editMenu = this->menuBar()->addMenu(tr("&Edit"));
}

void QtGLDemo::CreateStatusBar()
{
    statusInfoLabel = new QLabel(tr("status"));
    statusInfoLabel->setAlignment(Qt::AlignVCenter);
    statusBar()->addWidget(statusInfoLabel);
    connect(rendering_widget, SIGNAL(StatusInfo(const QString &)), this, SLOT(SetStatusInfo(const QString&)));
}
