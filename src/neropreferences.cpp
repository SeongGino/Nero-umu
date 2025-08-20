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

#include <QShortcut>

NeroManagerPreferences::NeroManagerPreferences(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NeroManagerPreferences)
{
    ui->setupUi(this);

    // shortcut ctrl/cmd + W to close the popup window
	QShortcut *shortcutClose = new QShortcut(QKeySequence::Close, this);
	connect(shortcutClose, &QShortcut::activated, this,&NeroManagerPreferences::close);
}

NeroManagerPreferences::~NeroManagerPreferences()
{
    if(accepted) {
        //managerCfg->setValue("UseNotifier", ui->runnerNotifs->isChecked());
        managerCfg->setValue("ShortcutHidesManager", ui->shortcutHide->isChecked());
    }
    delete ui;
}

void NeroManagerPreferences::BindSettings(QSettings *cfg)
{
    managerCfg = cfg;
    //ui->runnerNotifs->setChecked(managerCfg->value("UseNotifier").toBool());
    ui->shortcutHide->setChecked(managerCfg->value("ShortcutHidesManager").toBool());
}
