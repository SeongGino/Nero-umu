/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Proton Prefix Shortcut Creation Wizard Dialog.

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

#ifndef NEROSHORTCUT_H
#define NEROSHORTCUT_H

#include <QDialog>

namespace Ui {
class NeroShortcutWizard;
}

class NeroShortcutWizard : public QDialog
{
    Q_OBJECT

public:
    explicit NeroShortcutWizard(QWidget *parent = nullptr, const QString &newAppPath = "");
    ~NeroShortcutWizard();

    QString appPath;
    QString appIcon;
    QString shortcutName;

private slots:
    void on_shortcutName_textEdited(const QString &arg1);

    void on_selectBox_clicked();

    void on_appIcon_clicked();

    void on_buttonBox_accepted();

private:
    Ui::NeroShortcutWizard *ui;

    QString GetIcoextract();
    QString GetIcoutils();

    QStringList existingShortcuts;
};

#endif // NEROSHORTCUT_H
