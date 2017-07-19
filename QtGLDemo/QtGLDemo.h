#pragma once

#include <QtWidgets/QMainWindow>
// #include "ui_QtGLDemo.h"
#include "ConsoleMessageManager.h"
#include "TextConfigLoader.h"

class QtGLDemo : public QMainWindow
{
    Q_OBJECT

public:
    QtGLDemo(ConsoleMessageManager &_msg, TextConfigLoader &, QWidget *parent = Q_NULLPTR);

public slots:
    void Open();
    void Save();

private:
    //Ui::QtGLDemoClass ui;
    ConsoleMessageManager   &msg;
    TextConfigLoader        &gui_config;

    // Actions
    QAction *actOpen;
    QAction *actSave;

    // UI component
    QMenu *fileMenu;
    QMenu *editMenu;

    void CreateAction();
    void CreateMenu();
};
