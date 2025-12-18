#pragma once

#include <QFrame>
#include <QDir>


namespace Ui {
class VirtualDriveFrame;
}

class VirtualDriveFrame : public QFrame
{
    Q_OBJECT

public:
    explicit VirtualDriveFrame(
        const QDir prefix,
        const QString letter,
        const QString path,
        QWidget *parent = nullptr
    );
    ~VirtualDriveFrame();

signals:
    void letterChanged(
        const QString previous, const QString current
    );
    void driveDeleted(
        const QString letter
    );

public slots:
    void update(void);

private slots:
    void onDirLetterCurrentIndexChanged(int);
    void onDirWinLaberEdited(const QString);
    void onDirChangeClicked(bool);
    void onDirDeleteClicked(bool);

private:
    Ui::VirtualDriveFrame *ui;
    QDir m_prefix;
    QString m_letter;

    const QStringList m_letters = {
        "a:",
        "b:",
        "c:",
        "d:",
        "e:",
        "f:",
        "g:",
        "h:",
        "i:",
        "j:",
        "k:",
        "l:",
        "m:",
        "n:",
        "o:",
        "p:",
        "q:",
        "r:",
        "s:",
        "t:",
        "u:",
        "v:",
        "w:",
        "x:",
        "y:",
        "z:"
    };

};
