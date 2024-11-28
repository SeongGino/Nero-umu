/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    GUI manager frontend.

    Copyright (C) 2024  That One Seong

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "neromanager.h"
#include "./ui_neromanager.h"
#include "nerofs.h"
#include "neroico.h"
#include "neropreferences.h"
#include "neroprefixsettings.h"
#include "nerorunner.h"
#include "nerorunnerdialog.h"
#include "neroshortcut.h"
#include "nerotricks.h"
#include "nerowizard.h"

#include <QCryptographicHash>
#include <QFileDialog>
#include <QProcess>
#include <QTimer>
#include <QDebug>

NeroManagerWindow::NeroManagerWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NeroManagerWindow)
{
    // rand + undefined int, bit shifted to only give the three least significant bytes (0-7)
    // THIS can be set before window setup...
    switch(((LOLRANDOM + rand()) >> 29)) {
    case 0: this->setWindowIcon(QIcon(":/ico/narikiri/stahn")); break;
    case 1: this->setWindowIcon(QIcon(":/ico/narikiri/rutee")); break;
    case 2: this->setWindowIcon(QIcon(":/ico/narikiri/mary")); break;
    case 3: this->setWindowIcon(QIcon(":/ico/narikiri/chelsea")); break;
    case 4: this->setWindowIcon(QIcon(":/ico/narikiri/philia")); break;
    case 5: this->setWindowIcon(QIcon(":/ico/narikiri/lilith")); break;
    case 6: this->setWindowIcon(QIcon(":/ico/narikiri/woodrow")); break;
    case 7: this->setWindowIcon(QIcon(":/ico/narikiri/kongman")); break;
    }

    // required for good hidpi icon quality... because Qt doesn't set this automatically?
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    if(NeroFS::GetUmU().isEmpty()) {
        QMessageBox::critical(this,
                              "UMU!?",
                              "It seems like umu isn't detected as installed on your system!\n"
                              "Nero and Proton runners will not function without umu.\n"
                              "Please install umu from your package manager.\n\n"
                              "Nero will now exit, umu.");
        exit(1);
    }

    // load initial data
    if(!NeroFS::InitPaths()) { exit(1); }
    NeroFS::GetAvailableProtons();
    managerCfg = new QSettings(NeroFS::GetManagerCfg());
    managerCfg->beginGroup("NeroSettings");

    listFont.setPointSize(12);

    /* ^^ pre-UI popup  */

    ui->setupUi(this);

    // ...but why does this need to be set AFTER the window setup?
    this->setWindowTitle("Nero Manager");

    /* vv post-UI popup */

    if(managerCfg->value("WinSize").isValid())
        this->resize(managerCfg->value("WinSize").toSize());

    sysTray = new QSystemTrayIcon(this->windowIcon(), this);
    for(auto &action : sysTrayActions)
        sysTrayMenu.addAction(&action);
    connect(&sysTrayActions[0], &QAction::triggered, this, &NeroManagerWindow::actionExit_activated);
    sysTray->setContextMenu(&sysTrayMenu);
    sysTray->show();
    sysTray->setToolTip("Nero Manager");
    connect(sysTray, &QSystemTrayIcon::activated, this, &NeroManagerWindow::sysTray_activated);

    ui->prefixContentsScrollArea->setVisible(false);

    if(NeroFS::GetWinetricks().isEmpty()) {
        ui->prefixTricksBtn->setEnabled(false);
        ui->prefixTricksBtn->setText("Winetricks Not Found");
        ui->prefixTricksBtn->setStyleSheet("color: red");
    }

    blinkTimer = new QTimer();
    connect(blinkTimer, &QTimer::timeout, this, &NeroManagerWindow::blinkTimer_timeout);

    RenderPrefixes();
    SetHeader();
}

NeroManagerWindow::~NeroManagerWindow()
{
    managerCfg->setValue("WinSize", this->size());
    managerCfg->sync();
    delete ui;
}

// This also changes the "panel" from prefixes view to shortcuts view.
void NeroManagerWindow::SetHeader(const QString prefix, const unsigned int shortcutsCount)
{
    if(prefix.isEmpty()) {
        prefixIsSelected = false;
        ui->topTitle->setText("Select a Prefix");
        ui->topSubtitle->setVisible(false);
        ui->prefixContentsScrollArea->setVisible(false);
        ui->prefixesScrollArea->setVisible(true);
        ui->backButton->setEnabled(false);
        ui->backButton->setToolTip("");
        ui->backButton->setIcon(QIcon::fromTheme("user-bookmarks"));
        ui->addButton->setIcon(QIcon::fromTheme("folder-new"));
        ui->addButton->setToolTip("Create a new prefix.");
        ui->addButton->clearFocus();
        ui->oneTimeRunBtn->setVisible(false);
        ui->oneTimeRunArgs->setVisible(false);

        if(NeroFS::GetPrefixes().isEmpty()) { StartBlinkTimer(); }
        else { StopBlinkTimer(); }
    } else {
        prefixIsSelected = true;
        ui->topTitle->setText(prefix);
        ui->topSubtitle->setVisible(true);
        ui->prefixesScrollArea->setVisible(false);
        ui->prefixContentsScrollArea->setVisible(true);
        ui->backButton->setEnabled(true);
        ui->backButton->setIcon(QIcon::fromTheme("go-previous"));
        ui->backButton->setToolTip("Go back to prefixes list.");
        ui->backButton->clearFocus();
        ui->addButton->clearFocus();
        ui->addButton->setIcon(QIcon::fromTheme("list-add"));
        ui->addButton->setToolTip("Add a new shortcut to this prefix.");
        ui->oneTimeRunBtn->setVisible(true);
        ui->oneTimeRunArgs->setVisible(true);
        ui->oneTimeRunArgs->clear();

        if(shortcutsCount) {
            ui->topSubtitle->setText(QString("%1 Apps").arg(shortcutsCount));
            StopBlinkTimer();
        } else {
            ui->topSubtitle->setText("No apps registered. Click the + button to add one.");
            StartBlinkTimer();
        }
    }
}

