/***************************************************************************
 *   Copyright (C) 2008-2013 by Ilya Kotov                                 *
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

#include <QTimer>
#include <QDir>
#include <QSettings>
#include "qmmpsettings.h"
#include "output.h"
#include "volumecontrol_p.h"

VolumeControl::VolumeControl(QObject *parent)
        : QObject(parent)
{
    m_left = 0;
    m_right = 0;
    m_prev_block = false;
    m_volume = 0;
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), SLOT(checkVolume()));
    reload();
}

VolumeControl::~VolumeControl()
{
    if(m_volume)
        delete m_volume;
}

void VolumeControl::setVolume(int left, int right)
{
    VolumeSettings v;
    v.left = qBound(0,left,100);
    v.right = qBound(0,right,100);
    m_volume->setVolume(v);
    checkVolume();
}

void VolumeControl::changeVolume(int delta)
{
    int v = qMax(m_left, m_right);
    int balance = v > 0 ? (m_right - m_left)*100/v : 0;
    v = delta > 0 ? qMin(100, v + 5) : qMax(0, v - 5);
    setVolume(v-qMax(balance,0)*v/100, v+qMin(balance,0)*v/100);
}

void VolumeControl::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    setVolume(volume-qMax(balance(),0)*volume/100,
              volume+qMin(balance(),0)*volume/100);
}

void VolumeControl::setBalance(int balance)
{
    balance = qBound(-100, balance, 100);
    setVolume(volume()-qMax(balance,0)*volume()/100,
              volume()+qMin(balance,0)*volume()/100);
}

int VolumeControl::left() const
{
    return m_left;
}

int VolumeControl::right() const
{
    return m_right;
}

int VolumeControl::volume() const
{
    return qMax(m_right, m_left);
}

int VolumeControl::balance() const
{
    int v = volume();
    return v > 0 ? (m_right - m_left)*100/v : 0;
}

void VolumeControl::checkVolume()
{
    VolumeSettings v = m_volume->volume();
    int l = v.left;
    int r = v.right;

    l = (l > 100) ? 100 : l;
    r = (r > 100) ? 100 : r;
    l = (l < 0) ? 0 : l;
    r = (r < 0) ? 0 : r;
    if (m_left != l || m_right != r) //volume has been changed
    {
        m_left = l;
        m_right = r;
        emit volumeChanged(m_left, m_right);
        emit volumeChanged(volume());
        emit balanceChanged(balance());
    }
    else if(m_prev_block && !signalsBlocked ()) //signals have been unblocked
    {
        emit volumeChanged(m_left, m_right);
        emit volumeChanged(volume());
        emit balanceChanged(balance());
    }
    m_prev_block = signalsBlocked ();
}

void VolumeControl::reload()
{
    m_timer->stop();
    if(m_volume)
    {
        delete m_volume;
        m_volume = 0;
    }
    if(!QmmpSettings::instance()->useSoftVolume() && Output::currentFactory())
    {
        if((m_volume = Output::currentFactory()->createVolume()))
            m_timer->start(150);
    }
    if(!m_volume)
    {
        m_volume = new SoftwareVolume;
        blockSignals(true);
        checkVolume();
        blockSignals(false);
        QTimer::singleShot(125, this, SLOT(checkVolume()));
    }
}

SoftwareVolume *SoftwareVolume::m_instance = 0;

SoftwareVolume::SoftwareVolume()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    m_left = settings.value("Volume/left", 80).toInt();
    m_right = settings.value("Volume/right", 80).toInt();
    m_scaleLeft = (double)m_left/100.0;
    m_scaleRight = (double)m_right/100.0;
    m_instance = this;
}

SoftwareVolume::~SoftwareVolume()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("Volume/left", m_left);
    settings.setValue("Volume/right", m_right);
    m_instance = 0;
}

void SoftwareVolume::setVolume(const VolumeSettings &v)
{
    m_left = v.left;
    m_right = v.right;
    m_scaleLeft = (double)m_left/100.0;
    m_scaleRight = (double)m_right/100.0;
}

VolumeSettings SoftwareVolume::volume() const
{
    VolumeSettings v;
    v.left = m_left;
    v.right = m_right;
    return v;
}

void SoftwareVolume::changeVolume(Buffer *b, int chan, Qmmp::AudioFormat format)
{
    int samples = 0;
    qint32 sample1 = 0;
    qint32 sample2 = 0;
    switch(format)
    {
    case Qmmp::PCM_S8:
        samples = b->nbytes;
        if (chan > 1)
        {
            for (int i = 0; i < samples; i+=2)
            {
                ((char*)b->data)[i]*= m_scaleLeft;
                ((char*)b->data)[i+1]*= m_scaleRight;
            }
        }
        else
        {
            for (int i = 0; i < samples; i++)
                ((char*)b->data)[i]*= qMax(m_scaleRight, m_scaleLeft);
        }
    case Qmmp::PCM_S16LE:
        samples = b->nbytes/2;
        if (chan > 1)
        {
            for (int i = 0; i < samples; i+=2)
            {
                ((short*)b->data)[i]*= m_scaleLeft;
                ((short*)b->data)[i+1]*= m_scaleRight;
            }
        }
        else
        {
            for (int i = 0; i < samples; i++)
                ((short*)b->data)[i]*= qMax(m_scaleRight, m_scaleLeft);
        }
        break;
    case Qmmp::PCM_S24LE:
        samples = b->nbytes/4;
        if (chan > 1)
        {
            for (qint64 i = 0; i < samples; i+=2)
            {
                sample1 = ((qint32*)b->data)[i];
                sample2 = ((qint32*)b->data)[i+1];

                if (sample1 & 0x800000)
                    sample1 |= 0xff000000;

                if (sample2 & 0x800000)
                    sample2 |= 0xff000000;

                sample1 *= m_scaleLeft;
                sample2 *= m_scaleRight;

                ((qint32*)b->data)[i] = sample1;
                ((qint32*)b->data)[i+1] = sample2;
            }
        }
        else
        {
            for (qint64 i = 0; i < samples; i++)
            {
                sample1 = ((qint32*)b->data)[i];
                sample1 *= qMax(m_scaleRight, m_scaleLeft);

                if (sample1 & 0x800000)
                    sample1 |= 0xff000000;

                ((qint32*)b->data)[i] = sample1;
            }
        }
        break;
    case Qmmp::PCM_S32LE:
        samples = b->nbytes/4;
        if (chan > 1)
        {
            for (qint64 i = 0; i < samples; i+=2)
            {
                ((qint32*)b->data)[i]*= m_scaleLeft;
                ((qint32*)b->data)[i+1]*= m_scaleRight;
            }
        }
        else
        {
            for (qint64 i = 0; i < samples; i++)
                ((qint32*)b->data)[i]*= qMax(m_scaleRight, m_scaleLeft);
        }
        break;
    default:
        ;
    }
}

//static
SoftwareVolume *SoftwareVolume::instance()
{
    return m_instance;
}
