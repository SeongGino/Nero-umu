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
#include <QProcess>

// this needs to be re-generated each time the caller needs to reference current prefixCfg.
QSettings *prefixCfg;

QDir NeroFS::prefixesPath;
QDir NeroFS::protonsPath;
QString NeroFS::currentPrefix;
QString NeroFS::currentRunner;
QString NeroFS::currentUMU;
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
        for(int i = prefixes.count()-1; i >= 0; --i) {
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
    return QStandardPaths::findExecutable("icoextract");
}


QString NeroFS::GetIcoutils()
{
    return QStandardPaths::findExecutable("icotool");
}


QString NeroFS::GetUmU()
{
    // if empty, assume first time checking so that UMU is tested
    if(currentUMU.isEmpty()) {
        // default to last used path, if any
        currentUMU = managerCfg.value("UMUpath").toString();
        if(!QFile::exists(currentUMU)) {
            // fallback to system path first
            currentUMU = QStandardPaths::findExecutable("umu-run");
            if(currentUMU.isEmpty()) {
                // UMU not installed, prompt user for custom path
                QMessageBox umuPrompt(QMessageBox::Information,
                                      "Where be UMU?",
                                      "<p>UMU does not seem to be installed on this machine.</p>"
                                      "<p>Nero can make use of a user-provided instance of UMU in lieu of a system-wide install.<br>"
                                      "The latest version can be installed from "
                                      "<a href='https://github.com/Open-Wine-Components/umu-launcher/releases/latest'><span style=' text-decoration: underline; color:#8ab4f8;'>its GitHub releases.</span></a></p>"
                                      "<p>Would you like to select a custom UMU path?</p>",
                                      QMessageBox::Yes | QMessageBox::No);

                while(umuPrompt.result() == QMessageBox::NoButton) switch(umuPrompt.exec()) {
                case QMessageBox::Yes:
                    currentUMU = QFileDialog::getOpenFileName(NULL,
                                                              "Select custom UMU executable",
                                                              qEnvironmentVariable("HOME"));
                    if(currentUMU.isEmpty()) umuPrompt.setResult(QMessageBox::NoButton);
                    else if(!SetUmU(currentUMU)) currentUMU.clear();

                    break;
                case QMessageBox::Cancel:
                    break;
                }
            } else if(!SetUmU(currentUMU)) currentUMU.clear();
        } else if(!SetUmU(currentUMU)) currentUMU.clear();

        return currentUMU;
    } else return currentUMU;
}

bool NeroFS::SetUmU(const QString &umuPath)
{
    if(!umuPath.isEmpty()) {
        QProcess umuTester;
        umuTester.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        umuTester.start(umuPath, {"-v"});
        if(umuTester.waitForFinished()) {
            if(umuTester.exitCode() != 0) {
                printf("ERROR: UMU test ended with exit code %d - not usable!", umuTester.exitCode());
                return false;
            } else {
                printf("Successfully initialized %s from %s\n", umuTester.readAllStandardOutput().trimmed().constData(), umuPath.toLocal8Bit().constData());
                currentUMU = umuPath;
                return true;
            }
        } else {
            printf("ERROR: UMU test timed out - not usable!");
            umuTester.kill();
            return false;
        }
    } else return false;
}

