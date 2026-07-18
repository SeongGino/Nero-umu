/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Prefix Settings dialog.

    Copyright (C) 2024 That One Seong

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

#include "neroprefixsettings.h"
#include "ui_neroprefixsettings.h"
#include "neroconstants.h"
#include "nerodrives.h"
#include "nerofs.h"
#include "neroico.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAction>
#include <QProcess>
#include <QSpinBox>
#include <QShortcut>
#include <QThread>
#include <QStringBuilder>
#include <QWidget>
#include <QListWidgetItem>
#include <QStringBuilder>
#include "../lib/quazip/quazip/quazip.h"
#include "../lib/quazip/quazip/quazipfile.h"

NeroPrefixSettingsWindow::NeroPrefixSettingsWindow(QWidget *parent, const QString shortcutHash)
    : QDialog(parent)
    , ui(new Ui::NeroPrefixSettingsWindow)
{
    ui->setupUi(this);

    // shortcut ctrl/cmd + W to close the popup window
    QShortcut *shortcutClose = new QShortcut(QKeySequence::Close, this);
    connect(shortcutClose, &QShortcut::activated, this,&NeroPrefixSettingsWindow::close);

    boldFont.setBold(true);

    // env vars shouldn't be needed (and parsing it is a pita), so hide it for now
    ui->envBox->setVisible(false);

    ui->prefixRunner->blockSignals(true);
    // prefix runner box is used to govern availability of scaling options in both prefix and shortcut settings
    ui->prefixRunner->addItems(*NeroFS::GetAvailableProtons());
    ui->prefixRunner->setCurrentText(NeroFS::GetCurrentRunner());
    ui->prefixRunner->blockSignals(false);

    ui->envVarTable->setHorizontalHeaderLabels({"Variable", "Value"});

    if(shortcutHash.isEmpty()) {
        settings = NeroFS::GetCurrentPrefixSettings();

        ui->shortcutSettings->setVisible(false);
        ui->toggleShortcutPrefixOverride->setVisible(false);
        ui->windowsVerSection->setVisible(false);
        ui->runnerGroup->setVisible(false);
        ui->fpsBox->setVisible(false);

        if(settings.value("DiscordRPCinstalled").toBool()) {
            ui->prefixInstallDiscordRPC->setEnabled(false);
            ui->prefixInstallDiscordRPC->setText("Discord RPC Service Already Installed");
        }

        this->setWindowTitle("Prefix Settings");
    } else {
        currentShortcutHash = shortcutHash;
        settings = NeroFS::GetShortcutSettings(shortcutHash);

        existingShortcuts.append(NeroFS::GetCurrentPrefixShortcuts());
        existingShortcuts.removeOne(settings["Name"].toString());

        ui->prefixSettings->setVisible(false);
        ui->prefixServices->setVisible(false);
        ui->nameMatchWarning->setVisible(false);

        deleteShortcut = new QPushButton(QIcon::fromTheme("edit-delete"), "Delete Shortcut");
        ui->buttonBox->addButton(deleteShortcut, QDialogButtonBox::ResetRole);
        connect(deleteShortcut, &QPushButton::clicked, this, &NeroPrefixSettingsWindow::deleteShortcut_clicked);

        // shortcut view uses tristate items,
        // where PartiallyChecked is unmodified from global/undefined in shortcut config.
        for(const auto &child : this->findChildren<QCheckBox*>())
            child->setTristate(true), child->setCheckState(Qt::PartiallyChecked);
        ui->toggleShortcutPrefixOverride->setTristate(false);
        for(const auto &child : this->findChildren<QComboBox*>()) {
            if(child != ui->prefixRunner && child != ui->winVerBox) {
                child->insertItem(0, "[Use Default Setting]");
                child->setCurrentIndex(0);
            }
        }

        // Windows versions are listed from newest-first to oldest-last in the UI for convenience,
        // when the real order is from oldest to newest (in case Windows 12 somehow exists).
        for(int i = ui->winVerBox->count(); i > 0; --i)
            winVersionListBackwards.append(ui->winVerBox->itemText(i-1));
    }


    resValidator = new QIntValidator(0, 32767);
    ui->fsrCustomH->setValidator(resValidator);
    ui->fsrCustomW->setValidator(resValidator);
    ui->gamescopeAppHeight->setValidator(resValidator);
    ui->gamescopeAppWidth->setValidator(resValidator);
    ui->gamescopeWindowHeight->setValidator(resValidator);
    ui->gamescopeWindowWidth->setValidator(resValidator);

    ui->fsrSection->setVisible(false);
    ui->gamescopeSection->setVisible(false);
    ui->goat1->setVisible(false);
    ui->goat2->setVisible(false);

    // wine CPU topology UI setup
    // get number of threads on the system, then create
    // checkboxes of equivalent amount of cores
    int currRow = 0;
    for (int i = 0; i < QThread::idealThreadCount(); i++) {
        QString iStr = QString::number(i);
        QString core = "Core " % iStr;
        QListWidgetItem *box = new QListWidgetItem(core);
        QString description = "Enables CPU core #" % iStr % " for use when Wine CPU Topology is enabled. This feature is experimental and should only be used if facing issues.";
        box->setWhatsThis(description);
        box->setCheckState(Qt::Unchecked);
        ui->topologyList->addItem(box);
    }

    LoadSettings();

    dllList = new QCompleter(commonDLLsList);
    ui->dllAdder->setCompleter(dllList);
    connect(ui->dllAdder, &QLineEdit::returnPressed, this, &NeroPrefixSettingsWindow::on_dllAddBtn_clicked);
    // should be a way to method-ify this?
    for(const auto child : this->findChildren<QPushButton*>()) {
        if(!child->property("whatsThis").isNull())
            child->installEventFilter(this);
    }
    for(const auto child : this->findChildren<QCheckBox*>()) {
        if(!child->property("whatsThis").isNull())
            child->installEventFilter(this);
        if(!child->property("isFor").isNull())
            connect(
                child,
                &QCheckBox::stateChanged,
                this,
                &NeroPrefixSettingsWindow::OptionSet
            );
    }
    for(const auto child : this->findChildren<QLineEdit*>()) {
        if(!child->property("whatsThis").isNull())
            child->installEventFilter(this);
        if(!child->property("isFor").isNull())
            connect(
                child,
                &QLineEdit::textEdited,
                this,
                &NeroPrefixSettingsWindow::OptionSet
            );
    }
    for(const auto child : this->findChildren<QSpinBox*>()) {
        if(!child->property("whatsThis").isNull())
            child->installEventFilter(this);
        // QSpinboxes' "valueChanged" signal isn't new syntax friendly?
        if(!child->property("isFor").isNull())
            connect(
                child,
                SIGNAL(valueChanged(int)),
                this,
                SLOT(OptionSet())
            );
    }
    for(const auto child : this->findChildren<QComboBox*>()) {
        if(!child->property("whatsThis").isNull())
            child->installEventFilter(this);
        // QComboboxes aren't new syntax friendly?
        if(!child->property("isFor").isNull())
            connect(
                child,
                SIGNAL(activated(int)),
                this,
                SLOT(OptionSet())
            );
    }
    //manual setting as this is the only QGroupBox with a checkbox.
    connect(
        ui->wineTopology,
        &QGroupBox::toggled,
        this,
        &NeroPrefixSettingsWindow::OptionSet
    );

    // light mode styling adjustments:
    if(this->palette().window().color().value() > this->palette().text().color().value()) {
        ui->infoBox->setStyleSheet("QGroupBox::title { color: #909000 }");
        ui->infoText->setStyleSheet("color: doubledarkgray");
    }

    // do not use stylesheets on QScrollAreas, they destroy the scrollbar look
    ui->infoScrollArea->widget()->setAutoFillBackground(false);
    ui->infoScrollArea->viewport()->setAutoFillBackground(false);
}

