#include <QApplication>
#include <QFont>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));

    MainWindow window;
    window.show();

    return app.exec();
}
