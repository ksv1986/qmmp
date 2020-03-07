/***************************************************************************
 *   Copyright (C) 2009-2020 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include <QTextCodec>
#include <QSettings>
#include <QDir>
#include <qmmp/qmmp.h>
#include "decoder_cdaudio.h"
#include "settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
        : QDialog(parent)
{
    m_ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("cdaudio");
    m_ui.deviceLineEdit->setText(settings.value("device").toString());
    m_ui.deviceCheckBox->setChecked(!m_ui.deviceLineEdit->text().isEmpty());
    int speed = settings.value("speed", 0).toInt();
    m_ui.speedCheckBox->setChecked(speed > 0);
    m_ui.speedSpinBox->setValue(speed);
    m_ui.cdtextCheckBox->setChecked(settings.value("cdtext", true).toBool());
    settings.endGroup();
}

SettingsDialog::~SettingsDialog()
{}

void SettingsDialog::accept()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("cdaudio");
    if(m_ui.deviceCheckBox->isChecked())
        settings.setValue("device", m_ui.deviceLineEdit->text());
    else
        settings.remove("device");
    if(m_ui.speedCheckBox->isChecked())
        settings.setValue("speed", m_ui.speedSpinBox->value());
    else
        settings.setValue("speed", 0);
    settings.setValue("cdtext", m_ui.cdtextCheckBox->isChecked());
    settings.setValue("cdtext", m_ui.cdtextCheckBox->isChecked());
    settings.endGroup();
    settings.sync();
    DecoderCDAudio::clearTrackCache();
    QDialog::accept();
}
