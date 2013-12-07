/***************************************************************************
 *   Copyright (C) 2009-2013 by Ilya Kotov                                 *
 *   forkotov02@hotmail.ru                                                 *
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

#include <QtGui>
#include <QRegExp>
#include <cdio/version.h>
#include <cddb/version.h>
#include "settingsdialog.h"
#include "decoder_cdaudio.h"
#include "decodercdaudiofactory.h"


// DecoderCDAudioFactory

bool DecoderCDAudioFactory::supports(const QString &source) const
{
    return source == "/" || source.startsWith("/dev");
}

bool DecoderCDAudioFactory::canDecode(QIODevice *) const
{
    return false;
}

const DecoderProperties DecoderCDAudioFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("CD Audio Plugin");
    properties.shortName = "cdaudio";
    properties.protocols << "cdda";
    properties.hasAbout = true;
    properties.noInput = true;
    properties.hasSettings = true;
    return properties;
}

Decoder *DecoderCDAudioFactory::create(const QString &url, QIODevice *input)
{
    Q_UNUSED(input);
    return new DecoderCDAudio(url);
}

QList<FileInfo *> DecoderCDAudioFactory::createPlayList(const QString &url, bool useMetaData)
{
    Q_UNUSED(useMetaData);
    QList <FileInfo*> list;
    QString device_path = url;
    device_path.remove("cdda://");
    device_path.remove(QRegExp("#\\d+$"));
    QList <CDATrack> tracks = DecoderCDAudio::generateTrackList(device_path);
    foreach(CDATrack t, tracks)
    {
        list << new FileInfo(t.info);
    }
    return list;
}

MetaDataModel* DecoderCDAudioFactory::createMetaDataModel(const QString &path, QObject *parent)
{
    Q_UNUSED(parent);
    Q_UNUSED(path);
    return 0;
}

void DecoderCDAudioFactory::showSettings(QWidget *parent)
{
    SettingsDialog *d = new SettingsDialog(parent);
    d->show();
}

void DecoderCDAudioFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About CD Audio Plugin"),
                        tr("Qmmp CD Audio Plugin")+"\n"+
                        QString(tr("Compiled against libcdio-%1 and libcddb-%2")).arg(CDIO_VERSION)
                        .arg(CDDB_VERSION) + "\n" +
                        tr("Written by: Ilya Kotov <forkotov02@hotmail.ru>")+"\n"+
                        tr("Usage: open cdda:/// using Add URL dialog or command line"));
}

QTranslator *DecoderCDAudioFactory::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/cdaudio_plugin_") + locale);
    return translator;
}

Q_EXPORT_PLUGIN2(cdaudio, DecoderCDAudioFactory)
