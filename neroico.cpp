/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Icon extraction and creation.

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

/*  !! This app relies on the following external components: !!

    `icoextract`, MIT License, (C) 2015-2016 Fadhil Mandaga / (C) 2019 James Lu

    `icotool` as part of `icoutils`, GPLv3, (C) 1998 Oskar Liljeblad

    As this Free Software relies on existing binaries present on the user's system,
    any licenses attached to this file do not apply to these components,
    and are governed by their respective owners.
*/

#include "neroico.h"
#include "nerofs.h"

#include <QDir>
#include <QString>
#include <QProcess>
#include <QDebug>

QString NeroIcoExtractor::GetIcon(QString sourceFile)
{
    if(sourceFile.endsWith(".exe", Qt::CaseInsensitive)) {
        if(!NeroFS::GetIcoextract().isEmpty() && !NeroFS::GetIcoutils().isEmpty()) {
            QDir tmpDir(QDir::temp());
            // test if this is writable
            tmpDir.mkdir("nero-manager");
            if(tmpDir.cd("nero-manager")) {
                QProcess extProcess;
                extProcess.setWorkingDirectory(tmpDir.path());

                // extract .ico
                extProcess.start(NeroFS::GetIcoextract(), { sourceFile,
                                                            QString("%1.ico").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".exe")) });
                extProcess.waitForFinished();

                if(extProcess.exitCode() == 0) {
                    // list the ico file's contents to find the largest one.
                    extProcess.start(NeroFS::GetIcoutils(), { "-l",
                                                              "--icon",
                                                              QString("%1.ico").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".exe")) });

                    extProcess.waitForFinished();

                    QString stdout;

                    // TODO: is it safe to assume the largest will always be the last result?
                    // TODO: nope, DMC3SE's last entry is the low color depth icon.
                    // we may have to try first with descending bit-depth numbers before
                    // anything else.
                    while(extProcess.canReadLine()) { stdout = extProcess.readLine(); }

                    if(extProcess.exitCode() == 0) {
                        stdout = stdout.remove("--icon --index=").left(stdout.indexOf(' '));

                        // now we actually extract the file.
                        extProcess.start(NeroFS::GetIcoutils(), { "-x",
                                                                  QString("%1.ico").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".exe")),
                                                                  "--icon",
                                                                  "-i",
                                                                  stdout,
                                                                  "-o",
                                                                  QString("%1.png").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".exe")) } );

                        extProcess.waitForFinished();

                        if(extProcess.exitCode() == 0) {
                            return QString("%1/%2.png").arg(tmpDir.path(), sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".exe"));
                        } else {
                            printf("icotool failed to convert icon to png, aborting...\n");
                            return "";
                        }
                    } else {
                        printf("icotool couldn't list contents of icon file, aborting...\n");
                        return "";
                    }
                } else {
                    printf("icoextract couldn't extract an icon, likely because the requested file didn't have any. Aborting...\n");
                    return "";
                }
            } else {
                printf("Cannot create temp scratch directory, aborting...\n");
                return "";
            }
        } else {
            printf("icoextract and/or icoutils not found, aborting...\n");
            return "";
        }
    } else if(sourceFile.endsWith(".dll", Qt::CaseInsensitive)) {
        if(!NeroFS::GetIcoextract().isEmpty() && !NeroFS::GetIcoutils().isEmpty()) {
            QDir tmpDir(QDir::temp());
            // test if this is writable
            tmpDir.mkdir("nero-manager");
            if(tmpDir.cd("nero-manager")) {
                QProcess extProcess;
                extProcess.setWorkingDirectory(tmpDir.path());

                // extract .ico
                extProcess.start(NeroFS::GetIcoextract(), { sourceFile,
                                                           QString("%1.ico").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".dll")) });
                extProcess.waitForFinished();

                if(extProcess.exitCode() == 0) {
                    // list the ico file's contents to find the largest one.
                    extProcess.start(NeroFS::GetIcoutils(), { "-l",
                                                             "--icon",
                                                             QString("%1.ico").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".dll")) });

                    extProcess.waitForFinished();

                    QString stdout;

                    // TODO: is it safe to assume the largest will always be the last result?
                    while(extProcess.canReadLine()) { stdout = extProcess.readLine(); }

                    if(extProcess.exitCode() == 0) {
                        stdout = stdout.remove("--icon --index=").left(stdout.indexOf(' '));

                        // now we actually extract the file.
                        extProcess.start(NeroFS::GetIcoutils(), { "-x",
                                                                 QString("%1.ico").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".dll")),
                                                                 "--icon",
                                                                 "-i",
                                                                 stdout,
                                                                 "-o",
                                                                 QString("%1.png").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".dll")) } );

                        extProcess.waitForFinished();

                        if(extProcess.exitCode() == 0) {
                            return QString("%1/%2.png").arg(tmpDir.path(), sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".dll"));
                        } else {
                            printf("icotool failed to convert icon to png, aborting...\n");
                            return "";
                        }
                    } else {
                        printf("icotool couldn't list contents of icon file, aborting...\n");
                        return "";
                    }
                } else {
                    printf("icoextract couldn't extract an icon, likely because the requested file didn't have any. Aborting...\n");
                    return "";
                }
            } else {
                printf("Cannot create temp scratch directory, aborting...\n");
                return "";
            }
        } else {
            printf("icoextract and/or icoutils not found, aborting...\n");
            return "";
        }
    } else if(sourceFile.endsWith(".ico", Qt::CaseInsensitive)) {
        QDir tmpDir(QDir::temp());
        // test if this is writable
        tmpDir.mkdir("nero-manager");
        if(tmpDir.cd("nero-manager")) {
            QProcess extProcess;
            extProcess.setWorkingDirectory(tmpDir.path());

            // list the ico file's contents to find the largest one.
            extProcess.start(NeroFS::GetIcoutils(), { "-l",
                                                     "--icon",
                                                     sourceFile });

            extProcess.waitForFinished();

            QString stdout;

            // TODO: is it safe to assume the largest will always be the last result?
            while(extProcess.canReadLine()) { stdout = extProcess.readLine(); }

            if(extProcess.exitCode() == 0) {
                stdout = stdout.remove("--icon --index=").left(stdout.indexOf(' '));

                // now we actually extract the file.
                extProcess.start(NeroFS::GetIcoutils(), { "-x",
                                                         sourceFile,
                                                         "--icon",
                                                         "-i",
                                                         stdout,
                                                         "-o",
                                                         QString("%1.png").arg(sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".ico")) } );

                extProcess.waitForFinished();

                if(extProcess.exitCode() == 0) {
                    return QString("%1/%2.png").arg(tmpDir.path(), sourceFile.mid(sourceFile.lastIndexOf('/')+1).remove(".ico"));
                } else {
                    printf("icotool failed to convert icon to png, aborting...\n");
                    return "";
                }
            } else {
                printf("icotool couldn't list contents of icon file, aborting...\n");
                return "";
            }
        }
    } else if(sourceFile.endsWith(".png", Qt::CaseInsensitive)) {
        // needs no conversion, so just use as-is
        return sourceFile;
    } else {
        return "";
    }

    return "";
}
