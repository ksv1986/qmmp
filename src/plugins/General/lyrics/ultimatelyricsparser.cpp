/***************************************************************************
 *   Copyright (C) 2019-2020 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Based on Amarok 2 Ultimate Lyrics script                              *
 *   Copyright (C) 2009-2010 Vladimir Brkic <vladimir_brkic@yahoo.com>     *
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

#include <QXmlStreamReader>
#include <QFile>
#include <QtDebug>
#if (QT_VERSION < QT_VERSION_CHECK(5, 7, 0)) //qAsConst template
#include <qmmp/qmmp.h>
#endif
#include "ultimatelyricsparser.h"

UltimateLyricsParser::UltimateLyricsParser()
{}

UltimateLyricsParser::~UltimateLyricsParser()
{
    qDeleteAll(m_providers);
    m_providers.clear();
}

bool UltimateLyricsParser::load(const QString &path)
{
    qDeleteAll(m_providers);
    m_providers.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        m_errorString = file.errorString();
        return false;
    }

    QXmlStreamReader reader(&file);

    QString parentElement;
    QList<QPair<QString, QString> > args;

    while(!reader.atEnd())
    {
        reader.readNext();

        if(reader.isStartElement())
        {
            if(reader.name() == "provider")
            {
                LyricsProvider *provider = new LyricsProvider;
                QXmlStreamAttributes attrs = reader.attributes();
                provider->setName(attrs.value("name").toString());
                provider->setTitle(attrs.value("title").toString());
                provider->setUrl(attrs.value("url").toString());
                provider->setCharset(attrs.value("charser").toString());
                m_providers << provider;
            }
            else if(reader.name() == "urlFormat" && !m_providers.isEmpty())
            {
                m_providers.last()->addUrlFormat(reader.attributes().value("replace").toString(),
                                                 reader.attributes().value("with").toString());
            }
            else if(reader.name() == "extract" || reader.name() == "exclude")
            {
                parentElement = reader.name().toString();
            }
            else if(reader.name() == "invalidIndicator" && !m_providers.isEmpty())
            {
                m_providers.last()->addInvalidIndicator(reader.attributes().value("value").toString());
            }
            else if(reader.name() == "item")
            {
                QXmlStreamAttributes attrs = reader.attributes();
                QString arg1, arg2;
                if(attrs.hasAttribute("begin") && attrs.hasAttribute("end"))
                {
                    arg1 = attrs.value("begin").toString();
                    arg2 = attrs.value("end").toString();
                }
                else if(attrs.hasAttribute("tag"))
                {
                    arg1 = attrs.value("tag").toString();
                }
                else if(attrs.hasAttribute("url"))
                {
                    arg1 = attrs.value("url").toString();
                }
                args << qMakePair(arg1, arg2);
            }
        }
        else if(reader.isEndElement())
        {
            if(reader.name() == "extract" || reader.name() == "exclude")
            {
                parentElement.clear();
                m_providers.last()->addRule(args, reader.name() == "exclude");
                args.clear();
            }
        }

        if(reader.hasError())
        {
            m_errorString = tr("%1 (line: %2)").arg(reader.errorString()).arg(reader.lineNumber());
            return false;
        }
    }
    return true;
}

const QString &UltimateLyricsParser::errorString() const
{
    return m_errorString;
}

const QList<LyricsProvider *> &UltimateLyricsParser::providers()
{
    return m_providers;
}

LyricsProvider *UltimateLyricsParser::provider(const QString &name) const
{
    for(LyricsProvider *provider : qAsConst(m_providers))
    {
        if(provider->name() == name)
            return provider;
    }
    return nullptr;
}

QStringList UltimateLyricsParser::defaultProviders()
{
    QStringList out = {
        "Encyclopaedia Metallum",
        "letras.mus.br",
        "darklyrics.com"
    };

    return out;
}
