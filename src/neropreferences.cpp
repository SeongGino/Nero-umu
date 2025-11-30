/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    GUI Manager Settings dialog.

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

#include "neropreferences.h"
#include "ui_neropreferences.h"
#include "nerofs.h"

#include <QShortcut>
#include <QFileDialog>
#include <QProcess>
#include <QStandardPaths>

NeroManagerPreferences::NeroManagerPreferences(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NeroManagerPreferences)
{
    ui->setupUi(this);

    // shortcut ctrl/cmd + W to close the popup window
	QShortcut *shortcutClose = new QShortcut(QKeySequence::Close, this);
	connect(shortcutClose, &QShortcut::activated, this,&NeroManagerPreferences::close);

    // load settings from current prefs file
    managerCfg = NeroFS::GetManagerCfg();
    //ui->runnerNotifs->setChecked(managerCfg->value("UseNotifier").toBool());
    ui->shortcutHide->setChecked(managerCfg->value("ShortcutHidesManager").toBool());
    ui->umuPath->setText(managerCfg->value("UMUpath").toString());
    if(ui->umuPath->text().isEmpty()) {
        ui->umuPath->setPlaceholderText(ui->umuPath->placeholderText() + " (" +
                                        QStandardPaths::findExecutable("umu-run") + ")");
        ui->umuPathClearBtn->setVisible(false);
    }
}

NeroManagerPreferences::~NeroManagerPreferences()
{
    if(accepted) {
        //managerCfg->setValue("UseNotifier", ui->runnerNotifs->isChecked());
        managerCfg->setValue("ShortcutHidesManager", ui->shortcutHide->isChecked());

        if(ui->umuPath->text().isEmpty()) {
            if(!managerCfg->value("UMUpath").toString().isEmpty()) {
                if(NeroFS::SetUmU(QStandardPaths::findExecutable("umu-run"))) managerCfg->setValue("UMUpath", "");
                else QMessageBox::warning(NULL,
                                          "No working system UMU!",
                                          "System paths do not contain a working UMU instance!\n"
                                          "Reverting to previous working path.");
            }
        } else if(NeroFS::SetUmU(ui->umuPath->text())) managerCfg->setValue("UMUpath", ui->umuPath->text());
        else QMessageBox::warning(NULL,
                                  "No working UMU found!",
                                  "Selected path did not point to a working UMU instance!\n"
                                  "Reverting to previous working path.");
    }
    delete ui;
}

void NeroManagerPreferences::on_umuChangeBtn_clicked()
{
    QString newUmuPath = QFileDialog::getOpenFileName(NULL,
                                                      "Select custom UMU executable",
                                                      qEnvironmentVariable("HOME"));

    if(!newUmuPath.isEmpty()) ui->umuPath->setText(newUmuPath);
}


void NeroManagerPreferences::on_umuPathClearBtn_clicked()
{
    ui->umuPath->clear();
}


void NeroManagerPreferences::on_umuPath_textChanged(const QString &arg1)
{
    if(arg1.isEmpty()) ui->umuPathClearBtn->setVisible(false);
    else ui->umuPathClearBtn->setVisible(true);
}

