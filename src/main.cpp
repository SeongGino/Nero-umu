/*  Nero Launcher: A very basic Bottles-like manager using UMU.
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

#include "neromanager.h"
#include "nerofs.h"
#include "neroonetimedialog.h"
#include "nerorunner.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

void PrintHelp()
{
    printf(
        "usage: nero-umu [--prefix \"Prefix Name\" [--list] [--shortcut \"Shortcut Name\"]] executable [arg1] [arg2] [...]\n\n"
        "Nero-umu CLI: Launch Windows executables within a Nero-managed Prefix\n\n"
        "options:\n"
        "  --prefix \"Prefix Name\"        Run executable within \"Prefix Name\"\n"
        "  --list                        List contents of prefix specified with --prefix\n"
        "  --shortcut \"Shortcut Name\"    Launch a specific shortcut from specified --prefix, according to the prefix's current settings.\n"
        "  -h, --help                    Show this help. Helpful, huh? c:\n"
        );
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("Nero-UMU");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Nero-Launcher_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    if(argc > 1) {
        QStringList arguments;

        for(int i = 1; i < argc; ++i)
            arguments.append(argv[i]);

        if(NeroFS::InitPaths()) {
            QSettings* managerCfg = new QSettings(NeroFS::GetManagerCfg());
            managerCfg->beginGroup("NeroSettings");

            // One-time runner (executable only) when DefaultPrefix set 
            if(argc < 3 && (arguments.last().endsWith(".exe") || arguments.last().endsWith(".msi") || arguments.last().endsWith(".bat")) && 
                !managerCfg->value("DefaultPrefix").isNull()) {

                printf("Requested to open file!\n");

                NeroFS::SetCurrentPrefix(managerCfg->value("DefaultPrefix").toString());
                NeroRunner runner;
                return runner.StartOnetime(arguments.last(), {});
            // One-time runner (executable only) - prompt user for prefix
            } else if(managerCfg->value("DefaultPrefix").isNull()) {
                NeroOneTimeDialog oneTimeDiag;
                oneTimeDiag.exec();

                if(!oneTimeDiag.selected.isEmpty()) {
                    NeroFS::SetCurrentPrefix(oneTimeDiag.selected);
                    NeroRunner runner;
                    return runner.StartOnetime(arguments.last(), {});
                } else {
                    printf("No prefix selected! Aborting...\n");
                    return 1;
                }
            // One-time runner with provided prefix name
            } else if(argc > 3 && arguments.contains("--prefix") &&
                (arguments.at(arguments.indexOf("--prefix")+2).endsWith(".exe") ||
                arguments.at(arguments.indexOf("--prefix")+2).endsWith(".msi") ||
                arguments.at(arguments.indexOf("--prefix")+2).endsWith(".bat"))) {

                NeroFS::SetCurrentPrefix(arguments.takeAt(arguments.indexOf("--prefix")+1));
                arguments.removeAt(arguments.indexOf("--prefix"));

                NeroRunner runner;
                QString executable = arguments.takeFirst();
                return runner.StartOnetime(executable, false, arguments);
            // One-time runner using prefix with provided preset shortcut
            } else if(argc > 4 && arguments.contains("--prefix") && arguments.contains("--shortcut")) {
                NeroFS::SetCurrentPrefix(arguments.takeAt(arguments.indexOf("--prefix")+1));
                arguments.removeAt(arguments.indexOf("--prefix"));

                QString shortcutHash = NeroFS::GetCurrentShortcutsMap().value(arguments.takeLast());
                if(shortcutHash.isEmpty()) {
                    printf("Shortcut not found in prefix! Check that the spelling is correct, or run Nero Manager to create this shortcut if it doesn't exist.\n");
                    return 1;
                } else {
                    NeroRunner runner;
                    return runner.StartShortcut(shortcutHash);
                }
            // List available shortcuts in defined prefix
            } else if(argc > 3 && arguments.contains("--prefix") && arguments.last() == "--list") {
                NeroFS::SetCurrentPrefix(arguments.takeAt(arguments.indexOf("--prefix")+1));
                arguments.removeAt(arguments.indexOf("--prefix"));

                QStringList shortcutsList = NeroFS::GetCurrentPrefixShortcuts();
                if(shortcutsList.isEmpty()) {
                    printf("Prefix %s doesn't seem to contain any registered shortcuts.\n", NeroFS::GetCurrentPrefix().toLocal8Bit().constData());
                    return 0;
                } else {
                    shortcutsList.sort();

                    printf("\n - %s Shortcuts:\n", NeroFS::GetCurrentPrefix().toLocal8Bit().constData());
                    for(const QString &shortcut : std::as_const(shortcutsList))
                        printf("%s\n", shortcut.toLocal8Bit().constData());
                    return 0;
                }
            // Help printout
            } else if(argc < 3 && (arguments.last() == "-h" || arguments.last() == "--help")) {
                PrintHelp();
                return 0;
            // For unknown args, default to help printout
            } else {
                PrintHelp();
                return 1;
            }
        } else {
            printf("Nero cannot run without a home directory set! Aborting...\n");
            return 1;
        }
    // Start graphical manager when run with no arguments
    } else {
        NeroManagerWindow w;
        w.show();
        return a.exec();
    }
}
