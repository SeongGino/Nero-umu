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
#include <QDebug>
#include <QStringBuilder>

int NeroRunner::StartShortcut(const QString &hash, const bool &prefixAlreadyRunning)
{
    settings = NeroFS::GetCurrentPrefixCfg();
    // failsafe for cli runs
    if(NeroFS::GetUmU().isEmpty()) return -1;
    hashVal = hash;
    CombinedSetting pathSetting = CombinedSetting(NeroConfig::path, *this);
    QString prefixPath(NeroFS::GetPrefixesPath()->path() % '/' % NeroFS::GetCurrentPrefix());
    QString pathDir = pathSetting.toString();
    QString cPath = prefixPath % '/' % drive_c;
    QString workingDir = pathDir.left(pathDir.lastIndexOf("/")).replace(cDrive, cPath);

    bool startsWith = pathDir.startsWith(cDrive);
    bool fileExists = QFileInfo::exists(workingDir); //faster than declaring obj
    if(!startsWith && !fileExists) {
        // TODO: We should probably do something more
        return -1;
    }

    QProcess runner;

    // TODO: this is ass for prerun scripts that should be running persistently.
    CombinedSetting prerun = CombinedSetting(NeroConfig::prerunScript, *this);
    if(prerun.hasSetting()) {
        runner.start(prerun.toString(), (QStringList){});

        while(runner.state() != QProcess::NotRunning) {
            runner.waitForReadyRead(-1);
            printf("%s", runner.readAll().constData());
        }

        printf("%s", runner.readAll().constData());
    }

    runner.setProcessChannelMode(QProcess::ForwardedOutputChannel);
    runner.setReadChannel(QProcess::StandardError);

    env = QProcessEnvironment::systemEnvironment();
    env.insert(CliArgs::Wine::prefix, prefixPath);

    // Only explicit set GAMEID when not already declared by user
    // See SeongGino/Nero-umu#66 for more info
    if(!env.contains(CliArgs::gameId)) {
        env.insert(CliArgs::gameId, "0");
    }
    QString protonRunner = CombinedSetting(NeroConfig::currentRunner, *this).toString();
    QString runnerPath = NeroFS::GetProtonsPath()->path() % '/' % protonRunner;
    if(!QFile::exists(runnerPath)) {
        printf("Could not find %s in '%s', ", protonRunner.toLocal8Bit().constData(), NeroFS::GetProtonsPath()->absolutePath().toLocal8Bit().constData());
        if(!NeroFS::GetAvailableProtons()->isEmpty()) {
            protonRunner = NeroFS::GetAvailableProtons()->first();
        }
        printf("using %s instead\n", protonRunner.toLocal8Bit().constData());
    }
    env.insert(CliArgs::protonPath, runnerPath);
    bool isRuntimeUpdate = CombinedSetting(NeroConfig::runtimeUpdate, *this).toBool();
    if(isRuntimeUpdate && !env.contains(CliArgs::umuRuntimeUpdate)){
        env.insert(CliArgs::umuRuntimeUpdate, TRUE);
    }

    prefixAlreadyRunning
        ? env.insert(CliArgs::verb, CliArgs::run)
        : env.insert(CliArgs::verb, CliArgs::waitForExitRun);

    // WAS added here to unrotate Switch controllers,
    // but may not actually be necessary on newer versions based on SDL3? iunno
    if(!env.contains(CliArgs::sdlUseButtonLabels)) {
        env.insert(CliArgs::sdlUseButtonLabels, FALSE);
    }

    InitCache();

    //if(!settings->value("Shortcuts--"+hash+"/CustomEnvVars").toString().isEmpty()) {
    //   q) << settings->value("Shortcuts--"+hash+"/CustomEnvVars").toStringList();
    //}
    CombinedSetting dllOverride = CombinedSetting(NeroConfig::dllOverride, *this);
    CombinedSetting ignored(NeroConfig::ignoreGlobalDlls, *this);
    bool ignoreGlobalDlls = ignored.toBool();
    QStringList dllShortcutOverrides = ignoreGlobalDlls
                                    ? dllOverride.toStringList()
                                    : dllOverride.getPrefixVariant().toStringList() << dllOverride.toStringList();
    dllShortcutOverrides << env.value(CliArgs::Wine::dllOverrides);
    env.insert(CliArgs::Wine::dllOverrides, dllShortcutOverrides.join(';'));

    CombinedSetting forceWine = CombinedSetting(NeroConfig::Proton::forceWineD3D, *this);
    CombinedSetting disableD8vk(NeroConfig::Proton::noD8VK, *this);
    if (forceWine.hasSetting()) {
        bool isWine = forceWine.toBool();
        env.insert(CliArgs::Proton::useWineD3D, QString::number(isWine));
        // D8VK is dependent on DXVK's existence, so forcing WineD3D overrides D8VK.
        if (!disableD8vk.toBool() && !isWine)
            env.insert(CliArgs::Proton::dxvkD3D8, TRUE);
    } else {
        env.insert(CliArgs::Proton::dxvkD3D8, TRUE);
    }
    QMap<QString, QString> boolOptions{
        {NeroConfig::enableNvApi,               CliArgs::Proton::Nvidia::forceNvapi},
        {NeroConfig::Proton::limitGlExtensions, CliArgs::Proton::oldGl},
        {NeroConfig::vkCapture,                 CliArgs::obsVkCapture},
        {NeroConfig::forceIGpu,                 CliArgs::forceIgpu},
    };
    boolOptions = InsertArgs(boolOptions, false);
    int fpsLimit = CombinedSetting(NeroConfig::limitFps, *this).toInt();
    if(fpsLimit)
        env.insert(CliArgs::dxvkFrameRate, QString::number(fpsLimit));
    int syncType = CombinedSetting(NeroConfig::fileSyncMode, *this).toInt();
    SetSyncMode(protonRunner, syncType);
    CombinedSetting debug(NeroConfig::debugOutput, *this);
    if(debug.hasSetting()) {
        InitDebugProperties(debug.toInt());
    }
    // TODO: ideally, we should set this as a colon-separated list of whitelisted "0xVID/0xPID" pairs
    //       but I guess this'll do for now.
    CombinedSetting(NeroConfig::Proton::allowHidraw, *this).hasSettingAndToBool()
            ? env.insert(CliArgs::Proton::hiDraw, TRUE)
            : env.insert(CliArgs::Proton::preferSdl, TRUE);

    CombinedSetting(NeroConfig::Proton::useXalia, *this).hasSettingAndToBool()
            ? env.insert(CliArgs::Proton::useXalia, TRUE)
            : env.insert(CliArgs::Proton::useXalia, FALSE);

    bool isWaylandEnv = env.contains(CliArgs::waylandDisplay)
            ? !env.value(CliArgs::waylandDisplay).isEmpty()
            : false;
    CombinedSetting wayland(NeroConfig::Proton::useWayland, *this);
    if (isWaylandEnv && wayland.hasSetting() && wayland.toBool()) {
        env.insert(CliArgs::Proton::enableWayland, TRUE);
        bool isHdrEnabled = CombinedSetting(NeroConfig::Proton::useHdr, *this).toBool();
        if (isHdrEnabled) {
            env.insert(CliArgs::Proton::useHdr, TRUE);
        }
    }
    QStringList arguments = {NeroFS::GetUmU(), pathSetting.toString()};
    // some arguments are parsed as stringlists and others as string, so check which first.;

    QVariant argsVar =  CombinedSetting(NeroConfig::args, *this).getSettingVariant();
    int t = argsVar.type();
    if (t == QMetaType::QStringList && !argsVar.toStringList().isEmpty()) {
        arguments.append(argsVar.toStringList());
    } else if (t == QMetaType::QString && !argsVar.toString().isEmpty()) {
        // SUPER UNGA BUNGA: manually split string into a list
        QString buf = argsVar.toString();
        QStringList args;
        args.append("");
        bool quotation = false;
        for(const auto &chara : std::as_const(buf)) {
            if(!quotation) {
                if(chara != ' ' && chara != '"') args.last().append(chara);
                else switch(chara.unicode()) {
                    case '"': quotation = true;
                    case ' ': if(!args.last().isEmpty()) args.append(""); break;
                    default: break;
                }
            } else if(chara != '"') args.last().append(chara);
            else {
                quotation = false;
                args.append("");
            }
        }
        if(args.last().isEmpty()) args.removeLast();
        arguments.append(args);
    }

    if(CombinedSetting(NeroConfig::gamemode, *this).toBool())
        arguments.prepend(CliArgs::gamemoderun);

    int scalingMode = CombinedSetting(NeroConfig::Gamescope::scalingMode, *this).toInt();
    QStringList gamescopeArgs = SetScalingMode(scalingMode, fpsLimit, false);
    arguments = SetMangohud(gamescopeArgs, arguments);

    runner.setProcessEnvironment(env);
    // some apps requires working directory to be in the right location
    // (corrected if path starts with Windows drive letter prefix)
    // settings->value("Shortcuts--"+hash+"/Path").toString();

    runner.setWorkingDirectory(workingDir);
    QString command = arguments.takeFirst();
    QString name = CombinedSetting(NeroConfig::name, *this).toString();
    QDir logsDir(prefixPath);
    if(!logsDir.exists(Logs::logDirName))
        logsDir.mkdir(Logs::logDirName);
    logsDir.cd(Logs::logDirName);
    QFile log = QFile(logsDir.path() % '/' % name % '-' % hash % ".txt");
    if(loggingEnabled) {
        log.open(QIODevice::WriteOnly);
        log.resize(0);
        log.write(Logs::currentlyRunningEnv.toLocal8Bit());
        log.write(runner.environment().join('\n').toLocal8Bit());
        log.write(Logs::runningCommand.toLocal8Bit() % command.toLocal8Bit() % ' ' % arguments.join(' ').toLocal8Bit() % Logs::newLine.toLocal8Bit());
        log.write(Logs::blankLine.toLocal8Bit());
    }
    runner.start(command, arguments);
    runner.waitForStarted(-1);
    WaitLoop(runner, log);

    // in case settings changed from manager
    settings = NeroFS::GetCurrentPrefixCfg();

    CombinedSetting postrunScript = CombinedSetting(NeroConfig::postRunScript, *this);
    if(postrunScript.hasShortcutSetting()) {
        runner.start(postrunScript.toString(), (QStringList){});

        while(runner.state() != QProcess::NotRunning) {
            runner.waitForReadyRead(-1);
            printf("%s", runner.readAll().constData());
        }

        printf("%s", runner.readAll().constData());
    }

    return runner.exitCode();
}

