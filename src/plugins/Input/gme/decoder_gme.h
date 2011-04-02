/***************************************************************************
 *   Copyright (C) 2010 by Ilya Kotov                                      *
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

#ifndef DECODER_GME_H
#define DECODER_GME_H

#include <gme/gme.h>
#include <qmmp/decoder.h>

class GmeHelper;

/**
   @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class DecoderGme : public Decoder
{
public:
    DecoderGme(const QString &path);
    virtual ~DecoderGme();

    // Standard Decoder API
    bool initialize();
    qint64 totalTime();
    int bitrate();
    qint64 read(char *data, qint64 size);
    void seek(qint64);

private:
    GmeHelper m_helper;
    Music_Emu *m_emu;
    qint64 m_totalTime;
    QString m_path;
};

#endif // DECODER_GME_H
