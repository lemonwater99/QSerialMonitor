#include "QSerialMonitor.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QSerialMonitor window;
    window.resize(1024,680);
    window.show();
    return app.exec();
}