QString NeroRunner::GamescopeFilterType(int filterVal) {
    switch(filterVal) {
    case NeroConstant::GSfilterNearest:
        return CliArgs::Gamescope::Filter::nearest;
    case NeroConstant::GSfilterFSR:
        return CliArgs::Gamescope::Filter::fsr;
    case NeroConstant::GSfilterNLS:
        return CliArgs::Gamescope::Filter::nvidiaImageSharpening;
    case NeroConstant::GSfilterPixel:
        return CliArgs::Gamescope::Filter::pixel;
    // Need to confirm on this or if we should just return blank
    default:
        return "";
    }
}

int NeroRunner::StartOnetime(const QString &path, const bool &prefixAlreadyRunning, const QStringList &args)
{
    // failsafe for cli runs
    if(NeroFS::GetUmU().isEmpty()) return -1;

    settings = NeroFS::GetCurrentPrefixCfg();

    QProcess runner;
    // umu seems to direct both umu-run frontend and Proton output to stderr,
    // meaning stdout is virtually unused.
    runner.setProcessChannelMode(QProcess::ForwardedOutputChannel);
    runner.setReadChannel(QProcess::StandardError);

    env = QProcessEnvironment::systemEnvironment();
    QString prefixPath = NeroFS::GetPrefixesPath()->path() % '/' % NeroFS::GetCurrentPrefix();
    env.insert(CliArgs::Wine::prefix, prefixPath);

    // Only explicit set GAMEID when not already declared by user
    // See SeongGino/Nero-umu#66 for more info
    if(!env.contains(CliArgs::gameId)) {
        //This isn't a true false value, so dont use FALSE
        env.insert(CliArgs::gameId, "0");
    }
    QString protonRunner = PrefixSetting(NeroConfig::currentRunner, *this).toString();
    QString runnerPath = NeroFS::GetProtonsPath()->path()% '/' % protonRunner;
    if(!QFile::exists(runnerPath)) {
        printf("Could not find %s in '%s', ", protonRunner.toLocal8Bit().constData(), NeroFS::GetProtonsPath()->absolutePath().toLocal8Bit().constData());
        if(!NeroFS::GetAvailableProtons()->isEmpty()) {
            protonRunner = NeroFS::GetAvailableProtons()->first();
        }
        printf("using %s instead\n", protonRunner.toLocal8Bit().constData());
    }
    env.insert(CliArgs::protonPath, runnerPath);

    prefixAlreadyRunning
        ? env.insert(CliArgs::verb, CliArgs::run)
        : env.insert(CliArgs::verb, CliArgs::waitForExitRun);

    if(!env.contains(CliArgs::sdlUseButtonLabels)) {
        env.insert(CliArgs::sdlUseButtonLabels, FALSE);
    }
    InitCache();

    bool isRuntimeUpdateOnLaunch = PrefixSetting(NeroConfig::runtimeUpdate, *this).toBool();
    if(isRuntimeUpdateOnLaunch) {
        env.insert(CliArgs::umuRuntimeUpdate, TRUE);
    }
    QStringList dllOverrides = PrefixSetting(NeroConfig::dllOverride, *this).toStringList();
    if(!dllOverrides.isEmpty()) {
        env.insert(CliArgs::Wine::dllOverrides, dllOverrides.join(';')+';'+ env.value(CliArgs::Wine::dllOverrides));
    }
    bool isForcedWineD3D = PrefixSetting(NeroConfig::Proton::forceWineD3D, *this).toBool();
    bool disableD8Vk =  PrefixSetting(NeroConfig::Proton::noD8VK, *this).toBool();

    if(isForcedWineD3D) {
        env.insert(CliArgs::Proton::useWineD3D, TRUE);
    } else if(!disableD8Vk) {
        env.insert(CliArgs::Proton::dxvkD3D8, TRUE);
    }
    QMap<QString, QString> simpleBoolSettings {
        {NeroConfig::enableNvApi, CliArgs::Proton::Nvidia::forceNvapi},
        {NeroConfig::Proton::limitGlExtensions, CliArgs::Proton::oldGl},
        {NeroConfig::vkCapture, CliArgs::obsVkCapture},
        {NeroConfig::forceIGpu, CliArgs::forceIgpu}
    };
    bool isPrefixOnly = true;
    simpleBoolSettings = InsertArgs(simpleBoolSettings, isPrefixOnly);
    int fileSyncMode = PrefixSetting(NeroConfig::fileSyncMode, *this).toInt();
    SetSyncMode(protonRunner, fileSyncMode);
    int debugVal = PrefixSetting(NeroConfig::debugOutput, *this).toInt();
    InitDebugProperties(debugVal);
    // Different logic for Prefix Launch of this versus shortcut; is that intentional?
    // shortcut launch doesn't check env for this or PROTON_USE_XALIA beforehand
    bool isHiDraw  = PrefixSetting(NeroConfig::Proton::allowHidraw, *this).toBool();
    if (isHiDraw) {
        // only add if env doesn't have it already
        if (!env.contains(CliArgs::Proton::hiDraw)) {
            env.insert(CliArgs::Proton::hiDraw, TRUE);
        }
    } else {
        // Forces controllers (that otherwise get preferred by hidraw by default) to go through SDL backend instead
        env.insert(CliArgs::Proton::preferSdl, TRUE);
    }
    bool isUsingXalia = PrefixSetting(NeroConfig::Proton::useXalia, *this).toBool();
    QString xalia = CliArgs::Proton::useXalia;
    bool hasXaliaEnvArg = env.contains(xalia);
    if (!hasXaliaEnvArg) {
        isUsingXalia
            ? env.insert(xalia, TRUE)
            : env.insert(xalia, FALSE);
    }

    bool isWaylandEnv = env.contains(CliArgs::waylandDisplay)
            ? !env.value(CliArgs::waylandDisplay).isEmpty()
            : false;

    bool hasWayland = PrefixSetting(NeroConfig::Proton::useWayland, *this).hasSetting();
    if (isWaylandEnv && hasWayland) {
        env.insert(CliArgs::Proton::enableWayland, TRUE);
        bool isHdrEnabled = PrefixSetting(NeroConfig::Proton::useHdr, *this).toBool();
        if (isHdrEnabled) {
            env.insert(CliArgs::Proton::useHdr, TRUE);
        }
    }

    QStringList arguments = {NeroFS::GetUmU()};

    // Proton/umu should be able to translate Windows-type paths on its own, no conversion needed
    arguments.append(path);

    if(!args.isEmpty())
        arguments.append(args);
    bool gamemodeEnabled = PrefixSetting(NeroConfig::gamemode, *this).toBool();
    if(gamemodeEnabled) {
        arguments.prepend(CliArgs::gamemoderun);
    }
    int scalingMode = PrefixSetting(NeroConfig::Gamescope::scalingMode, *this).toInt();
    int fpsLimit = PrefixSetting(NeroConfig::limitFps, *this).toInt();
    QStringList gamescopeArgs = SetScalingMode(scalingMode, fpsLimit, false);
    arguments = SetMangohud(gamescopeArgs, arguments);

    runner.setProcessEnvironment(env);
    if(path.startsWith('/') || path.startsWith("~/") || path.startsWith("./")) {
        runner.setWorkingDirectory(path.left(path.lastIndexOf("/")).replace("C:", NeroFS::GetPrefixesPath()->canonicalPath()+'/'+NeroFS::GetCurrentPrefix()+"/drive_c/"));
    }

    QString command = arguments.takeFirst();

    QDir logsDir(prefixPath);
    if(!logsDir.exists(Logs::logDirName))
        logsDir.mkdir(Logs::logDirName);
    logsDir.cd(Logs::logDirName);
    QFile log = QFile(logsDir.path() % '/' % path.mid(path.lastIndexOf('/')+1) % ".txt");
    if(loggingEnabled) {
        log.open(QIODevice::WriteOnly);
        log.resize(0);
        log.write(Logs::currentlyRunningEnv.toLocal8Bit());
        log.write(runner.environment().join('\n').toLocal8Bit());
        log.write(Logs::runningCommand.toLocal8Bit() % command.toLocal8Bit() % ' ' % arguments.join(' ').toLocal8Bit() % Logs::newLine.toLocal8Bit());
        log.write(Logs::blankLine.toLocal8Bit());
    }
    runner.start(command, arguments);
    runner.waitForStarted(-1);
    WaitLoop(runner, log);

    return runner.exitCode();
}

