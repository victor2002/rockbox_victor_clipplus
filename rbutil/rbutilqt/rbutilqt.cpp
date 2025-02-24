/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>

#include "version.h"
#include "rbutilqt.h"
#include "ui_rbutilqtfrm.h"
#include "ui_aboutbox.h"
#include "configure.h"
#include "installwindow.h"
#include "installtalkwindow.h"
#include "createvoicewindow.h"
#include "httpget.h"
#include "themesinstallwindow.h"
#include "uninstallwindow.h"
#include "utils.h"
#include "rockboxinfo.h"
#include "sysinfo.h"
#include "system.h"
#include "systrace.h"
#include "rbsettings.h"
#include "serverinfo.h"
#include "systeminfo.h"
#include "ziputil.h"

#include "progressloggerinterface.h"

#include "bootloaderinstallbase.h"
#include "bootloaderinstallmpio.h"

#if defined(Q_OS_LINUX)
#include <stdio.h>
#endif
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#endif

QList<QTranslator*> RbUtilQt::translators;

RbUtilQt::RbUtilQt(QWidget *parent) : QMainWindow(parent)
{
    // startup log
    qDebug() << "======================================";
    qDebug() << "[System] Rockbox Utility " VERSION;
    qDebug() << "[System] Qt version:" << qVersion();
    qDebug() << "======================================";

    absolutePath = qApp->applicationDirPath();

    HttpGet::setGlobalUserAgent("rbutil/"VERSION);
    HttpGet::setGlobalProxy(proxy());
    // init startup & autodetection
    ui.setupUi(this);
#if defined(Q_OS_LINUX)
    QIcon windowIcon(":/icons/rockbox-clef.svg");
    this->setWindowIcon(windowIcon);
#endif
#if defined(Q_OS_WIN32)
    QIcon windowIcon = QIcon();
    windowIcon.addFile(":/icons/rockbox-16.png");
    windowIcon.addFile(":/icons/rockbox-32.png");
    windowIcon.addFile(":/icons/rockbox-48.png");
    windowIcon.addFile(":/icons/rockbox-64.png");
    windowIcon.addFile(":/icons/rockbox-128.png");
    windowIcon.addFile(":/icons/rockbox-256.png");
    this->setWindowIcon(windowIcon);
#endif
#if defined(Q_OS_MACX)
    // don't translate menu entries that are handled specially on OS X
    // (Configure, Quit). Qt handles them for us if they use english string.
    ui.action_Configure->setText("Configure");
    ui.actionE_xit->setText("Quit");
#endif
#if defined(Q_OS_WIN32)
    long ret;
    HKEY hk;
    ret = RegOpenKeyEx(HKEY_CURRENT_USER, _TEXT("Software\\Wine"),
        0, KEY_QUERY_VALUE, &hk);
    if(ret == ERROR_SUCCESS) {
        QMessageBox::warning(this, tr("Wine detected!"),
                tr("It seems you are trying to run this program under Wine. "
                    "Please don't do this, running under Wine will fail. "
                    "Use the native Linux binary instead."),
                QMessageBox::Ok, QMessageBox::Ok);
        qDebug() << "[RbUtil] WINE DETECTED!";
        RegCloseKey(hk);
    }
#endif
    downloadInfo();

    m_gotInfo = false;
    m_auto = false;

    // manual tab
    ui.radioPdf->setChecked(true);

    // info tab
    ui.treeInfo->setAlternatingRowColors(true);
    ui.treeInfo->setHeaderLabels(QStringList() << tr("File") << tr("Version"));
    ui.treeInfo->expandAll();
    ui.treeInfo->setColumnCount(2);
    ui.treeInfo->setLayoutDirection(Qt::LeftToRight);
    // disable quick install until version info is available
    ui.buttonSmall->setEnabled(false);
    ui.buttonComplete->setEnabled(false);
    ui.actionSmall_Installation->setEnabled(false);
    ui.actionComplete_Installation->setEnabled(false);

    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateTabs(int)));
    connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui.action_About, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.action_Help, SIGNAL(triggered()), this, SLOT(help()));
    connect(ui.action_Configure, SIGNAL(triggered()), this, SLOT(configDialog()));
    connect(ui.actionE_xit, SIGNAL(triggered()), this, SLOT(shutdown()));
    connect(ui.buttonChangeDevice, SIGNAL(clicked()), this, SLOT(configDialog()));
    connect(ui.buttonRockbox, SIGNAL(clicked()), this, SLOT(installBtn()));
    connect(ui.buttonBootloader, SIGNAL(clicked()), this, SLOT(installBootloaderBtn()));
    connect(ui.buttonFonts, SIGNAL(clicked()), this, SLOT(installFontsBtn()));
    connect(ui.buttonGames, SIGNAL(clicked()), this, SLOT(installDoomBtn()));
    connect(ui.buttonTalk, SIGNAL(clicked()), this, SLOT(createTalkFiles()));
    connect(ui.buttonCreateVoice, SIGNAL(clicked()), this, SLOT(createVoiceFile()));
    connect(ui.buttonVoice, SIGNAL(clicked()), this, SLOT(installVoice()));
    connect(ui.buttonThemes, SIGNAL(clicked()), this, SLOT(installThemes()));
    connect(ui.buttonRemoveRockbox, SIGNAL(clicked()), this, SLOT(uninstall()));
    connect(ui.buttonRemoveBootloader, SIGNAL(clicked()), this, SLOT(uninstallBootloader()));
    connect(ui.buttonDownloadManual, SIGNAL(clicked()), this, SLOT(downloadManual()));
    connect(ui.buttonSmall, SIGNAL(clicked()), this, SLOT(smallInstall()));
    connect(ui.buttonComplete, SIGNAL(clicked()), this, SLOT(completeInstall()));

    // actions accessible from the menu
    connect(ui.actionComplete_Installation, SIGNAL(triggered()), this, SLOT(completeInstall()));
    connect(ui.actionSmall_Installation, SIGNAL(triggered()), this, SLOT(smallInstall()));
    connect(ui.actionInstall_Bootloader, SIGNAL(triggered()), this, SLOT(installBootloaderBtn()));
    connect(ui.actionInstall_Rockbox, SIGNAL(triggered()), this, SLOT(installBtn()));
    connect(ui.actionFonts_Package, SIGNAL(triggered()), this, SLOT(installFontsBtn()));
    connect(ui.actionInstall_Themes, SIGNAL(triggered()), this, SLOT(installThemes()));
    connect(ui.actionInstall_Game_Files, SIGNAL(triggered()), this, SLOT(installDoomBtn()));
    connect(ui.actionInstall_Voice_File, SIGNAL(triggered()), this, SLOT(installVoice()));
    connect(ui.actionCreate_Voice_File, SIGNAL(triggered()), this, SLOT(createVoiceFile()));
    connect(ui.actionCreate_Talk_Files, SIGNAL(triggered()), this, SLOT(createTalkFiles()));
    connect(ui.actionRemove_bootloader, SIGNAL(triggered()), this, SLOT(uninstallBootloader()));
    connect(ui.actionUninstall_Rockbox, SIGNAL(triggered()), this, SLOT(uninstall()));
    connect(ui.action_System_Info, SIGNAL(triggered()), this, SLOT(sysinfo()));
    connect(ui.action_Trace, SIGNAL(triggered()), this, SLOT(trace()));

