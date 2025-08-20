/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    Prefix Virtual Drives Manager dialog.

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

#include "nerodrives.h"
#include "ui_nerodrives.h"
#include "nerofs.h"

#include <QShortcut>

NeroVirtualDriveDialog::NeroVirtualDriveDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NeroVirtualDriveDialog)
{
    ui->setupUi(this);

    // shortcut ctrl/cmd + W to close the popup window
	QShortcut *shortcutClose = new QShortcut(QKeySequence::Close, this);
	connect(shortcutClose, &QShortcut::activated, this,&NeroVirtualDriveDialog::close);

    lineEditFont.setPointSize(9);
    letterFont.setPointSize(11), letterFont.setBold(true);
    mainLabelFont.setPointSize(11), mainLabelFont.setItalic(true);

    prefixDir.setPath(NeroFS::GetPrefixesPath().path()+'/'+NeroFS::GetCurrentPrefix()+"/dosdevices");

    RenderList();
}

NeroVirtualDriveDialog::~NeroVirtualDriveDialog()
{
    for(int i = 0; i < dirWinLabel.count(); i++) {
        // skip empty/dummied entries that were deleted previously
        if(dirLetter.at(i)->currentIndex() != -1)
            if(!dirWinLabel.at(i)->text().remove('&').trimmed().isEmpty()) {
                QDir labelDir = prefixDir.absoluteFilePath(prefixDir.path()+'/'+dirLetter.at(i)->currentText().toLower());
                if(labelDir.exists()) {
                    labelDir.remove(".windows-label");
                    QFile labelFile(labelDir.canonicalPath()+"/.windows-label");
                    if(labelFile.open(QFile::ReadWrite)) {
                        labelFile.write(dirWinLabel.at(i)->text().remove('&').trimmed().toLocal8Bit());
                    }
                }
            }
    }

    delete ui;
}

void NeroVirtualDriveDialog::RenderList()
{
    // I don't THINK we need this,
    // but can't hurt to check in case re-rendering the list is ever necessary.
    if(dirLetter.isEmpty()) {
        dirLetter.clear();
        dirPath.clear();
        dirWinLabel.clear();
        dirChange.clear();
        dirDelete.clear();
        dirLetterPathLayout.clear();
        dirPathAndLabelsLayout.clear();
        dirWholeLine.clear();
    }
    QStringList currentLinks = prefixDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::Name);

    for(const QString &dir : currentLinks) {
        QFileInfo parser(prefixDir.path()+'/'+dir);
        if(parser.isDir())
            AddDir(parser.canonicalFilePath(), parser.fileName());
    }

    UpdateUsedLetters();
}

void NeroVirtualDriveDialog::AddDir(const QString newPath, const QString letter)
{
    dirLetter << new QComboBox();
    dirLetter.last()->addItems(letters);
    dirLetter.last()->setCurrentText(letter.toUpper());
    dirLetter.last()->setFont(letterFont);
    dirLetter.last()->setProperty("slot", dirLetter.size()-1);
    dirLetter.last()->setFrame(false);
    connect(dirLetter.last(), SIGNAL(activated(int)), this, SLOT(letterBox_activated(int)));

    dirPath << new QLabel(newPath);
    dirPath.last()->setFont(mainLabelFont);
    dirPath.last()->setProperty("letter", letter);

    QFile windowsLabel(newPath+"/.windows-label");
    if(windowsLabel.exists()) {
        windowsLabel.open(QIODevice::ReadWrite);
        dirWinLabel << new QLineEdit(windowsLabel.readLine().trimmed());
    } else dirWinLabel << new QLineEdit();
    dirWinLabel.last()->setPlaceholderText("Drive label");
    dirWinLabel.last()->setFont(lineEditFont);
    dirWinLabel.last()->setFrame(false);
    dirWinLabel.last()->setMaxLength(16);
    dirWinLabel.last()->setStyleSheet("background-color: transparent");
    dirWinLabel.last()->setProperty("slot", dirWinLabel.size()-1);
    connect(dirWinLabel.last(), &QLineEdit::textEdited, this, &NeroVirtualDriveDialog::winLabel_textEdited);

    dirChange << new QPushButton(QIcon::fromTheme("document-open"), "");
    dirChange.last()->setIconSize(QSize(24,24));
    dirChange.last()->setProperty("slot", dirChange.size()-1);
    dirChange.last()->setToolTip("Change symlink to new directory.");
    connect(dirChange.last(), &QPushButton::clicked, this, &NeroVirtualDriveDialog::changeBtn_clicked);

    dirDelete << new QPushButton(QIcon::fromTheme("edit-delete"),"");
    dirDelete.last()->setIconSize(QSize(24,24));
    dirDelete.last()->setFlat(true);
    dirDelete.last()->setProperty("slot", dirDelete.size()-1);
    dirDelete.last()->setToolTip("Delete this symlink.");
    connect(dirDelete.last(), &QPushButton::clicked, this, &NeroVirtualDriveDialog::deleteBtn_clicked);

    dirLetterPathLayout << new QHBoxLayout;
    dirLetterPathLayout.last()->addWidget(dirLetter.last());
    dirLetterPathLayout.last()->addWidget(dirPath.last());
    dirLetterPathLayout.last()->setStretch(1,1);

    dirPathAndLabelsLayout << new QVBoxLayout;
    dirPathAndLabelsLayout.last()->addLayout(dirLetterPathLayout.last());
    dirPathAndLabelsLayout.last()->addWidget(dirWinLabel.last());
    dirPathAndLabelsLayout.last()->setStretch(0,1);

    dirWholeLine << new QHBoxLayout;
    dirWholeLine.last()->addWidget(dirChange.last());
    dirWholeLine.last()->addLayout(dirPathAndLabelsLayout.last());
    dirWholeLine.last()->addWidget(dirDelete.last());
    dirWholeLine.last()->setStretch(1,1);

    ui->dirsList->addLayout(dirWholeLine.last());
    if(letter == "c:") {
        dirLetter.last()->setEnabled(false);
        dirPath.last()->setEnabled(false);
        dirWinLabel.last()->setEnabled(false);
        dirChange.last()->setVisible(false);
        dirDelete.last()->setVisible(false);
        dirLetterPathLayout.last()->setEnabled(false);
        dirPathAndLabelsLayout.last()->setEnabled(false);
        dirWholeLine.last()->setEnabled(false);
    }

    usedLetters.append(letter.toUpper());
}

