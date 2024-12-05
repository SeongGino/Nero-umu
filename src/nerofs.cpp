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

#include "nerofs.h"
#include "neroconstants.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

NeroFS::NeroFS() {}

// this needs to be re-generated each time the caller needs to reference current prefixCfg.
QSettings *prefixCfg;

QDir NeroFS::prefixesPath;
QDir NeroFS::protonsPath;
QString NeroFS::currentPrefix;
QString NeroFS::currentRunner;
QSettings NeroFS::managerCfg;
QStringList NeroFS::currentPrefixOverrides;
QStringList NeroFS::prefixes;
QStringList NeroFS::availableProtons;

bool NeroFS::InitPaths() {
    QSettings managerCfg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/Nero-UMU.ini", QSettings::IniFormat);
    managerCfg.beginGroup("NeroSettings");

    if(managerCfg.value("Home").toString().isEmpty()) {
        QMessageBox::information(NULL,
                                 "Welcome to Nero!",
                                 "This seems to be your first time using Nero.\n"
                                 "Nero uses a central home directory for its Proton prefixes.\n"
                                 "Please select a directory to store your prefixes.");

        // apparently "cancel" is just empty directory.
        QString dir = QFileDialog::getExistingDirectory(NULL,
                                             "Select Nero Home Directory",
                                              qEnvironmentVariable("HOME"));
        if(!dir.isEmpty()) managerCfg.setValue("Home", dir);
        else {
            QMessageBox::critical(NULL,
                                  "ERROR: No Home Directory!",
                                  "Directory is empty, or the operation was canceled.");
            return false;
        }
    }
    prefixesPath.setPath(managerCfg.value("Home").toString());

    QDir steamDir(qEnvironmentVariable("HOME") + "/.steam/steam/compatibilitytools.d");
    if(steamDir.exists()) {
        protonsPath.setPath(steamDir.path());
        printf("Steam detected, using existing compatibilitytools.d\n");
    } else {
        printf("Working Steam install not detected, using Nero data directory for Proton versions\n");
        protonsPath.setPath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Nero-UMU/compatibilitytools.d");
        if(!protonsPath.exists()) {
            printf("Nero directory doesn't exist! Creating paths...\n");
            protonsPath.mkpath(".");
        }
    }

    return true;
}

QStringList NeroFS::GetPrefixes()
{
    if(prefixes.isEmpty()) {
        prefixes = prefixesPath.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
        for(int i = prefixes.count()-1; i >= 0; i--) {
            // should we do ACTUAL ini verification? Or just checking to make sure it exists?
            if(!prefixesPath.exists(prefixes.at(i) + "/nero-settings.ini")) {
                prefixes.removeAt(i);
            }
        }
    }

    return prefixes;
}

