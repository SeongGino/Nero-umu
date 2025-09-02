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

#include <QAction>
#include <QProcess>
#include <QSpinBox>
#include <QShortcut>

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

    // prefix runner box is used to govern availability of scaling options in both prefix and shortcut settings
    ui->prefixRunner->addItems(NeroFS::GetAvailableProtons());
    ui->prefixRunner->setCurrentText(NeroFS::GetCurrentRunner());

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

    // FSR scalers are only implem'd in GE-Proton
    if(!ui->prefixRunner->currentText().startsWith("GE-Proton")) {
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRperformance, false);
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRbalanced, false);
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRquality, false);
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRhighquality, false);
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRhigherquality, false);
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRhighestquality, false);
        SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingFSRcustom, false);
        // HACK: offset by one when this is a shortcut settings panel and not prefix settings
        if(ui->setScalingBox->findText("Integer Scaling") == 1) SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingIntegerScale, false);
        else SetComboBoxItemEnabled(ui->setScalingBox, NeroConstant::ScalingGamescopeWindowed, false);
    }

    // Wayland requires base version to be Proton 10+
    if(!ui->prefixRunner->currentText().startsWith("GE-Proton10") && !ui->prefixRunner->currentText().startsWith("Proton 10")) {
        ui->toggleWayland->setEnabled(false);
        ui->toggleWaylandHDR->setEnabled(false);
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

    LoadSettings();

    dllList = new QCompleter(commonDLLsList);
    ui->dllAdder->setCompleter(dllList);
    connect(ui->dllAdder, &QLineEdit::returnPressed, this, &NeroPrefixSettingsWindow::on_dllAddBtn_clicked);

    // should be a way to method-ify this?
    for(const auto child : this->findChildren<QPushButton*>()) {
        if(!child->property("whatsThis").isNull()) child->installEventFilter(this);
    }
    for(const auto child : this->findChildren<QCheckBox*>()) {
        if(!child->property("whatsThis").isNull()) child->installEventFilter(this);
        if(!child->property("isFor").isNull()) connect(child, &QCheckBox::stateChanged, this, &NeroPrefixSettingsWindow::OptionSet);
    }
    for(const auto child : this->findChildren<QLineEdit*>()) {
        if(!child->property("whatsThis").isNull()) child->installEventFilter(this);
        if(!child->property("isFor").isNull()) connect(child, &QLineEdit::textEdited, this, &NeroPrefixSettingsWindow::OptionSet);
    }
    for(const auto child : this->findChildren<QSpinBox*>()) {
        if(!child->property("whatsThis").isNull()) child->installEventFilter(this);
        // QSpinboxes' "valueChanged" signal isn't new syntax friendly?
        if(!child->property("isFor").isNull()) connect(child, SIGNAL(valueChanged(int)), this, SLOT(OptionSet()));
    }
    for(const auto child : this->findChildren<QComboBox*>()) {
        if(!child->property("whatsThis").isNull()) child->installEventFilter(this);
        // QComboboxes aren't new syntax friendly?
        if(!child->property("isFor").isNull()) connect(child, SIGNAL(activated(int)), this, SLOT(OptionSet()));
    }

    // light mode styling adjustments:
    if(this->palette().window().color().value() > this->palette().text().color().value()) {
        ui->infoBox->setStyleSheet("QGroupBox::title { color: #909000 }");
        ui->infoText->setStyleSheet("color: doubledarkgray");
    }
}

