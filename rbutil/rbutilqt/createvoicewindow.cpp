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

#include "createvoicewindow.h"
#include "ui_createvoicefrm.h"

#include "configure.h"
#include "rbsettings.h"
#include "systeminfo.h"

CreateVoiceWindow::CreateVoiceWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    voicecreator = new VoiceFileCreator(this);
    updateSettings();
    connect(ui.change,SIGNAL(clicked()),this,SLOT(change()));
}

void CreateVoiceWindow::change()
{
    // save window settings
    saveSettings();

    // call configuration dialog
    Config *cw = new Config(this,4);
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    cw->show();    
}

void CreateVoiceWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    logger->show();    

    saveSettings();
    
    //configure voicecreator
    voicecreator->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());
    voicecreator->setLang(ui.comboLanguage->currentText());
    voicecreator->setWavtrimThreshold(ui.wavtrimthreshold->value());
       
    //start creating
    connect(voicecreator, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(voicecreator, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(voicecreator, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(logger,SIGNAL(aborted()),voicecreator,SLOT(abort()));
    voicecreator->createVoiceFile();
}


/** @brief update displayed settings
 */
void CreateVoiceWindow::updateSettings(void)
{
    // fill in language combobox
    QMap<QString, QString> languages = SystemInfo::languages();

    for(int i = 0; i < languages.keys().size(); i++) {
        QString key = languages.keys().at(i);
        ui.comboLanguage->addItem(languages.value(key), key);
    }
    // set saved lang
    int sel = ui.comboLanguage->findText(
            RbSettings::value(RbSettings::VoiceLanguage).toString());
    // if no saved language is found try to figure the language from the UI lang
    if(sel == -1) {
        QString f = RbSettings::value(RbSettings::Language).toString();
        // if no language is set default to english. Make sure not to check an empty string.
        if(f.isEmpty()) f = "english";
        sel = ui.comboLanguage->findData(f);
        qDebug() << "sel =" << sel;
        // still nothing found?
        if(sel == -1)
            sel = ui.comboLanguage->findText("english", Qt::MatchStartsWith);
    }
    ui.comboLanguage->setCurrentIndex(sel);

    QString ttsName = RbSettings::value(RbSettings::Tts).toString();
    TTSBase* tts = TTSBase::getTTS(this,ttsName);
    if(tts->configOk())
        ui.labelTtsProfile->setText(tr("Selected TTS engine: <b>%1</b>")
            .arg(TTSBase::getTTSName(ttsName)));
    else
        ui.labelTtsProfile->setText(tr("Selected TTS engine: <b>%1</b>")
            .arg("Invalid TTS configuration!"));
    
    QString encoder = SystemInfo::value(SystemInfo::CurEncoder).toString();
    // only proceed if encoder setting is set
    EncBase* enc = EncBase::getEncoder(this,encoder);
    if(enc != NULL) {
        if(enc->configOk())
            ui.labelEncProfile->setText(tr("Selected encoder: <b>%1</b>")
                .arg(EncBase::getEncoderName(encoder)));
        else
            ui.labelEncProfile->setText(tr("Selected encoder: <b>%1</b>")
                .arg("Invalid encoder configuration!"));
    }
    else
        ui.labelEncProfile->setText(tr("Selected encoder: <b>%1</b>")
            .arg("Invalid encoder configuration!"));
    ui.wavtrimthreshold->setValue(RbSettings::value(RbSettings::WavtrimThreshold).toInt());
    emit settingsUpdated();
}


/** @brief save options
  */
void CreateVoiceWindow::saveSettings(void)
{
    // save selected language
    RbSettings::setValue(RbSettings::VoiceLanguage,
                         ui.comboLanguage->currentText());
    // save wavtrim threshold value
    RbSettings::setValue(RbSettings::WavtrimThreshold,
                         ui.wavtrimthreshold->value());
    RbSettings::sync();
}

void CreateVoiceWindow::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        updateSettings();
    } else {
        QWidget::changeEvent(e);
    }
}

