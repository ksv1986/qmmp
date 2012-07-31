/***************************************************************************
 *   Copyright (C) 2009 by Ilya Kotov                                      *
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

#include <QApplication>
#include "ladspahost.h"
#include "ladspahelper.h"

LADSPAHelper::LADSPAHelper() : Effect()
{
    if(!LADSPAHost::instance())
        new LADSPAHost(qApp);
}

LADSPAHelper::~LADSPAHelper()
{}

void LADSPAHelper::applyEffect(Buffer *b)
{
    LADSPAHost::instance()->applyEffect((qint16 *)b->data, b->nbytes);
}

void LADSPAHelper::configure(quint32 freq, int chan, Qmmp::AudioFormat format)
{
    LADSPAHost::instance()->configure(freq,chan,format);
    Effect::configure(freq, chan, format);
}
