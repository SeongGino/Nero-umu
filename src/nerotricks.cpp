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

/*  !! This app relies on the following external component: !!

    `winetricks`, LGPLv2, Copyright (C) 2007-2024 Winetricks Contributors

    As this Free Software relies on existing binaries present on the user's system,
    any licenses attached to this file do not apply to these components,
    and are governed by their respective owners.
*/

#include "nerotricks.h"
#include "ui_nerotricks.h"
#include "nerofs.h"

#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>
//#include <QDebug>

QStringList NeroTricksWindow::winetricksAvailVerbs;
QStringList NeroTricksWindow::winetricksDescriptions;

NeroTricksWindow::NeroTricksWindow(QWidget *parent, const QString &runner)
    : QDialog(parent)
    , ui(new Ui::NeroTricksWindow)
{
    ui->setupUi(this);

    if(winetricksAvailVerbs.isEmpty()) { InitVerbs(runner); }

    for(int i = 0; i < winetricksAvailVerbs.count(); i++) {
        verbSelector << new QCheckBox(winetricksAvailVerbs.at(i), this);
        verbDesc << new QLabel(winetricksDescriptions.at(i), this);
        verbDesc.at(i)->setAlignment(Qt::AlignRight);
        verbDesc.at(i)->setWordWrap(true);
        ui->verbsList->addWidget(verbSelector.at(i), i, 0);
        ui->verbsList->addWidget(verbDesc.at(i), i, 1);
        verbIsSelected.insert(winetricksAvailVerbs.at(i), false);
        connect(verbSelector.at(i), SIGNAL(stateChanged(int)), this, SLOT(verbSelectors_stateChanged(int)));
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // for some reason, this fixes scroll area being invisible until the user resizes horizontally
    ui->scrollAreaContents->setVisible(false);
    ui->scrollAreaContents->setVisible(true);

    completer = new QCompleter(winetricksAvailVerbs, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->searchBox->setCompleter(completer);
}

NeroTricksWindow::~NeroTricksWindow()
{
    delete ui;
}

void NeroTricksWindow::InitVerbs(const QString &runner)
{
    if(winetricksAvailVerbs.isEmpty()) {
        if(!NeroFS::GetWinetricks(runner).isEmpty()) {
            QProcess winetricksList;
            QMessageBox waitBox(QMessageBox::NoIcon, "Winetricks Loading", "Please wait...");

            winetricksList.start(NeroFS::GetWinetricks(runner), {"dlls", "list"});
            waitBox.open();
            waitBox.raise();
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            // don't use blocking function so that the dialog shows and the UI doesn't freeze.
            while(winetricksList.state() != QProcess::NotRunning) { QApplication::processEvents(); }

            if(winetricksList.exitCode() == 0) {
                waitBox.setText("Organizing verbs...");

                if(!winetricksAvailVerbs.isEmpty() && !winetricksDescriptions.isEmpty()) {
                    winetricksAvailVerbs.clear();
                    winetricksDescriptions.clear();
                }

                if(NeroFS::GetWinetricks(runner).contains("protontricks"))
                    // read first line, which is boilerplate cd
                    winetricksList.readLine();

                // capture the rest of the verbs list output to organize.
                QString winetricksOutput = winetricksList.readAllStandardOutput();
                winetricksAvailVerbs << winetricksOutput.split("\n", Qt::SkipEmptyParts);

                for(int i = 0; i < winetricksAvailVerbs.count(); i++) {
                    // The winetricks listing uses a several-spaces-long padding to separate name from description,
                    // so use that as the split point to clean up both lists.
                    winetricksDescriptions.append(winetricksAvailVerbs[i].mid(winetricksAvailVerbs[i].indexOf("       ")).trimmed());
                    winetricksAvailVerbs[i] = winetricksAvailVerbs[i].left(winetricksAvailVerbs[i].indexOf("       "));

                    // cleanup "downloadable/cached" bits.
                    winetricksDescriptions[i].remove("[downloadable]");
                    winetricksDescriptions[i].remove("[downloadable,cached]");

                    // SLIGHTLY DIRTY HACK: add spaces in vcrun to clean up descriptions and allow proper wordwrap
                    if(winetricksAvailVerbs.at(i).contains("vcrun")) {
                        winetricksDescriptions[i].replace(',', ", ");
                        // VisualC versions >=2012 need this to avoid extraneous spaces after commas (i.e. after "Microsoft").
                        winetricksDescriptions[i].replace(",  ", ", ");
                    }
                }

                // allfonts isn't in the DLLs list, so weh.
                winetricksAvailVerbs.prepend("allfonts"), winetricksDescriptions.prepend("All fonts (various, 1998-2010) [Has a long install!]");

                // filter out these entries, since they're either not needed, are built into, or wouldn't work with Proton
                FilterTricks({"dxvk", "faudio", "galliumnine", "vkd3d"});
            }
            QGuiApplication::restoreOverrideCursor();
        } else {
            QMessageBox::critical(this,
                                  "No Winetricks!",
                                  "Winetricks doesn't seem to be installed!");
        }
    }
}

void NeroTricksWindow::FilterTricks(const QStringList filters)
{
    for(int i = 0; i < filters.length(); i++) {
        QStringList filterEntries = winetricksAvailVerbs.filter(filters.at(i));
        int slot;
        for(int e = 0; e < filterEntries.length(); e++) {
            slot = winetricksAvailVerbs.indexOf(filterEntries.at(e));
            winetricksAvailVerbs.removeAt(slot);
            winetricksDescriptions.removeAt(slot);
        }
    }
}

void NeroTricksWindow::AddTricks(const QStringList newTricks)
{
    int slot;
    for(int i = 0; i < newTricks.length(); i++) {
        verbSelector.at(winetricksAvailVerbs.indexOf(newTricks.at(i)))->setCheckState(Qt::Checked);
    }
}

void NeroTricksWindow::SetPreinstalledVerbs(const QStringList installed)
{
    int slot;
    for(const auto &verb : installed) {
        // in case verb isn't in the list
        slot = winetricksAvailVerbs.indexOf(verb);
        if(slot >= 0) {
            verbSelector.at(slot)->setCheckState(Qt::Checked);
            verbIsSelected[verb] = false;
            verbSelector.at(slot)->setEnabled(false);
            verbDesc.at(slot)->setEnabled(false);
        }
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

// Apparently "stateChanged" is being deprecated? wtf
// VERY DISTANT TODO: migrate to checkStateChanged(???)
void NeroTricksWindow::verbSelectors_stateChanged(int arg1)
{
    int slot;
    QObject* obj = sender();
    for(int i = 0;;i++) {
        if(obj == verbSelector.at(i)) {
            slot = i;
            break;
        }
    }

    verbIsSelected[winetricksAvailVerbs.at(slot)] = arg1;

    if(!verbIsSelected.key(true, "").isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void NeroTricksWindow::on_searchBox_textEdited(const QString &arg1)
{
    if(arg1.isEmpty()) {
        winetricksFilter.empty();
        for(int i = 0; i < winetricksAvailVerbs.length(); i++) {
            verbSelector.at(i)->setVisible(true), verbDesc.at(i)->setVisible(true);
        }
    } else {
        winetricksFilter = winetricksAvailVerbs.filter(arg1, Qt::CaseInsensitive);

        // is there a better way than hiding all elements and then unhiding what matches the filter?
        for(int i = 0; i < winetricksAvailVerbs.count(); i++) {
            verbSelector.at(i)->setVisible(false), verbDesc.at(i)->setVisible(false);
        }

        int slot;
        for(int i = 0; i < winetricksFilter.count(); i++) {
            slot = winetricksAvailVerbs.indexOf(winetricksFilter.at(i));
            verbSelector.at(slot)->setVisible(true), verbDesc.at(slot)->setVisible(true);
        }
    }
}

void NeroTricksWindow::on_buttonBox_rejected()
{
    QStringList verbsToClean = verbIsSelected.keys(true);
    if(!verbsToClean.isEmpty()) {
        for(int i = 0; i < verbsToClean.length(); i++) {
            verbSelector[winetricksAvailVerbs.indexOf(verbsToClean.at(i))]->setCheckState(Qt::Unchecked);
        }
    }
}