void NeroManagerWindow::RenderPrefixes()
{
    // TODO: use user-provided sorting option? StringList only provides "ascending" sort.
    // also doing this check twice is redundant af. Just doing it this way rn to maintain sortability.
    if(NeroFS::GetPrefixes().isEmpty()) {
        StartBlinkTimer();
    } else {
        StopBlinkTimer();

        for(int i = 0; i < NeroFS::GetPrefixes().count(); i++) {
            prefixMainButton << new QPushButton(NeroFS::GetPrefixes().at(i));
            prefixDeleteButton << new QPushButton(QIcon::fromTheme("edit-delete"), "");

            prefixMainButton.at(i)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            prefixMainButton.at(i)->setFont(listFont);
            prefixMainButton.at(i)->setProperty("slot", i);

            prefixDeleteButton.at(i)->setFlat(true);
            prefixDeleteButton.at(i)->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            prefixDeleteButton.at(i)->setToolTip(QString("Delete %1").arg(NeroFS::GetPrefixes().at(i)));
            prefixDeleteButton.at(i)->setProperty("slot", i);

            ui->prefixesList->addWidget(prefixMainButton.at(i), i, 0);
            ui->prefixesList->addWidget(prefixDeleteButton.at(i), i, 1);

            connect(prefixMainButton.at(i),   SIGNAL(clicked()), this, SLOT(prefixMainButtons_clicked()));
            connect(prefixDeleteButton.at(i), SIGNAL(clicked()), this, SLOT(prefixDeleteButtons_clicked()));
        }
    }
}

void NeroManagerWindow::RenderPrefixList()
{
    QSettings *currentPrefixIni = NeroFS::GetCurrentPrefixCfg();
    currentPrefixIni->beginGroup("Shortcuts");

    if(!currentPrefixIni->childKeys().isEmpty()) {
        QStringList sortedShortcuts = NeroFS::GetCurrentPrefixShortcuts();

        // TODO: implement sorting options here(?)
        sortedShortcuts.sort(Qt::CaseInsensitive);

        QMap<QString, QString> hashMap = NeroFS::GetCurrentShortcutsMap();

        // now start adding things
        for(int i = 0; i < sortedShortcuts.count(); i++) {
            if(QFile::exists(QString("%1/%2/.icoCache/%3").arg(NeroFS::GetPrefixesPath().path(),
                                                               NeroFS::GetCurrentPrefix(),
                                                                QString("%1-%2.png").arg(sortedShortcuts.at(i), hashMap[sortedShortcuts.at(i)])))) {
                prefixShortcutIco << new QIcon(QPixmap(QString("%1/%2/.icoCache/%3").arg(NeroFS::GetPrefixesPath().path(),
                                                                                         NeroFS::GetCurrentPrefix(),
                                                                                         QString("%1-%2.png").arg(sortedShortcuts.at(i), hashMap[sortedShortcuts.at(i)]))));
            } else prefixShortcutIco << new QIcon(QIcon::fromTheme("application-x-executable"));

            prefixShortcutIcon << new QLabel();
            // real talk: Silent Hill The Arcade can suck it. 16x16 in 2007, seriously???
            if(prefixShortcutIco.at(i)->actualSize(QSize(24,24)).height() < 24)
                prefixShortcutIcon.at(i)->setPixmap(prefixShortcutIco.at(i)->pixmap(prefixShortcutIco.at(i)->actualSize(QSize(24,24))).scaled(24,24,Qt::KeepAspectRatio,Qt::SmoothTransformation));
            else prefixShortcutIcon.at(i)->setPixmap(prefixShortcutIco.at(i)->pixmap(24,24));
            prefixShortcutIcon.at(i)->setAlignment(Qt::AlignCenter);

            prefixShortcutLabel << new QLabel(sortedShortcuts.at(i));
            prefixShortcutLabel.at(i)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            // media-playback-start should change to media-playback-stop when being slot is being played.
            prefixShortcutPlayButton << new QPushButton(QIcon::fromTheme("media-playback-start"), "");
            prefixShortcutPlayButton.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            prefixShortcutPlayButton.at(i)->setToolTip("Start " + sortedShortcuts.at(i));
            prefixShortcutPlayButton.at(i)->setIconSize(QSize(16, 16));
            prefixShortcutPlayButton.at(i)->setProperty("slot", i);

            prefixShortcutEditButton << new QPushButton(QIcon::fromTheme("document-properties"), "");
            prefixShortcutEditButton.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            prefixShortcutEditButton.at(i)->setIconSize(QSize(16, 16));
            prefixShortcutEditButton.at(i)->setToolTip("Edit properties of " + sortedShortcuts.at(i));
            prefixShortcutEditButton.at(i)->setFlat(true);
            prefixShortcutEditButton.at(i)->setProperty("slot", i);

            ui->prefixContentsGrid->addWidget(prefixShortcutIcon.at(i), i, 0);
            ui->prefixContentsGrid->addWidget(prefixShortcutLabel.at(i), i, 1);
            ui->prefixContentsGrid->addWidget(prefixShortcutPlayButton.at(i), i, 2, Qt::AlignLeft);
            ui->prefixContentsGrid->addWidget(prefixShortcutEditButton.at(i), i, 3, Qt::AlignLeft);

            connect(prefixShortcutPlayButton.at(i), SIGNAL(clicked()), this, SLOT(prefixShortcutPlayButtons_clicked()));
            connect(prefixShortcutEditButton.at(i), SIGNAL(clicked()), this, SLOT(prefixShortcutEditButtons_clicked()));
        }

        ui->prefixContentsGrid->setColumnStretch(1, 1);
    }
}

