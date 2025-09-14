/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Proton Prefix Creation Wizard Dialog.

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

#include "nerowizard.h"
#include "ui_nerowizard.h"
#include "nerofs.h"

#include <QMessageBox>
#include <QPushButton>
#include <QAction>
#include <QShortcut>

NeroPrefixWizard::NeroPrefixWizard(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NeroPrefixWizard)
{
    ui->setupUi(this);
    ui->nameMatchingWarning->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->symlinkDisclaimer->setVisible(false);

    // shortcut ctrl/cmd + W to close the popup window
	QShortcut *shortcutClose = new QShortcut(QKeySequence::Close, this);
	connect(shortcutClose, &QShortcut::activated, this,&NeroPrefixWizard::close);

    currentPrefixes = NeroFS::GetPrefixes();
    ui->protonRunnerBox->addItems(NeroFS::GetAvailableProtons());
    ui->protonRunnerBox->setCurrentIndex(0);

    boldFont.setPointSize(11);
    boldFont.setBold(true);
    normFont.setPointSize(11);

    winetricksPresets << new QAction("Include Original Fonts (Fixes missing characters in apps and games)")
                      << new QAction("Include Direct3D Extras (Fixes many games)")
                      << new QAction("Include VisualC Redists (Needed by most apps and games)")
                      << new QAction("Include XAudio Libraries (Needed for Xbox ports, e.g. Call of Duty, etc.)");
    ui->winetricksBox->addActions(winetricksPresets);
    connect(winetricksPresets.at(0), &QAction::triggered, this, &NeroPrefixWizard::SetFontTricks);
    connect(winetricksPresets.at(1), &QAction::triggered, this, &NeroPrefixWizard::SetDXtricks);
    connect(winetricksPresets.at(2), &QAction::triggered, this, &NeroPrefixWizard::SetVCRunTricks);
    connect(winetricksPresets.at(3), &QAction::triggered, this, &NeroPrefixWizard::SetXactTricks);

    // adjust font color for light mode
    if(this->palette().window().color().value() > this->palette().text().color().value())
        ui->symlinkDisclaimer->setStyleSheet("color: doubledarkgray");
}

NeroPrefixWizard::~NeroPrefixWizard()
{
    if(tricks != nullptr) delete tricks;
    delete ui;
}

void NeroPrefixWizard::UpdateTricksButtonText()
{
    if(verbsToInstall.isEmpty()) {
        ui->winetricksBox->setFont(normFont);
        ui->winetricksBox->setText("Select Winetricks to Preinstall...");
    } else {
        ui->winetricksBox->setFont(boldFont);
        ui->winetricksBox->setText("Winetricks (" + QString::number(verbsToInstall.count()) + " Selected to Preinstall)");
    }
}

void NeroPrefixWizard::on_prefixNameInput_textChanged(const QString &arg1)
{
    // TODO: maybe filter forward/backslashes? But for now, eh.
    if(arg1.isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->nameMatchingWarning->setVisible(false);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

        if(currentPrefixes.contains(arg1)) {
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            ui->nameMatchingWarning->setVisible(true);
            ui->prefixNameInput->setStyleSheet("color: red");
        } else {
            ui->nameMatchingWarning->setVisible(false);
            ui->prefixNameInput->setStyleSheet("");
        }
    }
    prefixName = arg1;
}

void NeroPrefixWizard::on_winetricksBox_clicked()
{
    if(tricks == nullptr) {
        tricks = new NeroTricksWindow(this, ui->protonRunnerBox->currentText());
        connect(tricks, &NeroTricksWindow::finished, this, &NeroPrefixWizard::tricksWindow_result);
    }

    prevVerbs = verbsToInstall;

    tricks->show();
}

void NeroPrefixWizard::tricksWindow_result()
{
    if(tricks->result() == QDialog::Accepted) {
        verbsToInstall = tricks->verbIsSelected.keys(true);
        verbsToInstall.removeDuplicates();
        if(QMessageBox::question(this,
                                  "Verbs Confirmation",
                                  "Are you sure you wish to install these verbs?\n\n"
                                      + verbsToInstall.join('\n'))
            == QMessageBox::Yes) {

            UpdateTricksButtonText();
        } else tricks->show();
    } else verbsToInstall = prevVerbs;
}