void NeroFS::CreateUserLinks(const QString &prefixName)
{
    QDir prefixDir(NeroFS::GetPrefixesPath().path() + '/' + prefixName);
    if(prefixDir.exists()) {
        // TODO: should we allow the user to selectively link certain directories?
        prefixDir.setPath(QString("%1/%2/drive_c/users/%3/Desktop").arg(NeroFS::GetPrefixesPath().path(),
                                                                        prefixName,
                                                                        qEnvironmentVariable("USER")));
        prefixDir.removeRecursively();
        prefixDir.setPath(QString("%1/%2/drive_c/users/%3/Documents").arg(NeroFS::GetPrefixesPath().path(),
                                                                          prefixName,
                                                                          qEnvironmentVariable("USER")));
        prefixDir.removeRecursively();
        prefixDir.setPath(QString("%1/%2/drive_c/users/%3/Downloads").arg(NeroFS::GetPrefixesPath().path(),
                                                                          prefixName,
                                                                          qEnvironmentVariable("USER")));
        prefixDir.removeRecursively();
        prefixDir.setPath(QString("%1/%2/drive_c/users/%3/Music").arg(NeroFS::GetPrefixesPath().path(),
                                                                      prefixName,
                                                                      qEnvironmentVariable("USER")));
        prefixDir.removeRecursively();
        prefixDir.setPath(QString("%1/%2/drive_c/users/%3/Pictures").arg(NeroFS::GetPrefixesPath().path(),
                                                                         prefixName,
                                                                         qEnvironmentVariable("USER")));
        prefixDir.removeRecursively();
        prefixDir.setPath(QString("%1/%2/drive_c/users/%3/Videos").arg(NeroFS::GetPrefixesPath().path(),
                                                                       prefixName,
                                                                       qEnvironmentVariable("USER")));
        prefixDir.removeRecursively();

        if(QDir(qEnvironmentVariable("HOME")).exists("Desktop")) {
            QFile::link(QString("%1/Desktop").arg(qEnvironmentVariable("HOME")),
                        QString("%1/%2/drive_c/users/%3/Desktop").arg(NeroFS::GetPrefixesPath().path(),
                                                                      prefixName,
                                                                      qEnvironmentVariable("USER")));
        }
        if(QDir(qEnvironmentVariable("HOME")).exists("Documents")) {
            QFile::link(QString("%1/Documents").arg(qEnvironmentVariable("HOME")),
                        QString("%1/%2/drive_c/users/%3/Documents").arg(NeroFS::GetPrefixesPath().path(),
                                                                        prefixName,
                                                                        qEnvironmentVariable("USER")));
        }
        if(QDir(qEnvironmentVariable("HOME")).exists("Downloads")) {
            QFile::link(QString("%1/Downloads").arg(qEnvironmentVariable("HOME")),
                        QString("%1/%2/drive_c/users/%3/Downloads").arg(NeroFS::GetPrefixesPath().path(),
                                                                        prefixName,
                                                                        qEnvironmentVariable("USER")));
        }
        if(QDir(qEnvironmentVariable("HOME")).exists("Music")) {
            QFile::link(QString("%1/Music").arg(qEnvironmentVariable("HOME")),
                        QString("%1/%2/drive_c/users/%3/Music").arg(NeroFS::GetPrefixesPath().path(),
                                                                    prefixName,
                                                                    qEnvironmentVariable("USER")));
        }
        if(QDir(qEnvironmentVariable("HOME")).exists("Pictures")) {
            QFile::link(QString("%1/Pictures").arg(qEnvironmentVariable("HOME")),
                        QString("%1/%2/drive_c/users/%3/Pictures").arg(NeroFS::GetPrefixesPath().path(),
                                                                       prefixName,
                                                                       qEnvironmentVariable("USER")));
        }
        if(QDir(qEnvironmentVariable("HOME")).exists("Videos")) {
            QFile::link(QString("%1/Videos").arg(qEnvironmentVariable("HOME")),
                        QString("%1/%2/drive_c/users/%3/Videos").arg(NeroFS::GetPrefixesPath().path(),
                                                                     prefixName,
                                                                     qEnvironmentVariable("USER")));
        }
    }
}

QStringList NeroFS::GetAvailableProtons()
{
    if(availableProtons.isEmpty())
        availableProtons << protonsPath.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    return availableProtons;
}

QString NeroFS::GetIcoextract()
{
    // TODO: this is for flexibility in sandboxed environments(?)
    // idk what the "good" path should be for Flatpak, so...
    if(QDir("/usr/bin").exists("icoextract")) {
        return "/usr/bin/icoextract";
    } else { return ""; }
}


QString NeroFS::GetIcoutils()
{
    // TODO: this is for flexibility in sandboxed environments(?)
    // idk what the "good" path should be for Flatpak, so...
    if(QDir("/usr/bin").exists("icotool")) {
        return "/usr/bin/icotool";
    } else { return ""; }
}


QString NeroFS::GetUmU()
{
    // TODO: this is for flexibility in sandboxed environments(?)
    // idk what the "good" path should be for Flatpak, so...
    if(QDir("/usr/bin").exists("umu-run")) {
        return "/usr/bin/umu-run";
    } else { return ""; }
}

