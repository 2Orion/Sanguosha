#include <QApplication>
#include <QFont>
#include "GameBootstrap.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));

    // 组合根创建所有对象（MainWindow、ViewModel、View 等）
    GameBootstrap bootstrap;
    bootstrap.mainWindow()->show();

    return app.exec();
}
