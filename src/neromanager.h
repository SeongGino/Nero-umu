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
#include "nerotricks.h"
#include "nerowizard.h"

#include <QMainWindow>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QTimer>
#include <QThread>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QMenu>

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
    NeroThreadWorker(const int &slot, const QString &params, const bool &prefixAlreadyRunning = false, const QStringList &extArgs = {}) {
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
    NeroThreadController(const int &slot, const QString &params, const bool &prefixAlreadyRunning = false, const QStringList &extArgs = {}) {
        umuWorker = new NeroThreadWorker(slot, params, prefixAlreadyRunning, extArgs);
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

// needed to prevent sysTray from holding up close events.
protected:
    void closeEvent(QCloseEvent *event) { delete sysTray; }

public slots:
    void handleUmuResults(const int &, const int &);
    void handleUmuSignal(const int &);

private slots:
    void prefixDefaultButtons_clicked();
    void prefixMainButtons_clicked();
    void prefixDeleteButtons_clicked();
    void prefixShortcutPlayButtons_clicked();
    void prefixShortcutEditButtons_clicked();
    void blinkTimer_timeout();
    void tricksWindow_result();
    void prefixWizard_result();
    void prefixSettings_result();
    void actionExit_activated();

    void sysTray_activated(QSystemTrayIcon::ActivationReason reason);

    void sysTray_messageClicked();

    void on_addButton_clicked();

    void on_backButton_clicked();

    void on_prefixTricksBtn_clicked();

    void on_prefixSettingsBtn_clicked();

    void on_oneTimeRunBtn_clicked();

    void on_managerSettings_clicked();

    void on_aboutBtn_clicked();

private:
    Ui::NeroManagerWindow *ui;
    NeroManagerPreferences *prefs = nullptr;
    NeroPrefixSettingsWindow *prefixSettings = nullptr;
    NeroRunnerDialog *runnerWindow = nullptr;
    NeroPrefixWizard *wizard = nullptr;
    NeroTricksWindow *tricks = nullptr;

    // METHODS
    void SetHeader(const QString prefix = "", const unsigned int shortcutsCount = 0);
    void CheckWinetricks();
    void RenderPrefixes();
    void RenderPrefixList();
    void CreatePrefix(const QString &, const QString &, QStringList tricksToInstall = {});
    void RenderShortcuts();
    void CleanupShortcuts();
    void StartBlinkTimer();
    void StopBlinkTimer();

    // VARS & OBJECTS
    unsigned int LOLRANDOM;

    // General manager stuff
    QSystemTrayIcon *sysTray;
    QMenu sysTrayMenu;
    QAction sysTrayActions[1] = { QAction("Exit Nero") };
    QSettings *managerCfg;
    QTimer *blinkTimer;
    int blinkingState = 1;
    bool prefixIsSelected = false;
    bool runnerPrefixIsDefault = false;
    QString oneTimeLastPath;

    // umu threads
    QList<NeroThreadController*> umuController;
    QList<int> currentlyRunning;
    int threadsCount = 0;
    QStringList oneOffsRunning;

    // Prefixes list assets
    QList<QPushButton*> prefixDefaultButton;
    QList<QPushButton*> prefixMainButton;
    QList<QPushButton*> prefixDeleteButton;

    // Prefix Shortcuts list assets
    QList<QLabel*> prefixShortcutLabel;
    QList<QIcon*> prefixShortcutIco;
    QList<QLabel*> prefixShortcutIcon;
    QList<QPushButton*> prefixShortcutPlayButton;
    QList<QPushButton*> prefixShortcutEditButton;
    QList<QSpacerItem*> prefixShortcutSpacer;

    QFont listFont;
};

#endif // NEROMANAGER_H