void NeroVirtualDriveDialog::UpdateUsedLetters()
{
    for(int i = 0; i < dirLetter.size(); i++) {
        // to make sure we don't access deleted objects, check if this is a dummy box.
        if(dirLetter.at(i)->currentIndex() != -1) {
            for(int place = 0; place < letters.size(); place++)
                SetComboBoxItemEnabled(dirLetter.at(i), place, true);
            for(const QString usedLetter : usedLetters) {
                if(usedLetter != dirPath.at(i)->property("letter").toString().toUpper())
                    SetComboBoxItemEnabled(dirLetter.at(i), letters.indexOf(usedLetter.toUpper()), false);
            }
        }
    }
}

void NeroVirtualDriveDialog::on_addDirBtn_clicked()
{
    externalDir.setPath(QFileDialog::getExistingDirectory(this,
                                                          "Select a directory",
                                                          qEnvironmentVariable("HOME")));
    if(!externalDir.path().isEmpty()) {
        QFile newLink;
        char letter[] = {'a', ':', '\0'};
        for(int i = 0; i < 26; i++) {
            letter[0] = 'a'+i;
            if(!newLink.exists(prefixDir.path()+'/'+letter)) {
                if(newLink.link(externalDir.canonicalPath(), prefixDir.path()+'/'+letter)) {
                    AddDir(externalDir.canonicalPath(), letter);
                    UpdateUsedLetters();
                    // in case this is the last possible letter used, disable for safety.
                    if(usedLetters.size() == letters.size())
                        ui->addDirBtn->setEnabled(false);
                    break;
                }
            }
        }
    }
}

void NeroVirtualDriveDialog::letterBox_activated(const int &arg)
{
    int slot = sender()->property("slot").toInt();

    usedLetters.removeOne(dirPath.at(slot)->property("letter").toString().toUpper());

    QFile renamer;
    renamer.rename(prefixDir.path()+'/'+dirPath.at(slot)->property("letter").toString(),
                   prefixDir.path()+'/'+letters.at(arg).toLower());
    dirPath.at(slot)->setProperty("letter", letters.at(arg).toLower());

    usedLetters.append(letters.at(arg));

    UpdateUsedLetters();
}

void NeroVirtualDriveDialog::deleteBtn_clicked()
{
    int slot = sender()->property("slot").toInt();
    QDir dirToDel(prefixDir.path());
    if(dirToDel.remove(dirLetter.at(slot)->currentText().toLower())) {
        usedLetters.removeOne(dirLetter.at(slot)->currentText());
        delete dirLetter.at(slot);
        delete dirPath.at(slot);
        delete dirWinLabel.at(slot);
        delete dirChange.at(slot);
        delete dirDelete.at(slot);
        delete dirLetterPathLayout.at(slot);
        delete dirPathAndLabelsLayout.at(slot);
        delete dirWholeLine.at(slot);

        // just fill the cleared slot with a dummy box
        // to prevent crashing when checking all boxes for letters
        dirLetter[slot] = new QComboBox;

        UpdateUsedLetters();
        // in the rare instance the button is disabled (due to using all letters),
        // enable the button again on confirmed deletion.
        ui->addDirBtn->setEnabled(true);
    }
}

void NeroVirtualDriveDialog::changeBtn_clicked()
{
    int slot = sender()->property("slot").toInt();
    externalDir.setPath(QFileDialog::getExistingDirectory(this,
                                                          "Select a directory",
                                                          qEnvironmentVariable("HOME")));
    if(!externalDir.path().isEmpty()) {
        QFile newLink;
        if(newLink.remove(prefixDir.path()+'/'+dirPath.at(slot)->property("letter").toString())) {
            if(newLink.link(externalDir.canonicalPath(), dirPath.at(slot)->property("letter").toString()))
                dirPath.at(slot)->setText(QString("<b>%1/</b> <i>[%2]</i>").arg(dirPath.at(slot)->property("letter").toString().toUpper(), externalDir.canonicalPath()));
        }
    }
}

void NeroVirtualDriveDialog::winLabel_textEdited(const QString arg1)
{
    int slot = sender()->property("slot").toInt();

    dirWinLabel.at(slot)->setText(arg1.toUpper().trimmed());
}
