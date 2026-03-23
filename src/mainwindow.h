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
    void onInstallFinished(int exitCode, QProcess::ExitStatus status);
    void onInstallReadyRead();
    void onErrorReadyRead();

private:
    void loadPackageInfo();
    void setupUi();

    QString m_packagePath;

    QLabel *m_nameLabel;
    QLabel *m_versionLabel;
    QLabel *m_descLabel;
    QLabel *m_archLabel;
    QLabel *m_sizeLabel;
    QPushButton *m_installButton;
    QPushButton *m_closeButton;
    QTextEdit *m_outputView;
    QProcess *m_installProcess;
    QGroupBox *m_infoGroup;
};

#endif // MAINWINDOW_H
