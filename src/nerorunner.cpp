/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Proton Runner backend.

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

#include "nerorunner.h"
#include "neroconstants.h"
#include "nerofs.h"

#include <QApplication>
#include <QProcess>
#include <QDir>

int NeroRunner::StartShortcut(const QString &hash, const bool &prefixAlreadyRunning)
{
    QSettings *settings = NeroFS::GetCurrentPrefixCfg();
    QFileInfo fileToRun(settings->value("Shortcuts--"+hash+"/Path").toString());

    if(fileToRun.exists()) {
        QProcess runner;

        if(!settings->value("Shortcuts--"+hash+"/PreRunScript").toString().isEmpty()) {
            runner.start(settings->value("Shortcuts--"+hash+"/PreRunScript").toString(), {""});

            while(runner.state() != QProcess::NotRunning) {
                runner.waitForReadyRead(-1);
                printf("%s", runner.readAll().constData());
            }
        }

        runner.setProcessChannelMode(QProcess::ForwardedOutputChannel);
        runner.setReadChannel(QProcess::StandardError);

        env = QProcessEnvironment::systemEnvironment();
        env.insert("WINEPREFIX", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
        env.insert("GAMEID", "0");
        env.insert("PROTONPATH", NeroFS::GetProtonsPath().path()+'/'+settings->value("PrefixSettings/CurrentRunner").toString());

        if(settings->value("PrefixSettings/RuntimeUpdateOnLaunch").toBool())
            env.insert("UMU_RUNTIME_UPDATE", "1");

        if(prefixAlreadyRunning)
            env.insert("PROTON_VERB", "run");
        else env.insert("PROTON_VERB", "waitforexitandrun");

        QDir cachePath(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
        if(cachePath.exists(".shaderCache")) cachePath.mkdir(".shaderCache");
        env.insert("DXVK_STATE_CACHE_PATH", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.shaderCache");
        env.insert("VKD3D_SHADER_CACHE_PATH", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.shaderCache");

        // unfortunately, env insert does NOT allow settings bools properly as-is,
        // so all booleans have to be converted to an int string.

        //if(!settings->value("Shortcuts--"+hash+"/CustomEnvVars").toString().isEmpty()) {
        //   qDebug() << settings->value("Shortcuts--"+hash+"/CustomEnvVars").toStringList();
        //}
        if(!settings->value("Shortcuts--"+hash+"/DLLoverrides").toStringList().isEmpty()) {
            if(settings->value("Shortcuts--"+hash+"/IgnoreGlobalDLLs").toBool() || settings->value("PrefixSettings/DLLoverrides").toStringList().isEmpty())
                env.insert("WINEDLLOVERRIDES", settings->value("Shortcuts--"+hash+"/DLLoverrides").toStringList().join(';'));
            // if overrides are duplicated, last overrides take priority over first overrides
            else env.insert("WINEDLLOVERRIDES", settings->value("PrefixSettings/DLLoverrides").toStringList().join(';')+';'
                                                + settings->value("Shortcuts--"+hash+"/DLLoverrides").toStringList().join(';'));
        } else if(!settings->value("PrefixSettings/DLLoverrides").toStringList().isEmpty())
            env.insert("WINEDLLOVERRIDES", settings->value("PrefixSettings/DLLoverrides").toStringList().join(';'));

        if(settings->value("Shortcuts--"+hash+"/ForceWineD3D").toString().isEmpty())
            env.insert("PROTON_USE_WINED3D", QString::number(settings->value("Shortcuts--"+hash+"/ForceWineD3D").toInt()));
        else env.insert("PROTON_USE_WINED3D", QString::number(settings->value("PrefixSettings/ForceWineD3D").toInt()));

        if(settings->value("Shortcuts--"+hash+"/NoD8VK").toString().isEmpty()) {
            if(!settings->value("Shortcuts--"+hash+"/NoD8VK").toBool()) env.insert("PROTON_DXVK_D3D8", "1");
        } else if(!settings->value("PrefixSettings/NoD8VK").toBool()) env.insert("PROTON_DXVK_D3D8", "1");

        if(!settings->value("Shortcuts--"+hash+"/EnableNVAPI").toString().isEmpty()) {
            if(settings->value("Shortcuts--"+hash+"/EnableNVAPI").toBool()) env.insert("PROTON_ENABLE_NVAPI", "1");
        } else if(settings->value("PrefixSettings/EnableNVAPI").toBool())
            env.insert("PROTON_ENABLE_NVAPI", "1");

        if(!settings->value("Shortcuts--"+hash+"/LimitGLextensions").toString().isEmpty()) {
            if(settings->value("Shortcuts--"+hash+"LimitGLextensions").toBool()) env.insert("PROTON_OLD_GL_STRING", "1");
        } else if(settings->value("PrefixSettings/LimitGLextensions").toBool())
            env.insert("PROTON_OLD_GL_STRING", "1");

        if(!settings->value("Shortcuts--"+hash+"/VKcapture").toString().isEmpty()) {
            if(settings->value("Shortcuts--"+hash+"VKcapture").toBool()) env.insert("OBS_VKCAPTURE", "1");
        } else if(settings->value("PrefixSettings/VKcapture").toBool())
            env.insert("OBS_VKCAPTURE", "1");

        if(settings->value("Shortcuts--"+hash+"/LimitFPS").toInt())
            env.insert("DXVK_FRAME_RATE", QString::number(settings->value("Shortcuts--"+hash+"/LimitFPS").toInt()));

        if(!settings->value("Shortcuts--"+hash+"/FileSyncMode").toString().isEmpty()) {
            switch(settings->value("Shortcuts--"+hash+"/FileSyncMode").toInt()) {
            case NeroConstant::NoSync:
                env.insert("PROTON_NO_ESYNC", "1");
            case NeroConstant::Esync:
                env.insert("PROTON_NO_FSYNC", "1"); break;
            }
        } else switch(settings->value("PrefixSettings/FileSyncMode").toInt()) {
            case NeroConstant::NoSync:
                env.insert("PROTON_NO_ESYNC", "1");
            case NeroConstant::Esync:
                env.insert("PROTON_NO_FSYNC", "1"); break;
            }

        if(!settings->value("Shortcuts--"+hash+"/DebugOutput").toString().isEmpty()) {
            switch(settings->value("Shortcuts--"+hash+"/DebugOutput").toInt()) {
            case NeroConstant::DebugFull:
                env.insert("WINEDEBUG", "+loaddll,debugstr,mscoree");
                break;
            case NeroConstant::DebugLoadDLL:
                env.insert("WINEDEBUG", "+loaddll");
                break;
            }
        } else switch(settings->value("PrefixSettings/DebugOutput").toInt()) {
            case NeroConstant::DebugFull:
                env.insert("WINEDEBUG", "+loaddll,debugstr,mscoree");
                break;
            case NeroConstant::DebugLoadDLL:
                env.insert("WINEDEBUG", "+loaddll");
                break;
        }

        QStringList arguments;
        arguments.append("umu-run");

        arguments.append(settings->value("Shortcuts--"+hash+"/Path").toString());

        if(!settings->value("Shortcuts--"+hash+"/Args").toString().isEmpty())
            arguments.append(settings->value("Shortcuts--"+hash+"/Args").toStringList());

        if(!settings->value("Shortcuts--"+hash+"/Gamemode").toString().isEmpty()) {
            if(settings->value("Shortcuts--"+hash+"/Gamemode").toBool())
                arguments.prepend("gamemoderun");
        } else if(settings->value("PrefixSettings/Gamemode").toBool())
            arguments.prepend("gamemoderun");

        if(!settings->value("Shortcuts--"+hash+"/ScalingMode").toString().isEmpty()) {
            switch(settings->value("Shortcuts--"+hash+"/ScalingMode").toInt()) {
            case NeroConstant::ScalingIntegerScale:
                env.insert("WINE_FULLSCREEN_INTEGER_SCALING", "1"); break;
            case NeroConstant::ScalingFSRperformance:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "0");
                break;
            case NeroConstant::ScalingFSRbalanced:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "1");
                break;
            case NeroConstant::ScalingFSRquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "2");
                break;
            case NeroConstant::ScalingFSRhighquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "3");
                break;
            case NeroConstant::ScalingFSRhigherquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "4");
                break;
            case NeroConstant::ScalingFSRhighestquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "5");
                break;
            case NeroConstant::ScalingFSRcustom:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_CUSTOM_MODE",
                           settings->value("Shortcuts--"+hash+"/FSRcustomResW").toString()+'x'+
                           settings->value("Shortcuts--"+hash+"/FSRcustomResH").toString());
                break;
            case NeroConstant::ScalingGamescopeFullscreen:
                arguments.prepend("-f");
                if(settings->value("Shortcuts--"+hash+"/GamescopeOutResH").toInt()) {
                    arguments.prepend(settings->value("Shortcuts--"+hash+"/GamescopeOutResH").toString());
                    arguments.prepend("-h");
                }
                if(settings->value("Shortcuts--"+hash+"/GamescopeOutResW").toInt()) {
                    arguments.prepend(settings->value("Shortcuts--"+hash+"/GamescopeOutResW").toString());
                    arguments.prepend("-w");
                }
                if(settings->value("Shortcuts--"+hash+"/GamescopeFilter").toInt()) {
                    switch(settings->value("Shortcuts--"+hash+"/GamescopeFilter").toInt()) {
                    case NeroConstant::GSfilterNearest:
                        arguments.prepend("nearest"); break;
                    case NeroConstant::GSfilterFSR:
                        arguments.prepend("fsr"); break;
                    case NeroConstant::GSfilterNLS:
                        arguments.prepend("nis"); break;
                    case NeroConstant::GSfilterPixel:
                        arguments.prepend("pixel"); break;
                    }
                    arguments.prepend("-F");
                }
                arguments.prepend("gamescope");
                break;
            case NeroConstant::ScalingGamescopeBorderless:
                arguments.prepend("-b");
            case NeroConstant::ScalingGamescopeWindowed:
                if(settings->value("Shortcuts--"+hash+"/GamescopeWinResH").toInt()) {
                    arguments.prepend(settings->value("Shortcuts--"+hash+"/GamescopeWinResH").toString());
                    arguments.prepend("-H");
                }
                if(settings->value("Shortcuts--"+hash+"/GamescopeWinResW").toInt()) {
                    arguments.prepend(settings->value("Shortcuts--"+hash+"/GamescopeWinResW").toString());
                    arguments.prepend("-W");
                }
                if(settings->value("Shortcuts--"+hash+"/GamescopeOutResH").toInt()) {
                    arguments.prepend(settings->value("Shortcuts--"+hash+"/GamescopeOutResH").toString());
                    arguments.prepend("-h");
                }
                if(settings->value("Shortcuts--"+hash+"/GamescopeOutResW").toInt()) {
                    arguments.prepend(settings->value("Shortcuts--"+hash+"/GamescopeOutResW").toString());
                    arguments.prepend("-w");
                }
                if(settings->value("Shortcuts--"+hash+"/GamescopeFilter").toInt()) {
                    switch(settings->value("Shortcuts--"+hash+"/GamescopeFilter").toInt()) {
                    case NeroConstant::GSfilterNearest:
                        arguments.prepend("nearest"); break;
                    case NeroConstant::GSfilterFSR:
                        arguments.prepend("fsr"); break;
                    case NeroConstant::GSfilterNLS:
                        arguments.prepend("nis"); break;
                    case NeroConstant::GSfilterPixel:
                        arguments.prepend("pixel"); break;
                    }
                    arguments.prepend("-F");
                }
                arguments.prepend("gamescope");
                break;
            }
        } else switch(settings->value("PrefixSettings/ScalingMode").toInt()) {
            case NeroConstant::ScalingIntegerScale:
                env.insert("WINE_FULLSCREEN_INTEGER_SCALING", "1"); break;
            case NeroConstant::ScalingFSRperformance:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "0");
                break;
            case NeroConstant::ScalingFSRbalanced:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "1");
                break;
            case NeroConstant::ScalingFSRquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "2");
                break;
            case NeroConstant::ScalingFSRhighquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "3");
                break;
            case NeroConstant::ScalingFSRhigherquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "4");
                break;
            case NeroConstant::ScalingFSRhighestquality:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "5");
                break;
            case NeroConstant::ScalingFSRcustom:
                env.insert("WINE_FULLSCREEN_FSR", "1");
                env.insert("WINE_FULLSCREEN_FSR_CUSTOM_MODE", settings->value("PrefixSettings/FSRcustomResW").toString()+'x'+
                                                              settings->value("PrefixSettings/FSRcustomResH").toString());
                break;
            case NeroConstant::ScalingGamescopeFullscreen:
                arguments.prepend("-f");
                if(settings->value("PrefixSettings/GamescopeOutResH").toInt()) {
                    arguments.prepend(settings->value("PrefixSettings/GamescopeOutResH").toString());
                    arguments.prepend("-h");
                }
                if(settings->value("PrefixSettings/GamescopeOutResW").toInt()) {
                    arguments.prepend(settings->value("PrefixSettings/GamescopeOutResW").toString());
                    arguments.prepend("-w");
                }
                if(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
                    switch(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
                    case NeroConstant::GSfilterNearest:
                        arguments.prepend("nearest"); break;
                    case NeroConstant::GSfilterFSR:
                        arguments.prepend("fsr"); break;
                    case NeroConstant::GSfilterNLS:
                        arguments.prepend("nis"); break;
                    case NeroConstant::GSfilterPixel:
                        arguments.prepend("pixel"); break;
                    }
                    arguments.prepend("-F");
                }
                arguments.prepend("gamescope");
                break;
            case NeroConstant::ScalingGamescopeBorderless:
                arguments.prepend("-b");
            case NeroConstant::ScalingGamescopeWindowed:
                if(settings->value("PrefixSettings/GamescopeWinResH").toInt()) {
                    arguments.prepend(settings->value("PrefixSettings/GamescopeWinResH").toString());
                    arguments.prepend("-H");
                }
                if(settings->value("PrefixSettings/GamescopeWinResW").toInt()) {
                    arguments.prepend(settings->value("PrefixSettings/GamescopeWinResW").toString());
                    arguments.prepend("-W");
                }
                if(settings->value("PrefixSettings/GamescopeOutResH").toInt()) {
                    arguments.prepend(settings->value("PrefixSettings/GamescopeOutResH").toString());
                    arguments.prepend("-h");
                }
                if(settings->value("PrefixSettings/GamescopeOutResW").toInt()) {
                    arguments.prepend(settings->value("PrefixSettings/GamescopeOutResW").toString());
                    arguments.prepend("-w");
                }
                if(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
                    switch(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
                    case NeroConstant::GSfilterNearest:
                        arguments.prepend("nearest"); break;
                    case NeroConstant::GSfilterFSR:
                        arguments.prepend("fsr"); break;
                    case NeroConstant::GSfilterNLS:
                        arguments.prepend("nis"); break;
                    case NeroConstant::GSfilterPixel:
                        arguments.prepend("pixel"); break;
                    }
                    arguments.prepend("-F");
                }
                arguments.prepend("gamescope");
                break;
        }

        if(!settings->value("Shortcuts--"+hash+"/Mangohud").toString().isEmpty()) {
            if(settings->value("Shortcuts--"+hash+"/Mangohud").toBool()) {
                if(arguments.contains("gamescope"))
                    arguments.insert(1, "--mangoapp");
                else arguments.prepend("mangohud");
            }
        } else if(settings->value("PrefixSettings/Mangohud").toBool()) {
            if(arguments.contains("gamescope"))
                arguments.insert(1, "--mangoapp");
            else arguments.prepend("mangohud");
        }

        runner.setProcessEnvironment(env);
        runner.setWorkingDirectory(settings->value("Shortcuts--"+hash+"/Path").toString().left(settings->value("Shortcuts--"+hash+"/Path").toString().lastIndexOf("/")));
        QString command = arguments.takeFirst();

        QDir logsDir(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
        if(!logsDir.exists(".logs"))
            logsDir.mkdir(".logs");
        logsDir.cd(".logs");
        QFile log(logsDir.path()+'/'+settings->value("Shortcuts--"+hash+"/Name").toString()+'-'+hash+".txt");
        log.open(QIODevice::WriteOnly);
        log.resize(0);

        runner.start(command, arguments);
        runner.waitForStarted(-1);

        WaitLoop(runner, log);

        if(!settings->value("Shortcuts--"+hash+"/PostRunScript").toString().isEmpty()) {
            runner.start(settings->value("Shortcuts--"+hash+"/PostRunScript").toString(), {""});

            while(runner.state() != QProcess::NotRunning) {
                runner.waitForReadyRead(-1);
                printf("%s", runner.readAll().constData());
            }
        }

        return runner.exitCode();
    } else {
        return -1;
    }
}