QStringList NeroRunner::SetScalingMode(int scalingMode, int fpsLimit, bool isPrefixOnly) {
    switch(scalingMode) {
    case NeroConstant::ScalingIntegerScale:
        env.insert(CliArgs::Gamescope::fsrScaling, TRUE);
        env.insert(CliArgs::Gamescope::intScaling, TRUE);
        break;
    case NeroConstant::ScalingFSRperformance:
    case NeroConstant::ScalingFSRbalanced:
    case NeroConstant::ScalingFSRquality:
    case NeroConstant::ScalingFSRhighquality:
    case NeroConstant::ScalingFSRhigherquality:
    case NeroConstant::ScalingFSRhighestquality:
        env.insert(CliArgs::Gamescope::fsrScaling, TRUE);
        env.insert(CliArgs::Gamescope::fsrStrength, QString::number(scalingMode-2));
        break;
    case NeroConstant::ScalingFSRcustom: {
        env.insert(CliArgs::Gamescope::fsrScaling, TRUE);
        QString fsrHeight = initSetting(isPrefixOnly, NeroConfig::Gamescope::fsrCustomH).toString();
        QString fsrWidth = initSetting(isPrefixOnly, NeroConfig::Gamescope::fsrCustomW).toString();
        env.insert(CliArgs::Gamescope::fsrCustom, fsrWidth % 'x' % fsrHeight);
        break;
    }
    case NeroConstant::ScalingGamescopeFullscreen: {
        return SetGamescopeArgs(scalingMode, fpsLimit, isPrefixOnly);
    }
    case NeroConstant::ScalingGamescopeBorderless: {
    case NeroConstant::ScalingGamescopeWindowed:
        return SetGamescopeArgs(scalingMode, fpsLimit, isPrefixOnly);
    }
    default:
        break;
    }
    return QStringList();
}

