#pragma once

#include <QtWidgets/QMainWindow>
// #include "ui_QtGLDemo.h"
#include "ConsoleMessageManager.h"
#include "TextConfigLoader.h"
#include "RenderingWidget.h"

class QtGLDemo : public QMainWindow
{
    Q_OBJECT

public:
    QtGLDemo(ConsoleMessageManager &_msg, TextConfigLoader &, QWidget *parent = Q_NULLPTR);

public slots:
    void File_Open_Mesh();
    void File_Open_PointCloud();
    void File_Save();
    void Edit_Unify_Mesh();
    void Edit_Unify_PointCloud();
    void SetStatusInfo(const QString &);

private:
    //Ui::QtGLDemoClass ui;
    ConsoleMessageManager   &msg;
    TextConfigLoader        &gui_config;

    // Actions
    QAction *actFileOpenMesh;
    QAction *actFileOpenPointCloud;
    QAction *actFileOpenDistanceField;
    QAction *actFileSaveMesh;
    QAction *actFileSavePointCloud;
    QAction *actFileSaveDistanceField;

    QAction *actEditUnifyMesh;
    QAction *actEditUnifyPointCloud;

    QAction *actEditTranslateMeshX;
    QAction *actEditTranslateMeshY;
    QAction *actEditTranslateMeshZ;

    QAction *actEditTranslatePointCloudX;
    QAction *actEditTranslatePointCloudY;
    QAction *actEditTranslatePointCloudZ;

    QAction *actEditBuildDistanceField;

    QAction *actViewChangeColorBackground;
    QAction *actViewChangeColorMesh;
    QAction *actViewChangeColorPointCloud;

    QAction *actViewSwitchEnableSlicing;
    QAction *actViewCheckSlicingDirectionX;
    QAction *actViewCheckSlicingDirectionY;
    QAction *actViewCheckSlicingDirectionZ;
    QActionGroup *actViewCheckSlicingDirectionGroup;
    QAction *actViewSwitchSlicingMaxDivision;

    QAction *actViewConfig;

    // UI component
    QMenu   *fileMenu;
    QMenu   *editMenu;
    QMenu   *viewMenu;

    QLabel  *statusInfoLabel;

    RenderingWidget *rendering_widget;
    ConfigBundle config_bundle;

    void CreateAction();
    void CreateMenu();
    void CreateStatusBar();
    ConfigBundle &ExtractConfigBundle();
    ConfigBundle &ShowConfigDialog(ConfigBundle &curr);
};
