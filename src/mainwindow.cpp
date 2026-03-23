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
#include <QPropertyAnimation>

MainWindow::MainWindow(const QString &packagePath, QWidget *parent)
    : QMainWindow(parent)
    , m_packagePath(packagePath)
    , m_actionProcess(nullptr)
{
    setupUi();
    loadPackageInfo();
}

void MainWindow::setupUi()
{
    setWindowTitle("paccy");
    setMinimumSize(480, 400);
    resize(520, 460);

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
    m_installedLabel = makeInfoRow("Installed:");

    mainLayout->addWidget(m_infoGroup);

    // Status/throbber area - hidden by default
    auto *statusContainer = new QWidget();
    auto *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(10);

    m_throbber = new QProgressBar();
    m_throbber->setRange(0, 0);
    m_throbber->setFixedHeight(6);
    m_throbber->setFixedWidth(120);
    m_throbber->setTextVisible(false);
    statusLayout->addWidget(m_throbber);

    m_statusLabel = new QLabel("Ready");
    QFont statusFont = m_statusLabel->font();
    statusFont.setItalic(true);
    m_statusLabel->setFont(statusFont);
    statusLayout->addWidget(m_statusLabel, 1);

    m_statusContainer = statusContainer;
    m_statusContainer->hide();
    mainLayout->addWidget(m_statusContainer);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(sep);

    // Collapsible output toggle
    m_outputToggle = new QToolButton();
    m_outputToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_outputToggle->setArrowType(Qt::RightArrow);
    m_outputToggle->setText("  Output");
    m_outputToggle->setCheckable(true);
    m_outputToggle->setChecked(false);
    m_outputToggle->setStyleSheet(
        "QToolButton { border: none; padding: 4px; font-weight: bold; }"
        "QToolButton:hover { background: palette(highlight); color: palette(highlighted-text); }"
    );
    connect(m_outputToggle, &QToolButton::clicked, this, &MainWindow::toggleOutput);
    mainLayout->addWidget(m_outputToggle);

    // Output container - hidden by default
    m_outputContainer = new QWidget();
    auto *outputLayout = new QVBoxLayout(m_outputContainer);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->setSpacing(0);

    m_outputView = new QTextEdit();
    m_outputView->setReadOnly(true);
    m_outputView->setMaximumHeight(150);
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(9);
    m_outputView->setFont(monoFont);
    m_outputView->setPlaceholderText("Output will appear here...");
    outputLayout->addWidget(m_outputView);

    m_outputContainer->hide();
    mainLayout->addWidget(m_outputContainer);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_closeButton = new QPushButton("Close");
    m_closeButton->setFixedWidth(100);
    connect(m_closeButton, &QPushButton::clicked, this, &QMainWindow::close);

    m_actionButton = new QPushButton("Install");
    m_actionButton->setFixedWidth(100);
    m_actionButton->setDefault(true);
    QFont installFont = m_actionButton->font();
    installFont.setBold(true);
    m_actionButton->setFont(installFont);
    connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::installPackage);

    buttonLayout->addWidget(m_closeButton);
    buttonLayout->addWidget(m_actionButton);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addStretch();
    setCentralWidget(central);
}

void MainWindow::toggleOutput()
{
    bool expanded = m_outputToggle->isChecked();
    m_outputContainer->setVisible(expanded);
    m_outputToggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);

    if (expanded) {
        resize(width(), height() + 160);
    } else {
        resize(width(), height() - 160);
    }
}

void MainWindow::showActivity(const QString &text)
{
    m_throbber->show();
    m_statusContainer->show();
    m_statusLabel->setText(text);
}

void MainWindow::hideActivity()
{
    m_statusContainer->hide();
}

void MainWindow::loadPackageInfo()
{
    QFileInfo fi(m_packagePath);
    if (!fi.exists()) {
        m_nameLabel->setText("File not found");
        m_actionButton->setEnabled(false);
        return;
    }

    // Check valid extension
    QString name = fi.fileName();
    if (!(name.endsWith(".pkg.tar.zst") || name.endsWith(".pkg.tar.xz") || name.endsWith(".pkg.tar.gz") || name.endsWith(".pkg.tar"))) {
        m_nameLabel->setText("Invalid package");
        m_actionButton->setEnabled(false);
        return;
    }

    // Read .PKGINFO from archive
    QProcess proc;

    // Try natively first (libarchive with zstd support)
    proc.start("tar", {"-xf", m_packagePath, "-O", ".PKGINFO"});
    proc.waitForFinished(5000);

    QString pkginfo = proc.readAllStandardOutput();

    // If that failed, try with explicit decompressor
    if (pkginfo.isEmpty()) {
        proc.start("tar", {"--use-compress-program=zstd -d", "-xf", m_packagePath, "-O", ".PKGINFO"});
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

    m_packageName = name_val;
    m_nameLabel->setText(name_val.isEmpty() ? fi.baseName() : name_val);
    m_versionLabel->setText(version_val.isEmpty() ? "Unknown" : version_val);
    m_descLabel->setText(desc_val.isEmpty() ? "No description" : desc_val);
    m_archLabel->setText(arch_val.isEmpty() ? "Unknown" : arch_val);
    m_sizeLabel->setText(size_val.isEmpty() ? "Unknown" : size_val);

    checkIfInstalled();
}

void MainWindow::checkIfInstalled()
{
    if (m_packageName.isEmpty()) return;

    QProcess proc;
    proc.start("pacman", {"-Qi", m_packageName});
    proc.waitForFinished(3000);

    m_isInstalled = (proc.exitCode() == 0);

    if (m_isInstalled) {
        m_installedLabel->setText("Yes");
        m_actionButton->setText("Uninstall");
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::installPackage);
        connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::uninstallPackage);
    } else {
        m_installedLabel->setText("No");
        m_actionButton->setText("Install");
        disconnect(m_actionButton, &QPushButton::clicked, this, &MainWindow::uninstallPackage);
        connect(m_actionButton, &QPushButton::clicked, this, &MainWindow::installPackage);
    }
}

