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
    QAction *actFileSave;
    QAction *actEditUnifyMesh;
    QAction *actEditUnifyPointCloud;
    QAction *actEditTranslateMeshX;
    QAction *actEditTranslateMeshY;
    QAction *actEditTranslateMeshZ;
    QAction *actEditTranslatePointCloudX;
    QAction *actEditTranslatePointCloudY;
    QAction *actEditTranslatePointCloudZ;

    // UI component
    QMenu   *fileMenu;
    QMenu   *editMenu;
    QLabel  *statusInfoLabel;

    RenderingWidget *rendering_widget;

    void CreateAction();
    void CreateMenu();
    void CreateStatusBar();
};
