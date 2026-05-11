#include <QApplication>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("StegoLab");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("StegoLab");

    // Set working directory to executable location
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow w;
    w.show();
    return app.exec();
}
