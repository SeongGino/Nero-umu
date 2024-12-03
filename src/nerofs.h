/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Host Filesystem Management.

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

#ifndef NEROFS_H
#define NEROFS_H

#include <QDir>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>


class NeroFS
{
private:
    // VARS
    static QDir prefixesPath;
    static QDir protonsPath;
    static QString currentPrefix;
    static QString currentRunner;
    static QSettings managerCfg;
    static QStringList currentPrefixOverrides;
    static QStringList prefixes;
    static QStringList availableProtons;

public:
    NeroFS();

    // METHODS
    static bool InitPaths();

    static QDir GetPrefixesPath() { return prefixesPath; }
    static QDir GetProtonsPath() { return protonsPath; }
    static QString GetCurrentPrefix() { return currentPrefix; }
    static QString GetCurrentRunner() { return currentRunner; }
    static QStringList GetCurrentOverrides() { return currentPrefixOverrides; }
    static QStringList GetAvailableProtons();
    static QStringList GetPrefixes();
    static QStringList GetCurrentPrefixShortcuts();
    static QMap<QString, QVariant> GetCurrentPrefixSettings();
    static QMap<QString, QString> GetCurrentShortcutsMap();
    static QMap<QString, QVariant> GetShortcutSettings(const QString);
    static QSettings GetManagerCfg() { return QSettings(QString("%1/NeroLauncher.ini").arg(qEnvironmentVariable("XDG_CONFIG_HOME")), QSettings::IniFormat); }
    static void CreateUserLinks(const QString);
    static void AddNewPrefix(const QString, const QString);
    static void AddNewShortcut(const QString, const QString, const QString);
    static bool DeletePrefix(const QString);
    static void DeleteShortcut(const QString);

    static QSettings* GetCurrentPrefixCfg();

    static QString GetIcoextract();
    static QString GetIcoutils();
    static QString GetUmU();
    static QString GetWinetricks(const QString & = "");

    static void SetCurrentPrefix(const QString);
    static bool SetCurrentPrefixCfg(const QString, const QString, const QVariant);
    static void AddNewShortcutSetting(const QString shortcutHash, const QString key, const QVariant value) {
        SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(shortcutHash), key, value);
    }
};

#endif // NEROFS_H
