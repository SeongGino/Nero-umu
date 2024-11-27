/*  Nero Launcher: A very basic Bottles-like manager using UMU.
    GUI manager frontend.

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

#ifndef NEROMANAGER_H
#define NEROMANAGER_H

#include "neropreferences.h"
#include "neroprefixsettings.h"
#include "nerorunner.h"
#include "nerorunnerdialog.h"
#include "neroshortcut.h"
#include "nerotricks.h"
#include "nerowizard.h"

#include <QMainWindow>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QTimer>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class NeroManagerWindow;
}
QT_END_NAMESPACE

// may or may not be copy/paste Qt doc example here
class NeroThreadWorker : public QObject
{
    Q_OBJECT

public:
    NeroThreadWorker(const int &slot, const QString &params, const QStringList &extArgs = {}, const bool &prefixAlreadyRunning = false) {
        currentSlot = slot, currentParameters = params, oneTimeArgs = extArgs, alreadyRunning = prefixAlreadyRunning;
    }
    ~NeroThreadWorker() {};
    NeroRunner Runner;
public slots:
    void umuRunnerProcess();
signals:
    void umuExited(const int &, const int &);
private:
    bool alreadyRunning = false;
    int currentSlot;
    QString currentParameters;
    QStringList oneTimeArgs;
};

class NeroThreadController : public QObject
{
    Q_OBJECT
    QThread umuThread;
public:
    NeroThreadController(const int &slot, const QString &params, const QStringList &extArgs = {}, const bool &prefixAlreadyRunning = false) {
        umuWorker = new NeroThreadWorker(slot, params, extArgs, prefixAlreadyRunning);
        umuWorker->moveToThread(&umuThread);
        connect(&umuThread, &QThread::finished, umuWorker, &QObject::deleteLater);
        connect(this, &NeroThreadController::operate, umuWorker, &NeroThreadWorker::umuRunnerProcess);
        connect(umuWorker, &NeroThreadWorker::umuExited, this, &NeroThreadController::handleUmuResults);
        umuThread.start();
    }
    ~NeroThreadController() {
        umuThread.quit();
        umuThread.wait();
    }
    NeroThreadWorker *umuWorker;
    void Stop() { umuWorker->Runner.halt = true; }
signals:
    void operate();
    void passUmuResults(const int &, const int &);
public slots:
    void handleUmuResults(const int &buttonSlot, const int &result) { emit passUmuResults(buttonSlot, result); }
};

class NeroManagerWindow : public QMainWindow
{
    Q_OBJECT

public:
    NeroManagerWindow(QWidget *parent = nullptr);
    ~NeroManagerWindow();

public slots:
    void handleUmuResults(const int &, const int &);
    void handleUmuSignal(const int &);

private slots:
    void prefixMainButtons_clicked();
    void prefixDeleteButtons_clicked();
    void prefixShortcutPlayButtons_clicked();
    void prefixShortcutEditButtons_clicked();
    void blinkTimer_timeout();
    void prefixSettings_result();

    void on_addButton_clicked();

    void on_backButton_clicked();

    void on_prefixTricksBtn_clicked();

    void on_prefixSettingsBtn_clicked();

    void on_actionAbout_Nero_triggered();

    void on_oneTimeRunBtn_clicked();

private:
    Ui::NeroManagerWindow *ui;
    NeroPrefixSettingsWindow *prefixSettings = nullptr;
    NeroRunnerDialog *runnerWindow = nullptr;

    // METHODS
    void SetHeader(const QString prefix = "", const unsigned int shortcutsCount = 0);
    void RenderPrefixes();
    void RenderPrefixList();
    void CreatePrefix(const QString newPrefix, const QString runner, QStringList tricksToInstall = {});
    void AddTricks(QStringList verbs, const QString prefix);
    void RenderShortcuts();
    void CleanupShortcuts();
    void StartBlinkTimer();
    void StopBlinkTimer();

    // VARS
    unsigned int LOLRANDOM;

    QList<QPushButton*> prefixMainButton;
    QList<QPushButton*> prefixDeleteButton;

    QList<QLabel*> prefixShortcutLabel;
    QList<QIcon*> prefixShortcutIco;
    QList<QLabel*> prefixShortcutIcon;
    QList<QPushButton*> prefixShortcutPlayButton;
    QList<QPushButton*> prefixShortcutEditButton;
    QList<QSpacerItem*> prefixShortcutSpacer;

    QFont listFont;

    QTimer *blinkTimer;
    int blinkingState = 1;

    bool prefixIsSelected = false;

    QList<int> currentlyRunning;
    int threadsCount = 0;

    QList<NeroThreadController*> umuController;
};

#endif // NEROMANAGER_H
