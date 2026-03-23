#include "mainwindow.h"
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QScrollBar>
#include <QFont>
#include <QFrame>
#include <QDir>

MainWindow::MainWindow(const QString &packagePath, QWidget *parent)
    : QMainWindow(parent)
    , m_packagePath(packagePath)
    , m_installProcess(nullptr)
{
    setupUi();
    loadPackageInfo();
}

void MainWindow::setupUi()
{
    setWindowTitle("paccy");
    setMinimumSize(480, 520);
    resize(520, 560);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // Header
    auto *headerLabel = new QLabel("Install Package");
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(18);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    mainLayout->addWidget(headerLabel);

    // Package info group
    m_infoGroup = new QGroupBox("Package Information");
    auto *infoLayout = new QVBoxLayout(m_infoGroup);
    infoLayout->setSpacing(6);

    auto makeInfoRow = [&](const QString &labelText) -> QLabel * {
        auto *row = new QHBoxLayout();
        auto *label = new QLabel(labelText);
        QFont lblFont = label->font();
        lblFont.setBold(true);
        label->setFont(lblFont);
        label->setFixedWidth(90);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto *value = new QLabel("...");
        value->setWordWrap(true);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        row->addWidget(label);
        row->addWidget(value, 1);
        infoLayout->addLayout(row);
        return value;
    };

    m_nameLabel = makeInfoRow("Name:");
    m_versionLabel = makeInfoRow("Version:");
    m_archLabel = makeInfoRow("Arch:");
    m_sizeLabel = makeInfoRow("Size:");
    m_descLabel = makeInfoRow("Description:");

    mainLayout->addWidget(m_infoGroup);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(sep);

    // Output view
    m_outputView = new QTextEdit();
    m_outputView->setReadOnly(true);
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::Monospace);
    m_outputView->setFont(monoFont);
    m_outputView->setPlaceholderText("Installation output will appear here...");
    mainLayout->addWidget(m_outputView, 1);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_closeButton = new QPushButton("Close");
    m_closeButton->setFixedWidth(100);
    connect(m_closeButton, &QPushButton::clicked, this, &QMainWindow::close);

    m_installButton = new QPushButton("Install");
    m_installButton->setFixedWidth(100);
    m_installButton->setDefault(true);
    QFont installFont = m_installButton->font();
    installFont.setBold(true);
    m_installButton->setFont(installFont);
    connect(m_installButton, &QPushButton::clicked, this, &MainWindow::installPackage);

    buttonLayout->addWidget(m_closeButton);
    buttonLayout->addWidget(m_installButton);
    mainLayout->addLayout(buttonLayout);

    setCentralWidget(central);
}

void MainWindow::loadPackageInfo()
{
    QFileInfo fi(m_packagePath);
    if (!fi.exists()) {
        m_nameLabel->setText("File not found");
        m_outputView->setText("Error: " + m_packagePath + " does not exist.");
        m_installButton->setEnabled(false);
        return;
    }

    // Check valid extension
    QString name = fi.fileName();
    if (!(name.endsWith(".pkg.tar.zst") || name.endsWith(".pkg.tar.xz") || name.endsWith(".pkg.tar.gz") || name.endsWith(".pkg.tar"))) {
        m_nameLabel->setText("Invalid package");
        m_outputView->setText("Error: Not a valid pacman package file.\nSupported formats: .pkg.tar.zst, .pkg.tar.xz, .pkg.tar.gz, .pkg.tar");
        m_installButton->setEnabled(false);
        return;
    }

    // Read .PKGINFO from archive
    QProcess proc;
    QStringList args;
    args << "--use-compress-program=zstd -d";
    args << "-xf" << m_packagePath;
    args << "-O" << ".PKGINFO";

    // Try natively first (libarchive with zstd support)
    proc.start("tar", {"-xf", m_packagePath, "-O", ".PKGINFO"});
    proc.waitForFinished(5000);

    QString pkginfo = proc.readAllStandardOutput();

    // If that failed, try with explicit decompressor
    if (pkginfo.isEmpty()) {
        proc.start("tar", args);
        proc.waitForFinished(5000);
        pkginfo = proc.readAllStandardOutput();
    }

    if (pkginfo.isEmpty()) {
        m_nameLabel->setText(fi.baseName());
        m_descLabel->setText("Could not read package metadata");
        return;
    }

    // Parse .PKGINFO
    QString name_val, version_val, desc_val, arch_val, size_val;

    for (const QString &line : pkginfo.split('\n')) {
        if (line.startsWith("#"))
            continue;

        QString key, val;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        key = line.left(eq).trimmed();
        val = line.mid(eq + 1).trimmed();

        if (key == "pkgname") name_val = val;
        else if (key == "pkgver") version_val = val;
        else if (key == "pkgdesc") desc_val = val;
        else if (key == "arch") arch_val = val;
        else if (key == "size") {
            qint64 bytes = val.toLongLong();
            if (bytes > 1024 * 1024)
                size_val = QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MiB";
            else if (bytes > 1024)
                size_val = QString::number(bytes / 1024.0, 'f', 1) + " KiB";
            else
                size_val = val + " B";
        }
    }

    m_nameLabel->setText(name_val.isEmpty() ? fi.baseName() : name_val);
    m_versionLabel->setText(version_val.isEmpty() ? "Unknown" : version_val);
    m_descLabel->setText(desc_val.isEmpty() ? "No description" : desc_val);
    m_archLabel->setText(arch_val.isEmpty() ? "Unknown" : arch_val);
    m_sizeLabel->setText(size_val.isEmpty() ? "Unknown" : size_val);
}

void MainWindow::installPackage()
{
    if (m_installProcess) return;

    m_installButton->setEnabled(false);
    m_installButton->setText("Installing...");
    m_outputView->clear();
    m_outputView->append("Running: pkexec pacman -U \"" + m_packagePath + "\"\n");

    m_installProcess = new QProcess(this);
    connect(m_installProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onInstallReadyRead);
    connect(m_installProcess, &QProcess::readyReadStandardError, this, &MainWindow::onErrorReadyRead);
    connect(m_installProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onInstallFinished);

    m_installProcess->start("pkexec", {"pacman", "-U", "--noconfirm", m_packagePath});
}

void MainWindow::onInstallReadyRead()
{
    QString output = m_installProcess->readAllStandardOutput();
    m_outputView->moveCursor(QTextCursor::End);
    m_outputView->insertPlainText(output);
    m_outputView->verticalScrollBar()->setValue(m_outputView->verticalScrollBar()->maximum());
}

void MainWindow::onErrorReadyRead()
{
    QString output = m_installProcess->readAllStandardError();
    m_outputView->moveCursor(QTextCursor::End);
    m_outputView->insertPlainText(output);
    m_outputView->verticalScrollBar()->setValue(m_outputView->verticalScrollBar()->maximum());
}

void MainWindow::onInstallFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        m_outputView->append("\n--- Installation process crashed ---");
    } else if (exitCode != 0) {
        m_outputView->append("\n--- Installation failed (exit code " + QString::number(exitCode) + ") ---");
    } else {
        m_outputView->append("\n--- Package installed successfully ---");
    }

    m_installProcess->deleteLater();
    m_installProcess = nullptr;

    m_installButton->setEnabled(true);
    m_installButton->setText("Install");
}
