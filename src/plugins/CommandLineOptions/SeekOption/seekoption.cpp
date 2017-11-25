/***************************************************************************
 *   Copyright (C) 2010-2017 by Ilya Kotov                                 *
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

#include <QtPlugin>
#include <QTranslator>
#include <QLocale>
#include <QRegExp>
#include <qmmp/soundcore.h>
#include "seekoption.h"

bool SeekOption::identify(const QString &str) const
{
    QStringList opts;
    opts << "--seek" << "--seek-fwd" << "--seek-bwd";
    return opts.contains(str);
}

const QStringList SeekOption::helpString() const
{
    return QStringList()
            << QString("--seek <time>") + "||" + tr("Seek to position in the current track")
            << QString("--seek-fwd <time>") + "||" + tr("Seek forward")
            << QString("--seek-bwd <time>") + "||" + tr("Seek backwards");
}

QString SeekOption::executeCommand(const QString &opt_str, const QStringList &args)
{
    SoundCore *core = SoundCore::instance();
    if(core->state() != Qmmp::Playing && core->totalTime())
        return QString();
    if(args.isEmpty())
        return QString();

    int seek_pos = -1;
    int elapsed = core->elapsed()/1000;

    QRegExp seek_regexp1 ("^([0-9]{1,4})$");
    QRegExp seek_regexp2 ("^([0-9]{1,2}):([0-9]{1,2})$");

    if(seek_regexp1.indexIn(args.at(0)) != -1)
        seek_pos = seek_regexp1.cap(1).toInt();
    else if(seek_regexp2.indexIn(args.at(0)) != -1)
        seek_pos = seek_regexp2.cap(1).toInt()*60 + seek_regexp2.cap(2).toInt();

    //seek absolute
    if(opt_str == "--seek")
        ;
    else if(opt_str == "--seek-fwd")
            seek_pos += elapsed;
    else if(opt_str == "--seek-bwd")
        seek_pos = elapsed - seek_pos;
    qDebug("SeekOption: position = %d", seek_pos);

    if(seek_pos >= 0 && seek_pos < core->totalTime())
        core->seek(seek_pos * 1000);
    else
        return QString();
    return QString();
}

const QString SeekOption::name() const
{
    return "SeekOption";
}

QTranslator *SeekOption::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/seek_plugin_") + locale);
    return translator;
}