#if !defined(STATIC)
    ui.actionInstall_Rockbox_Utility_on_player->setEnabled(false);
#else
    connect(ui.actionInstall_Rockbox_Utility_on_player, SIGNAL(triggered()), this, SLOT(installPortable()));
#endif

}


void RbUtilQt::shutdown(void)
{
    // restore default message handler to prevent trace accesses during
    // object destruction -- the trace object could already be destroyed.
    // Fixes segfaults on exit.
    qInstallMsgHandler(0);
    SysTrace::save();
    this->close();
}


void RbUtilQt::trace(void)
{
    SysTrace wnd(this);
    wnd.exec();
}

void RbUtilQt::sysinfo(void)
{
    Sysinfo info(this);
    info.exec();
}

void RbUtilQt::updateTabs(int count)
{
    switch(count) {
        case 6:
            updateInfo();
            break;
        default:
            break;
    }
}


void RbUtilQt::downloadInfo()
{
    // try to get the current build information
    daily = new HttpGet(this);
    connect(daily, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), daily, SLOT(abort()));
    if(RbSettings::value(RbSettings::CacheOffline).toBool())
        daily->setCache(true);
    else
        daily->setCache(false);
    ui.statusbar->showMessage(tr("Downloading build information, please wait ..."));
    qDebug() << "[RbUtil] downloading build info";
    daily->setFile(&buildInfo);
    daily->getFile(QUrl(SystemInfo::value(SystemInfo::ServerConfUrl).toString()));
}


