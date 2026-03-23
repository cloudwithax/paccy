#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGroupBox>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &packagePath, QWidget *parent = nullptr);

private slots:
    void installPackage();
    void uninstallPackage();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessReadyRead();
    void onProcessErrorRead();

private:
    void loadPackageInfo();
    void checkIfInstalled();
    void setupUi();

    QString m_packagePath;
    QString m_packageName;
    bool m_isInstalled = false;

    QLabel *m_nameLabel;
    QLabel *m_versionLabel;
    QLabel *m_descLabel;
    QLabel *m_archLabel;
    QLabel *m_sizeLabel;
    QLabel *m_installedLabel;
    QPushButton *m_actionButton;
    QPushButton *m_closeButton;
    QTextEdit *m_outputView;
    QProcess *m_actionProcess;
    QGroupBox *m_infoGroup;
};

#endif // MAINWINDOW_H
