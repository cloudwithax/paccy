#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("paccy");
    app.setWindowIcon(QIcon::fromTheme("package-x-generic"));

    QString packagePath;
    if (argc > 1) {
        packagePath = QString::fromUtf8(argv[1]);
    }

    if (packagePath.isEmpty()) {
        QMessageBox::warning(nullptr, "paccy",
            "No package file specified.\n\nUsage: paccy <package.pkg.tar.zst>");
        return 1;
    }

    // Resolve to absolute path
    QFileInfo fi(packagePath);
    if (!fi.exists()) {
        QMessageBox::critical(nullptr, "paccy",
            "File not found:\n" + packagePath);
        return 1;
    }
    packagePath = fi.absoluteFilePath();

    MainWindow window(packagePath);
    window.show();

    return app.exec();
}