QMap<QString, QString> NeroRunner::InsertArgs(QMap<QString, QString> properties, bool isPrefixOnly)
{
    for (auto i = properties.begin(), end = properties.end(); i != end; i++) {
        QString neroOption = i.key();
        bool isValid = isPrefixOnly
            ? PrefixSetting(neroOption, *this).hasSettingAndToBool()
            : CombinedSetting(neroOption, *this).hasSettingAndToBool();
        if (isValid) {
            QString cliArg = i.value();
            env.insert(cliArg, TRUE);
        }
    }
    properties.clear();
    return properties;
}

QStringList NeroRunner::SetMangohud(QStringList gamescopeArgs, QStringList arguments)
{
    bool mangoHudEnabled = CombinedSetting(NeroConfig::mangohud, *this).hasSettingAndToBool();
    if(mangoHudEnabled) {
        bool isMangoEnv = env.contains(NeroConfig::mangohud.toUpper());
        if(gamescopeArgs.contains(CliArgs::Gamescope::name)) {
            gamescopeArgs << CliArgs::mangoapp;
        } else if (!isMangoEnv) {
            arguments.prepend(NeroConfig::mangohud.toLower());
        }
    }
    return gamescopeArgs.isEmpty()
               ? arguments
               : (gamescopeArgs << CliArgs::doubleDash) + arguments;

}
struct ResAxes {
    QString setting;
    QString arg;
    ResAxes(QString setting, QString arg) {
        this->arg = arg;
        this->setting = setting;
    }
};