void RbUtilQt::downloadDone(bool error)
{
    if(error) {
        qDebug() << "[RbUtil] network error:" << daily->error();
        ui.statusbar->showMessage(tr("Can't get version information!"));
        QMessageBox::critical(this, tr("Network error"),
                tr("Can't get version information.\n"
                   "Network error: %1. Please check your network and proxy settings.")
                    .arg(daily->errorString()));
        return;
    }
    qDebug() << "[RbUtil] network status:" << daily->error();

    // read info into ServerInfo object
    buildInfo.open();
    ServerInfo::readBuildInfo(buildInfo.fileName());
    buildInfo.close();
    
    // start bleeding info download
    bleeding = new HttpGet(this);
    connect(bleeding, SIGNAL(done(bool)), this, SLOT(downloadBleedingDone(bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), bleeding, SLOT(abort()));
    if(RbSettings::value(RbSettings::CacheOffline).toBool())
        bleeding->setCache(true);
    bleeding->setFile(&bleedingInfo);
    bleeding->getFile(QUrl(SystemInfo::value(SystemInfo::BleedingInfo).toString()));
    ui.statusbar->showMessage(tr("Downloading build information, please wait ..."));

}


void RbUtilQt::downloadBleedingDone(bool error)
{
    if(error) {
        qDebug() << "[RbUtil] network error:" << bleeding->error();
        ui.statusbar->showMessage(tr("Can't get version information!"));
        QMessageBox::critical(this, tr("Network error"),
                tr("Can't get version information.\n"
                   "Network error: %1. Please check your network and proxy settings.")
                    .arg(bleeding->errorString()));
        return;
    }
    else {
        bleedingInfo.open();
        ServerInfo::readBleedingInfo(bleedingInfo.fileName());
        bleedingInfo.close();
      
        ui.statusbar->showMessage(tr("Download build information finished."), 5000);
        updateSettings();
        m_gotInfo = true;
        
        //start check for updates
        checkUpdate();
    }
}


void RbUtilQt::about()
{
    QDialog *window = new QDialog(this);
    Ui::aboutBox about;
    about.setupUi(window);
    window->setLayoutDirection(Qt::LeftToRight);
    window->setModal(true);

    QFile licence(":/docs/gpl-2.0.html");
    licence.open(QIODevice::ReadOnly);
    QTextStream c(&licence);
    about.browserLicense->insertHtml(c.readAll());
    about.browserLicense->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

    QFile speexlicense(":/docs/COPYING.SPEEX");
    speexlicense.open(QIODevice::ReadOnly);
    QTextStream s(&speexlicense);
    about.browserSpeexLicense->insertHtml("<pre>" + s.readAll() + "</pre>");
    about.browserSpeexLicense->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

    QFile credits(":/docs/CREDITS");
    credits.open(QIODevice::ReadOnly);
    QTextStream r(&credits);
    r.setCodec(QTextCodec::codecForName("UTF-8"));
    while(!r.atEnd()) {
        QString line = r.readLine();
        // filter out header.
        line.remove(QRegExp("^ +.*"));
        line.remove(QRegExp("^People.*"));
        about.browserCredits->append(line);
    }
    about.browserCredits->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    QString title = QString("<b>The Rockbox Utility</b><br/>Version %1").arg(FULLVERSION);
    about.labelTitle->setText(title);

    window->show();

}


void RbUtilQt::help()
{
    QUrl helpurl("http://www.rockbox.org/wiki/RockboxUtility");
    QDesktopServices::openUrl(helpurl);
}


void RbUtilQt::configDialog()
{
    Config *cw = new Config(this);
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    cw->show();
}


void RbUtilQt::updateSettings()
{
    qDebug() << "[RbUtil] updating current settings";
    updateDevice();
    updateManual();
    HttpGet::setGlobalProxy(proxy());
    HttpGet::setGlobalCache(RbSettings::value(RbSettings::CachePath).toString());
    HttpGet::setGlobalDumbCache(RbSettings::value(RbSettings::CacheOffline).toBool());
    
     if(RbSettings::value(RbSettings::RbutilVersion) != PUREVERSION) {
        QApplication::processEvents();
        QMessageBox::information(this, tr("New installation"),
            tr("This is a new installation of Rockbox Utility, or a new version. "
                "The configuration dialog will now open to allow you to setup the program, "
                " or review your settings."));
        configDialog();
    }
    else if(chkConfig(false)) {
        QApplication::processEvents();
        QMessageBox::critical(this, tr("Configuration error"),
            tr("Your configuration is invalid. This is most likely due "
                "to a changed device path. The configuration dialog will "
                "now open to allow you to correct the problem."));
        configDialog();
    }    
}


void RbUtilQt::updateDevice()
{
    /* TODO: We should check the flags of the bootloaderinstall classes, and not
     * just check if its != none or != "fwpatcher" */

    /* Enable bootloader installation, if possible */
    bool bootloaderInstallable =
        SystemInfo::value(SystemInfo::CurBootloaderMethod) != "none";
    ui.buttonBootloader->setEnabled(bootloaderInstallable);
    ui.labelBootloader->setEnabled(bootloaderInstallable);
    ui.actionInstall_Bootloader->setEnabled(bootloaderInstallable);

    /* Enable bootloader uninstallation, if possible */
    bool bootloaderUninstallable = bootloaderInstallable &&
        SystemInfo::value(SystemInfo::CurBootloaderMethod) != "fwpatcher";
    ui.labelRemoveBootloader->setEnabled(bootloaderUninstallable);
    ui.buttonRemoveBootloader->setEnabled(bootloaderUninstallable);
    ui.actionRemove_bootloader->setEnabled(bootloaderUninstallable);

    /* Disable the whole tab widget if configuration is invalid */
    bool configurationValid = !chkConfig(false);
    ui.tabWidget->setEnabled(configurationValid);
    ui.menuA_ctions->setEnabled(configurationValid);

    // displayed device info
    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    QString brand = SystemInfo::value(SystemInfo::CurBrand).toString();
    QString name = SystemInfo::value(SystemInfo::CurName).toString() +
        " (" + ServerInfo::value(ServerInfo::CurStatus).toString() + ")";
    QString mountdisplay = QDir::toNativeSeparators(mountpoint);
    QString label = Utils::filesystemName(mountpoint);
    if(!label.isEmpty())
        mountdisplay += " (" + label + ")";

    if(name.isEmpty()) name = "&lt;none&gt;";
    if(mountpoint.isEmpty())
        mountpoint = "&lt;invalid&gt;";
    ui.labelDevice->setText(tr("<b>%1 %2</b> at <b>%3</b>")
            .arg(brand, name, mountdisplay));

    // hide quickstart buttons if no release available        
    bool installable = !ServerInfo::value(ServerInfo::CurReleaseVersion).toString().isEmpty();
    ui.buttonSmall->setEnabled(installable);
    ui.buttonComplete->setEnabled(installable);
    ui.actionSmall_Installation->setEnabled(installable);
    ui.actionComplete_Installation->setEnabled(installable);
}


void RbUtilQt::updateManual()
{
    if(RbSettings::value(RbSettings::Platform) != "")
    {
        QString manual = SystemInfo::value(SystemInfo::CurManual).toString();
        QString buildservermodel = SystemInfo::value(SystemInfo::CurBuildserverModel).toString();
        QString pdfmanual = SystemInfo::value(SystemInfo::ManualUrl).toString();
        QString htmlmanual = pdfmanual;

        pdfmanual.replace("%EXTENSION%", "pdf");
        htmlmanual.replace("%EXTENSION%", "html");
        if(manual.isEmpty()) {
            pdfmanual.replace("%MANUALBASENAME%", "rockbox-" + buildservermodel);
            htmlmanual.replace("%MANUALBASENAME%", "rockbox-" + buildservermodel + "/rockbox-build");
        }
        else {
            pdfmanual.replace("%MANUALBASENAME%", "rockbox-" + manual);
            htmlmanual.replace("%MANUALBASENAME%", "rockbox-" + manual + "/rockbox-build");
        }

        ui.labelPdfManual->setText(tr("<a href='%1'>PDF Manual</a>")
            .arg(pdfmanual));
        ui.labelHtmlManual->setText(tr("<a href='%1'>HTML Manual (opens in browser)</a>")
            .arg(htmlmanual));
    }
    else {
        ui.labelPdfManual->setText(tr("Select a device for a link to the correct manual"));
        ui.labelHtmlManual->setText(tr("<a href='%1'>Manual Overview</a>")
            .arg("http://www.rockbox.org/manual.shtml"));

    }
}

void RbUtilQt::completeInstall()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to perform a complete installation?\n\n"
              "This will install Rockbox %1. To install the most recent "
              "development build available press \"Cancel\" and "
              "use the \"Installation\" tab.")
              .arg(ServerInfo::value(ServerInfo::CurReleaseVersion).toString()),
              QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
        return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    if(smallInstallInner())
        return;
    logger->setRunning();
    // Fonts
    m_error = false;
    m_installed = false;
    if(!installFontsAuto())
        return;
    else
    {
        // wait for installation finished
        while(!m_installed)
           QApplication::processEvents();
    }
    if(m_error) return;
    logger->setRunning();

    // Doom
    if(hasDoom())
    {
        m_error = false;
        m_installed = false;
        if(!installDoomAuto())
            return;
        else
        {
            // wait for installation finished
            while(!m_installed)
               QApplication::processEvents();
        }
        if(m_error) return;
    }

    // theme
    // this is a window
    // it has its own logger window,so close our.
    logger->close();
    installThemes();

}