void NeroManagerWindow::CreatePrefix(const QString newPrefix, const QString runner, QStringList tricksToInstall)
{
    QProcess umu;
    QMessageBox waitBox(QMessageBox::NoIcon, "Generating Prefix", "Please wait...", QMessageBox::NoButton, this, Qt::Dialog | Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    env.insert("WINEPREFIX", QString("%1/%2").arg(NeroFS::GetPrefixesPath().path(), newPrefix));
    env.insert("GAMEID", "0");
    env.insert("PROTONPATH", QString("%1/%2").arg(NeroFS::GetProtonsPath().path(), runner));
    env.insert("UMU_RUNTIME_UPDATE", "0");
    umu.setProcessEnvironment(env);
    umu.setProcessChannelMode(QProcess::MergedChannels);

    if(tricksToInstall.isEmpty()) {
        umu.start(NeroFS::GetUmU(), {"createprefix"});
    } else {
        tricksToInstall.prepend("winetricks");
        umu.start(NeroFS::GetUmU(), tricksToInstall);
        tricksToInstall.removeFirst();
    }

    waitBox.open();
    waitBox.raise();
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // don't use blocking function so that the dialog shows and the UI doesn't freeze.
    while(umu.state() != QProcess::NotRunning) {
        QApplication::processEvents();
        QByteArray stdout;
        // TODO: use QTextStream instead of printing line-by-line?
        umu.waitForReadyRead(1000);
        if(umu.canReadLine()) {
            stdout = umu.readLine();
            printf(stdout);
            if(stdout.contains("Proton: Upgrading")) {
                waitBox.setText(QString("Creating prefix %1 using %2...").arg(newPrefix, runner));
            } else if(stdout.contains("Downloading latest steamrt sniper")) {
                waitBox.setText("umu: Updating runtime to latest version...");
            } else if(stdout.contains("Proton: Running winetricks verbs in prefix:")) {
                waitBox.setText(QString("Running installations for Winetricks verbs:\n\n%1\n\nThis stage may take a while...").arg(tricksToInstall.join('\n')));
            }
        }
    }

    if(umu.exitCode() >= 0) {
        NeroFS::AddNewPrefix(newPrefix, runner);

        if(!NeroFS::GetPrefixes().isEmpty()) {
            StopBlinkTimer();
        }

        unsigned int pos = prefixMainButton.count();

        prefixMainButton << new QPushButton(newPrefix);
        prefixDeleteButton << new QPushButton(QIcon::fromTheme("edit-delete"), "");

        prefixMainButton.at(pos)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        prefixMainButton.at(pos)->setFont(listFont);
        prefixMainButton.at(pos)->setProperty("slot", pos);

        prefixDeleteButton.at(pos)->setFlat(true);
        prefixDeleteButton.at(pos)->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        prefixDeleteButton.at(pos)->setToolTip(QString("Delete %1").arg(newPrefix));
        prefixDeleteButton.at(pos)->setProperty("slot", pos);

        ui->prefixesList->addWidget(prefixMainButton.at(pos), pos, 0);
        ui->prefixesList->addWidget(prefixDeleteButton.at(pos), pos, 1);

        connect(prefixMainButton.at(pos),   SIGNAL(clicked()), this, SLOT(prefixMainButtons_clicked()));
        connect(prefixDeleteButton.at(pos), SIGNAL(clicked()), this, SLOT(prefixDeleteButtons_clicked()));

        QApplication::alert(this);
    }
    QGuiApplication::restoreOverrideCursor();
}

void NeroManagerWindow::AddTricks(QStringList verbs, const QString prefix)
{
    QProcess umu;
    QMessageBox waitBox(QMessageBox::NoIcon, "Generating Prefix", "Please wait...", QMessageBox::NoButton, this, Qt::Dialog | Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QMap<QString, QVariant> settingsMap = NeroFS::GetCurrentPrefixSettings();

    env.insert("WINEPREFIX", QString("%1/%2").arg(NeroFS::GetPrefixesPath().path(), prefix));
    env.insert("GAMEID", "0");
    env.insert("PROTONPATH", QString("%1/%2").arg(NeroFS::GetProtonsPath().path(), settingsMap["CurrentRunner"].toString()));
    env.insert("UMU_RUNTIME_UPDATE", "0");
    umu.setProcessEnvironment(env);
    umu.setProcessChannelMode(QProcess::MergedChannels);


    verbs.prepend("winetricks");
    umu.start(NeroFS::GetUmU(), verbs);
    verbs.removeFirst();

    waitBox.open();
    waitBox.raise();
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // don't use blocking function so that the dialog shows and the UI doesn't freeze.
    while(umu.state() != QProcess::NotRunning) {
        QApplication::processEvents();
        QByteArray stdout;
        // TODO: use QTextStream instead of printing line-by-line?
        umu.waitForReadyRead(100);
        if(umu.canReadLine()) {
            stdout = umu.readLine();
            printf(stdout);
            if(stdout.contains("Proton: Upgrading")) {
                waitBox.setText(QString("Updating %1 with new Proton %2...").arg(prefix, settingsMap["CurrentRunner"].toString()));
            } else if(stdout.contains("Downloading latest steamrt sniper")) {
                waitBox.setText("umu: Updating runtime to latest version...");
            } else if(stdout.contains("Proton: Running winetricks verbs in prefix:")) {
                waitBox.setText(QString("Running installations for Winetricks verbs:\n\n%1\n\nThis stage may take a while...").arg(verbs.join('\n')));
            }
        }
    }

    if(umu.exitCode() >= 0) { QApplication::alert(this); }
    QGuiApplication::restoreOverrideCursor();
}

void NeroManagerWindow::on_addButton_clicked()
{
    ui->addButton->setStyleSheet("");
    ui->addButton->setFlat(true);
    blinkTimer->stop();

    if(prefixIsSelected) {
        QString newApp(QFileDialog::getOpenFileName(this,
                                                    "Select a Windows Executable",
                                                    qEnvironmentVariable("HOME"),
        "Compatible Windows Files (*.bat *.exe *.msi);;Windows Batch Script Files (*.bat);;Windows Executable (*.exe);;Windows Installer Package (*.msi)",
                                                    nullptr,
                                                    QFileDialog::DontResolveSymlinks));

        if(!newApp.isEmpty()) {
            NeroShortcutWizard shortcutAdd(this, newApp);
            shortcutAdd.exec();

            if(!shortcutAdd.appPath.isEmpty()) {
                // hash function here
                QString hashName(QCryptographicHash::hash(QString(LOLRANDOM).toLocal8Bit(), QCryptographicHash::Md5).toHex(0));

                QSettings *currentPrefixIni = NeroFS::GetCurrentPrefixCfg();
                currentPrefixIni->beginGroup("Shortcuts");

                // if this hash matches anything, repeatedly generate hashes until a unique one is found
                while(true) {
                    if(currentPrefixIni->childKeys().contains(hashName)) {
                        hashName = QCryptographicHash::hash(QString(LOLRANDOM+rand()).toLocal8Bit(), QCryptographicHash::Md5).toHex(0);
                    } else {
                        break;
                    }
                }

                NeroFS::AddNewShortcut(hashName, shortcutAdd.shortcutName, shortcutAdd.appPath);

                // because the Shortcuts getter always returns a resorted list, just add to the bottom for user convenience.
                unsigned int pos = ui->prefixContentsGrid->rowCount();

                if(shortcutAdd.appIcon.isEmpty()) {
                    prefixShortcutIco << new QIcon(QIcon::fromTheme("application-x-executable"));
                } else {
                    QFile::copy(shortcutAdd.appIcon, QString("%1/%2/.icoCache/%3").arg(NeroFS::GetPrefixesPath().path(),
                                                                                        NeroFS::GetCurrentPrefix(),
                                                                                        QString("%1-%2.png").arg(shortcutAdd.shortcutName, hashName)));
                    prefixShortcutIco << new QIcon(QPixmap(QString("%1/%2/.icoCache/%3").arg(NeroFS::GetPrefixesPath().path(),
                                                                                        NeroFS::GetCurrentPrefix(),
                                                                                        QString("%1-%2.png").arg(shortcutAdd.shortcutName, hashName))));
                }

                prefixShortcutIcon << new QLabel();
                // real talk: Silent Hill The Arcade can suck it. 16x16 in 2007, seriously???
                if(prefixShortcutIco.at(pos)->actualSize(QSize(24,24)).height() < 24)
                    prefixShortcutIcon.at(pos)->setPixmap(prefixShortcutIco.at(pos)->pixmap(prefixShortcutIco.at(pos)->actualSize(QSize(24,24))).scaled(24,24,Qt::KeepAspectRatio,Qt::SmoothTransformation));
                else prefixShortcutIcon.at(pos)->setPixmap(prefixShortcutIco.at(pos)->pixmap(24,24));
                prefixShortcutIcon.at(pos)->setAlignment(Qt::AlignCenter);

                prefixShortcutLabel << new QLabel(shortcutAdd.shortcutName);
                prefixShortcutLabel.at(pos)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                // media-playback-start should change to media-playback-stop when playing
                prefixShortcutPlayButton << new QPushButton(QIcon::fromTheme("media-playback-start"), "");
                prefixShortcutPlayButton.at(pos)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                prefixShortcutPlayButton.at(pos)->setIconSize(QSize(16, 16));
                prefixShortcutPlayButton.at(pos)->setToolTip("Start "+shortcutAdd.shortcutName);
                prefixShortcutPlayButton.at(pos)->setProperty("slot", pos);

                prefixShortcutEditButton << new QPushButton(QIcon::fromTheme("document-properties"), "");
                prefixShortcutEditButton.at(pos)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                prefixShortcutEditButton.at(pos)->setIconSize(QSize(16, 16));
                prefixShortcutEditButton.at(pos)->setToolTip("Edit properties of " + shortcutAdd.shortcutName);
                prefixShortcutEditButton.at(pos)->setFlat(true);
                prefixShortcutEditButton.at(pos)->setProperty("slot", pos);

                ui->prefixContentsGrid->addWidget(prefixShortcutIcon.at(pos), pos, 0);
                ui->prefixContentsGrid->addWidget(prefixShortcutLabel.at(pos), pos, 1);
                ui->prefixContentsGrid->addWidget(prefixShortcutPlayButton.at(pos), pos, 2, Qt::AlignLeft);
                ui->prefixContentsGrid->addWidget(prefixShortcutEditButton.at(pos), pos, 3, Qt::AlignLeft);

                connect(prefixShortcutPlayButton.at(pos), SIGNAL(clicked()), this, SLOT(prefixShortcutPlayButtons_clicked()));
                connect(prefixShortcutEditButton.at(pos), SIGNAL(clicked()), this, SLOT(prefixShortcutEditButtons_clicked()));

                if(pos == 0) { ui->prefixContentsGrid->setColumnStretch(1, 1); }

                SetHeader(NeroFS::GetCurrentPrefix(), NeroFS::GetCurrentPrefixShortcuts().count());
            }
        }

    } else {
        NeroPrefixWizard wizard(this);
        wizard.setFixedSize(wizard.size());
        wizard.exec();

        if(wizard.result() == QDialog::Accepted) {
            CreatePrefix(wizard.prefixName, NeroFS::GetAvailableProtons().at(wizard.protonRunner), wizard.verbsToInstall);

            if(wizard.userSymlinks) { NeroFS::CreateUserLinks(wizard.prefixName); }
        } else {
            if(NeroFS::GetPrefixes().isEmpty()) { StartBlinkTimer(); }
        }
    }
}

void NeroManagerWindow::on_backButton_clicked()
{
    // this also handles the page toggling
    if(prefixIsSelected) {
        if(currentlyRunning.count() > 0) {
            if(runnerWindow == nullptr) {
                runnerWindow = new NeroRunnerDialog(this);
                runnerWindow->SetupWindow(false, "all running apps in current prefix");
                runnerWindow->show();
            }
            for(int i = threadsCount; i > 0; i--) {
                // for the current prefix, we only need to run the prefix kill command once to end them all!
                if(umuController[i-1] != nullptr) {
                    umuController.at(i-1)->Stop();
                    break;
                }
            }
        } else SetHeader();
    } else {
        qDebug() << "TODO: implement favorites";
    }
}

void NeroManagerWindow::prefixMainButtons_clicked()
{
    int slot = sender()->property("slot").toInt();

    if(NeroFS::GetCurrentPrefix() != prefixMainButton.at(slot)->text()) {
        if(prefixShortcutLabel.count())
            CleanupShortcuts();

        NeroFS::SetCurrentPrefix(prefixMainButton.at(slot)->text());

        RenderPrefixList();
    }

    SetHeader(NeroFS::GetCurrentPrefix(), NeroFS::GetCurrentPrefixShortcuts().count());
}

void NeroManagerWindow::prefixDeleteButtons_clicked()
{
    int slot = sender()->property("slot").toInt();

    if(QMessageBox::question(this,
                             "Removing Prefix",
                             QString("Are you sure you wish to delete %1?\n\n"
                                     "All data inside the prefix will be deleted.\n"
                                     "This operation CAN NOT BE UNDONE.")
                             .arg(prefixMainButton.at(slot)->text())
                            ) == QMessageBox::Yes)
    {
        if(NeroFS::DeletePrefix(prefixMainButton.at(slot)->text())) {
            if(NeroFS::GetCurrentPrefix() == prefixMainButton.at(slot)->text())
                CleanupShortcuts();

            delete prefixMainButton.at(slot);
            delete prefixDeleteButton.at(slot);
            prefixMainButton[slot] = nullptr;
            prefixDeleteButton[slot] = nullptr;
        }
    }
}

void NeroManagerWindow::prefixShortcutPlayButtons_clicked()
{

    int slot = sender()->property("slot").toInt();

    if(currentlyRunning.contains(slot)) {
        if(runnerWindow == nullptr) {
            runnerWindow = new NeroRunnerDialog(this);
            runnerWindow->SetupWindow(false, prefixShortcutLabel.at(slot)->text(), prefixShortcutIco.at(slot));
            runnerWindow->show();
        }

        umuController.at(prefixShortcutPlayButton.at(slot)->property("thread").toInt())->Stop();
    } else {
        prefixShortcutPlayButton.at(slot)->setIcon(QIcon::fromTheme("media-playback-stop"));
        prefixShortcutPlayButton.at(slot)->setToolTip("Stop " + prefixShortcutLabel.at(slot)->text());
        ui->backButton->setIcon(QIcon::fromTheme("media-playback-stop"));
        ui->backButton->setToolTip("Shut down all running programs in this prefix.");
        threadsCount += 1;
        currentlyRunning.append(slot);

        QMap<QString, QString> settings = NeroFS::GetCurrentShortcutsMap();

        if(managerCfg->value("ShortcutHidesManager").toBool())
            this->hide();

        if(runnerWindow == nullptr) {
            runnerWindow = new NeroRunnerDialog(this);
            runnerWindow->SetupWindow(true, prefixShortcutLabel.at(slot)->text(), prefixShortcutIco.at(slot));
            runnerWindow->show();
        }

        if(currentlyRunning.count() > 1)
            umuController << new NeroThreadController(slot, settings.value(prefixShortcutLabel.at(slot)->text()), {""}, true);
        else umuController << new NeroThreadController(slot, settings.value(prefixShortcutLabel.at(slot)->text()), {""});

        umuController.last()->setProperty("slot", threadsCount-1);
        prefixShortcutPlayButton.at(slot)->setProperty("thread", threadsCount-1);
        connect(umuController.last(), &NeroThreadController::passUmuResults, this, &NeroManagerWindow::handleUmuResults);
        connect(&umuController.last()->umuWorker->Runner, &NeroRunner::StatusUpdate, this, &NeroManagerWindow::handleUmuSignal);
        emit umuController.last()->operate();
    }
}

void NeroManagerWindow::prefixShortcutEditButtons_clicked()
{
    int slot = sender()->property("slot").toInt();

    QMap<QString, QString> settings = NeroFS::GetCurrentShortcutsMap();

    prefixSettings = new NeroPrefixSettingsWindow(this, settings.value(prefixShortcutLabel.at(slot)->text()));
    prefixSettings->setProperty("slot", slot);
    connect(prefixSettings, &NeroPrefixSettingsWindow::finished, this, &NeroManagerWindow::prefixSettings_result);
    prefixSettings->show();
}

void NeroManagerWindow::on_oneTimeRunBtn_clicked()
{
    QString oneTimeApp(QFileDialog::getOpenFileName(this,
                                                      "Select an Executable to Start in Prefix",
                                                      qEnvironmentVariable("HOME"),
    "Compatible Windows Executables (*.bat *.exe *.msi);;Windows Batch Script Files (*.bat);;Windows Portable Executable (*.exe);;Windows Installer Package (*.msi)",
                                                      nullptr,
                                                      QFileDialog::DontResolveSymlinks));

    if(!oneTimeApp.isEmpty()) {
        ui->backButton->setIcon(QIcon::fromTheme("media-playback-stop"));
        ui->backButton->setToolTip("Shut down all running programs in this prefix.");
        threadsCount += 1;
        currentlyRunning.append(-1);

        if(runnerWindow == nullptr) {
            QIcon icon;
            QString iconPath = NeroIcoExtractor::GetIcon(oneTimeApp);
            if(!iconPath.isEmpty())
                icon = QIcon(QPixmap(iconPath));
            runnerWindow = new NeroRunnerDialog(this);
            runnerWindow->setModal(true);
            runnerWindow->SetupWindow(true, oneTimeApp.mid(oneTimeApp.lastIndexOf('/')+1), &icon);
            runnerWindow->show();
            QDir tempDir(QString("%1/nero-manager").arg(QDir::tempPath()));
            tempDir.removeRecursively();
        }

        // TODO: flawed args split if the argument contains a path :/
        if(currentlyRunning.count() > 1)
            umuController << new NeroThreadController(-1, oneTimeApp, ui->oneTimeRunArgs->text().split(' '), true);
        else umuController << new NeroThreadController(-1, oneTimeApp, ui->oneTimeRunArgs->text().split(' '));

        umuController.last()->setProperty("slot", threadsCount-1);
        connect(umuController.last(), &NeroThreadController::passUmuResults, this, &NeroManagerWindow::handleUmuResults);
        connect(&umuController.last()->umuWorker->Runner, &NeroRunner::StatusUpdate, this, &NeroManagerWindow::handleUmuSignal);
        emit umuController.last()->operate();
    }
}

void NeroManagerWindow::CleanupShortcuts()
{
    if(!NeroFS::GetCurrentPrefixShortcuts().isEmpty()) {
        for(unsigned int i = 0; i < prefixShortcutLabel.count(); i++) {
            delete prefixShortcutIco[i];
            delete prefixShortcutIcon[i];
            delete prefixShortcutLabel[i];
            delete prefixShortcutPlayButton[i];
            delete prefixShortcutEditButton[i];
        }
        prefixShortcutIco.clear();
        prefixShortcutIcon.clear();
        prefixShortcutLabel.clear();
        prefixShortcutPlayButton.clear();
        prefixShortcutEditButton.clear();
    }
}

void NeroManagerWindow::on_prefixSettingsBtn_clicked()
{
    prefixSettings = new NeroPrefixSettingsWindow(this);
    prefixSettings->setProperty("slot", -1);
    connect(prefixSettings, &NeroPrefixSettingsWindow::finished, this, &NeroManagerWindow::prefixSettings_result);
    prefixSettings->show();
}

void NeroManagerWindow::on_prefixTricksBtn_clicked()
{
    // use winetricks.log as a basis.
    QFile winetricksLog(QString("%1/%2/winetricks.log").arg(NeroFS::GetPrefixesPath().path(),
                                                            NeroFS::GetCurrentPrefix()));
    QStringList verbsInstalled;

    if(winetricksLog.exists()) {
        if(winetricksLog.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while(!winetricksLog.atEnd()) { verbsInstalled.append(winetricksLog.readLine().trimmed()); }
            verbsInstalled.removeDuplicates();
        }
    } else {
        printf("Prefix has no winetricks file, skipping...\n");
    }

    NeroTricksWindow tricks(this);

    if(!verbsInstalled.isEmpty()) tricks.SetPreinstalledVerbs(verbsInstalled);

    bool confirmed = false;
    QStringList verbsToInstall;

    while(!confirmed) {
        tricks.exec();
        if(tricks.result() == QDialog::Accepted) {
            verbsToInstall.append(tricks.verbIsSelected.keys(true));
            verbsToInstall.removeDuplicates();
            if(QMessageBox::question(this,
                                      "Verbs Confirmation",
                                      QString("Are you sure you wish to install these verbs?\n\n%1").arg(verbsToInstall.join('\n')))
                == QMessageBox::Yes) {

                confirmed = true;
                AddTricks(verbsToInstall, NeroFS::GetCurrentPrefix());
            }
        } else {
            // user doesn't want to do verbs installation after all, so stop asking and revert to prev state.
            confirmed = true;
        }
    }
}

void NeroManagerWindow::prefixSettings_result()
{
    int slot = prefixSettings->property("slot").toInt();

    if(slot >= 0) {
        if(prefixSettings->result() == QDialog::Accepted) {
            // update app icon if changed
            if(!prefixSettings->newAppIcon.isEmpty()) {
                prefixShortcutIco.at(slot)->addFile(prefixSettings->newAppIcon);
                if(prefixShortcutIco.at(slot)->actualSize(QSize(24,24)).height() < 24)
                    prefixShortcutIcon.at(slot)->setPixmap(prefixShortcutIco.at(slot)->pixmap(prefixShortcutIco.at(slot)->actualSize(QSize(24,24))).scaled(24,24,Qt::KeepAspectRatio,Qt::SmoothTransformation));
                else prefixShortcutIcon.at(slot)->setPixmap(prefixShortcutIco.at(slot)->pixmap(24,24));
            }
            // update app name if changed
            if(prefixSettings->appName != prefixShortcutLabel.at(slot)->text()) {
                QMap<QString, QString> settings = NeroFS::GetCurrentShortcutsMap();
                NeroFS::SetCurrentPrefixCfg("Shortcuts", settings.value(prefixShortcutLabel.at(slot)->text()), prefixSettings->appName);
                // move existing ico (if any) to new name
                QFile ico(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.icoCache/"+
                          prefixShortcutLabel.at(slot)->text()+'-'+settings.value(prefixShortcutLabel.at(slot)->text())+".png");
                if(ico.exists())
                    ico.rename(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.icoCache/"+
                               prefixSettings->appName+'-'+settings.value(prefixShortcutLabel.at(slot)->text())+".png");
                prefixShortcutLabel.at(slot)->setText(prefixSettings->appName);
            }
        // delete shortcut signal
        } else if(prefixSettings->result() == -1) {
            QMap<QString, QString> settings = NeroFS::GetCurrentShortcutsMap();
            NeroFS::DeleteShortcut(settings.value(prefixShortcutLabel.at(slot)->text()));
            delete prefixShortcutIco[slot];
            delete prefixShortcutIcon[slot];
            delete prefixShortcutLabel[slot];
            delete prefixShortcutPlayButton[slot];
            delete prefixShortcutEditButton[slot];
            prefixShortcutIco[slot] = nullptr;
            prefixShortcutIcon[slot] = nullptr;
            prefixShortcutLabel[slot] = nullptr;
            prefixShortcutPlayButton[slot] = nullptr;
            prefixShortcutEditButton[slot] = nullptr;

            SetHeader(NeroFS::GetCurrentPrefix(), NeroFS::GetCurrentPrefixShortcuts().count());
        }
    }
    delete prefixSettings;
}

void NeroManagerWindow::on_managerSettings_clicked()
{
    prefs = new NeroManagerPreferences(this);
    prefs->BindSettings(managerCfg);
    prefs->setAttribute(Qt::WA_DeleteOnClose);
    prefs->show();
}

void NeroManagerWindow::sysTray_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason) {
    case QSystemTrayIcon::Trigger:
        if(this->isHidden()) this->show();
        else this->hide();
        break;
    // this doesn't seem to get used?
    //case QSystemTrayIcon::Context:
    //    break;
    default:
        break;
    }
}