QStringList NeroRunner::SetGamescopeArgs(int scalingMode, int fpsLimit, bool isPrefixOnly)
{
    QString windowArg;
    QList<ResAxes> reses;
    bool isWindowed = scalingMode == NeroConstant::ScalingGamescopeBorderless
                      || scalingMode == NeroConstant::ScalingGamescopeWindowed;
    if (isWindowed) {
        windowArg = scalingMode == NeroConstant::ScalingGamescopeBorderless
                        ? CliArgs::Gamescope::borderless
                        : ""; //blank string for bordered windowed
        reses
            << ResAxes(NeroConfig::Gamescope::outputResW,    CliArgs::Gamescope::width)
            << ResAxes(NeroConfig::Gamescope::outputResH,    CliArgs::Gamescope::height)
            << ResAxes(NeroConfig::Gamescope::windowedResW,  CliArgs::Gamescope::windowedWidth)
            << ResAxes(NeroConfig::Gamescope::windowedResH,  CliArgs::Gamescope::windowedHeight);
    } else if (scalingMode == NeroConstant::ScalingGamescopeFullscreen) {
        windowArg = CliArgs::Gamescope::fullscreen;
        reses
            << ResAxes(NeroConfig::Gamescope::outputResW,    CliArgs::Gamescope::width)
            << ResAxes(NeroConfig::Gamescope::outputResH,    CliArgs::Gamescope::height);
    }
    QStringList gsArgs(CliArgs::Gamescope::name);
    for(int i = 0; i < reses.length(); i++) {
        ResAxes axe = reses[i];
        PrefixSetting s = initSetting(isPrefixOnly, axe.setting);
        if (s.toInt() && !s.toString().isEmpty())
            gsArgs << axe.arg << s.toString();
    }
    if (!windowArg.isEmpty()) {
        gsArgs << windowArg;
    }

    int filterVal = initSetting(isPrefixOnly, NeroConfig::Gamescope::filter).toInt();
    QString filterType = GamescopeFilterType(filterVal);
    if(!filterType.isEmpty()) {
        gsArgs << CliArgs::Gamescope::filter << filterType;
    }


    int scalerVal = initSetting(isPrefixOnly, NeroConfig::Gamescope::scalingType).toInt();
    switch (scalerVal) {
    case NeroConstant::GSscalerInteger:
        gsArgs << CliArgs::Gamescope::scaler << CliArgs::Gamescope::Scaler::integer;
        break;
    case NeroConstant::GSscalerFit:
        gsArgs << CliArgs::Gamescope::scaler << CliArgs::Gamescope::Scaler::fit;
        break;
    case NeroConstant::GSscalerFill:
        gsArgs << CliArgs::Gamescope::scaler << CliArgs::Gamescope::Scaler::fill;
        break;
    case NeroConstant::GSscalerStretch:
        gsArgs << CliArgs::Gamescope::scaler << CliArgs::Gamescope::Scaler::stretch;
        break;
    default:
        break;
    }

    if(!isPrefixOnly && fpsLimit) {
        QByteArray lim = QByteArray::number(fpsLimit);
        gsArgs
            << CliArgs::Gamescope::fpsLimit
            << lim
            << CliArgs::Gamescope::unfocusedFpsLimit
            << lim;
    }
    gsArgs << ("--adaptive-sync");

    return gsArgs;
}