void NeroPrefixWizard::SetFontTricks()
{
    if(tricks == nullptr) {
        tricks = new NeroTricksWindow(this);
        connect(tricks, &NeroTricksWindow::finished, this, &NeroPrefixWizard::tricksWindow_result);
    }

    QStringList prevVerbs = verbsToInstall;

    verbsToInstall.append("allfonts");

    verbsToInstall.removeDuplicates();

    if(QMessageBox::question(this,
                              "Verbs Confirmation",
                              "This will add all available fonts to your Winetricks queue,\n"
                              "in addition to any other verbs that were already selected.\n"
                              "Be warned: the fonts package takes a longer time to install than most other verbs.\n\n"
                              "Are you sure you wish to add all fonts?")
        == QMessageBox::Yes) {

        tricks->AddTricks(verbsToInstall);
        UpdateTricksButtonText();
    } else verbsToInstall = prevVerbs;
}

void NeroPrefixWizard::SetDXtricks()
{
    if(tricks == nullptr) {
        tricks = new NeroTricksWindow(this);
        connect(tricks, &NeroTricksWindow::finished, this, &NeroPrefixWizard::tricksWindow_result);
    }

    QStringList prevVerbs = verbsToInstall;

    verbsToInstall.append({"d3dx9",
                           "d3dx10",
                           "d3dcompiler_42",
                           "d3dcompiler_43",
                           "d3dcompiler_46",
                           "d3dcompiler_47"});

    verbsToInstall.removeDuplicates();

    if(QMessageBox::question(this,
                              "Verbs Confirmation",
                              "This will add the following verbs to your Winetricks queue,\n"
                              "in addition to any that were already selected:\n\n"
                              "d3dx9\n"
                              "d3dx10\n"
                              "d3dcompiler_42\n"
                              "d3dcompiler_43\n"
                              "d3dcompiler_46\n"
                              "d3dcompiler_47\n\n"
                              "Are you sure you wish to add these?")
        == QMessageBox::Yes) {

        tricks->AddTricks(verbsToInstall);
        UpdateTricksButtonText();
    } else verbsToInstall = prevVerbs;
}

void NeroPrefixWizard::SetVCRunTricks()
{
    if(tricks == nullptr) {
        tricks = new NeroTricksWindow(this);
        connect(tricks, &NeroTricksWindow::finished, this, &NeroPrefixWizard::tricksWindow_result);
    }

    QStringList prevVerbs = verbsToInstall;

    verbsToInstall.append({"vcrun2005",
                           "vcrun2008",
                           "vcrun2010",
                           "vcrun2012",
                           "vcrun2013",
                           "vcrun2022"});

    verbsToInstall.removeDuplicates();

    if(QMessageBox::question(this,
                              "Verbs Confirmation",
                              "This will add the following verbs to your Winetricks queue,\n"
                              "in addition to any that were already selected:\n\n"
                              "vcrun2005\n"
                              "vcrun2008\n"
                              "vcrun2010\n"
                              "vcrun2012\n"
                              "vcrun2013\n"
                              "vcrun2022\n\n"
                              "Are you sure you wish to add these?")
        == QMessageBox::Yes) {

        tricks->AddTricks(verbsToInstall);
        UpdateTricksButtonText();
    } else verbsToInstall = prevVerbs;
}

void NeroPrefixWizard::SetXactTricks()
{
    if(tricks == nullptr) {
        tricks = new NeroTricksWindow(this);
        connect(tricks, &NeroTricksWindow::finished, this, &NeroPrefixWizard::tricksWindow_result);
    }

    QStringList prevVerbs = verbsToInstall;

    verbsToInstall.append({"xact",
                           "xact_x64"});

    verbsToInstall.removeDuplicates();

    if(QMessageBox::question(this,
                              "Verbs Confirmation",
                              "This will add the following verbs to your Winetricks queue,\n"
                              "in addition to any that were already selected:\n\n"
                              "xact\n"
                              "xact_x64\n\n"
                              "Are you sure you wish to add these?")
        == QMessageBox::Yes) {

        tricks->AddTricks(verbsToInstall);
        UpdateTricksButtonText();
    } else verbsToInstall = prevVerbs;
}

void NeroPrefixWizard::on_symlinksCheckbox_stateChanged(int arg1)
{
    userSymlinks = arg1;
    ui->symlinkDisclaimer->setVisible(arg1);
}

void NeroPrefixWizard::on_defaultPrefixCheckbox_stateChanged(int arg1)
{
    defaultPrefix = arg1;
}
