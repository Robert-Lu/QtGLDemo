#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtGLDemo.h"

class QtGLDemo : public QMainWindow
{
    Q_OBJECT

public:
    QtGLDemo(QWidget *parent = Q_NULLPTR);

private:
    Ui::QtGLDemoClass ui;
};
