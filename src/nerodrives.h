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

#ifndef NERODRIVES_H
#define NERODRIVES_H

#include <QDialog>
#include <QDir>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStandardItemModel>

namespace Ui {
class NeroVirtualDriveDialog;
}

class NeroVirtualDriveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NeroVirtualDriveDialog(QWidget *parent = nullptr);
    ~NeroVirtualDriveDialog();

private slots:
    void on_addDirBtn_clicked();

    void letterBox_activated(const int &);

    void changeBtn_clicked();

    void deleteBtn_clicked();

    void winLabel_textEdited(const QString);

private:
    Ui::NeroVirtualDriveDialog *ui;

    void RenderList();

    void AddDir(const QString, const QString);

    void UpdateUsedLetters();

    void SetComboBoxItemEnabled(QComboBox * comboBox, const int index, const bool enabled) {
        auto * model = qobject_cast<QStandardItemModel*>(comboBox->model());
        auto * item = model->item(index);
        item->setEnabled(enabled);
    }

    QDir prefixDir;
    QDir externalDir;

    QStringList usedLetters;

    const QStringList letters = {
        "A:",
        "B:",
        "C:",
        "D:",
        "E:",
        "F:",
        "G:",
        "H:",
        "I:",
        "J:",
        "K:",
        "L:",
        "M:",
        "N:",
        "O:",
        "P:",
        "Q:",
        "R:",
        "S:",
        "T:",
        "U:",
        "V:",
        "W:",
        "X:",
        "Y:",
        "Z:"
    };

    QList<QComboBox*> dirLetter;
    QList<QLabel*> dirPath;
    QList<QLineEdit*> dirWinLabel;
    QList<QPushButton*> dirChange;
    QList<QPushButton*> dirDelete;
    QList<QHBoxLayout*> dirLetterPathLayout;
    QList<QVBoxLayout*> dirPathAndLabelsLayout;
    QList<QHBoxLayout*> dirWholeLine;

    QFont letterFont;
    QFont lineEditFont;
    QFont mainLabelFont;
};

#endif // NERODRIVES_H