void NeroRunner::SetSyncMode(QString protonRunner, int syncType)
{
    // ntsync SHOULD be better in all scenarios compared to other sync options, but requires kernel 6.14+ and GE-Proton10-9+
        // For older Protons, they should be safely ignoring this and fallback to fsync anyways.
        // Newer protons than GE10-9 should enable this automatically from its end, and doesn't require WOW64
        // (and currently, WOW64 seems problematic for some fringe cases, like TeknoParrot's BudgieLoader not spawning a window)

    switch(syncType) {
        case NeroConstant::NTsync:
            if(protonRunner == ge109) {
                env.insert(CliArgs::Proton::Sync::ntSync, TRUE);
                env.insert(CliArgs::useWow64, TRUE);
            }
            break;
        case NeroConstant::Fsync:
            env.insert(CliArgs::Proton::Sync::noNtSync, TRUE);
            break;
        case NeroConstant::NoSync:
            env.insert(CliArgs::Proton::Sync::noEsync, TRUE);
        case NeroConstant::Esync:
            env.insert(CliArgs::Proton::Sync::noNtSync, TRUE);
            env.insert(CliArgs::Proton::Sync::noFsync, TRUE);
            break;
        default:
            break;
    }
}

void NeroRunner::WaitLoop(QProcess &runner, QFile &log)
{
    QByteArray stdout;
    while(runner.state() != QProcess::NotRunning) {
        if(!halt) {
            runner.waitForReadyRead(1000);
            if(runner.canReadLine()) {
                stdout = runner.readLine();
                printf("%s", stdout.constData());
                if(loggingEnabled)
                    log.write(stdout);

                if(stdout.contains("umu-launcher"))
                    emit StatusUpdate(NeroRunner::RunnerStarting);
                else if(stdout.contains("steamrt3 is up to date"))
                    emit StatusUpdate(NeroRunner::RunnerUpdated);
                else if(stdout.startsWith("Proton: Executable") || stdout.contains("SteamAPI_Init"))
                    emit StatusUpdate(NeroRunner::RunnerProtonStarted);
            }
        } else {
            emit StatusUpdate(NeroRunner::RunnerProtonStopping);
            StopProcess();
            emit StatusUpdate(NeroRunner::RunnerProtonStopped);
            break;
        }
    }

    while(!runner.atEnd()) {
        stdout = runner.readLine();
        printf("%s", stdout.constData());
        if(loggingEnabled)
            log.write(stdout);
    }

    if(loggingEnabled && log.isOpen())
        log.close();
}