bool NeroPrefixSettingsWindow::eventFilter(QObject* object, QEvent* event)
{
    if (umuRunning)
        return QWidget::eventFilter(object, event);
    //owo whats this
    QVariant whatsThis = object->property("whatsThis");
    bool hasEnterEvent = event->type() == QEvent::Enter;
    if(hasEnterEvent && !whatsThis.isNull()) {
        ui->infoText->setText(whatsThis.toString()),
        ui->infoBox->setTitle(object->property("accessibleName").toString());
    }
    return QWidget::eventFilter(object, event);
}

void NeroPrefixSettingsWindow::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    // disable these defaults so the user can use Return key to commit new dll overrides.
    ui->buttonBox->button(QDialogButtonBox::Save)->setDefault(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setDefault(false);
    ui->buttonBox->button(QDialogButtonBox::Reset)->setDefault(false);
}

NeroPrefixSettingsWindow::~NeroPrefixSettingsWindow()
{
    delete ui;
}

// used for initial load and resetting values when Reset Btn is pressed
void NeroPrefixSettingsWindow::LoadSettings()
{
    NeroPrefixSettingsWindow::blockSignals(true);

    if(!settings.value("EnableNVAPI").toString().isEmpty()) {
        ui->toggleNVAPI->setChecked(settings.value("EnableNVAPI").toBool());
    }
    QHash<QString, QLineEdit*> lineEdits = {
        // general tab->graphics group
        {"FSRcustomResW",      ui->fsrCustomW},
        {"FSRcustomResH",      ui->fsrCustomH},
        {"GamescopeOutResW",   ui->gamescopeAppWidth},
        {"GamescopeOutResH",   ui->gamescopeAppHeight},
        {"GamescopeWinResW",   ui->gamescopeWindowWidth},
        {"GamescopeWinResH",   ui->gamescopeWindowHeight},
        // general tab->prefix shortcut settings group
    };
    for (auto i = lineEdits.begin(), end = lineEdits.end(); i != end; i++) {
        QString neroSetting = i.key();
        QLineEdit* field = i.value();
        field->setText(settings.value(neroSetting).toString());
    }

    // there was no good name for these once I realized all options
    // could be consolidated into a scrollable field,
    // so lets just hide that fact so this can look a bit pretty
    QString up = "ImageReconstructionUpgrade";
    QString ind = "ImageReconstructionIndicator";
    QHash<QString, QComboBox*> comboBoxes = {
        // general tab->graphics group
        {"ScalingMode",        ui->setScalingBox},
        {"GamescopeScaler",    ui->gamescopeSetScalerBox},
        {"GamescopeFilter",    ui->gamescopeSetUpscalingBox},
        // advanced tab
        {"DebugOutput",        ui->debugBox},
        {"FileSyncMode",       ui->fileSyncBox},
        //Experimental
        {up,                   ui->imageReconstructionBox},
        {ind,                  ui->imageReconstructionIndBox},
    };
    for (auto i = comboBoxes.begin(), end = comboBoxes.end(); i != end; i++) {
        QString neroSetting = i.key();
        QComboBox* field = i.value();
        field->setCurrentIndex(settings.value(neroSetting).toInt());
    }
    // compatibility tab
    if(!settings.value("DLLoverrides").toStringList().isEmpty()) {
        dllSetting.clear();
        dllDelete.clear();

        const QStringList dllsToAdd = settings.value("DLLoverrides").toStringList();
        for(const QString &dll : std::as_const(dllsToAdd)) {
            const QString dllName = dll.left(dll.indexOf('='));
            const QString dllType = dll.mid(dll.indexOf('=')+1);
            if(dllType == "n,b")           AddDLL(dllName, NeroConstant::DLLNativeThenBuiltin);
            else if(dllType == "builtin")  AddDLL(dllName, NeroConstant::DLLBuiltinOnly);
            else if(dllType == "b,n")      AddDLL(dllName, NeroConstant::DLLBuiltinThenNative);
            else if(dllType == "native")   AddDLL(dllName, NeroConstant::DLLNativeOnly);
            else if(dllType == "disabled") AddDLL(dllName, NeroConstant::DLLDisabled);
        }
    }

    QHash<QString, QCheckBox*> checkboxes = {
        // TODO: Change These to be the namespace constants
        // general tab->services group
        {"Gamemode",           ui->toggleGamemode},
        {"Mangohud",           ui->toggleMangohud},
        {"VKcapture",          ui->toggleVKcap},
        //advanced
        {"ForceiGPU",          ui->toggleiGPU},
        {"LimitGLextensions",  ui->toggleLimitGL},
        {"NoD8VK",             ui->toggleNoD8VK},
        {"ForceWineD3D",       ui->toggleWineD3D},
        //experimental
        {"UseZink",            ui->toggleZink},
        {"UseWayland",         ui->toggleWayland},
        {"DisableHdr",         ui->toggleDisableHdr},
        {"AllowHidraw",        ui->toggleHidraw},
        {"UseXalia",           ui->toggleXalia},
        {"UseNvidiaLibs",      ui->toggleNvidiaLibs},
        {"SteamInputDisabled", ui->toggleSteamInput},
        {"UseNoDecorations",   ui->toggleWindowDecorations},
        {"UseOptiscaler",      ui->toggleOptiscaler},
        {"UseLowLatency",      ui->toggleLowLatency},
    };
    for (auto i = checkboxes.begin(), end = checkboxes.end(); i != end; i++) {
        QString neroOption = i.key();
        QCheckBox* widget = i.value();
        SetCheckboxState(neroOption, widget);
    }

    if(currentShortcutHash.isEmpty()) {
        // for prefix general settings, checkboxes are normal two-state

        // general tab->prefix global settings group
        // if prefix runner doesn't exist, just set to whatever's the first entry.
        QString currentRunner = settings.value("CurrentRunner").toString();
        if(NeroFS::GetAvailableProtons()->contains(currentRunner))
            ui->prefixRunner->setCurrentText(currentRunner);
        else ui->prefixRunner->setCurrentIndex(0),
             ui->prefixRunner->setFont(boldFont);
        ui->togglePrefixRuntimeUpdates->setChecked(settings.value("RuntimeUpdateOnLaunch").toBool());

        // advanced tab
        //ui->prefixEnvVars->setText(settings.value("CustomEnvVars").toString());
    } else {
        // for shortcut settings, any defined settings set the button to either 0 (Unchecked) or 2 (Checked)
        // else, undefined settings remain at 1 (Partially Checked)

        // general tab->prefix shortcut settings group
        ui->shortcutName->setText(settings["Name"].toString());
        ui->shortcutPath->setText(settings["Path"].toString());
        ui->shortcutArgs->setText(settings["Args"].toStringList().join(' '));
        ui->preRunScriptPath->setText(settings.value("PreRunScript").toString());
        ui->postRunScriptPath->setText(settings.value("PostRunScript").toString());
        ui->umuId->setText(settings.value("UmuId").toString());
        if(ui->preRunScriptPath->text().isEmpty())  ui->preRunClearBtn->setVisible(false);
        if(ui->postRunScriptPath->text().isEmpty()) ui->postRunClearBtn->setVisible(false);

        if(QFileInfo::exists(settings["Path"].toString().replace("C:/",
                                                                 NeroFS::GetPrefixesPath()->canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/")))
            ui->pathNoExistWarning->setVisible(false);
        else {
            ui->openToShortcutPath->setEnabled(false);
            // adjust font recoloring for light mode (unused, but just in case?)
            /* if(this->palette().window().color().value() > this->palette().text().color().value())
                ui->shortcutPath->setStyleSheet("color: darkred");
            else */
                ui->shortcutPath->setStyleSheet("color: red");
        }

        QDir ico(NeroFS::GetPrefixesPath()->path()+'/'+NeroFS::GetCurrentPrefix()+"/.icoCache");
        if(ico.exists(settings["Name"].toString()+'-'+currentShortcutHash+".png")) {
            if(QPixmap(ico.path()+'/'+settings["Name"].toString()+'-'+currentShortcutHash+".png").height() < 64)
                ui->shortcutIco->setIcon(QPixmap(ico.path()+'/'+
                                                 settings["Name"].toString()+'-'+
                                                 currentShortcutHash+".png")
                                                .scaled(64,64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            else ui->shortcutIco->setIcon(QPixmap(ico.path()+'/'+
                                                  settings["Name"].toString()+'-'+
                                                  currentShortcutHash+".png"));
        }
        this->setWindowTitle("Shortcut Settings");
        this->setWindowIcon(QPixmap(ico.path()+'/'+
                                    settings["Name"].toString()+'-'+
                                    currentShortcutHash+".png"));

        ui->limitFPSbox->setValue(settings.value("LimitFPS").toInt());

        ui->toggleShortcutPrefixOverride->setChecked(settings.value("IgnoreGlobalDLLs").toBool());

        // if value is filled, scoot comboboxes down one index
        for(const auto &child : this->findChildren<QComboBox*>())
            if(child != ui->prefixRunner &&
               child != ui->winVerBox)
                if(!settings.value(child->property("isFor").toString()).toString().isEmpty())
                    child->setCurrentIndex(child->currentIndex()+1);

        if(settings.value("WindowsVersion").toString().isEmpty())
            ui->winVerBox->setCurrentIndex(-2);
        else ui->winVerBox->setCurrentText(winVersionListBackwards.at(settings.value("WindowsVersion").toInt()));

        // if current scaler is set as disabled due to incompatible runner, set to Normal.
        auto *model = qobject_cast<QStandardItemModel*>(ui->setScalingBox->model());
        auto *item = model->item(ui->setScalingBox->currentIndex());
        if(!item->isEnabled()) ui->setScalingBox->setCurrentIndex(0);
    }

    // set visibility of scaling box contents based on loaded value (since signals are still stopped by now)
    if(ui->setScalingBox->currentText() == "AMD FSR 1 - Custom Resolution") ui->fsrSection->setVisible(true);
    else ui->fsrSection->setVisible(false);

    if(ui->setScalingBox->currentText().startsWith("Gamescope")) {
        ui->gamescopeSection->setVisible(true);
        QList<QWidget*> gamescope = {
            ui->gamescopeWindowLabelX,
            ui->gamescopeWindowLabel,
            ui->gamescopeWindowHeight,
            ui->gamescopeWindowWidth,
        };
        bool isWindowed = !ui->setScalingBox->currentText().endsWith("Fullscreen");
        for (int i = 0; i < gamescope.length(); i++) {
            QWidget* widget = gamescope[i];
            widget->setVisible(isWindowed);
        }
    } else ui->gamescopeSection->setVisible(false);
    //WineCpuTopology UI Setup
    QVariant topology = settings.value("WineCpuTopology");
    bool isEnabled = settings.value("CpuTopologyEnabled").toBool();
    bool isEmptyStr = topology.toString().isEmpty();
    if (isEnabled && !isEmptyStr) {
        ui->wineTopology->setChecked(true);
    }
    //set this even if its not enabled so that the stored values aren't lost.
    if (!isEmptyStr) {
        QStringList split = topology.toString().split(":");
        QStringList enabledCores = split[1].split(",");
        // Iterate through the topology core counts versus enabled ones.
        // Each time we find a match, remove it so that subsequent
        // worst case iterations are slightly cheaper
        for (int i = 0; i < ui->topologyList->count(); ++i) {
            QListWidgetItem* item = ui->topologyList->item(i);
            for (int j = 0; j < enabledCores.length(); ++j) {
                int core = enabledCores[j].toInt();
                if (core == i) {
                    enabledCores.removeAt(j);
                    item->setCheckState(Qt::Checked);
                    break;
                }
            }
        }
    }
    auto prefixSettings = NeroFS::GetCurrentPrefixSettings();
    // always map to prefix settings
    QStringList envVars = prefixSettings.value("EnvVars").toString().split(NeroFS::PATH_SEPERATOR);
    bool enabledEnv = settings.value("EnvVarsEnabled").toBool();
    if (enabledEnv && !envVars.isEmpty()) {
        ui->envVarBox->setChecked(true);
    }
    auto noEntries = envVars.size() == 1 && envVars[0].isEmpty();
    if (!noEntries) {
        for (int i = 0; i < envVars.size(); i++) {
            QStringList args = envVars[i].split('=');
            if (args[0].isEmpty() || args[1] == nullptr)
                continue;
            QTableWidgetItem *variable = new QTableWidgetItem(args[0]);
            if (args[0].startsWith("[E]")) {
                // int
                variable->setCheckState(Qt::Checked);
                //get all but the first 3 characters, which is the enabled flag
                int charsToTrim = args[0].length() - 3;
                auto var = args[0].right(charsToTrim);
                variable->setText(var);
            } else {
                variable->setCheckState(Qt::Unchecked);
                variable->setText(args[0]);
            }
            QTableWidgetItem *val = new QTableWidgetItem(args[1]);
            val->setText(args[1]);
            ui->envVarTable->setItem(i, 0, variable);
            ui->envVarTable->setItem(i, 1, val);
        }
    }
    //add blank rows
    for (int i = noEntries ? 0 : envVars.size(); i < ui->envVarTable->rowCount(); i++) {
        QTableWidgetItem *b = new QTableWidgetItem();
        b->setCheckState(Qt::Unchecked);
        ui->envVarTable->setItem(i, 0, b);
    }
    // disable env vars entirely for shortcuts, as
    // getting these to stay synced is a nightmare. We could
    // alternatively handle this by having them be completely desynced,
    // but that gets handled by just disabling on the prefix level I feel.
    if (!currentShortcutHash.isEmpty()) {
        ui->envVarBox->setChecked(false);
        ui->envVarBox->setEnabled(false);
    }
    for(const auto &child : this->findChildren<QCheckBox*>())
        child->setFont(QFont());

    for(const auto &child : this->findChildren<QLineEdit*>())
        child->setFont(QFont());

    for(const auto &child : this->findChildren<QComboBox*>())
        child->setFont(QFont());
    // manually call this to make sure it refreshes its state
    on_toggleWayland_checkStateChanged(ui->toggleWayland->checkState());
    NeroPrefixSettingsWindow::blockSignals(false);
}


void NeroPrefixSettingsWindow::SetCheckboxState(const QString &varName, QCheckBox* checkBox)
{
    if(!settings.value(varName).toString().isEmpty())
        if(settings.value(varName).toBool())  checkBox->setCheckState(Qt::Checked);
        else                                  checkBox->setCheckState(Qt::Unchecked);
    else if(checkBox->isTristate())           checkBox->setCheckState(Qt::PartiallyChecked);
    else                                      checkBox->setCheckState(Qt::Unchecked);
}


void NeroPrefixSettingsWindow::on_shortcutIco_clicked()
{
    QString newIcon = QFileDialog::getOpenFileName(this,
                                                   "Select a Windows Executable",
                                                   ui->shortcutPath->text().replace("C:/",
                                                                                    NeroFS::GetPrefixesPath()->canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/"),
        "Windows Executable, Dynamic Link Library, Icon Resource File, or Portable Network Graphics File (*.dll *.exe *.ico *.png);;Windows Dynamic Link Library (*.dll);;Windows Executable (*.exe);;Windows Icon Resource (*.ico);;Portable Network Graphics File (*.png)");

    if(!newIcon.isEmpty()) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        newIcon = NeroIcoExtractor::GetIcon(newIcon);
        QGuiApplication::restoreOverrideCursor();

        if(!newIcon.isEmpty()) {
            newAppIcon = newIcon;
            if(QPixmap(newAppIcon).height() < 64)
                ui->shortcutIco->setIcon(QPixmap(newAppIcon).scaled(64,64,Qt::KeepAspectRatio,Qt::SmoothTransformation));
            else ui->shortcutIco->setIcon(QPixmap(newAppIcon));
        }
    }
}


void NeroPrefixSettingsWindow::on_shortcutName_textEdited(const QString &arg1)
{
    if(arg1.isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        ui->nameMatchWarning->setVisible(false);
    } else {
        if(existingShortcuts.contains(arg1.trimmed())) {
            ui->nameMatchWarning->setVisible(true);
            ui->shortcutName->setStyleSheet("color: red");
            ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        } else {
            ui->nameMatchWarning->setVisible(false);
            ui->shortcutName->setStyleSheet("");
            ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
        }
    }
}


void NeroPrefixSettingsWindow::on_shortcutPathBtn_clicked()
{
    QString newApp = QFileDialog::getOpenFileName(this,
                                                  "Select a Windows Executable",
                                                  ui->shortcutPath->text().replace("C:/",
                                                                                   NeroFS::GetPrefixesPath()->canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/"),
    "Compatible Windows Executables (*.bat *.cmd *.exe *.msi);;Windows Batch Script Files (*.bat *.cmd);;Windows Portable Executable (*.exe);;Windows Installer Package (*.msi)",
                                                  nullptr,
                                                  QFileDialog::DontResolveSymlinks);
    if(!newApp.isEmpty()) {
        ui->shortcutPath->setText(newApp.replace(NeroFS::GetPrefixesPath()->path()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c", "C:"));
        if(newApp != settings.value("Path").toString())
            ui->shortcutPath->setFont(boldFont);
        else ui->shortcutPath->setFont(QFont());

        // probably assumed that in this decision path, the file is gonna be valid
        if(ui->pathNoExistWarning->isVisible()) {
            ui->openToShortcutPath->setEnabled(true);
            ui->pathNoExistWarning->setVisible(false);
            ui->shortcutPath->setStyleSheet("color: gray");
        }
    }
}

void NeroPrefixSettingsWindow::on_setScalingBox_activated(int index)
{
    if(ui->setScalingBox->currentText() == "AMD FSR 1 - Custom Resolution") ui->fsrSection->setVisible(true);
    else ui->fsrSection->setVisible(false);

    if(ui->setScalingBox->currentText().startsWith("Gamescope")) {
        ui->gamescopeSection->setVisible(true);
        if(ui->setScalingBox->currentText().endsWith("Fullscreen")) {
            ui->gamescopeWindowLabelX->setVisible(false);
            ui->gamescopeWindowLabel->setVisible(false);
            ui->gamescopeWindowHeight->setVisible(false);
            ui->gamescopeWindowWidth->setVisible(false);
        } else {
            ui->gamescopeWindowLabelX->setVisible(true);
            ui->gamescopeWindowLabel->setVisible(true);
            ui->gamescopeWindowHeight->setVisible(true);
            ui->gamescopeWindowWidth->setVisible(true);
        }
    } else ui->gamescopeSection->setVisible(false);
    ui->setScalingBox->setFont(boldFont);
}


void NeroPrefixSettingsWindow::on_dllAdder_textEdited(const QString &arg1)
{
    if(arg1.trimmed().isEmpty()) ui->dllAddBtn->setEnabled(false);
    else ui->dllAddBtn->setEnabled(true);
}


void NeroPrefixSettingsWindow::on_dllAddBtn_clicked()
{
    AddDLL(ui->dllAdder->text(), NeroConstant::DLLNativeThenBuiltin);
    dllSetting.last()->setFont(boldFont);
    ui->dllAdder->clear(), ui->dllAddBtn->setEnabled(false);
}


void NeroPrefixSettingsWindow::AddDLL(const QString newDLL, const int newDLLtype)
{
    dllOverrides[newDLL] = newDLLtype;
    dllSetting << new QToolButton(this);
    dllDelete << new QPushButton(this);

    int iterator = 0;
    for(const QString &item : dllOverrideNames) {
        dllOptions << new QAction(item,this);
        dllOptions.last()->setData(iterator);
        dllOptions.last()->setProperty("slot", dllSetting.size()-1);
        dllSetting.last()->addAction(dllOptions.last());
        iterator++;
    }

    dllSetting.last()->setText(QString("%1 [%2]").arg(newDLL, dllOverrideNames.at(newDLLtype)));
    dllSetting.last()->setPopupMode(QToolButton::InstantPopup);
    dllSetting.last()->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    dllDelete.last()->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
    dllDelete.last()->setIcon(QIcon::fromTheme("edit-delete"));
    dllDelete.last()->setFlat(true);
    dllDelete.last()->setProperty("slot", dllDelete.size()-1);

    connect(dllSetting.last(), &QToolButton::triggered, this, &NeroPrefixSettingsWindow::dll_action_triggered);
    connect(dllDelete.last(), &QPushButton::clicked, this, &NeroPrefixSettingsWindow::dll_delete_clicked);

    ui->dllOverridesGrid->addWidget(dllSetting.last(), dllDelete.size()-1, 0);
    ui->dllOverridesGrid->addWidget(dllDelete.last(), dllDelete.size()-1, 1);
}


void NeroPrefixSettingsWindow::dll_action_triggered(QAction* action)
{
    const QString name = dllSetting.at(action->property("slot").toInt())->text().left(dllSetting.at(action->property("slot").toInt())->text().indexOf('[')-1).remove('&').trimmed();

    switch(action->data().toInt()) {
    case 0:
        dllOverrides[name] = NeroConstant::DLLNativeThenBuiltin;
        break;
    case 1:
        dllOverrides[name] = NeroConstant::DLLBuiltinOnly;
        break;
    case 2:
        dllOverrides[name] = NeroConstant::DLLBuiltinThenNative;
        break;
    case 3:
        dllOverrides[name] = NeroConstant::DLLNativeOnly;
        break;
    case 4:
        dllOverrides[name] = NeroConstant::DLLDisabled;
        break;
    }
    dllSetting.at(action->property("slot").toInt())->setText(QString("%1 [%2]").arg(name, dllOverrideNames.at(action->data().toInt())));
    dllSetting.at(action->property("slot").toInt())->setFont(boldFont);
}


void NeroPrefixSettingsWindow::dll_delete_clicked()
{
    int slot = sender()->property("slot").toInt();

    QString dllToRemove = dllSetting.at(slot)->text().left(dllSetting.at(slot)->text().indexOf('[')-1).trimmed();
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
// workaround annoying as fuck anpersand being needlessly implicitly added to widget texts in Qt 6.8
    dllToRemove.remove('&');
#endif
    dllOverrides.remove(dllToRemove);
    delete dllSetting.at(slot);
    delete dllDelete.at(slot);
    dllSetting[slot] = nullptr;
    dllDelete[slot] = nullptr;
}


void NeroPrefixSettingsWindow::on_preRunButton_clicked()
{
    QString scriptPath = QFileDialog::getOpenFileName(this,
                                     "Select a Pre-run Script",
                                     qEnvironmentVariable("HOME"),
                                     "Unix Bash Script (.sh)");
    if(!scriptPath.isEmpty()) {
        ui->preRunScriptPath->setText(scriptPath);
        ui->preRunClearBtn->setVisible(true);
        if(scriptPath != settings.value("PreRunScript").toString())
            ui->preRunScriptPath->setFont(boldFont);
        else ui->preRunScriptPath->setFont(QFont());
    }
}


void NeroPrefixSettingsWindow::on_postRunButton_clicked()
{
    QString scriptPath = QFileDialog::getOpenFileName(this,
                                                      "Select a Post-run Script",
                                                      qEnvironmentVariable("HOME"),
                                                      "Unix Bash Script (.sh)");
    if(!scriptPath.isEmpty()) {
        ui->postRunScriptPath->setText(scriptPath);
        ui->postRunClearBtn->setVisible(true);
        if(scriptPath != settings.value("PostRunScript").toString())
            ui->postRunScriptPath->setFont(boldFont);
        else ui->postRunScriptPath->setFont(QFont());
    }
}


void NeroPrefixSettingsWindow::on_preRunClearBtn_clicked()
{
    ui->preRunScriptPath->clear();
    if(ui->preRunScriptPath->text() != settings.value("PreRunScript").toString())
        ui->preRunScriptPath->setFont(boldFont);
    else ui->preRunScriptPath->setFont(QFont());
    ui->preRunClearBtn->setVisible(false);
}


void NeroPrefixSettingsWindow::on_postRunClearBtn_clicked()
{
    ui->postRunScriptPath->clear();
    if(ui->postRunScriptPath->text() != settings.value("PostRunScript").toString())
        ui->postRunScriptPath->setFont(boldFont);
    else ui->postRunScriptPath->setFont(QFont());
    ui->postRunClearBtn->setVisible(false);
}


void NeroPrefixSettingsWindow::on_prefixInstallDiscordRPC_clicked()
{
    QDir tmpDir(QDir::temp());
    if(!tmpDir.exists("nero-manager"))
        tmpDir.mkdir("nero-manager");

    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QUrl url("https://github.com/EnderIce2/rpc-bridge/releases/latest/download/bridge.zip");
    QNetworkRequest req(url);
    QNetworkReply* reply = manager->get(req);

    NeroPrefixSettingsWindow::blockSignals(true);
    umuRunning = true;
    enableWidgets(false);
    ui->infoBox->setTitle("Downloading Discord RPC Bridge...");
    ui->infoText->setText("");

    do {
        QApplication::processEvents();
    } while (!reply->isFinished());

    if(reply->error() != QNetworkReply::NoError) {
        QString errorReply = "Error Message: " % reply->errorString();
        QString failedDownload = "Nero failed to download the Discord RPC Bridge.\n" % errorReply;
        displayError(failedDownload);
        return; //failed to download, return here
    }

    QByteArray bytes = reply->readAll();
    QString path(tmpDir.path() + "/nero-manager/bridge.zip");
    QSaveFile bridgeZip = QSaveFile(path);
    if (!bridgeZip.open(QIODevice::WriteOnly)) {
        QString error = "Error Message: " % reply->errorString();
        QString message = "Nero failed to extract the Discord RPC Bridge.\n" % error;
        displayError(message);
        return; //failed to extract
    }
    QDataStream zipStream(&bridgeZip);
    zipStream << bytes;
    bridgeZip.commit();
    printf("Extracting bridge.zip...\n");
    ui->infoBox->setTitle("Extracting bridge package...");

    // QuaZip is like minizip, except it actually works here.
    QuaZip zipFile(tmpDir.absoluteFilePath("nero-manager/bridge.zip"));
    zipFile.open(QuaZip::mdUnzip);
    zipFile.setCurrentFile("bridge.exe");
    QuaZipFile exeToExtract(&zipFile);

    if(!exeToExtract.open(QIODevice::ReadOnly)) {
        QFile outFile(tmpDir.absoluteFilePath("nero-manager/bridge.exe"));
        if (!outFile.open(QIODevice::ReadWrite)) {
            QString error = "Error Message: " % reply->errorString();
            QString message = "Nero failed to extract the Discord RPC Bridge.\n" % error;
            displayError(message);
            return;
        }
        outFile.write(exeToExtract.readAll());
        outFile.close();

        StartUmu(tmpDir.absoluteFilePath("nero-manager/bridge.exe"), { "--install" });

        ui->prefixInstallDiscordRPC->setEnabled(false);
        ui->prefixInstallDiscordRPC->setText("Discord RPC Service Already Installed");
        settings.value("DiscordRPCinstalled", true);
        NeroFS::SetCurrentPrefixCfg("PrefixSettings", "DiscordRPCinstalled", true);
    } else {
        QMessageBox::warning(this,
                             "Error!",
                             QString("Bridge extraction exited with the error:\n\n%1").arg(exeToExtract.errorString()));
        ui->infoBox->setTitle("");
        ui->infoText->setText(QString("Bridge extraction exited with the error: %1").arg(exeToExtract.errorString()));
    }

    enableWidgets(true);
    umuRunning = false;
    NeroPrefixSettingsWindow::blockSignals(false);
}

void NeroPrefixSettingsWindow::displayError(QString error)
{
    QMessageBox::warning(this,
                         "Error!",
                         error);
    ui->infoBox->setTitle("");
    ui->infoText->setText(error);
    enableWidgets(true);
}

void NeroPrefixSettingsWindow::enableWidgets(bool isEnabled)
{
    ui->tabWidget->setEnabled(isEnabled);
    ui->buttonBox->setEnabled(isEnabled);
    ui->infoBox->setEnabled(isEnabled);
}


void NeroPrefixSettingsWindow::on_prefixDrivesBtn_clicked()
{
    NeroVirtualDriveDialog drives(this);
    drives.exec();
}


void NeroPrefixSettingsWindow::on_prefixRegeditBtn_clicked()
{
    StartUmu("regedit");
    ui->tabWidget->setEnabled(true);
    ui->buttonBox->setEnabled(true);
    ui->infoBox->setEnabled(true);
    umuRunning = false;
    NeroPrefixSettingsWindow::blockSignals(false);
}


void NeroPrefixSettingsWindow::on_prefixWinecfgBtn_clicked()
{
    StartUmu("winecfg");
    ui->tabWidget->setEnabled(true);
    ui->buttonBox->setEnabled(true);
    ui->infoBox->setEnabled(true);
    umuRunning = false;
    NeroPrefixSettingsWindow::blockSignals(false);
}


void NeroPrefixSettingsWindow::StartUmu(const QString command, QStringList args)
{
    if(!NeroFS::GetUmU().isEmpty()) {
        args.prepend(command);

        QProcess umu(this);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

        env.insert("WINEPREFIX", QString("%1/%2").arg(NeroFS::GetPrefixesPath()->path(), NeroFS::GetCurrentPrefix()));

        env.insert("GAMEID", "0");
        env.insert("PROTONPATH", QString("%1/%2").arg(NeroFS::GetProtonsPath()->path(), NeroFS::GetCurrentRunner()));
        env.insert("PROTON_USE_XALIA", "0");
        env.insert("UMU_RUNTIME_UPDATE", "0");
        umu.setProcessEnvironment(env);
        umu.setProcessChannelMode(QProcess::MergedChannels);

        umu.start(NeroFS::GetUmU(), args);

        NeroPrefixSettingsWindow::blockSignals(true);
        umuRunning = true;
        ui->tabWidget->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        ui->infoBox->setEnabled(false);
        ui->infoBox->setTitle("Starting umu...");
        ui->infoText->setText("");

        // don't use blocking function so that the UI doesn't freeze.
        while(umu.state() != QProcess::NotRunning) {
            QApplication::processEvents();
            umu.waitForReadyRead(1000);
            printf("%s", umu.readAll().constData());
        }

        if(umu.exitStatus() == 0) {
            ui->infoBox->setTitle("");
            ui->infoText->setText("Umu exited successfully.");
        } else {
            QMessageBox::warning(this,
                                 "Error!",
                                 QString("Umu exited with an error code:\n\n%1").arg(umu.exitStatus()));
            ui->infoBox->setTitle("");
            ui->infoText->setText(QString("Umu exited with an error code: %1.").arg(umu.exitStatus()));
        }
    }
}


void NeroPrefixSettingsWindow::on_tabWidget_currentChanged(int index)
{
    ui->infoBox->setTitle("");
    ui->infoText->setText("Hover over an option to display info about it here.");
}

void NeroPrefixSettingsWindow::OptionSet()
{
    // declaring here means that we dont have to iterate
    // through settings map every time, plus avoids declaring
    // in every flow and having extra braces.
    QString val = sender()->property("isFor").toString();
    QVariant cfg = settings.value(val);
    // is there like a way to inline declarations in cpp? id love to just have these be declared
    // at the start of each if else instead of this or making the flow syntax worse.
    QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    QSpinBox* spinBox = qobject_cast<QSpinBox*>(sender());
    QString type = sender()->metaObject()->className();
    if (comboBox != nullptr) {
        // Shortcut settings: handle "use default" (blank entry)
        if (cfg.toString().isEmpty())
            comboBox->currentIndex() > 0
                ? comboBox->setFont(boldFont)
                : comboBox->setFont(QFont());
        // Global prefix settings: always set to an index
        else if (!currentShortcutHash.isEmpty())
            comboBox->currentIndex()-1 != cfg.toInt()
                ? comboBox->setFont(boldFont)
                : comboBox->setFont(QFont());
        else
            comboBox->currentIndex() != cfg.toInt()
                ? comboBox->setFont(boldFont)
                : comboBox->setFont(QFont());
        // extra check for Windows box, since the index doesn't quite match the way it's listed in the UI.
        if (comboBox == ui->winVerBox) {
            QVariant win = settings.value("WindowsVersion");
            if(win.toString().isEmpty())
                // ensures that the box is marked when selecting the topmost entry.
                comboBox->setFont(boldFont);
            else if(comboBox->currentText() == winVersionListBackwards[win.toInt()])
                comboBox->setFont(QFont());
        }
    } else if (checkBox != nullptr) {
        // Shortcut settings: handle "use default" half-checked state (blank entry)
        if(checkBox->isTristate()) {
            if(cfg.toString().isEmpty())
                checkBox->checkState() != Qt::PartiallyChecked
                    ? checkBox->setFont(boldFont)
                    : checkBox->setFont(QFont());
            else if(checkBox->checkState() != Qt::Checked && cfg.toBool())
                checkBox->setFont(boldFont);
            else if(checkBox->checkState() != Qt::Unchecked && !cfg.toBool())
                checkBox->setFont(boldFont);
            else checkBox->setFont(QFont());
        } else {
            // Normal Global prefix style on/off toggles
            checkBox->isChecked() != settings.value(val).toBool()
                ? checkBox->setFont(boldFont)
                : checkBox->setFont(QFont());
        }
    } else if (spinBox != nullptr) {
        spinBox->value() != cfg.toInt()
            ? spinBox->setFont(boldFont)
            : spinBox->setFont(QFont());

    } else if (lineEdit != nullptr) {
        lineEdit->text() != cfg.toString()
            ? lineEdit->setFont(boldFont)
            : lineEdit->setFont(QFont());
    }
}

void NeroPrefixSettingsWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->standardButton(button)) {
    case QDialogButtonBox::Reset:
        LoadSettings();
        break;
    case QDialogButtonBox::Cancel:
        break;
    case QDialogButtonBox::Save:
        SaveSettings();
        break;
    default:
        break;
    }
}

void NeroPrefixSettingsWindow::SaveSettings() {
    QStringList dllsToAdd;
    for(const QString &key : dllOverrides.keys()) {
        switch(dllOverrides.value(key)) {
        case NeroConstant::DLLNativeThenBuiltin:
            dllsToAdd.append(QString("%1=n,b").arg(key));
            break;
        case NeroConstant::DLLBuiltinOnly:
            dllsToAdd.append(QString("%1=builtin").arg(key));
            break;
        case NeroConstant::DLLBuiltinThenNative:
            dllsToAdd.append(QString("%1=b,n").arg(key));
            break;
        case NeroConstant::DLLNativeOnly:
            dllsToAdd.append(QString("%1=native").arg(key));
            break;
        case NeroConstant::DLLDisabled:
            dllsToAdd.append(QString("%1=disabled").arg(key));
            break;
        }
    }
    // iterate through each bolded (modified) field, and add that change to the config file.
    // For checkboxes, we check if its a shortcut or prefix entry and set it accordingly.
    bool isShortcut = !currentShortcutHash.isEmpty();
    QString cfg = isShortcut
                    ? QString("Shortcuts--" % currentShortcutHash)
                    : "PrefixSettings";
    for(const auto &child : this->findChildren<QCheckBox*>()) {
        if(child->font() == boldFont) {
            isShortcut && child->checkState() == Qt::PartiallyChecked
                ? NeroFS::SetCurrentPrefixCfg(cfg, child->property("isFor").toString(), "")
                : NeroFS::SetCurrentPrefixCfg(cfg, child->property("isFor").toString(), child->isChecked());
        }
    }
    for(const auto &child : this->findChildren<QLineEdit*>())
        if(child->font() == boldFont)
            NeroFS::SetCurrentPrefixCfg(cfg, child->property("isFor").toString(), child->text().trimmed());

    for(const auto &child : this->findChildren<QSpinBox*>())
        if(child->font() == boldFont)
            NeroFS::SetCurrentPrefixCfg(cfg, child->property("isFor").toString(), child->value());

    for(const auto &child : this->findChildren<QComboBox*>()) {
        if(child->font() == boldFont) {
            int childIndex = isShortcut
                                ? child->currentIndex() - 1
                                : child->currentIndex();
            QString propertyToSave = child->property("isFor").toString();
            // if childIndex < 0, clear the value if its a shortcut.
            if (isShortcut && childIndex < 0) {
                NeroFS::SetCurrentPrefixCfg(cfg, propertyToSave, "");
            } else {
                NeroFS::SetCurrentPrefixCfg(cfg, propertyToSave, childIndex);
            }
        }
    }
    if (!isShortcut) {
        NeroFS::SetCurrentPrefixCfg("PrefixSettings", "CurrentRunner", ui->prefixRunner->currentText());
    }
    // Funky Experimental Options
    QStringList envVars;
    //we're iterating by row here
    for (int x = 0; x < ui->envVarTable->rowCount(); x++) {
        QTableWidgetItem* envVar = ui->envVarTable->item(x, 0);
        QTableWidgetItem* val = ui->envVarTable->item(x, 1);
        // not considering if the row is currently checked or not as we want to save all values
        if (envVar != nullptr && val != nullptr && !envVar->text().isEmpty() && !val->text().isEmpty()) {
            QString enabled = envVar->checkState() == Qt::Checked ? "[E]" : QString();
            envVars << enabled % envVar->text() % '=' % val->text();
        }
    }

    // |> was the only thing i could think of that couldn't possibly show up in a path
    // without a very good reason for it being there.
    NeroFS::SetCurrentPrefixCfg(cfg, "EnvVars", envVars.join(NeroFS::PATH_SEPERATOR));
    NeroFS::SetCurrentPrefixCfg(cfg, "EnvVarsEnabled", ui->envVarBox->isChecked());

    QStringList checkedCpus;
    for (int i = 0; i < ui->topologyList->count(); ++i) {
        QListWidgetItem* item = ui->topologyList->item(i);
        int core = item->text().mid(item->text().lastIndexOf(' ')).toInt();
        Qt::CheckState state = item->checkState();
        if (state == Qt::Checked) {
            checkedCpus.append(QString::number(core));
        }
    }

    QString topCommand = checkedCpus.length() > 0
        ? QString::number(checkedCpus.length()) % ':' % checkedCpus.join(',')
        : QString("");

    NeroFS::SetCurrentPrefixCfg(cfg, "WineCpuTopology", QVariant(topCommand.trimmed()));
    NeroFS::SetCurrentPrefixCfg(cfg, "CpuTopologyEnabled", ui->wineTopology->isChecked());
    // check if new ico was set.
    if(!newAppIcon.isEmpty() && isShortcut)
        QFile::copy(newAppIcon, QString("%1/%2/.icoCache/%3").arg(NeroFS::GetPrefixesPath()->path(),
                                                                  NeroFS::GetCurrentPrefix(),
                                                                  QString("%1-%2.png").arg(settings.value("Name").toString(), currentShortcutHash)));

    // windows version overrides are currently a one-way op (e.g. can't be unset from the UI)
    if(ui->winVerBox->font() == boldFont && isShortcut) {
        int winVerSelected = winVersionListBackwards.indexOf(ui->winVerBox->itemText(ui->winVerBox->currentIndex()));
        NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, "WindowsVersion", winVerSelected);

        QDir prefixPath(NeroFS::GetPrefixesPath()->path()+'/'+NeroFS::GetCurrentPrefix());
        if(prefixPath.exists("user.reg")) {
            QFile regFile(prefixPath.path()+"/user.reg");
            regFile.open(QFile::ReadWrite);
            QString newReg;
            QString line;
            const QString exe = settings.value("Path").toString().mid(settings.value("Path").toString().lastIndexOf('/')+1);
            const QString compareString = QString("[Software\\\\Wine\\\\AppDefaults\\\\%1]").arg(exe);
            bool exists = false;

            while(!regFile.atEnd()) {
                line = regFile.readLine();
                newReg.append(line);
                // winreg adds timestamp info, so just check the beginning of this line.
                if(line.startsWith(compareString)) {
                    // in case this is a reg entry that's been absorbed into WinReg format (which adds timestamps)
                    line = regFile.readLine();
                    if(line.startsWith("#time=")) newReg.append(line), regFile.readLine();
                    newReg.append(QString("\"Version\"=\"%1\"\n").arg(winVersionVerb.at(winVerSelected)));
                    exists = true;
                }
            }

            if(!exists)
                newReg.append(QString("\n[Software\\\\Wine\\\\AppDefaults\\\\%1]\n\"Version\"=\"%2\"\n").arg(exe, winVersionVerb.at(winVerSelected)));

            regFile.resize(0);
            regFile.write(newReg.toUtf8());
            regFile.close();
        }
    }
    dllsToAdd.count()
        ? NeroFS::SetCurrentPrefixCfg(cfg, "DLLoverrides", dllsToAdd)
        : NeroFS::SetCurrentPrefixCfg(cfg, "DLLoverrides", "");

    appName = ui->shortcutName->text().trimmed();
}
void NeroPrefixSettingsWindow::deleteShortcut_clicked()
{
    if(QMessageBox::warning(this,
                             "Delete Shortcut?",
                             "Are you sure you wish to delete this shortcut?\n",
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
        == QMessageBox::Yes) {
        this->done(-1);
    }
}

void NeroPrefixSettingsWindow::on_openToShortcutPath_clicked()
{
    // in case path begins with a Windows drive letter prefix
    QDesktopServices::openUrl(QUrl::fromLocalFile(ui->shortcutPath->text().left(ui->shortcutPath->text().lastIndexOf('/'))
                                                                          .replace("C:", NeroFS::GetPrefixesPath()->path()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c")));
}

void NeroPrefixSettingsWindow::on_logFolderButton_clicked()
{
    NeroFS::openLogDirectory();
}

void NeroPrefixSettingsWindow::on_toggleWayland_checkStateChanged(Qt::CheckState state)
{
    if (state == Qt::Unchecked) {
        ui->toggleDisableHdr->setEnabled(false);
        ui->toggleWindowDecorations->setEnabled(false);
    } else {
        ui->toggleDisableHdr->setEnabled(true);
        ui->toggleWindowDecorations->setEnabled(true);
    }
}