QString NeroFS::GetWinetricks(const QString &runner)
{
    if(!runner.isEmpty()) {
        if(QDir(protonsPath.path() + '/' + runner + "/protonfixes").exists("winetricks"))
            return protonsPath.path() + '/' + runner + "/protonfixes/winetricks";
        else {
            // fall back to system winetricks
            return QStandardPaths::findExecutable("winetricks");
        }
    } else if(QDir(protonsPath.path() + '/' + currentRunner + "/protonfixes").exists("winetricks"))
        return protonsPath.path() + '/' + currentRunner + "/protonfixes/winetricks";
    else {
        // fall back to system winetricks
        return QStandardPaths::findExecutable("winetricks");
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
    if(prefixCfg != nullptr) {
        delete prefixCfg;
        prefixCfg = nullptr;
    }

    if(!currentPrefix.isEmpty())
        prefixCfg = new QSettings(prefixesPath.path() + '/' + currentPrefix + "/nero-settings.ini", QSettings::IniFormat);

    return prefixCfg;
}

bool NeroFS::SetCurrentPrefixCfg(const QString &group, const QString &key, const QVariant &value)
{
    if(GetCurrentPrefixCfg() != nullptr) {
        prefixCfg->beginGroup(group);

        // Only delete blank values if this is a shortcut
        if(group != "PrefixSettings") {
            if((value.type() != QMetaType::QStringList && value.toString().isEmpty()) ||
               (value.type() == QMetaType::QStringList && value.toStringList().isEmpty()))
                prefixCfg->remove(key);
            else prefixCfg->setValue(key, value);
        }
        else prefixCfg->setValue(key, value);

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

    for(const auto &key : std::as_const(settings))
        settingsMap[key] = prefixCfg->value(key);

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
    prefixCfg->setValue("ForceiGPU", false);
    prefixCfg->setValue("LimitGLextensions", false);
    prefixCfg->setValue("DebugOutput", NeroConstant::DebugDisabled);
    prefixCfg->setValue("FileSyncMode", NeroConstant::NTsync);
    prefixCfg->setValue("NoD8VK", false);
    prefixCfg->setValue("ForceWineD3D", false);
    prefixCfg->setValue("UseWayland", false);
    prefixCfg->setValue("UseHDR", false);
    prefixCfg->setValue("AllowHidraw", false);
    prefixCfg->setValue("UseXalia", false);
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
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "LimitFPS", 0);
    //SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "GamescopeFilterStrength", 0);
    SetCurrentPrefixCfg(QString("Shortcuts--%1").arg(newShortcutHash), "IgnoreGlobalDLLs", false);
}

QMap<QString, QVariant> NeroFS::GetShortcutSettings(const QString &shortcutHash)
{
    if(GetCurrentPrefixCfg() != nullptr) {
        if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
        prefixCfg->beginGroup("Shortcuts--" + shortcutHash);

        const QStringList settingKeys = prefixCfg->childKeys();
        QMap<QString, QVariant> settings;

        for(const auto &key : std::as_const(settingKeys))
            settings[key] = prefixCfg->value(key);

        return settings;
    } else {
        printf("THIS SHOULDN'T HAVE HAPPENED: GetCurrentPrefixCfg returned null in GetShortcutSettings which EXPECTS a real pointer!\n");
        return QMap<QString, QVariant>();
    }
}

QStringList NeroFS::GetCurrentPrefixShortcuts()
{
    if(GetCurrentPrefixCfg() != nullptr) {
        if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
        prefixCfg->beginGroup("Shortcuts");

        QStringList hashes = prefixCfg->childKeys();
        QStringList names;

        for(const auto &hash : std::as_const(hashes))
            names.append(prefixCfg->value(hash).toString());

        return names;
    } else {
        printf("THIS SHOULDN'T HAVE HAPPENED: GetCurrentPrefixCfg returned null in GetCurrentPrefixShortcuts which EXPECTS a real pointer!\n");
        return QStringList();
    }
}

QMap<QString, QString> NeroFS::GetCurrentShortcutsMap()
{
    if(GetCurrentPrefixCfg() != nullptr) {
        if(!prefixCfg->group().isEmpty()) prefixCfg->endGroup();
        prefixCfg->beginGroup("Shortcuts");

        QStringList hashes = prefixCfg->childKeys();
        QStringList names;

        for(const auto &hash : std::as_const(hashes))
            names.append(prefixCfg->value(hash).toString());

        // QString Left = name, QString Right = hash
        QMap<QString, QString> shortcutsMap;

        for(int i = 0; i < hashes.count(); ++i)
            shortcutsMap[names.at(i)] = hashes.at(i);

        return shortcutsMap;
    } else {
        printf("THIS SHOULDN'T HAVE HAPPENED: GetCurrentPrefixCfg returned null in GetCurrentShortcutsMap which EXPECTS a real pointer!\n");
        return QMap<QString, QString>();
    }
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
    if(GetCurrentPrefixCfg() != nullptr) {
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
    } else {
        printf("THIS SHOULDN'T HAVE HAPPENED: GetCurrentPrefixCfg returned null in DeleteShortcut which EXPECTS a real pointer!\n");
    }
}
