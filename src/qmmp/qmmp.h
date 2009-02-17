/***************************************************************************
 *   Copyright (C) 2008-2009 by Ilya Kotov                                 *
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
#ifndef QMMP_H
#define QMMP_H

#include <QUrl>

/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class Qmmp
{
public:
    enum State {Playing = 0, Paused, Stopped, Buffering, NormalError, FatalError};
    enum MetaData {TITLE = 0, ARTIST, ALBUM, COMMENT, GENRE, YEAR, TRACK, URL};
    static QString configFile();
    static QString strVersion();
    //global proxy
    static bool useProxy();
    static bool useProxyAuth();
    static QUrl proxy();
    static void setProxyEnabled(bool yes);
    static void setProxyAuthEnabled(bool yes);
    static void setProxy (const QUrl &proxy);

};

#endif
