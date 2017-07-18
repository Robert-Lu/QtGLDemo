#include "stdafx.h"
#include "QtGLDemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtGLDemo w;
    w.show();
    return a.exec();
}
