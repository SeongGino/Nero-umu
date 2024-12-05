/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    One-time Runner dialog.

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

#include "neroonetimedialog.h"
#include "ui_neroonetimedialog.h"
#include "nerofs.h"

NeroOneTimeDialog::NeroOneTimeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NeroOneTimeDialog)
{
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/ico/systrayPhi"));

    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // required for good hidpi icon quality because Qt < 6 didn't set this automatically.
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    #endif

    QStringList prefixes = NeroFS::GetPrefixes();

    for(const QString prefix : prefixes) {
        prefixesBtns << new QPushButton(prefix);
        connect(prefixesBtns.last(), &QPushButton::clicked, this, &NeroOneTimeDialog::prefixBtn_clicked);
        ui->prefixes->addWidget(prefixesBtns.last());
    }
}

NeroOneTimeDialog::~NeroOneTimeDialog()
{
    delete ui;
}
