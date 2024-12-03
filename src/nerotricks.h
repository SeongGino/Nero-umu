/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Winetricks backend and manager.

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

#ifndef NEROTRICKS_H
#define NEROTRICKS_H

#include <QDialog>
#include <QCheckBox>
#include <QCompleter>
#include <QLabel>
#include <QHash>

namespace Ui {
class NeroTricksWindow;
}

class NeroTricksWindow : public QDialog
{
    Q_OBJECT

public:
    explicit NeroTricksWindow(QWidget *parent = nullptr, const QString & = "");
    ~NeroTricksWindow();

    void InitVerbs(const QString & = "");

    void AddTricks(const QStringList newTricks);
    void SetPreinstalledVerbs(const QStringList installed);

    QStringList GetAvailableVerbs() { return winetricksAvailVerbs; }
    QStringList GetAvailableDescs() { return winetricksDescriptions; }

    QHash<QString, bool> verbIsSelected;

private slots:
    void on_searchBox_textEdited(const QString &arg1);

    void verbSelectors_stateChanged(int arg1);

    void on_buttonBox_rejected();

private:
    Ui::NeroTricksWindow *ui;

    void FilterTricks(const QStringList filters);

    static QStringList winetricksAvailVerbs;
    static QStringList winetricksDescriptions;

    QStringList winetricksFilter;

    QList<QCheckBox*> verbSelector;
    QList<QLabel*> verbDesc;

    QCompleter *completer;

};

#endif // NEROTRICKS_H
