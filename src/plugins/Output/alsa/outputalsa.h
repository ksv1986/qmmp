/***************************************************************************
 *   Copyright (C) 2006-2012 by Ilya Kotov                                 *
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

#ifndef   OUTPUTALSA_H
#define   OUTPUTALSA_H

class OutputALSA;


#include <QObject>
extern "C"
{
#include <alsa/asoundlib.h>
}

#include <qmmp/output.h>
#include <qmmp/volume.h>


class OutputALSA : public Output
{
    Q_OBJECT
public:
    OutputALSA(QObject * parent = 0);
    ~OutputALSA();

    bool initialize(quint32, int, Qmmp::AudioFormat format);
    qint64 latency();

private:
    //output api
    qint64 writeAudio(unsigned char *data, qint64 maxSize);
    void drain();
    void reset();
    void suspend();
    void resume();

    // helper functions
    long alsa_write(unsigned char *data, long size);
    void uninitialize();

    bool m_inited;
    bool m_use_mmap;
    //alsa
    snd_pcm_t *pcm_handle;
    char *pcm_name;
    snd_pcm_uframes_t m_chunk_size;
    size_t m_bits_per_frame;
    //prebuffer
    uchar *m_prebuf;
    qint64 m_prebuf_size;
    qint64 m_prebuf_fill;
    bool m_can_pause;
};

class VolumeALSA : public Volume
{
public:
    VolumeALSA();
    ~VolumeALSA();

    void setVolume(int channel, int value);
    int volume(int channel);

private:
    //alsa mixer
    int setupMixer(QString card, QString device);
    void parseMixerName(char *str, char **name, int *index);
    int getMixer(snd_mixer_t **mixer, QString card);
    snd_mixer_elem_t* getMixerElem(snd_mixer_t *mixer, char *name, int index);
    snd_mixer_t *mixer;
    snd_mixer_elem_t *pcm_element;
    bool m_use_mmap;
};

#endif // OUTPUTALSA_H
