#include "stdafx.h"
#include "QtGLDemo.h"
#include <QtWidgets/QApplication>

#include <iostream>
#include "ConsoleMessageManager.h"
#include "GlobalConfig.h"

int main(int argc, char *argv[])
{
    TextConfigLoader gui_config{ GUI_CONFIG_FILENAME };

    ConsoleMessageManager msg(std::cout);
    msg.enable(TRIVIAL_MSG);
    msg.enable(MATRIX_MSG);

    QApplication a(argc, argv);
    auto global_font = gui_config.get_string("Global_Font");
    a.setFont(QFont(global_font)); 
    QtGLDemo w{ msg, gui_config };
    w.show();

    return a.exec();
}
