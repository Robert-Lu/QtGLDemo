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
    void Open();
    void Save();
    void SetStatusInfo(const QString &);

private:
    //Ui::QtGLDemoClass ui;
    ConsoleMessageManager   &msg;
    TextConfigLoader        &gui_config;

    // Actions
    QAction *actOpen;
    QAction *actSave;

    // UI component
    QMenu   *fileMenu;
    QMenu   *editMenu;
    QLabel  *statusInfoLabel;

    RenderingWidget *rendering_widget;

    void CreateAction();
    void CreateMenu();
    void CreateStatusBar();
};