QString NeroFS::GetWinetricks(const QString &runner)
{
    if(!runner.isEmpty()) {
        if(QDir(protonsPath.path() + '/' + runner + "/protonfixes").exists("winetricks"))
            return protonsPath.path() + '/' + runner + "/protonfixes/winetricks";
        else {
            // fall back to system winetricks
            if(QDir("/usr/bin").exists("winetricks"))
                return "/usr/bin/winetricks";
            else return "";
        }
    } else if(QDir(protonsPath.path() + '/' + currentRunner + "/protonfixes").exists("winetricks"))
        return protonsPath.path() + '/' + currentRunner + "/protonfixes/winetricks";
    else {
        // fall back to system winetricks
        if(QDir("/usr/bin").exists("winetricks"))
            return "/usr/bin/winetricks";
        else return "";
    }
}

void NeroFS::SetCurrentPrefix(const QString &prefix)
{
    currentPrefix = prefix;

    GetCurrentPrefixCfg();
    prefixCfg->beginGroup("PrefixSettings");
    currentRunner = prefixCfg->value("CurrentRunner").toString();
}

// TODO: yeah, this is kinda ugly ngl...
QSettings* NeroFS::GetCurrentPrefixCfg()
{
    if(prefixCfg != nullptr) { delete prefixCfg; }
    if(!currentPrefix.isEmpty()) {
        prefixCfg = new QSettings(prefixesPath.path() + '/' + currentPrefix + "/nero-settings.ini", QSettings::IniFormat);
        return prefixCfg;
    } else {
        // if we're running into this problem, then something's gone horribly wrong.
        return nullptr;
    }
}

bool NeroFS::SetCurrentPrefixCfg(const QString &group, const QString &key, const QVariant &value)
{
    GetCurrentPrefixCfg();
    if(prefixCfg != nullptr) {
        prefixCfg->beginGroup(group);
        prefixCfg->setValue(key, value);
        // sync current runner to config
        if(key == "CurrentRunner") currentRunner = value.toString();
        return true;
    } else {
        // no prefix is loaded, so no config to set.
        return false;
    }
}

QMap<QString, QVariant> NeroFS::GetCurrentPrefixSettings()
{
    GetCurrentPrefixCfg();
    prefixCfg->beginGroup("PrefixSettings");

    const QStringList settings = prefixCfg->childKeys();
    QMap<QString, QVariant> settingsMap;

    for(const auto &key : settings) {
        settingsMap[key] = prefixCfg->value(key);
    }

    return settingsMap;
}

void NeroFS::AddNewPrefix(const QString &newPrefix, const QString &runner)
{
    prefixes.append(newPrefix);
    SetCurrentPrefix(newPrefix);
    GetCurrentPrefixCfg();
    prefixCfg->beginGroup("PrefixSettings");
    prefixCfg->setValue("Name", newPrefix);
    prefixCfg->setValue("CurrentRunner", runner);
    prefixCfg->setValue("WindowsVersion", NeroConstant::WinVer10);
    prefixCfg->setValue("Gamemode", false);
    prefixCfg->setValue("VKcapture", false);
    prefixCfg->setValue("Mangohud", false);
    prefixCfg->setValue("EnableNVAPI", false);
    prefixCfg->setValue("ScalingMode", NeroConstant::ScalingNormal);
    prefixCfg->setValue("FSRcustomResW", "");
    prefixCfg->setValue("FSRcustomResH", "");
    prefixCfg->setValue("GamescopeOutResW", "");
    prefixCfg->setValue("GamescopeOutResH", "");
    prefixCfg->setValue("GamescopeWinResW", "");
    prefixCfg->setValue("GamescopeWinResH", "");
    prefixCfg->setValue("GamescopeScaler", NeroConstant::GSscalerAuto);
    prefixCfg->setValue("GamescopeFilter", NeroConstant::GSfilterLinear);
    //prefixCfg->setValue("GamescopeFilterStrength", 0);
    prefixCfg->setValue("DLLoverrides", {""});
    prefixCfg->setValue("LimitGLextensions", false);
    prefixCfg->setValue("DebugOutput", NeroConstant::DebugDisabled);
    prefixCfg->setValue("FileSyncMode", NeroConstant::Fsync);
    prefixCfg->setValue("NoD8VK", false);
    prefixCfg->setValue("ForceWineD3D", false);
    prefixCfg->setValue("CustomEnvVars", {""});
    prefixCfg->setValue("RuntimeUpdateOnLaunch", true);
    prefixCfg->setValue("DiscordRPCinstalled", false);
    prefixCfg->endGroup();
    // since we aren't actually selecting this prefix, just clear the value.
    currentPrefix.clear();
}

