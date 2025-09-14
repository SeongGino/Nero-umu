/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Proton Prefix Creation Wizard Dialog.

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

#ifndef NEROWIZARD_H
#define NEROWIZARD_H

#include "nerotricks.h"

#include <QDialog>

namespace Ui {
class NeroPrefixWizard;
}

class NeroPrefixWizard : public QDialog
{
    Q_OBJECT

public:
    explicit NeroPrefixWizard(QWidget *parent = nullptr);
    ~NeroPrefixWizard();

    bool userSymlinks = false;
    bool defaultPrefix = false;
    int protonRunner;
    QString prefixName;
    QStringList verbsToInstall;
    QStringList prevVerbs;
    QStringList currentPrefixes;

private slots:
    void on_symlinksCheckbox_stateChanged(int arg1);

    void on_defaultPrefixCheckbox_stateChanged(int arg1);

    void on_protonRunnerBox_currentIndexChanged(int index) { protonRunner = index; }

    void on_prefixNameInput_textChanged(const QString &arg1);

    void on_winetricksBox_clicked();

    void tricksWindow_result();

    void SetFontTricks();
    void SetDXtricks();
    void SetVCRunTricks();
    void SetXactTricks();

    void UpdateTricksButtonText();

private:
    Ui::NeroPrefixWizard *ui;
    NeroTricksWindow *tricks = nullptr;

    QList<QAction*> winetricksPresets;

    QFont boldFont;
    QFont normFont;
};

#endif // NEROWIZARD_H