void RbUtilQt::smallInstall()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to perform a minimal installation? "
              "A minimal installation will contain only the absolutely "
              "necessary parts to run Rockbox.\n\n"
              "This will install Rockbox %1. To install the most recent "
              "development build available press \"Cancel\" and "
              "use the \"Installation\" tab.")
              .arg(ServerInfo::value(ServerInfo::CurReleaseVersion).toString()),
              QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
        return;

    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    smallInstallInner();
}

bool RbUtilQt::smallInstallInner()
{
    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountpoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->setFinished();
        return true;
    }
    // Bootloader
    if(SystemInfo::value(SystemInfo::CurBootloaderMethod) != "none")
    {
        m_error = false;
        m_installed = false;
        m_auto = true;
        if(!installBootloaderAuto()) {
            logger->setFinished();
            return true;
        }
        else
        {
            // wait for boot loader installation finished
            while(!m_installed)
                QApplication::processEvents();
        }
        m_auto = false;
        if(m_error) return true;
        logger->setRunning();
    }

    // Rockbox
    m_error = false;
    m_installed = false;
    if(!installAuto())
        return true;
    else
    {
       // wait for installation finished
       while(!m_installed)
          QApplication::processEvents();
    }

    installBootloaderPost(false);
    return false;
}

void RbUtilQt::installdone(bool error)
{
    qDebug() << "[RbUtil] install done";
    m_installed = true;
    m_error = error;
}

void RbUtilQt::installBtn()
{
    if(chkConfig(true)) return;
    install();
}

bool RbUtilQt::installAuto()
{
    QString file = SystemInfo::value(SystemInfo::ReleaseUrl).toString();
    file.replace("%MODEL%", SystemInfo::value(SystemInfo::CurBuildserverModel).toString());
    file.replace("%RELVERSION%", ServerInfo::value(ServerInfo::CurReleaseVersion).toString());

    // check installed Version and Target
    QString warning = Utils::checkEnvironment(false);
    if(!warning.isEmpty())
    {
        if(QMessageBox::warning(this, tr("Really continue?"), warning,
            QMessageBox::Ok | QMessageBox::Abort, QMessageBox::Abort)
                == QMessageBox::Abort)
        {
            logger->addItem(tr("Aborted!"), LOGERROR);
            logger->setFinished();
            return false;
        }
    }

    // check version
    RockboxInfo rbinfo(RbSettings::value(RbSettings::Mountpoint).toString());
    if(rbinfo.version() != "")
    {
        if(QMessageBox::question(this, tr("Installed Rockbox detected"),
           tr("Rockbox installation detected. Do you want to backup first?"),
           QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            bool result;
            logger->addItem(tr("Starting backup..."),LOGINFO);
            QString backupName = RbSettings::value(RbSettings::Mountpoint).toString()
                + "/.backup/rockbox-backup-" + rbinfo.version() + ".zip";

            //! create dir, if it doesnt exist
            QFileInfo backupFile(backupName);
            if(!QDir(backupFile.path()).exists())
            {
                QDir a;
                a.mkpath(backupFile.path());
            }

            logger->addItem(tr("Beginning Backup..."),LOGINFO);
            QCoreApplication::processEvents();

            //! create backup
            ZipUtil zip(this);
            connect(&zip, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
            connect(&zip, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
            zip.open(backupName, QuaZip::mdCreate);
            QString mp = RbSettings::value(RbSettings::Mountpoint).toString();
            QString folder = mp + "/.rockbox";
            result = zip.appendDirToArchive(folder, mp);
            zip.close();
            if(result)
            {
                logger->addItem(tr("Backup successful"),LOGOK);
            }
            else
            {
                logger->addItem(tr("Backup failed!"),LOGERROR);
                logger->setFinished();
                return false;
            }
        }
    }

    //! install current build
    ZipInstaller* installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setLogSection("Rockbox (Base)");
    installer->setLogVersion(ServerInfo::value(ServerInfo::CurReleaseVersion).toString());
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);
    installer->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());

    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();
    return true;
}


void RbUtilQt::install()
{
    InstallWindow *installWindow = new InstallWindow(this);
    installWindow->show();
}

bool RbUtilQt::installBootloaderAuto()
{
    installBootloader();
    return !m_error;
}

void RbUtilQt::installBootloaderBtn()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to install the Bootloader?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    installBootloader();
}

