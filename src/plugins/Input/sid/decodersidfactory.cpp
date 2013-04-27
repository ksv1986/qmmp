/***************************************************************************
 *   Copyright (C) 2013 by Ilya Kotov                                      *
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
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>
#include "decoder_sid.h"
#include "sidhelper.h"
#include "decodersidfactory.h"

// DecoderSIDFactory

bool DecoderSIDFactory::supports(const QString &source) const
{
    foreach(QString filter, properties().filters)
    {
        QRegExp regexp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
        if (regexp.exactMatch(source))
            return true;
    }
    return false;
}

bool DecoderSIDFactory::canDecode(QIODevice *input) const
{
    char buf[4];
    if (input->peek(buf, 4) != 4)
        return false;
    return (!memcmp(buf, "RSID", 4) || !memcmp(buf, "PSID", 4));
}

const DecoderProperties DecoderSIDFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("SID Plugin");
    properties.filters << "*.sid" << "*.mus" << "*.str" << "*.prg" << "*.P00";
    properties.description = tr("SID Files");
    //properties.contentType = ;
    properties.shortName = "sid";
    properties.hasAbout = true;
    properties.hasSettings = false;
    properties.noInput = true;
    properties.protocols << "sid";
    return properties;
}

Decoder *DecoderSIDFactory::create(const QString &path, QIODevice *input)
{
    Q_UNUSED(input);
    return new DecoderSID(path);
}

QList<FileInfo *> DecoderSIDFactory::createPlayList(const QString &fileName, bool useMetaData)
{
    SIDHelper helper;
    helper.load(fileName);
    QList <FileInfo*> list = helper.createPlayList(useMetaData);
    if(list.isEmpty())
        return list;
    if(fileName.contains("://")) //is it url?
    {
        int track = fileName.section("#", -1).toInt();
        if(track > list.count() || track < 1)
        {
            qDeleteAll(list);
            list.clear();
            return list;
        }
        FileInfo *info = list.takeAt(track - 1);
        qDeleteAll(list);
        return QList<FileInfo *>() << info;
    }
    return list;
}

MetaDataModel* DecoderSIDFactory::createMetaDataModel(const QString &path, QObject *parent)
{
    Q_UNUSED(path);
    Q_UNUSED(parent);
    return 0;
}

void DecoderSIDFactory::showSettings(QWidget *parent)
{
    Q_UNUSED(parent);
}

void DecoderSIDFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About SID Audio Plugin"),
                        tr("Qmmp SID Audio Plugin")+"\n"+
                        tr("This plugin plays Commodore 64 music files using libsidplayfp library")+"\n"+
                        tr("Written by: Ilya Kotov <forkotov02@hotmail.ru>"));
}

QTranslator *DecoderSIDFactory::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/sid_plugin_") + locale);
    return translator;
}
Q_EXPORT_PLUGIN2(gme,DecoderSIDFactory)