void NeroFS::AddNewShortcut(const QString &newShortcutHash, const QString &newShortcutName, const QString &newAppPath) {
    SetCurrentPrefixCfg("Shortcuts", newShortcutHash, newShortcutName);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "Name", newShortcutName);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "Path", newAppPath);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "Args", {""});
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "WindowsVersion", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "Gamemode", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "VKcapture", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "Mangohud", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "EnableNVAPI", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "ScalingMode", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "LimitFPS", 0);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "FSRcustomResW", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "FSRcustomResH", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeOutResW", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeOutResH", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeResW", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeResH", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeScaler", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeFilter", "");
    //SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeFilterStrength", 0);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "DLLoverrides", {""});
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "IgnoreGlobalDLLs", false);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "LimitGLextensions", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "DebugOutput", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "FileSyncMode", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "NoD8VK", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "ForceWineD3D", "");
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "CustomEnvVars", {""});
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "PreRunScript", {""});
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "PostRunScript", {""});
}

QMap<QString, QVariant> NeroFS::GetShortcutSettings(const QString &shortcutHash)
{
    GetCurrentPrefixCfg();
    if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
    prefixCfg->beginGroup("Shortcuts--" + shortcutHash);
    const QStringList settingKeys = prefixCfg->childKeys();
    QMap<QString, QVariant> settings;
    for(const auto &key : settingKeys) {
        settings[key] = prefixCfg->value(key);
    }
    return settings;
}

QStringList NeroFS::GetCurrentPrefixShortcuts()
{
    GetCurrentPrefixCfg();
    if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
    prefixCfg->beginGroup("Shortcuts");

    QStringList hashes = prefixCfg->childKeys();
    QStringList names;

    for(int i = 0; i < hashes.count(); i++) {
        names.append(prefixCfg->value(hashes.at(i)).toString());
    }

    return names;
}

QMap<QString, QString> NeroFS::GetCurrentShortcutsMap()
{
    GetCurrentPrefixCfg();
    if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
    prefixCfg->beginGroup("Shortcuts");

    QStringList hashes = prefixCfg->childKeys();
    QStringList names;

    for(int i = 0; i < hashes.count(); i++) {
        names.append(prefixCfg->value(hashes.at(i)).toString());
    }

    // QString Left = name, QString Right = hash
    QMap<QString, QString> shortcutsMap;

    for(int i = 0; i < hashes.count(); i++) {
        shortcutsMap[names.at(i)] = hashes.at(i);
    }

    return shortcutsMap;
}

bool NeroFS::DeletePrefix(const QString &prefix)
{
    prefixes.removeOne(prefix);
    if(QDir(prefixesPath.path() + '/' + prefix).removeRecursively())
        return true;
    else return false;
}

void NeroFS::DeleteShortcut(const QString &shortcutHash)
{
    GetCurrentPrefixCfg();
    if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
    prefixCfg->beginGroup("Shortcuts");
    QString name = prefixCfg->value(shortcutHash).toString();
    prefixCfg->remove(shortcutHash);
    prefixCfg->endGroup();
    prefixCfg->beginGroup("Shortcuts--" + shortcutHash);
    prefixCfg->remove("");
    prefixCfg->endGroup();
    QFile icoFile(prefixesPath.path() + '/' + currentPrefix + "/.icoCache/" + name + '-' + shortcutHash + ".png");
    if(icoFile.exists()) icoFile.remove();
}
