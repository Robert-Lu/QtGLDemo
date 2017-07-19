#include "stdafx.h"
#include "QtGLDemo.h"
#include <QtWidgets/QApplication>

#include <iostream>
#include "ConsoleMessageManager.h"

#define     GUI_CONFIG_FILENAME     "./config/gui.config"

int main(int argc, char *argv[])
{
    TextConfigLoader gui_config{ GUI_CONFIG_FILENAME };
    auto global_font = gui_config.get_string("Global_Font");

    ConsoleMessageManager msg(std::cout);
    msg.enable(TRIVIAL_MSG);

    msg.log("main begin.", TRIVIAL_MSG);

    QApplication a(argc, argv);
    a.setFont(QFont(global_font)); 
    QtGLDemo w{ msg, gui_config };
    w.show();

    msg.log("main end.", TRIVIAL_MSG);
    return a.exec();
}
