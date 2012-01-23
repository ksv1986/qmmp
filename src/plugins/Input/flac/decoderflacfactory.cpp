/***************************************************************************
 *   Copyright (C) 2008-2012 by Ilya Kotov                                 *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QtGui>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/tmap.h>

#include "cueparser.h"
#include "decoder_flac.h"
#include "flacmetadatamodel.h"
#include "decoderflacfactory.h"


// DecoderFLACFactory

bool DecoderFLACFactory::supports(const QString &source) const
{
    return source.endsWith(".flac", Qt::CaseInsensitive) || source.endsWith(".oga", Qt::CaseInsensitive);
}

bool DecoderFLACFactory::canDecode(QIODevice *input) const
{
    char buf[36];
    if (input->peek(buf, 36) != 36)
        return false;
    if(!memcmp(buf, "fLaC", 4)) //native flac
        return true;
    if(!memcmp(buf, "OggS", 4) && !memcmp(buf + 29, "FLAC", 4)) //ogg flac
        return true;
    return false;
}

const DecoderProperties DecoderFLACFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("FLAC Plugin");
    properties.filters << "*.flac" << "*.oga";
    properties.description = tr("FLAC Files");
    properties.contentTypes << "audio/x-flac";
    properties.shortName = "flac";
    properties.protocols << "flac";
    properties.hasAbout = true;
    properties.hasSettings = false;
    return properties;
}

Decoder *DecoderFLACFactory::create(const QString &path, QIODevice *i)
{
    return new DecoderFLAC(path, i);
}

QList<FileInfo *> DecoderFLACFactory::createPlayList(const QString &fileName, bool useMetaData)
{
    QList <FileInfo*> list;
    TagLib::Ogg::XiphComment *tag = 0;
    TagLib::FLAC::Properties *ap = 0;

    TagLib::FLAC::File *flacFile = 0;
    TagLib::Ogg::FLAC::File *oggFlacFile = 0;

    //extract metadata of one cue track
    if(fileName.contains("://"))
    {
        QString path = QUrl(fileName).path();
        path.replace(QString(QUrl::toPercentEncoding("#")), "#");
        path.replace(QString(QUrl::toPercentEncoding("?")), "?");
        path.replace(QString(QUrl::toPercentEncoding("%")), "%");
        path.replace(QString(QUrl::toPercentEncoding(":")), ":");
        int track = fileName.section("#", -1).toInt();
        list = createPlayList(path, true);
        if (list.isEmpty() || track <= 0 || track > list.count())
        {
            qDeleteAll(list);
            list.clear();
            return list;
        }
        FileInfo *info = list.takeAt(track - 1);
        qDeleteAll(list);
        return QList<FileInfo *>() << info;
    }

    if(fileName.endsWith(".flac", Qt::CaseInsensitive))
    {
        flacFile = new TagLib::FLAC::File(fileName.toLocal8Bit().constData());
        tag = useMetaData ? flacFile->xiphComment() : 0;
        ap = flacFile->audioProperties();
    }
    else if(fileName.endsWith(".oga", Qt::CaseInsensitive))
    {
        oggFlacFile = new TagLib::Ogg::FLAC::File(fileName.toLocal8Bit().constData());
        tag = useMetaData ? oggFlacFile->tag() : 0;
        ap = oggFlacFile->audioProperties();
    }
    else
        return list;

    FileInfo *info = new FileInfo(fileName);
    if (tag && !tag->isEmpty())
    {
        info->setMetaData(Qmmp::ALBUM,
                          QString::fromUtf8(tag->album().toCString(true)).trimmed());
        info->setMetaData(Qmmp::ARTIST,
                          QString::fromUtf8(tag->artist().toCString(true)).trimmed());
        info->setMetaData(Qmmp::COMMENT,
                          QString::fromUtf8(tag->comment().toCString(true)).trimmed());
        info->setMetaData(Qmmp::GENRE,
                          QString::fromUtf8(tag->genre().toCString(true)).trimmed());
        info->setMetaData(Qmmp::TITLE,
                          QString::fromUtf8(tag->title().toCString(true)).trimmed());
        info->setMetaData(Qmmp::YEAR, tag->year());
        info->setMetaData(Qmmp::TRACK, tag->track());

        if (tag->fieldListMap().contains("CUESHEET"))
        {
            CUEParser parser(tag->fieldListMap()["CUESHEET"].toString().toCString(true), fileName);
            list = parser.createPlayList();
            delete info;
            if(flacFile)
                delete flacFile;
            if(oggFlacFile)
                delete oggFlacFile;
            return list;
        }

        //additional metadata
        TagLib::StringList fld;
        if(!(fld = tag->fieldListMap()["COMPOSER"]).isEmpty())
            info->setMetaData(Qmmp::COMPOSER,
                              QString::fromUtf8(fld.toString().toCString(true)).trimmed());
        if(!(fld = tag->fieldListMap()["DISCNUMBER"]).isEmpty())
            info->setMetaData(Qmmp::DISCNUMBER,
                              QString::fromUtf8(fld.toString().toCString(true)).trimmed());
    }
    if(ap)
        info->setLength(ap->length());
    list << info;
    if(flacFile)
        delete flacFile;
    if(oggFlacFile)
        delete oggFlacFile;
    return list;
}

MetaDataModel*DecoderFLACFactory::createMetaDataModel(const QString &path, QObject *parent)
{
    if (!path.contains("://") || path.startsWith("flac://"))
        return new FLACMetaDataModel(path, parent);
    else
        return 0;
}

void DecoderFLACFactory::showSettings(QWidget *)
{}

void DecoderFLACFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About FLAC Audio Plugin"),
                        tr("Qmmp FLAC Audio Plugin")+"\n"+
                        tr("Written by: Ilya Kotov <forkotov02@hotmail.ru>"));
}

QTranslator *DecoderFLACFactory::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/flac_plugin_") + locale);
    return translator;
}

Q_EXPORT_PLUGIN2(flac,DecoderFLACFactory)