bool NeroPrefixSettingsWindow::eventFilter(QObject* object, QEvent* event)
{
    if(!umuRunning)
        if(event->type() == QEvent::Enter)
            if(!object->property("whatsThis").isNull()) {
                ui->infoText->setText(object->property("whatsThis").toString()),
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

    // general tab->graphics group
    if(!settings.value("EnableNVAPI").toString().isEmpty()) ui->toggleNVAPI->setChecked(settings.value("EnableNVAPI").toBool());
    ui->setScalingBox->setCurrentIndex(settings.value("ScalingMode").toInt());
    ui->fsrCustomW->setText(settings.value("FSRcustomResW").toString());
    ui->fsrCustomH->setText(settings.value("FSRcustomResH").toString());
    ui->gamescopeAppWidth->setText(settings.value("GamescopeOutResW").toString());
    ui->gamescopeAppHeight->setText(settings.value("GamescopeOutResH").toString());
    ui->gamescopeWindowWidth->setText(settings.value("GamescopeWinResW").toString());
    ui->gamescopeWindowHeight->setText(settings.value("GamescopeWinResH").toString());
    ui->gamescopeSetScalerBox->setCurrentIndex(settings.value("GamescopeScaler").toInt());
    ui->gamescopeSetUpscalingBox->setCurrentIndex(settings.value("GamescopeFilter").toInt());
    // general tab->services group
    SetCheckboxState("Gamemode",  ui->toggleGamemode);
    SetCheckboxState("Mangohud",  ui->toggleMangohud);
    SetCheckboxState("VKcapture", ui->toggleVKcap);

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

    // advanced tab
    ui->debugBox->setCurrentIndex(settings.value("DebugOutput").toInt());
    ui->fileSyncBox->setCurrentIndex(settings.value("FileSyncMode").toInt());
    SetCheckboxState("ForceiGPU",            ui->toggleiGPU);
    SetCheckboxState("LimitGLextensions",  ui->toggleLimitGL);
    SetCheckboxState("NoD8VK",             ui->toggleNoD8VK);
    SetCheckboxState("ForceWineD3D",       ui->toggleWineD3D);
    SetCheckboxState("UseWayland",         ui->toggleWayland);
    SetCheckboxState("UseHDR",             ui->toggleWaylandHDR);
    SetCheckboxState("AllowHidraw",        ui->toggleHidraw);
    SetCheckboxState("UseXalia",           ui->toggleXalia);

    if(currentShortcutHash.isEmpty()) {
        // for prefix general settings, checkboxes are normal two-state

        // general tab->prefix global settings group
        // if prefix runner doesn't exist, just set to whatever's the first entry.
        if(NeroFS::GetAvailableProtons().contains(settings.value("CurrentRunner").toString()))
            ui->prefixRunner->setCurrentText(settings.value("CurrentRunner").toString());
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
        if(ui->preRunScriptPath->text().isEmpty())  ui->preRunClearBtn->setVisible(false);
        if(ui->postRunScriptPath->text().isEmpty()) ui->postRunClearBtn->setVisible(false);

        if(QFileInfo::exists(settings["Path"].toString().replace("C:/",
                                                                 NeroFS::GetPrefixesPath().canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/")))
            ui->pathNoExistWarning->setVisible(false);
        else {
            ui->openToShortcutPath->setEnabled(false);
            // adjust font recoloring for light mode (unused, but just in case?)
            /* if(this->palette().window().color().value() > this->palette().text().color().value())
                ui->shortcutPath->setStyleSheet("color: darkred");
            else */
                ui->shortcutPath->setStyleSheet("color: red");
        }

        QDir ico(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.icoCache");
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

    for(const auto &child : this->findChildren<QCheckBox*>())
        child->setFont(QFont());

    for(const auto &child : this->findChildren<QLineEdit*>())
        child->setFont(QFont());

    for(const auto &child : this->findChildren<QComboBox*>())
        child->setFont(QFont());

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
                                                                                    NeroFS::GetPrefixesPath().canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/"),
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
                                                                                   NeroFS::GetPrefixesPath().canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/"),
    "Compatible Windows Executables (*.bat *.exe *.msi);;Windows Batch Script Files (*.bat);;Windows Portable Executable (*.exe);;Windows Installer Package (*.msi)",
                                                  nullptr,
                                                  QFileDialog::DontResolveSymlinks);
    if(!newApp.isEmpty()) {
        ui->shortcutPath->setText(newApp.replace(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c", "C:"));
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
    // TODO(?): curl could be pulled as a dependency and done in code,
    // but it wouldn't be much different since curl is already on most distros anyways?
    // Maybe later.
    QDir tmpDir(QDir::temp());
    if(!tmpDir.exists("nero-manager"))
        tmpDir.mkdir("nero-manager");
    QProcess process;
    process.setWorkingDirectory(tmpDir.path()+"/nero-manager");
    process.start(QStandardPaths::findExecutable("curl"), { "-o", "bridge.zip", "-L", "https://github.com/EnderIce2/rpc-bridge/releases/latest/download/bridge.zip" });
    printf("Downloading Discord RPC Bridge...\n");

    NeroPrefixSettingsWindow::blockSignals(true);
    umuRunning = true;
    ui->tabWidget->setEnabled(false);
    ui->buttonBox->setEnabled(false);
    ui->infoBox->setEnabled(false);
    ui->infoBox->setTitle("Downloading Discord RPC Bridge...");
    ui->infoText->setText("");

    while(process.state() != QProcess::NotRunning) QApplication::processEvents();

    if(process.exitStatus() == 0) {
        printf("Extracting bridge.zip...\n");
        ui->infoBox->setTitle("Extracting bridge package...");

        // QuaZip is like minizip, except it actually works here.
        QuaZip zipFile(tmpDir.absoluteFilePath("nero-manager/bridge.zip"));
        zipFile.open(QuaZip::mdUnzip);
        zipFile.setCurrentFile("bridge.exe");
        QuaZipFile exeToExtract(&zipFile);

        if(exeToExtract.open(QIODevice::ReadOnly)) {
            QFile outFile(tmpDir.absoluteFilePath("nero-manager/bridge.exe"));
            outFile.open(QIODevice::WriteOnly);
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

    } else {
        QMessageBox::warning(this,
                             "Error!",
                             QString("wget exited with an error code:\n\n%1").arg(process.exitStatus()));
        ui->infoBox->setTitle("");
        ui->infoText->setText(QString("wget exited with an error code: %1.").arg(process.exitStatus()));
    }

    ui->tabWidget->setEnabled(true);
    ui->buttonBox->setEnabled(true);
    ui->infoBox->setEnabled(true);
    umuRunning = false;
    NeroPrefixSettingsWindow::blockSignals(false);
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

        env.insert("WINEPREFIX", QString("%1/%2").arg(NeroFS::GetPrefixesPath().path(), NeroFS::GetCurrentPrefix()));
        env.insert("GAMEID", "0");
        env.insert("PROTONPATH", QString("%1/%2").arg(NeroFS::GetProtonsPath().path(), NeroFS::GetCurrentRunner()));
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
    if(sender()->inherits("QComboBox")) {
        QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
        // Shortcut settings: handle "use default" (blank entry)
        if(settings.value(comboBox->property("isFor").toString()).toString().isEmpty())
            if(comboBox->currentIndex() > 0)
                comboBox->setFont(boldFont);
            else comboBox->setFont(QFont());
        // Global prefix settings: always set to an index
        else if(!currentShortcutHash.isEmpty()) {
            if(comboBox->currentIndex()-1 != settings.value(comboBox->property("isFor").toString()).toInt())
                comboBox->setFont(boldFont);
            else comboBox->setFont(QFont());
        } else if(comboBox->currentIndex() != settings.value(comboBox->property("isFor").toString()).toInt())
            comboBox->setFont(boldFont);
        else comboBox->setFont(QFont());
        // extra check for Windows box, since the index doesn't quite match the way it's listed in the UI.
        if(comboBox == ui->winVerBox) {
            if(settings.value("WindowsVersion").toString().isEmpty())
                // ensures that the box is marked when selecting the topmost entry.
                comboBox->setFont(boldFont);
            else if(comboBox->currentText() == winVersionListBackwards[settings.value("WindowsVersion").toInt()])
                comboBox->setFont(QFont()); }

    } else if(sender()->inherits("QCheckBox")) {
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
        // Shortcut settings: handle "use default" half-checked state (blank entry)
        if(checkBox->isTristate())
            if(settings.value(checkBox->property("isFor").toString()).toString().isEmpty())
                if(checkBox->checkState() != Qt::PartiallyChecked)
                    checkBox->setFont(boldFont);
                else checkBox->setFont(QFont());
            else if(checkBox->checkState() != Qt::Checked && settings.value(checkBox->property("isFor").toString()).toBool())
                checkBox->setFont(boldFont);
            else if(checkBox->checkState() != Qt::Unchecked && !settings.value(checkBox->property("isFor").toString()).toBool())
                checkBox->setFont(boldFont);
            else checkBox->setFont(QFont());
        // Normal Global prefix style on/off toggles
        else if(checkBox->isChecked() != settings.value(checkBox->property("isFor").toString()).toBool())
            checkBox->setFont(boldFont);
        else checkBox->setFont(QFont());

    } else if(sender()->inherits("QSpinBox")) {
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(sender());
        if(spinBox->value() != settings.value(spinBox->property("isFor").toString()).toInt())
            spinBox->setFont(boldFont);
        else spinBox->setFont(QFont());

    } else if(sender()->inherits("QLineEdit")) {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
        if(lineEdit->text() != settings.value(lineEdit->property("isFor").toString()).toString())
            lineEdit->setFont(boldFont);
        else lineEdit->setFont(QFont());
    }
}


void NeroPrefixSettingsWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->standardButton(button) == QDialogButtonBox::Reset) {
        LoadSettings();
    } else if(ui->buttonBox->standardButton(button) == QDialogButtonBox::Save) {
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

        if(currentShortcutHash.isEmpty()) {
            // prefix-wide settings

            // for the generic input fields, changed values will have boldFont
            for(const auto &child : this->findChildren<QCheckBox*>())
                if(child->font() == boldFont)
                    NeroFS::SetCurrentPrefixCfg("PrefixSettings", child->property("isFor").toString(), child->isChecked());

            for(const auto &child : this->findChildren<QLineEdit*>())
                if(child->font() == boldFont)
                    NeroFS::SetCurrentPrefixCfg("PrefixSettings", child->property("isFor").toString(), child->text().trimmed());

            for(const auto &child : this->findChildren<QComboBox*>())
                if(child->font() == boldFont)
                    NeroFS::SetCurrentPrefixCfg("PrefixSettings", child->property("isFor").toString(), child->currentIndex());

            NeroFS::SetCurrentPrefixCfg("PrefixSettings", "CurrentRunner", ui->prefixRunner->currentText());

            if(dllsToAdd.count()) NeroFS::SetCurrentPrefixCfg("PrefixSettings", "DLLoverrides", dllsToAdd);
            else NeroFS::SetCurrentPrefixCfg("PrefixSettings", "DLLoverrides", "");


        } else {
            // per-shortcut settings

            // check if new ico was set.
            if(!newAppIcon.isEmpty())
                QFile::copy(newAppIcon, QString("%1/%2/.icoCache/%3").arg(NeroFS::GetPrefixesPath().path(),
                                                                          NeroFS::GetCurrentPrefix(),
                                                                          QString("%1-%2.png").arg(settings.value("Name").toString(), currentShortcutHash)));

            // for the generic input fields, changed values will have boldFont
            for(const auto &child : this->findChildren<QCheckBox*>())
                if(child->font() == boldFont) {
                    if(child->checkState() == Qt::PartiallyChecked)
                        NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, child->property("isFor").toString(), "");
                    else NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, child->property("isFor").toString(), child->isChecked());
                }

            for(const auto &child : this->findChildren<QLineEdit*>())
                if(child->font() == boldFont)
                    NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, child->property("isFor").toString(), child->text().trimmed());

            for(const auto &child : this->findChildren<QSpinBox*>())
                if(child->font() == boldFont)
                    NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, child->property("isFor").toString(), child->value());

            for(const auto &child : this->findChildren<QComboBox*>())
                if(child->font() == boldFont) {
                    if(child->currentIndex() < 1)
                        NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, child->property("isFor").toString(), "");
                    else NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, child->property("isFor").toString(), child->currentIndex()-1);
                }

            // windows version overrides are currently a one-way op (e.g. can't be unset from the UI)
            if(ui->winVerBox->font() == boldFont) {
                int winVerSelected = winVersionListBackwards.indexOf(ui->winVerBox->itemText(ui->winVerBox->currentIndex()));
                NeroFS::SetCurrentPrefixCfg("Shortcuts--"+currentShortcutHash, "WindowsVersion", winVerSelected);

                QDir prefixPath(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
                if(prefixPath.exists("user.reg")) {
                    QFile regFile(prefixPath.path()+"/user.reg");
                    if(regFile.open(QFile::ReadWrite)) {
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
            }

            if(dllsToAdd.count()) NeroFS::SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(currentShortcutHash), "DLLoverrides", dllsToAdd);
            else NeroFS::SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(currentShortcutHash), "DLLoverrides", "");


            appName = ui->shortcutName->text().trimmed();

        }
    // cancel button case isn't needed, since we filter by font to find changed values.
    }
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
                                                                          .replace("C:", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c")));
}