void NeroManagerWindow::actionExit_activated()
{
    close();
}

void NeroManagerWindow::on_actionAbout_Nero_triggered()
{
    QString vInfo;
    #ifdef NERO_VERSION
    vInfo.append(QString("v%1").arg(NERO_VERSION));
    #endif // NERO_VERSION
    #ifdef NERO_GITHASH
    vInfo.append(QString("-%1").arg(NERO_GITHASH));
    #endif // NERO_GITHASH
    vInfo.append(QString("\nRunning on Qt %1.%2.%3").arg(QT_VERSION_MAJOR).arg(QT_VERSION_MINOR).arg(QT_VERSION_PATCH));
    QMessageBox::about(this,
                       "About Nero Manager",
                       QString(
                       "Nero Manager %1"
                       "\n\nA simple Proton manager.").arg(vInfo)
                       );
}


void NeroManagerWindow::blinkTimer_timeout()
{
    switch(blinkingState) {
    case 0:
        ui->addButton->setFlat(true);
        ui->addButton->setStyleSheet("");
        blinkingState++;
        break;
    case 1:
        blinkingState++;
        break;
    case 2:
        ui->addButton->setFlat(false);
        ui->addButton->setStyleSheet("background-color: #777777");
        blinkingState = 0;
        break;
    }
}

