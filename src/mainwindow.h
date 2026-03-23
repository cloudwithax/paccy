#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QToolButton>
#include <QProgressBar>

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
    void toggleOutput();

private:
    void loadPackageInfo();
    void checkIfInstalled();
    void setupUi();
    void showActivity(const QString &text);
    void hideActivity();

    QString m_packagePath;
    QString m_packageName;
    QString m_packageVersion;
    bool m_isInstalled = false;
    bool m_isUpgrade = false;

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
    QToolButton *m_outputToggle;
    QWidget *m_outputContainer;
    QProgressBar *m_throbber;
    QLabel *m_statusLabel;
    QWidget *m_statusContainer;
};

#endif // MAINWINDOW_H
