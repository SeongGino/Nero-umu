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

#ifndef NEROONETIMEDIALOG_H
#define NEROONETIMEDIALOG_H

#include "nerofs.h"

#include <QDialog>
#include <QPushButton>

namespace Ui {
class NeroOneTimeDialog;
}

class NeroOneTimeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NeroOneTimeDialog(QWidget *parent = nullptr);
    ~NeroOneTimeDialog();

    QString selected;

private slots:
    void prefixBtn_clicked() {
        QPushButton *btn = qobject_cast<QPushButton*>(sender());
        selected = NeroFS::GetPrefixes().at(btn->property("slot").toInt());
        close();
    }

private:
    Ui::NeroOneTimeDialog *ui;

    unsigned int LOLRANDOM;
    QList<QPushButton*> prefixesBtns;
};

#endif // NEROONETIMEDIALOG_H