void NeroManagerWindow::StartBlinkTimer()
{
    blinkTimer->start(800);
    if(!prefixIsSelected) { ui->missingPrefixesLabel->setVisible(true); }
}

void NeroManagerWindow::StopBlinkTimer()
{
    ui->addButton->setStyleSheet("");
    ui->addButton->setFlat(true);
    blinkTimer->stop();
    if(!prefixIsSelected) { ui->missingPrefixesLabel->setVisible(false); }
}

// umu runner stuff here!
void NeroThreadWorker::umuRunnerProcess()
{
    int result;
    if(currentSlot >= 0) {
        // for shortcuts, parameters = hash
        result = Runner.StartShortcut(currentParameters, alreadyRunning);
    } else {
        // for one time, parameters = path, oneTimeArgs = contents of oneTimeArguments
        result = Runner.StartOnetime(currentParameters, oneTimeArgs, alreadyRunning);
    }

    emit umuExited(currentSlot, result);
}

void NeroManagerWindow::handleUmuResults(const int &buttonSlot, const int &result)
{
    const unsigned int threadSlot = sender()->property("slot").toInt();

    if(buttonSlot >= 0) {
        prefixShortcutPlayButton.at(buttonSlot)->setIcon(QIcon::fromTheme("media-playback-start"));
        prefixShortcutPlayButton.at(buttonSlot)->setToolTip("Start " + prefixShortcutLabel.at(buttonSlot)->text());

        if(managerCfg->value("ShortcutHidesManager").toBool())
            if(this->isHidden()) this->show();
    }

    delete umuController[threadSlot];
    umuController[threadSlot] = nullptr;

    currentlyRunning.removeOne(buttonSlot);
    if(currentlyRunning.count() == 0) {
        currentlyRunning.clear();
        threadsCount = 0;
        umuController.clear();
        ui->backButton->setIcon(QIcon::fromTheme("go-previous"));
        ui->backButton->setToolTip("Go back to prefixes list.");
    }
}

void NeroManagerWindow::handleUmuSignal(const int &signalType)
{
    if(runnerWindow != nullptr) {
        switch(signalType) {
        case NeroRunner::RunnerStarting:
            runnerWindow->SetText("umu launching...");
            break;
        case NeroRunner::RunnerUpdated:
            runnerWindow->SetText("umu runtime updated, starting Proton...");
            break;
        case NeroRunner::RunnerProtonStarted:
            delete runnerWindow;
            runnerWindow = nullptr;
            break;
        case NeroRunner::RunnerProtonStopping:
            runnerWindow->SetText("Stopping Proton process...");
            break;
        case NeroRunner::RunnerProtonStopped:
            delete runnerWindow;
            runnerWindow = nullptr;
            break;
        }
    }
}
