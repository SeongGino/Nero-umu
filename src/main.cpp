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
        "usage: nero-umu [--prefix \"Prefix Name\"] executable [arg1] [arg2] [...]\n\n"
        "Nero-umu CLI: Launch Windows executables within a Nero-managed Prefix\n\n"
        "options:\n"
        "  --prefix \"Prefix Name\"  Run executable within \"Prefix Name\"\n"
        "  -h, --help                  Show this help. Helpful, huh? c:\n"
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

        for(uint8_t i = 1; i < argc; i++)
            arguments.append(argv[i]);

        if(argc < 3 && (arguments.last().endsWith(".exe") || arguments.last().endsWith(".msi") || arguments.last().endsWith(".bat"))) {
            printf("Requested to open file!\n");

            if(NeroFS::InitPaths()) {
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
            } else {
                printf("Nero cannot run without a home directory set! Aborting...\n");
                return 1;
            }
        } else if(argc > 3 && arguments.contains("--prefix") && (arguments.last().endsWith(".exe") || arguments.last().endsWith(".msi") || arguments.last().endsWith(".bat"))) {
            if(NeroFS::InitPaths()) {
                NeroFS::SetCurrentPrefix(arguments.takeAt(arguments.indexOf("--prefix")+1));
                arguments.removeAt(arguments.indexOf("--prefix"));
                NeroRunner runner;
                QString executable = arguments.takeFirst();
                return runner.StartOnetime(executable, false, arguments);
            } else {
                printf("Nero cannot run without a home directory set! Aborting...\n");
                return 1;
            }
        } else if(argc < 3 && (arguments.last() == "-h" || arguments.last() == "--help")) {
            PrintHelp();
            return 0;
        } else {
            PrintHelp();
            return 1;
        }
    } else {
        NeroManagerWindow w;
        w.show();
        return a.exec();
    }
}