int NeroRunner::StartOnetime(const QString &path, const QStringList args, const bool &prefixAlreadyRunning)
{
    QSettings *settings = NeroFS::GetCurrentPrefixCfg();

    QProcess runner;
    // umu seems to direct both umu-run frontend and Proton output to stderr,
    // meaning stdout is virtually unused.
    runner.setProcessChannelMode(QProcess::ForwardedOutputChannel);
    runner.setReadChannel(QProcess::StandardError);

    env = QProcessEnvironment::systemEnvironment();
    env.insert("WINEPREFIX", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
    env.insert("GAMEID", "0");
    env.insert("PROTONPATH", NeroFS::GetProtonsPath().path()+'/'+settings->value("PrefixSettings/CurrentRunner").toString());

    if(prefixAlreadyRunning)
        env.insert("PROTON_VERB", "run");
    else env.insert("PROTON_VERB", "waitforexitandrun");

    QDir cachePath(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
    if(cachePath.exists(".shaderCache")) cachePath.mkdir(".shaderCache");
    env.insert("DXVK_STATE_CACHE_PATH", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.shaderCache");
    env.insert("VKD3D_SHADER_CACHE_PATH", NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/.shaderCache");

    if(settings->value("PrefixSettings/RuntimeUpdateOnLaunch").toBool())
        env.insert("UMU_RUNTIME_UPDATE", "1");
    if(!settings->value("PrefixSettings/DLLoverrides").toStringList().isEmpty())
        env.insert("WINEDLLOVERRIDES", settings->value("PrefixSettings/DLLoverrides").toStringList().join(';'));
    if(settings->value("PrefixSettings/ForceWineD3D").toBool())
        env.insert("PROTON_USE_WINED3D", "1");
    else if(!settings->value("PrefixSettings/NoD8VK").toBool())
        env.insert("PROTON_DXVK_D3D8", "1");
    if(settings->value("PrefixSettings/EnableNVAPI").toBool())
        env.insert("PROTON_ENABLE_NVAPI", "1");
    if(settings->value("PrefixSettings/LimitGLextensions").toBool())
        env.insert("PROTON_OLD_GL_STRING", "1");
    if(settings->value("PrefixSettings/VKcapture").toBool())
        env.insert("OBS_VKCAPTURE", "1");

    switch(settings->value("PrefixSettings/FileSyncMode").toInt()) {
    case NeroConstant::NoSync:
        env.insert("PROTON_NO_ESYNC", "1");
    case NeroConstant::Esync:
        env.insert("PROTON_NO_FSYNC", "1"); break;
    }

    switch(settings->value("PrefixSettings/DebugOutput").toInt()) {
    case NeroConstant::DebugFull:
        env.insert("WINEDEBUG", "+loaddll,debugstr,mscoree");
        break;
    case NeroConstant::DebugLoadDLL:
        env.insert("WINEDEBUG", "+loaddll");
        break;
    }

    // TODO: parse custom env vars into an env.insert

    QStringList arguments;
    arguments.append("umu-run");
    arguments.append(path);

    if(!args.isEmpty())
        arguments.append(args);

    if(settings->value("PrefixSettings/Gamemode").toBool())
        arguments.prepend("gamemoderun");

    switch(settings->value("PrefixSettings/ScalingMode").toInt()) {
    case NeroConstant::ScalingIntegerScale:
        env.insert("WINE_FULLSCREEN_INTEGER_SCALING", "1");
        break;
    case NeroConstant::ScalingFSRperformance:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "0");
        break;
    case NeroConstant::ScalingFSRbalanced:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "1");
        break;
    case NeroConstant::ScalingFSRquality:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "2");
        break;
    case NeroConstant::ScalingFSRhighquality:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "3");
        break;
    case NeroConstant::ScalingFSRhigherquality:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "4");
        break;
    case NeroConstant::ScalingFSRhighestquality:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_STRENGTH", "5");
        break;
    case NeroConstant::ScalingFSRcustom:
        env.insert("WINE_FULLSCREEN_FSR", "1");
        env.insert("WINE_FULLSCREEN_FSR_CUSTOM_MODE", QString("%1x%2").arg(settings->value("PrefixSettings/FSRcustomResW").toString(),
                                                                           settings->value("PrefixSettings/FSRcustomResH").toString()));
        break;
    case NeroConstant::ScalingGamescopeFullscreen:
        arguments.prepend("-f");
        if(settings->value("PrefixSettings/GamescopeOutResH").toInt()) {
            arguments.prepend(settings->value("PrefixSettings/GamescopeOutResH").toString());
            arguments.prepend("-h");
        }
        if(settings->value("PrefixSettings/GamescopeOutResW").toInt()) {
            arguments.prepend(settings->value("PrefixSettings/GamescopeOutResW").toString());
            arguments.prepend("-w");
        }
        if(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
            switch(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
            case NeroConstant::GSfilterNearest:
                arguments.prepend("nearest"); break;
            case NeroConstant::GSfilterFSR:
                arguments.prepend("fsr"); break;
            case NeroConstant::GSfilterNLS:
                arguments.prepend("nis"); break;
            case NeroConstant::GSfilterPixel:
                arguments.prepend("pixel"); break;
            }
            arguments.prepend("-F");
        }
        arguments.prepend("gamescope");
        break;
    case NeroConstant::ScalingGamescopeBorderless:
        arguments.prepend("-b");
    case NeroConstant::ScalingGamescopeWindowed:
        if(settings->value("PrefixSettings/GamescopeWinResH").toInt()) {
            arguments.prepend(settings->value("PrefixSettings/GamescopeWinResH").toString());
            arguments.prepend("-H");
        }
        if(settings->value("PrefixSettings/GamescopeWinResW").toInt()) {
            arguments.prepend(settings->value("PrefixSettings/GamescopeWinResW").toString());
            arguments.prepend("-W");
        }
        if(settings->value("PrefixSettings/GamescopeOutResH").toInt()) {
            arguments.prepend(settings->value("PrefixSettings/GamescopeOutResH").toString());
            arguments.prepend("-h");
        }
        if(settings->value("PrefixSettings/GamescopeOutResW").toInt()) {
            arguments.prepend(settings->value("PrefixSettings/GamescopeOutResW").toString());
            arguments.prepend("-w");
        }
        if(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
            switch(settings->value("PrefixSettings/GamescopeFilter").toInt()) {
            case NeroConstant::GSfilterNearest:
                arguments.prepend("nearest"); break;
            case NeroConstant::GSfilterFSR:
                arguments.prepend("fsr"); break;
            case NeroConstant::GSfilterNLS:
                arguments.prepend("nis"); break;
            case NeroConstant::GSfilterPixel:
                arguments.prepend("pixel"); break;
            }
            arguments.prepend("-F");
        }
        arguments.prepend("gamescope");
        break;
    }

    if(settings->value("PrefixSettings/Mangohud").toBool()) {
        if(arguments.contains("gamescope"))
            arguments.insert(1, "--mangoapp");
        else arguments.prepend("mangohud");
    }

    runner.setProcessEnvironment(env);
    runner.setWorkingDirectory(path.left(path.lastIndexOf("/")));
    QString command = arguments.takeFirst();

    QDir logsDir(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix());
    if(!logsDir.exists(".logs"))
        logsDir.mkdir(".logs");
    logsDir.cd(".logs");
    QFile log(logsDir.path()+'/'+path.mid(path.lastIndexOf('/')+1)+".txt");
    log.open(QIODevice::WriteOnly);
    log.resize(0);

    runner.start(command, arguments);
    runner.waitForStarted(-1);

    WaitLoop(runner, log);

    return runner.exitCode();
}

