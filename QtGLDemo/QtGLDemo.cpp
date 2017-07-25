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

    this->rendering_widget->SyncConfigBundle(ExtractConfigBundle());
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
    this->rendering_widget->SavePointCloudToFile();
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
    // File
    actFileOpenMesh = new QAction(tr("&Mesh"));
    connect(actFileOpenMesh, SIGNAL(triggered()), this, SLOT(File_Open_Mesh()));

    actFileOpenPointCloud = new QAction(tr("&Point Cloud"));
    connect(actFileOpenPointCloud, SIGNAL(triggered()), this, SLOT(File_Open_PointCloud()));

    actFileOpenDistanceField = new QAction(tr("Distance Field"));
    connect(actFileOpenDistanceField, &QAction::triggered, this, [this]() {
        this->rendering_widget->ReadDistanceFieldFromFile();
    });

    actFileSaveMesh = new QAction(tr("&Mesh"));
    connect(actFileSaveMesh, SIGNAL(triggered()), this, SLOT(File_Save()));

    actFileSavePointCloud = new QAction(tr("&Point Cloud"));
    connect(actFileSavePointCloud, &QAction::triggered, this, [this]() {
        this->rendering_widget->SaveMeshToFile();
    });

    actFileSaveDistanceField = new QAction(tr("&Distance Field"));
    connect(actFileSaveDistanceField, &QAction::triggered, this, [this]() {
        this->rendering_widget->SaveDistanceFieldToFile();
    });

    // Edit
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

    actEditBuildDistanceField = new QAction(tr("&Build Dis Field"));
    connect(actEditBuildDistanceField, &QAction::triggered, this, [this]() {
        this->rendering_widget->BuildDistanceFieldFromPointCloud();
    });

    // View
    actViewChangeColorBackground = new QAction(tr("&Background"));
    connect(actViewChangeColorBackground, &QAction::triggered, this, [this]() {
        this->rendering_widget->ChangeColorBackground();
    }); 

    actViewChangeColorMesh = new QAction(tr("&Mesh"));
    connect(actViewChangeColorMesh, &QAction::triggered, this, [this]() {
        this->rendering_widget->ChangeColorMesh();
    }); 

    actViewChangeColorPointCloud = new QAction(tr("&Point Cloud"));
    connect(actViewChangeColorPointCloud, &QAction::triggered, this, [this]() {
        this->rendering_widget->ChangeColorPointCloud();
    });
    
    actViewSwitchEnableSlicing = new QAction(tr("Enable &Slicing"));
    actViewSwitchEnableSlicing->setCheckable(true);
    actViewSwitchEnableSlicing->setChecked(false);
    actViewCheckSlicingDirectionX = new QAction(tr("&X"));
    actViewCheckSlicingDirectionX->setCheckable(true);
    actViewCheckSlicingDirectionX->setChecked(true);
    actViewCheckSlicingDirectionY = new QAction(tr("&Y"));
    actViewCheckSlicingDirectionY->setCheckable(true);
    actViewCheckSlicingDirectionZ = new QAction(tr("&Z"));
    actViewCheckSlicingDirectionZ->setCheckable(true);
    actViewCheckSlicingDirectionGroup = new QActionGroup(this);
    actViewCheckSlicingDirectionGroup->setExclusive(true);
    actViewCheckSlicingDirectionGroup->setEnabled(false);
    actViewCheckSlicingDirectionGroup->addAction(actViewCheckSlicingDirectionX);
    actViewCheckSlicingDirectionGroup->addAction(actViewCheckSlicingDirectionY);
    actViewCheckSlicingDirectionGroup->addAction(actViewCheckSlicingDirectionZ);

    connect(actViewCheckSlicingDirectionX, &QAction::changed, this, [this]() {
        this->rendering_widget->SyncConfigBundle(ExtractConfigBundle());
    });
    connect(actViewCheckSlicingDirectionY, &QAction::changed, this, [this]() {
        this->rendering_widget->SyncConfigBundle(ExtractConfigBundle());
    });
    connect(actViewCheckSlicingDirectionZ, &QAction::changed, this, [this]() {
        this->rendering_widget->SyncConfigBundle(ExtractConfigBundle());
    });

    connect(actViewSwitchEnableSlicing, &QAction::changed, this, [this]() {
        bool checked = this->actViewSwitchEnableSlicing->isChecked();
        this->actViewCheckSlicingDirectionGroup->setEnabled(checked);
        this->rendering_widget->SyncConfigBundle(ExtractConfigBundle());
    });

    actViewSwitchSlicingMaxDivision = new QAction(tr("&Max Division"));
    connect(actViewSwitchSlicingMaxDivision, &QAction::triggered, this, [this]() {
        bool ok;
        int i = QInputDialog::getInt(this, tr("QInputDialog::getInteger()"),
            tr("Max Division:"), 10, 0, 15, 1, &ok);
        if (ok)
            this->rendering_widget->UpdateSlicingPlane(i);
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
    // File/Open/Distance Field
    openMenu->addAction(actFileOpenDistanceField);
    // File/Save
    auto saveMenu = fileMenu->addMenu(tr("&Save"));
    // File/Save/Mesh
    saveMenu->addAction(actFileSaveMesh);
    // File/Save/Point Cloud
    saveMenu->addAction(actFileSavePointCloud);
    // File/Save/Distance Field
    saveMenu->addAction(actFileSaveDistanceField);

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
    // Edit/Build Dis Field
    editMenu->addAction(actEditBuildDistanceField);

    // View
    viewMenu = this->menuBar()->addMenu(tr("&View"));
    // View/Change Color
    auto changeColorMenu = viewMenu->addMenu(tr("Change &Color"));
    changeColorMenu->addAction(actViewChangeColorBackground);
    changeColorMenu->addAction(actViewChangeColorMesh);
    changeColorMenu->addAction(actViewChangeColorPointCloud);
    // View/===
    viewMenu->addSeparator();
    // View/Slice
    auto sliceMenu = viewMenu->addMenu(tr("Set &Slicing"));
    sliceMenu->addAction(actViewSwitchEnableSlicing);
    sliceMenu->addSeparator();
    sliceMenu->addAction(actViewCheckSlicingDirectionX);
    sliceMenu->addAction(actViewCheckSlicingDirectionY);
    sliceMenu->addAction(actViewCheckSlicingDirectionZ);
    sliceMenu->addSeparator();
    sliceMenu->addAction(actViewSwitchSlicingMaxDivision);
    //auto sliceMenu = viewMenu->addMenu(tr("&Slice"));
    
}

void QtGLDemo::CreateStatusBar()
{
    statusInfoLabel = new QLabel(tr("status"));
    statusInfoLabel->setAlignment(Qt::AlignVCenter);
    statusBar()->addWidget(statusInfoLabel);
    connect(rendering_widget, SIGNAL(StatusInfo(const QString &)), this, SLOT(SetStatusInfo(const QString&)));
}

ConfigBundle QtGLDemo::ExtractConfigBundle()
{
    ConfigBundle cb;
    cb.slice_config.use_slice = actViewSwitchEnableSlicing->isChecked();
    cb.slice_config.slice_x = actViewCheckSlicingDirectionX->isChecked();
    cb.slice_config.slice_y = actViewCheckSlicingDirectionY->isChecked();
    cb.slice_config.slice_z = actViewCheckSlicingDirectionZ->isChecked();

    return cb;
}
