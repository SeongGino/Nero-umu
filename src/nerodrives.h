/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Prefix Virtual Drives Manager dialog.

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

#ifndef NERODRIVES_H
#define NERODRIVES_H

#include <QDialog>
#include <QDir>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStandardItemModel>

#include "widgets/virtualdriveframe.h"

namespace Ui {
class NeroVirtualDriveDialog;
}

class NeroVirtualDriveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NeroVirtualDriveDialog(QWidget *parent = nullptr);
    ~NeroVirtualDriveDialog();

private slots:
    void onVirtualDriveLetterChanged(const QString, const QString);
    void onVirtualDriveDriveDeleted(const QString);

    void onAddDirBtnClicked(bool);

signals:
    void updateWidgets(void);

private:
    Ui::NeroVirtualDriveDialog *ui;

    void addVirtualDrive(const QString, const QString);

    QDir m_prefix;

    QStringList m_usedLetters;
    const QStringList m_letters = {
        "a:",
        "b:",
        "c:",
        "d:",
        "e:",
        "f:",
        "g:",
        "h:",
        "i:",
        "j:",
        "k:",
        "l:",
        "m:",
        "n:",
        "o:",
        "p:",
        "q:",
        "r:",
        "s:",
        "t:",
        "u:",
        "v:",
        "w:",
        "x:",
        "y:",
        "z:"
    };

    QList<VirtualDriveFrame*> dirFrames;
};

#endif // NERODRIVES_H