void NeroRunner::InitDebugProperties(int value)
{
    switch (value) {
        case NeroConstant::DebugDisabled:
            break;
        case NeroConstant::DebugFull:
            loggingEnabled = true;
            env.insert("WINEDEBUG", "+loaddll,debugstr,mscoree,seh");
            break;
        case NeroConstant::DebugLoadDLL:
            loggingEnabled = true;
            env.insert("WINEDEBUG", "+loaddll");
            break;
    }
}

void NeroRunner::StopProcess()
{
    // TODO: there's almost certainly a not-shitty way of stopping spawned Wine processes
    //       currently, the Kingdom Hearts Re-Fined patches will consistently lock up the prefix and remains resident
    //       even after this shutdown op has supposedly completed.
    QApplication::processEvents();
    QProcess wineStopper;
    env.insert("UMU_NO_PROTON", "1");
    env.remove("UMU_RUNTIME_UPDATE");
    env.insert("UMU_RUNTIME_UPDATE", "0");
    wineStopper.setProcessEnvironment(env);
    wineStopper.start(NeroFS::GetUmU(), { NeroFS::GetProtonsPath()->path()+'/'+NeroFS::GetCurrentRunner()+'/'+"proton", "runinprefix", "wineboot", "-e" });
    wineStopper.waitForFinished();
}

void NeroRunner::InitCache()
{
    QDir cachePath(NeroFS::GetPrefixesPath()->path()+'/'+NeroFS::GetCurrentPrefix());
    if(!cachePath.exists(".shaderCache")) {
        cachePath.mkdir(".shaderCache");
        if(!cachePath.cd(".shaderCache")) printf("Prefix cache directory not available! Not using cache...");
        else {
            env.insert("DXVK_STATE_CACHE_PATH", cachePath.absolutePath());
            env.insert("VKD3D_SHADER_CACHE_PATH", cachePath.absolutePath());
        }
    }
}
