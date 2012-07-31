/***************************************************************************
 *   Copyright (C) 2007-2012 by Ilya Kotov                                 *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#ifndef SRCONVERTER_H
#define SRCONVERTER_H

#include <samplerate.h>
#include <qmmp/effect.h>

/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/

class SRConverter : public Effect
{
public:
    SRConverter();
    virtual ~SRConverter();

    void applyEffect(Buffer *b);
    void configure(quint32 freq, int chan, Qmmp::AudioFormat format);

private:
    void freeSRC();
    SRC_STATE *m_src_state;
    SRC_DATA m_src_data;
    quint32 m_overSamplingFs;
    int m_srcError;
    int m_converter_type;
    quint32 m_freq;
};

#endif