void NeroRunner::WaitLoop(QProcess &runner, QFile &log)
{
    QByteArray stdout;

    while(runner.state() != QProcess::NotRunning) {
        if(!halt) {
            runner.waitForReadyRead(1000);
            if(runner.canReadLine()) {
                stdout = runner.readLine();
                log.write(stdout);
                if(stdout.contains("umu-launcher"))
                    emit StatusUpdate(NeroRunner::RunnerStarting);
                else if(stdout.contains("steamrt is up to date"))
                    emit StatusUpdate(NeroRunner::RunnerUpdated);
                else if(stdout.startsWith("Proton: Executable"))
                    emit StatusUpdate(NeroRunner::RunnerProtonStarted);
            }
        } else {
            emit StatusUpdate(NeroRunner::RunnerProtonStopping);
            StopProcess();
            emit StatusUpdate(NeroRunner::RunnerProtonStopped);
            break;
        }
    }

    log.close();
}

void NeroRunner::StopProcess()
{
    QApplication::processEvents();
    QProcess wineStopper;
    env.insert("UMU_NO_PROTON", "1");
    env.remove("UMU_RUNTIME_UPDATE");
    env.insert("UMU_RUNTIME_UPDATE", "0");
    wineStopper.setProcessEnvironment(env);
    wineStopper.start("umu-run", { NeroFS::GetProtonsPath().path()+'/'+NeroFS::GetCurrentRunner()+'/'+"proton", "runinprefix", "wineboot", "-e" });
    wineStopper.waitForFinished();
}
