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
    connect(shortcutClose, &QShortcut::activated, this, &NeroVirtualDriveDialog::close);

    m_prefix.setPath(NeroFS::GetPrefixesPath()->path()+'/'+NeroFS::GetCurrentPrefix()+"/dosdevices");

    for (const QString& letter : m_letters) {
        QFileInfo parser(m_prefix.path().append("/").append(letter));
        if ( parser.isDir() ) {
            addVirtualDrive(parser.fileName(), parser.canonicalFilePath());
        }
    }

    connect(ui->addDirBtn, &QPushButton::clicked, this, &NeroVirtualDriveDialog::onAddDirBtnClicked);
}

NeroVirtualDriveDialog::~NeroVirtualDriveDialog()
{
    for(int i = dirFrames.count(); i > 0; --i) {
        auto widget = dirFrames.at( i - 1 );
        delete widget;
    }

    delete ui;
}

void NeroVirtualDriveDialog::onVirtualDriveLetterChanged(const QString previous, const QString current)
{
    m_usedLetters.removeOne(previous);
    m_usedLetters.append(current);
    m_usedLetters.sort(Qt::CaseSensitivity::CaseInsensitive);
    emit updateWidgets();
}

void NeroVirtualDriveDialog::onVirtualDriveDriveDeleted(const QString letter)
{
    auto widget = sender();
    ui->dirsList->removeWidget(static_cast<QWidget*>(widget));
    delete widget;
    m_usedLetters.removeOne(letter);
    m_usedLetters.sort(Qt::CaseSensitivity::CaseInsensitive);

    ui->addDirBtn->setDisabled( m_letters.count() == m_usedLetters.count() );
    emit updateWidgets();

}

void NeroVirtualDriveDialog::addVirtualDrive(const QString letter, const QString path)
{

    auto frame = new VirtualDriveFrame(m_prefix, letter, path, this);
    connect( frame, &VirtualDriveFrame::letterChanged, this, &NeroVirtualDriveDialog::onVirtualDriveLetterChanged );
    connect( frame, &VirtualDriveFrame::driveDeleted, this, &NeroVirtualDriveDialog::onVirtualDriveDriveDeleted );
    connect( this, &NeroVirtualDriveDialog::updateWidgets, frame, &VirtualDriveFrame::update );
    m_usedLetters.append(letter);
    m_usedLetters.sort(Qt::CaseSensitivity::CaseInsensitive);

    ui->dirsList->addWidget(frame);
    ui->addDirBtn->setDisabled( m_letters.count() == m_usedLetters.count() );
    emit updateWidgets();
}

void NeroVirtualDriveDialog::onAddDirBtnClicked(bool checked)
{
    auto new_path = QDir();
    new_path.setPath(
        QFileDialog::getExistingDirectory(this, "Select a directory", qEnvironmentVariable("HOME"))
        );

    if ( !new_path.path().isEmpty() ) {
        QString letter;

        for ( const auto& let: m_letters ) {
            if ( m_usedLetters.contains(let) ) {
                continue;
            } else {
                letter = let;
                break;
            }
            return;
        }

        QFile file;
        if ( !file.exists(m_prefix.path().append("/").append(letter)) ) {
            if ( file.link(new_path.canonicalPath(), m_prefix.path().append("/").append(letter)) ) {
                addVirtualDrive(letter, new_path.canonicalPath() );
            }
        }
    }
}