void MainWindow::installPackage()
{
    if (m_actionProcess) return;

    m_actionButton->setEnabled(false);
    m_actionButton->setText("Installing...");
    m_outputView->clear();
    m_outputView->append("Running: pkexec pacman -U \"" + m_packagePath + "\"\n");

    // Show output and throbber
    if (!m_outputToggle->isChecked()) toggleOutput();
    showActivity("Installing package...");

    m_actionProcess = new QProcess(this);
    connect(m_actionProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessReadyRead);
    connect(m_actionProcess, &QProcess::readyReadStandardError, this, &MainWindow::onProcessErrorRead);
    connect(m_actionProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onProcessFinished);

    m_actionProcess->start("pkexec", {"pacman", "-U", "--noconfirm", m_packagePath});
}

void MainWindow::uninstallPackage()
{
    if (m_actionProcess) return;

    m_actionButton->setEnabled(false);
    showActivity("Checking dependencies...");

    // Dry run to check for dependency issues
    QProcess checkProc;
    checkProc.start("pacman", {"-Rsc", "--print", m_packageName});
    checkProc.waitForFinished(5000);

    QString stdoutOut = checkProc.readAllStandardOutput().trimmed();
    QString stderrOut = checkProc.readAllStandardError().trimmed();

    if (checkProc.exitCode() != 0) {
        // Dependency conflict detected
        hideActivity();
        m_actionButton->setEnabled(true);

        // Parse :: lines from stderr for the actual dependency conflicts
        QStringList conflicts;
        for (const QString &line : stderrOut.split('\n')) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith(":: "))
                conflicts.append(trimmed.mid(3));
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Cannot Uninstall");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Cannot remove <b>" + m_packageName + "</b>.");
        msgBox.setInformativeText(conflicts.isEmpty()
            ? stderrOut
            : conflicts.join("\n"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    // Parse what would be removed - if more than just the target, warn
    QStringList removedList;
    for (const QString &line : stdoutOut.split('\n')) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            removedList.append(trimmed);
    }

    // removedList items are like "7zip-26.00-1.1" from pacman --print output
    if (removedList.size() > 1) {
        hideActivity();
        m_actionButton->setEnabled(true);

        // Build a readable list of extra packages
        QStringList extras;
        for (const QString &entry : removedList) {
            if (!entry.contains(m_packageName))
                extras.append(entry);
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Confirm Removal");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Removing <b>" + m_packageName + "</b> will also remove the following:");
        msgBox.setInformativeText(extras.join("\n"));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.button(QMessageBox::Ok)->setText("Remove All");
        msgBox.button(QMessageBox::Cancel)->setText("Cancel");

        if (msgBox.exec() != QMessageBox::Ok) {
            return;
        }
    }

    // Proceed with uninstall
    m_actionButton->setEnabled(false);
    m_actionButton->setText("Uninstalling...");
    m_outputView->clear();
    m_outputView->append("Running: pkexec pacman -R \"" + m_packageName + "\"\n");

    if (!m_outputToggle->isChecked()) toggleOutput();
    showActivity("Removing package...");

    m_actionProcess = new QProcess(this);
    connect(m_actionProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessReadyRead);
    connect(m_actionProcess, &QProcess::readyReadStandardError, this, &MainWindow::onProcessErrorRead);
    connect(m_actionProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onProcessFinished);

    m_actionProcess->start("pkexec", {"pacman", "-Rscn", "--noconfirm", m_packageName});
}

void MainWindow::onProcessReadyRead()
{
    QString output = m_actionProcess->readAllStandardOutput();
    m_outputView->moveCursor(QTextCursor::End);
    m_outputView->insertPlainText(output);
    m_outputView->verticalScrollBar()->setValue(m_outputView->verticalScrollBar()->maximum());
}

void MainWindow::onProcessErrorRead()
{
    QString output = m_actionProcess->readAllStandardError();
    m_outputView->moveCursor(QTextCursor::End);
    m_outputView->insertPlainText(output);
    m_outputView->verticalScrollBar()->setValue(m_outputView->verticalScrollBar()->maximum());
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    bool wasInstall = (m_actionButton->text() == "Installing...");

    if (status == QProcess::CrashExit) {
        m_outputView->append("\n--- Process crashed ---");
        m_statusLabel->setText("Error");
    } else if (exitCode != 0) {
        m_outputView->append("\n--- Operation failed (exit code " + QString::number(exitCode) + ") ---");
        m_statusLabel->setText("Failed");
    } else {
        m_outputView->append(wasInstall ? "\n--- Package installed successfully ---" : "\n--- Package uninstalled successfully ---");
        m_statusLabel->setText("Done");
    }

    // Replace throbber with status text
    m_throbber->hide();

    m_actionProcess->deleteLater();
    m_actionProcess = nullptr;

    m_actionButton->setEnabled(true);
    checkIfInstalled();
}