void RbUtilQt::installBootloader()
{
    QString platform = RbSettings::value(RbSettings::Platform).toString();
    QString backupDestination = "";
    m_error = false;

    // create installer
    BootloaderInstallBase *bl = BootloaderInstallBase::createBootloaderInstaller(this,
                                    SystemInfo::value(SystemInfo::CurBootloaderMethod).toString());
    if(bl == NULL) {
        logger->addItem(tr("No install method known."), LOGERROR);
        logger->setFinished();
        return;
    }
   
    // set bootloader filename. Do this now as installed() needs it.
    QStringList blfile = SystemInfo::value(SystemInfo::CurBootloaderFile).toStringList();
    QStringList blfilepath;
    for(int a = 0; a < blfile.size(); a++) {
        blfilepath.append(RbSettings::value(RbSettings::Mountpoint).toString()
                + blfile.at(a));
    }
    bl->setBlFile(blfilepath);
    QUrl url(SystemInfo::value(SystemInfo::BootloaderUrl).toString()
            + SystemInfo::value(SystemInfo::CurBootloaderName).toString());
    bl->setBlUrl(url);
    bl->setLogfile(RbSettings::value(RbSettings::Mountpoint).toString()
                    + "/.rockbox/rbutil.log");

    if(bl->installed() == BootloaderInstallBase::BootloaderRockbox) {
        if(QMessageBox::question(this, tr("Bootloader detected"),
            tr("Bootloader already installed. Do you want to reinstall the bootloader?"),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                if(m_auto) {
                    // keep logger open for auto installs.
                    // don't consider abort as error in auto-mode.
                    logger->addItem(tr("Bootloader installation skipped"), LOGINFO);
                    installBootloaderPost(false);
                }
                else {
                    logger->close();
                    installBootloaderPost(true);
                }
                delete bl;
                return;
        }
    }
    else if(bl->installed() == BootloaderInstallBase::BootloaderOther
        && bl->capabilities() & BootloaderInstallBase::Backup)
    {
        QString targetFolder = SystemInfo::value(SystemInfo::CurPlatformName).toString()
                                + " Firmware Backup";
        // remove invalid character(s)
        targetFolder.remove(QRegExp("[:/]"));
        if(QMessageBox::question(this, tr("Create Bootloader backup"),
            tr("You can create a backup of the original bootloader "
               "file. Press \"Yes\" to select an output folder on your "
               "computer to save the file to. The file will get placed "
               "in a new folder \"%1\" created below the selected folder.\n"
               "Press \"No\" to skip this step.").arg(targetFolder),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            backupDestination = QFileDialog::getExistingDirectory(this,
                    tr("Browse backup folder"), QDir::homePath());
            if(!backupDestination.isEmpty())
                backupDestination += "/" + targetFolder;

            qDebug() << "[RbUtil] backing up to" << backupDestination;
            // backup needs to be done after the logger has been set up.
        }
    }

    if(bl->capabilities() & BootloaderInstallBase::NeedsOf)
    {
        int ret;
        ret = QMessageBox::information(this, tr("Prerequisites"),
            bl->ofHint(),QMessageBox::Ok | QMessageBox::Abort);
        if(ret != QMessageBox::Ok) {
            // consider aborting an error to close window / abort automatic
            // installation.
            m_error = true;
            logger->addItem(tr("Bootloader installation aborted"), LOGINFO);
            logger->setFinished();
            return;
        }
        // open dialog to browse to of file
        QString offile;
        offile = QFileDialog::getOpenFileName(this,
                tr("Select firmware file"), QDir::homePath());
        if(!QFileInfo(offile).isReadable()) {
            logger->addItem(tr("Error opening firmware file"), LOGERROR);
            logger->setFinished();
            m_error = true;
            return;
        }
        bl->setOfFile(offile);
    }

    // the bootloader install class does NOT use any GUI stuff.
    // All messages are passed via signals.
    connect(bl, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(bl, SIGNAL(done(bool)), this, SLOT(installBootloaderPost(bool)));
    connect(bl, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(bl, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));

    // start install.
    if(!backupDestination.isEmpty()) {
        if(!bl->backup(backupDestination)) {
            if(QMessageBox::warning(this, tr("Backup error"),
                    tr("Could not create backup file. Continue?"),
                    QMessageBox::No | QMessageBox::Yes)
                == QMessageBox::No) {
                logger->setFinished();
                return;
            }
        }
    }
    bl->install();
}

void RbUtilQt::installBootloaderPost(bool error)
{
    qDebug() << "[RbUtil] Bootloader Post-Installation, error state:" << error;
    // if an error occured don't perform post install steps.
    if(error) {
        m_error = true;
        return;
    }
    else
        m_error = false;

    m_installed = true;
    // end here if automated install
    if(m_auto)
        return;

    QString msg = BootloaderInstallBase::postinstallHints(
                    RbSettings::value(RbSettings::Platform).toString());
    if(!msg.isEmpty()) {
        QMessageBox::information(this, tr("Manual steps required"), msg);
        logger->close();
    }

}

void RbUtilQt::installFontsBtn()
{
    if(chkConfig(true)) return;
    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    RockboxInfo installInfo(mountpoint);
    if(installInfo.revision().isEmpty() && installInfo.release().isEmpty()) {
        QMessageBox::critical(this, tr("No Rockbox installation found"),
                tr("Could not determine the installed Rockbox version. "
                    "Please install a Rockbox build before installing "
                    "fonts."));
        return;
    }
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to install the fonts package?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    installFonts();
}

bool RbUtilQt::installFontsAuto()
{
    installFonts();

    return !m_error;
}

void RbUtilQt::installFonts()
{
    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    RockboxInfo installInfo(mountpoint);
    QString fontsurl;
    QString logversion;
    QString relversion = installInfo.release();
    if(relversion.isEmpty()) {
        // release is empty for non-release versions (i.e. daily / current)
        fontsurl = SystemInfo::value(SystemInfo::DailyFontUrl).toString();
        logversion = installInfo.revision();
    }
    else {
        fontsurl = SystemInfo::value(SystemInfo::ReleaseFontUrl).toString();
        logversion = installInfo.release();
    }
    fontsurl.replace("%RELEASEVER%", relversion);

    // create zip installer
    installer = new ZipInstaller(this);
    installer->setUrl(fontsurl);
    installer->setLogSection("Fonts");
    installer->setLogVersion(logversion);
    installer->setMountPoint(mountpoint);
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);

    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();
}


void RbUtilQt::installVoice()
{
    if(chkConfig(true)) return;

    if(m_gotInfo == false)
    {
       QMessageBox::warning(this, tr("Warning"),
       tr("The Application is still downloading Information about new Builds."
          " Please try again shortly."));
        return;
    }

    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    RockboxInfo installInfo(mountpoint);

    QString voiceurl;
    QString logversion;
    QString relversion = installInfo.release();
    // if no version is found abort.
    if(installInfo.revision().isEmpty() && relversion.isEmpty()) {
        QMessageBox::critical(this, tr("No Rockbox installation found"),
                tr("Could not determine the installed Rockbox version. "
                    "Please install a Rockbox build before installing "
                    "voice files."));
        return;
    }
    if(relversion.isEmpty()) {
        // release is empty for non-release versions (i.e. daily / current)
        voiceurl = SystemInfo::value(SystemInfo::DailyVoiceUrl).toString();
        logversion = installInfo.revision();
    }
    else {
        voiceurl = SystemInfo::value(SystemInfo::ReleaseVoiceUrl).toString();
        logversion = installInfo.release();
    }
    if(QMessageBox::question(this, tr("Confirm Installation"),
       tr("Do you really want to install the voice file?"),
       QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QDate date = QDate::fromString(
            ServerInfo::value(ServerInfo::DailyDate).toString(), Qt::ISODate);
    QString model = SystemInfo::value(SystemInfo::CurBuildserverModel).toString();
    // replace placeholder in voice url
    voiceurl.replace("%DATE%", date.toString("yyyyMMdd"));
    voiceurl.replace("%MODEL%", model);
    voiceurl.replace("%RELVERSION%", relversion);

    qDebug() << "[RbUtil] voicefile URL:" << voiceurl;

    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    // create zip installer
    installer = new ZipInstaller(this);

    installer->setUrl(voiceurl);
    installer->setLogSection("Voice");
    installer->setLogVersion(logversion);
    installer->setMountPoint(mountpoint);
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);
    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();

}

void RbUtilQt::installDoomBtn()
{
    if(chkConfig(true)) return;
    if(!hasDoom()){
        QMessageBox::critical(this, tr("Error"),
            tr("Your device doesn't have a doom plugin. Aborting."));
        return;
    }

    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to install the game addon files?"),
           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    installDoom();
}
bool RbUtilQt::installDoomAuto()
{
    installDoom();
    return !m_error;
}

bool RbUtilQt::hasDoom()
{
    QFile doomrock(RbSettings::value(RbSettings::Mountpoint).toString()
                    +"/.rockbox/rocks/games/doom.rock");
    return doomrock.exists();
}

void RbUtilQt::installDoom()
{
    // create zip installer
    installer = new ZipInstaller(this);

    installer->setUrl(SystemInfo::value(SystemInfo::DoomUrl).toString());
    installer->setLogSection("Game Addons");
    installer->setLogVersion(ServerInfo::value(ServerInfo::DailyDate).toString());
    installer->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);
    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();

}

void RbUtilQt::installThemes()
{
    if(chkConfig(true)) return;
    ThemesInstallWindow* tw = new ThemesInstallWindow(this);
    tw->setModal(true);
    tw->show();
}

void RbUtilQt::createTalkFiles(void)
{
    if(chkConfig(true)) return;
    InstallTalkWindow *installWindow = new InstallTalkWindow(this);
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    installWindow->show();

}

void RbUtilQt::createVoiceFile(void)
{
    if(chkConfig(true)) return;
    CreateVoiceWindow *installWindow = new CreateVoiceWindow(this);
    
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    installWindow->show();
}

void RbUtilQt::uninstall(void)
{
    if(chkConfig(true)) return;
    UninstallWindow *uninstallWindow = new UninstallWindow(this);
    uninstallWindow->show();

}

void RbUtilQt::uninstallBootloader(void)
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Uninstallation"),
           tr("Do you really want to uninstall the Bootloader?"),
           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->setProgressVisible(false);
    logger->show();

    QString platform = RbSettings::value(RbSettings::Platform).toString();

    // create installer
    BootloaderInstallBase *bl = BootloaderInstallBase::createBootloaderInstaller(this,
                                    SystemInfo::value(SystemInfo::CurBootloaderMethod).toString());

    if(bl == NULL ) {
        logger->addItem(tr("No uninstall method for this target known."), LOGERROR);
        logger->setFinished();
        return;
    }
    QStringList blfile = SystemInfo::value(SystemInfo::CurBootloaderFile).toStringList();
    QStringList blfilepath;
    for(int a = 0; a < blfile.size(); a++) {
        blfilepath.append(RbSettings::value(RbSettings::Mountpoint).toString()
                + blfile.at(a));
    }
    bl->setBlFile(blfilepath);

    BootloaderInstallBase::BootloaderType currentbl = bl->installed();
    if((bl->capabilities() & BootloaderInstallBase::Uninstall) == 0
            || currentbl == BootloaderInstallBase::BootloaderUnknown
            || currentbl == BootloaderInstallBase::BootloaderOther)
    {
        logger->addItem(tr("Rockbox Utility can not uninstall the bootloader on this target. "
                            "Try a normal firmware update to remove the booloader."), LOGERROR);
        logger->setFinished();
        delete bl;
        return;
    }

    connect(bl, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(bl, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));

    bl->uninstall();

    logger->setFinished();

}


void RbUtilQt::downloadManual(void)
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm download"),
       tr("Do you really want to download the manual? The manual will be saved "
            "to the root folder of your player."),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QString manual = SystemInfo::value(SystemInfo::CurManual).toString();
    if(manual.isEmpty())
        manual = "rockbox-"
            + SystemInfo::value(SystemInfo::CurBuildserverModel).toString();

    QDate date = QDate::fromString(ServerInfo::value(ServerInfo::DailyDate).toString(),Qt::ISODate);

    QString manualurl;
    QString target;
    QString section;
    if(ui.radioPdf->isChecked()) {
        target = "/" + manual + ".pdf";
        section = "Manual (PDF)";
    }
    else {
        target = "/" + manual + "-" + date.toString("yyyyMMdd") + "-html.zip";
        section = "Manual (HTML)";
    }
    manualurl = SystemInfo::value(SystemInfo::ManualUrl).toString() + "/" + target;
    qDebug() << "[RbUtil] Manual URL:" << manualurl;

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();
    installer = new ZipInstaller(this);
    installer->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);
    installer->setLogSection(section);
    installer->setLogVersion(ServerInfo::value(ServerInfo::DailyDate).toString());
    installer->setUrl(manualurl);
    installer->setUnzip(false);
    installer->setTarget(target);
    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();
}


