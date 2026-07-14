#include <QApplication>
#include <QFont>
#include "App/SGSApp.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));

    // 组合根创建所有对象（MainWindow、ViewModel、View 等）
    SGSApp sgsApp;
    sgsApp.mainWindow()->show();

    return app.exec();
}
