/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Proton Prefix Shortcut Creation Wizard Dialog.

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

#include "neroshortcut.h"
#include "ui_neroshortcut.h"
#include "nerofs.h"
#include "neroico.h"

#include <QDir>
#include <QFileDialog>
#include <QProcess>

NeroShortcutWizard::NeroShortcutWizard(QWidget *parent, const QString &newAppPath)
    : QDialog(parent)
    , ui(new Ui::NeroShortcutWizard)
{
    ui->setupUi(this);
    ui->nameMatchWarning->setVisible(false);

    // if exe is inside of prefix, convert path to Windows path inside C:/
    QString newAppPathConv = newAppPath;
    ui->appPath->setText(newAppPathConv.replace(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c", "C:"));

    NeroIcoExtractor::CheckIcoCache(QDir(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()));

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    appIcon = NeroIcoExtractor::GetIcon(newAppPath);
    QGuiApplication::restoreOverrideCursor();

    if(!appIcon.isEmpty()) {
        if(QPixmap(appIcon).height() < 48)
            ui->appIcon->setIcon(QPixmap(appIcon).scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        else ui->appIcon->setIcon(QPixmap(appIcon));
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    existingShortcuts.append(NeroFS::GetCurrentPrefixShortcuts());
}

NeroShortcutWizard::~NeroShortcutWizard()
{
    QDir tempDir(QDir::tempPath()+"/nero-manager");
    tempDir.removeRecursively();

    delete ui;
}


void NeroShortcutWizard::on_shortcutName_textEdited(const QString &arg1)
{
    if(arg1.isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->nameMatchWarning->setVisible(false);
    } else {
        if(existingShortcuts.contains(arg1)) {
            ui->nameMatchWarning->setVisible(true);
            ui->shortcutName->setStyleSheet("color: red");
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else {
            ui->nameMatchWarning->setVisible(false);
            ui->shortcutName->setStyleSheet("");
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }
}


void NeroShortcutWizard::on_selectBox_clicked()
{
    QString newApp = QFileDialog::getOpenFileName(this,
                                                  "Select a Windows Executable",
                                                  qEnvironmentVariable("HOME"),
    "Compatible Windows Executables (*.bat *.exe *.msi);;Windows Batch Script Files (*.bat);;Windows Portable Executable (*.exe);;Windows Installer Package (*.msi)",
                                                  nullptr,
                                                  QFileDialog::DontResolveSymlinks);

    if(!newApp.isEmpty()) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        appIcon = NeroIcoExtractor::GetIcon(newApp);
        QGuiApplication::restoreOverrideCursor();

        if(!appIcon.isEmpty()) {
            if(QPixmap(appIcon).height() < 48)
                ui->appIcon->setIcon(QPixmap(appIcon).scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
            else ui->appIcon->setIcon(QPixmap(appIcon));
        } else ui->appIcon->setIcon(QIcon::fromTheme("application-x-executable"));

        // if exe is inside of prefix, convert path to Windows path inside C:/
        if(newApp.startsWith(NeroFS::GetPrefixesPath().canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c"))
            newApp = newApp.replace(NeroFS::GetPrefixesPath().canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c", "C:");

        ui->appPath->setText(newApp);
    }
}


void NeroShortcutWizard::on_appIcon_clicked()
{
    QString newIcon = QFileDialog::getOpenFileName(this,
                                                   "Select a Windows Executable",
                                                   qEnvironmentVariable("HOME"),
    "Windows Executable, Dynamic Link Library, Icon Resource File, or Portable Network Graphics File (*.dll *.exe *.ico *.png);;Windows Dynamic Link Library (*.dll);;Windows Executable (*.exe);;Windows Icon Resource (*.ico);;Portable Network Graphics File (*.png)",
                                                   nullptr,
                                                   QFileDialog::DontResolveSymlinks);

    if(!newIcon.isEmpty()) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        newIcon = NeroIcoExtractor::GetIcon(newIcon);
        QGuiApplication::restoreOverrideCursor();

        if(!newIcon.isEmpty()) {
            appIcon = newIcon;
            if(QPixmap(appIcon).height() < 48)
                ui->appIcon->setIcon(QPixmap(appIcon).scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
            else ui->appIcon->setIcon(QPixmap(appIcon));
        }
    }
}


void NeroShortcutWizard::on_buttonBox_accepted()
{
    appPath = ui->appPath->text();
    shortcutName = ui->shortcutName->text();
}