void RbUtilQt::installPortable(void)
{
    if(QMessageBox::question(this, tr("Confirm installation"),
       tr("Do you really want to install Rockbox Utility to your player? "
        "After installation you can run it from the players hard drive."),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->setProgressMax(0);
    logger->setProgressValue(0);
    logger->show();
    logger->addItem(tr("Installing Rockbox Utility"), LOGINFO);

    // check mountpoint
    if(!QFileInfo(RbSettings::value(RbSettings::Mountpoint).toString()).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->setFinished();
        return;
    }

    // remove old files first.
    QFile::remove(RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.exe");
    QFile::remove(RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.ini");
    // copy currently running binary and currently used settings file
    if(!QFile::copy(qApp->applicationFilePath(),
            RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.exe")) {
        logger->addItem(tr("Error installing Rockbox Utility"), LOGERROR);
        logger->setFinished();
        return;
    }
    logger->addItem(tr("Installing user configuration"), LOGINFO);
    if(!QFile::copy(RbSettings::userSettingFilename(),
            RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.ini")) {
        logger->addItem(tr("Error installing user configuration"), LOGERROR);
        logger->setFinished();
        return;
    }
    logger->addItem(tr("Successfully installed Rockbox Utility."), LOGOK);
    logger->setFinished();
    logger->setProgressMax(1);
    logger->setProgressValue(1);

}


void RbUtilQt::updateInfo()
{
    qDebug() << "[RbUtil] updating server info";

    QString mp = RbSettings::value(RbSettings::Mountpoint).toString();
    QSettings log(mp + "/.rockbox/rbutil.log", QSettings::IniFormat, this);
    QStringList groups = log.childGroups();
    QList<QTreeWidgetItem *> items;
    QTreeWidgetItem *w, *w2;
    QString min, max;
    int olditems = 0;

    // remove old list entries (if any)
    int l = ui.treeInfo->topLevelItemCount();
    while(l--) {
        QTreeWidgetItem *m;
        m = ui.treeInfo->takeTopLevelItem(l);
        // delete childs (single level deep, no recursion here)
        int n = m->childCount();
        while(n--)
            delete m->child(n);
    }
    // get and populate new items
    for(int a = 0; a < groups.size(); a++) {
        log.beginGroup(groups.at(a));
        QStringList keys = log.allKeys();
        w = new QTreeWidgetItem;
        w->setFlags(Qt::ItemIsEnabled);
        w->setText(0, groups.at(a));
        items.append(w);
        // get minimum and maximum version information so we can hilight old files
        min = max = log.value(keys.at(0)).toString();
        for(int b = 0; b < keys.size(); b++) {
            if(log.value(keys.at(b)).toString() > max)
                max = log.value(keys.at(b)).toString();
            if(log.value(keys.at(b)).toString() < min)
                min = log.value(keys.at(b)).toString();
        }

        for(int b = 0; b < keys.size(); b++) {
            QString file;
            file = mp + "/" + keys.at(b);
            if(QFileInfo(file).isDir())
                continue;
            w2 = new QTreeWidgetItem(w, QStringList() << "/"
                    + keys.at(b) << log.value(keys.at(b)).toString());
            if(log.value(keys.at(b)).toString() != max) {
                w2->setForeground(0, QBrush(QColor(255, 0, 0)));
                w2->setForeground(1, QBrush(QColor(255, 0, 0)));
                olditems++;
            }
            items.append(w2);
        }
        log.endGroup();
        if(min != max)
            w->setData(1, Qt::DisplayRole, QString("%1 / %2").arg(min, max));
        else
            w->setData(1, Qt::DisplayRole, max);
    }
    ui.treeInfo->insertTopLevelItems(0, items);
    ui.treeInfo->resizeColumnToContents(0);
}


QUrl RbUtilQt::proxy()
{
    QUrl proxy;
    if(RbSettings::value(RbSettings::ProxyType) == "manual")
        proxy.setEncodedUrl(RbSettings::value(RbSettings::Proxy).toByteArray());
    else if(RbSettings::value(RbSettings::ProxyType) == "system")
        proxy = System::systemProxy();
    qDebug() << proxy.userName() << proxy.password() << proxy.host() << proxy.port();
    return proxy;
}


bool RbUtilQt::chkConfig(bool warn)
{
    bool error = false;
    if(RbSettings::value(RbSettings::Platform).toString().isEmpty()
        || RbSettings::value(RbSettings::Mountpoint).toString().isEmpty()
        || !QFileInfo(RbSettings::value(RbSettings::Mountpoint).toString()).isWritable()) {
        error = true;

        if(warn) QMessageBox::critical(this, tr("Configuration error"),
            tr("Your configuration is invalid. Please go to the configuration "
                "dialog and make sure the selected values are correct."));
    }
    return error;
}

void RbUtilQt::checkUpdate(void)
{
    QString url = SystemInfo::value(SystemInfo::RbutilUrl).toString();
#if defined(Q_OS_WIN32)   
    url += "win32/";
#elif defined(Q_OS_LINUX)
    url += "linux/";
#elif defined(Q_OS_MACX)
    url += "macosx/";
#endif

    update = new HttpGet(this);
    connect(update, SIGNAL(done(bool)), this, SLOT(downloadUpdateDone(bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), update, SLOT(abort()));
    if(RbSettings::value(RbSettings::CacheOffline).toBool())
        update->setCache(true);

    ui.statusbar->showMessage(tr("Checking for update ..."));
    update->getFile(QUrl(url));
}

void RbUtilQt::downloadUpdateDone(bool error)
{
    if(error) {
        qDebug() << "[RbUtil] network error:" << update->error();
    }
    else {
        QString toParse(update->readAll());

        QRegExp searchString("<a[^>]*>([a-zA-Z]+[^<]*)</a>");
        QStringList rbutilList;
        int pos = 0;
        while ((pos = searchString.indexIn(toParse, pos)) != -1)
        {
            rbutilList << searchString.cap(1);
            pos += searchString.matchedLength();
        }
        qDebug() << "[RbUtilQt] Checking for update";

        QString newVersion = "";
        QString foundVersion = "";
        // check if there is a binary with higher version in this list
        for(int i=0; i < rbutilList.size(); i++)
        {
            QString item = rbutilList.at(i);
#if defined(Q_OS_LINUX) 
#if defined(__amd64__)
            // skip if it isn't a 64 bit build
            if( !item.contains("64bit"))
                continue;
            // strip the "64bit" suffix for comparison
            item = item.remove("64bit");
#else
            //skip if it is a 64bit build
            if(item.contains("64bit"))
                continue;
#endif
#endif
            // check if it is newer, and remember newest
            if(Utils::compareVersionStrings(VERSION, item) == 1)
            {
                if(Utils::compareVersionStrings(newVersion, item) == 1)
                {
                    newVersion = item;
                    foundVersion = rbutilList.at(i);
                }
            }
        }

        // if we found something newer, display info
        if(foundVersion != "")
        {
            QString url = SystemInfo::value(SystemInfo::RbutilUrl).toString();
#if defined(Q_OS_WIN32)   
            url += "win32/";
#elif defined(Q_OS_LINUX)
            url += "linux/";
#elif defined(Q_OS_MACX)
            url += "macosx/";
#endif
            url += foundVersion;

            QMessageBox::information(this,tr("RockboxUtility Update available"),
                        tr("<b>New RockboxUtility Version available.</b> <br><br>"
                        "Download it from here: <a href='%1'>%2</a>")
                        .arg(url).arg(foundVersion));
            ui.statusbar->showMessage(tr("New version of Rockbox Utility available."));
        }
        else {
            ui.statusbar->showMessage(tr("Rockbox Utility is up to date."), 5000);
        }
    }
}


void RbUtilQt::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        buildInfo.open();
        ServerInfo::readBuildInfo(buildInfo.fileName());
        buildInfo.close();
        updateDevice();
    } else {
        QMainWindow::changeEvent(e);
    }
}

