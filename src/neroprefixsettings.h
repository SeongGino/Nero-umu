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

#ifndef NEROPREFIXSETTINGS_H
#define NEROPREFIXSETTINGS_H

#include <QDialog>
#include <QLabel>
#include <QMap>
#include <QCompleter>
#include <QToolButton>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDesktopServices>

namespace Ui {
class NeroPrefixSettingsWindow;
}

class NeroPrefixSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit NeroPrefixSettingsWindow(QWidget *parent = nullptr, const QString shortcutHash = "");
    ~NeroPrefixSettingsWindow();

    bool eventFilter(QObject* object, QEvent* event) override;

    void showEvent(QShowEvent* event) override;

    QString newAppIcon;
    QString appName;

    QPushButton *deleteShortcut = nullptr;

private slots:
    void on_shortcutIco_clicked();

    void on_shortcutName_textEdited(const QString &arg1);

    void on_shortcutPathBtn_clicked();

    void on_setScalingBox_activated(int index);

    void on_dllAdder_textEdited(const QString &arg1);

    void on_dllAddBtn_clicked();

    void dll_action_triggered(QAction* action);

    void dll_delete_clicked();

    void on_preRunButton_clicked();

    void on_postRunButton_clicked();

    void on_preRunClearBtn_clicked();

    void on_postRunClearBtn_clicked();

    void on_prefixInstallDiscordRPC_clicked();

    void OptionSet();

    void deleteShortcut_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_tabWidget_currentChanged(int index);

    void on_prefixDrivesBtn_clicked();

    void on_prefixRegeditBtn_clicked();

    void on_prefixWinecfgBtn_clicked();

    void on_openToShortcutPath_clicked();

private:
    Ui::NeroPrefixSettingsWindow *ui;

    void LoadSettings();
    void AddDLL(const QString, const int);
    void StartUmu(const QString, QStringList = {});

    void SetComboBoxItemEnabled(QComboBox * comboBox, const int index, const bool enabled) {
        auto * model = qobject_cast<QStandardItemModel*>(comboBox->model());
        auto * item = model->item(index);
        item->setEnabled(enabled);
    }

    void SetCheckboxState(const QString &, QCheckBox*);

    QString currentShortcutHash;

    QStringList existingShortcuts;

    QStringList winVersionListBackwards;

    QIntValidator *resValidator;

    QMap<QString, QVariant> settings;

    QCompleter *dllList;

    QMap<QString, int> dllOverrides;

    QList<QToolButton*> dllSetting;
    QList<QPushButton*> dllDelete;

    QList<QAction*> dllOptions;

    QFont boldFont;

    bool umuRunning = false;

    const QStringList dllOverrideNames = {
        "Native, then Built-in",
        "Built-in Only",
        "Built-in, then Native",
        "Native Only",
        "Disabled"
    };

    const QStringList commonDLLsList = {
        "d3d8",
        "d3d9",
        "d3d10",
        "d3d11",
        "d3d12",
        "d3dcompiler_42",
        "d3dcompiler_43",
        "d3dcompiler_46",
        "d3dcompiler_47",
        "dxgi",
        "ir50_32",
        "mscoree",
        "msvcr71",
        "version"
    };

    const QStringList winVersionVerb = {
        "win20",
        "win30",
        "win31",
        "nt351",
        "nt40",
        "win95",
        "win98",
        "winme",
        "win2k",
        "winxp",
        "winxp64",
        "win2003",
        "winvista",
        "win2008",
        "win7",
        "win2008r2",
        "win8",
        "win81",
        "win10",
        "win11"
    };
};


#endif // NEROPREFIXSETTINGS_H
