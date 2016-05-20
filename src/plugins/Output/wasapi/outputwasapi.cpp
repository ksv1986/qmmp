/***************************************************************************
 *   Copyright (C) 2014-2016 by Ilya Kotov                                 *
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

#include <QObject>
#include <QSettings>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <qmmp/buffer.h>
#include <math.h>
#include "outputwasapi.h"

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

#define DS_BUFSIZE (96*1024)

OutputWASAPI *OutputWASAPI::instance = 0;
//VolumeDirectSound *OutputDirectSound::volumeControl = 0;
OutputWASAPI::DSoundChannels OutputWASAPI::m_dsound_pos[10]  = {
   {Qmmp::CHAN_FRONT_LEFT, SPEAKER_FRONT_LEFT},
   {Qmmp::CHAN_FRONT_RIGHT, SPEAKER_FRONT_RIGHT},
   {Qmmp::CHAN_FRONT_CENTER, SPEAKER_FRONT_CENTER},
   {Qmmp::CHAN_LFE, SPEAKER_LOW_FREQUENCY},
   {Qmmp::CHAN_REAR_LEFT, SPEAKER_BACK_LEFT},
   {Qmmp::CHAN_REAR_RIGHT, SPEAKER_BACK_RIGHT},
   {Qmmp::CHAN_REAR_CENTER, SPEAKER_BACK_CENTER},
   {Qmmp::CHAN_SIDE_LEFT, SPEAKER_SIDE_LEFT},
   {Qmmp::CHAN_SIDE_RIGHT, SPEAKER_BACK_RIGHT},
   {Qmmp::CHAN_NULL, 0}
};

OutputWASAPI::OutputWASAPI() : Output()
{
    //m_ds = 0;
    //m_primaryBuffer = 0;
    //m_dsBuffer = 0;
    m_dsBufferAt = 0;
    m_pEnumerator = 0;
    m_pDevice = 0;
    m_pAudioClient = 0;
    m_pRenderClient = 0;
    instance = this;
}

OutputWASAPI::~OutputWASAPI()
{
    instance = 0;
    uninitialize();
}

bool OutputWASAPI::initialize(quint32 freq, ChannelMap map, Qmmp::AudioFormat format)
{
    HRESULT result = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&m_pEnumerator);

    //DSBUFFERDESC bufferDesc;

    //HRESULT result = DirectSoundCreate8(0, &m_ds, 0);
    if(result != S_OK)
    {
        qWarning("OutputWASAPI: CoCreateInstance failed, error code = 0x%lx", result);
        m_pEnumerator = 0;
        return false;
    }

    if((result = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice)) != S_OK)
    {
        qWarning("OutputWASAPI: IMMDeviceEnumerator::GetDefaultAudioEndpoint failed, error code = 0x%lx", result);
        m_pDevice = 0;
        return false;
    }

    if((result = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient)) != S_OK)
    {
        qWarning("OutputWASAPI: IMMDevice::Activate failed, error code = 0x%lx", result);
        m_pAudioClient = 0;
        return false;
    }

    WAVEFORMATEXTENSIBLE wfex;
    wfex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels       = map.count();
    wfex.Format.nSamplesPerSec  = freq;
    wfex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);

    if(format == Qmmp::PCM_S16LE)
    {
        wfex.Format.wBitsPerSample = 16;
        wfex.Samples.wValidBitsPerSample = 16;
    }
    else if(format == Qmmp::PCM_S24LE)
    {
        wfex.Format.wBitsPerSample  = 32;
        wfex.Samples.wValidBitsPerSample = 24;
    }
    else if(format == Qmmp::PCM_S32LE)
    {
        wfex.Format.wBitsPerSample  = 32;
        wfex.Samples.wValidBitsPerSample = 32;
    }
    else
    {
        format = Qmmp::PCM_S16LE;
        wfex.Format.wBitsPerSample  = 16;
        wfex.Samples.wValidBitsPerSample = 16;
    }

    wfex.Format.nBlockAlign     = (wfex.Format.wBitsPerSample / 8) * wfex.Format.nChannels;
    wfex.Format.nAvgBytesPerSec = wfex.Format.nSamplesPerSec * wfex.Format.nBlockAlign;

    //generate channel order
    ChannelMap out_map;
    int i = 0;
    DWORD mask = 0;
    while(m_dsound_pos[i].pos != Qmmp::CHAN_NULL)
    {
        if(map.contains(m_dsound_pos[i].pos))
        {
            mask |= m_dsound_pos[i].chan_mask;
            out_map << m_dsound_pos[i].pos;
        }
        i++;
    }

    wfex.dwChannelMask = mask;
    wfex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    if((result = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,0, 200, 0, (WAVEFORMATEX *)&wfex, NULL)) != S_OK)
    {
        qWarning("OutputWASAPI: IAudioClient::Initialize failed, error code = 0x%lx", result);
        return false;
    }

    if((result = m_pAudioClient->GetBufferSize(&m_bufferSize)) != S_OK)
    {
        qWarning("OutputWASAPI: IAudioClient::GetBufferSize failed, error code = 0x%lx", result);
        return false;
    }

     if((result = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient)) != S_OK)
     {
         qWarning("OutputWASAPI: IAudioClient::GetService failed, error code = 0x%lx", result);
         return false;
     }

     if((result = m_pAudioClient->Start()) != S_OK)
     {
         qWarning("OutputWASAPI: IAudioClient::Start failed, error code = 0x%lx", result);
         return false;
     }

     configure(freq, out_map, format);


    /*if((result = m_ds->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY)) != DS_OK)
    {
        qWarning("OutputDirectSound: SetCooperativeLevel failed, error code = 0x%lx", result);
        return false;
    }

    ZeroMemory(&bufferDesc, sizeof(DSBUFFERDESC));
    bufferDesc.dwSize        = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags       = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_LOCHARDWARE;
    bufferDesc.dwBufferBytes = 0;
    bufferDesc.lpwfxFormat   = NULL;

    if((result = m_ds->CreateSoundBuffer(&bufferDesc, &m_primaryBuffer, NULL)) != DS_OK)
    {
        m_primaryBuffer = 0;
        qWarning("OutputDirectSound: CreateSoundBuffer failed, error code = 0x%lx", result);
        return false;
    }

    WAVEFORMATEXTENSIBLE wfex;
    wfex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfex.Format.nChannels       = map.count();
    wfex.Format.nSamplesPerSec  = freq;
    wfex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);

    if(format == Qmmp::PCM_S16LE)
    {
        wfex.Format.wBitsPerSample = 16;
        wfex.Samples.wValidBitsPerSample = 16;
    }
    else if(format == Qmmp::PCM_S24LE)
    {
        wfex.Format.wBitsPerSample  = 32;
        wfex.Samples.wValidBitsPerSample = 24;
    }
    else if(format == Qmmp::PCM_S32LE)
    {
        wfex.Format.wBitsPerSample  = 32;
        wfex.Samples.wValidBitsPerSample = 32;
    }
    else
    {
        format = Qmmp::PCM_S16LE;
        wfex.Format.wBitsPerSample  = 16;
        wfex.Samples.wValidBitsPerSample = 16;
    }

    wfex.Format.nBlockAlign     = (wfex.Format.wBitsPerSample / 8) * wfex.Format.nChannels;
    wfex.Format.nAvgBytesPerSec = wfex.Format.nSamplesPerSec * wfex.Format.nBlockAlign;

    //generate channel order
    ChannelMap out_map;
    int i = 0;
    DWORD mask = 0;
    while(m_dsound_pos[i].pos != Qmmp::CHAN_NULL)
    {
        if(map.contains(m_dsound_pos[i].pos))
        {
            mask |= m_dsound_pos[i].chan_mask;
            out_map << m_dsound_pos[i].pos;
        }
        i++;
    }

    wfex.dwChannelMask = mask;
    wfex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    if((result = m_primaryBuffer->SetFormat((WAVEFORMATEX*)&wfex)) != DS_OK)
    {
        qWarning("OutputDirectSound: SetFormat failed, error code = 0x%lx", result);
        return false;
    }

    if((result = m_primaryBuffer->Play(0, 0, DSBPLAY_LOOPING)) != DS_OK)
    {
        qWarning("OutputDirectSound: Play failed, error code = 0x%lx", result);
        return false;
    }

    ZeroMemory(&bufferDesc, sizeof(DSBUFFERDESC));
    bufferDesc.dwSize        = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags       = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |
            DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;
    bufferDesc.lpwfxFormat   = (WAVEFORMATEX*)&wfex;
    bufferDesc.dwBufferBytes = DS_BUFSIZE; // buffer size

    IDirectSoundBuffer *pDSB;
    if((result = m_ds->CreateSoundBuffer(&bufferDesc, &pDSB, NULL)) != DS_OK)
    {
        qWarning("OutputDirectSound: CreateSoundBuffer failed, error code = 0x%lx", result);
        return false;
    }

    if((result = pDSB->QueryInterface(IID_IDirectSoundBuffer8, (void**)&m_dsBuffer)) != DS_OK)
    {
        m_dsBuffer = 0;
        qWarning("OutputDirectSound: QueryInterface failed, error code = 0x%lx", result);
        pDSB->Release();
        return false;
    }

    m_dsBuffer->SetCurrentPosition(0);
    m_dsBuffer->Play(0,0,DSBPLAY_LOOPING);
    m_dsBufferAt = 0;
    configure(freq, out_map, format);
    //if(volumeControl)
    //    volumeControl->restore();*/
    return true;
}


qint64 OutputWASAPI::latency()
{
    return 0;
}

qint64 OutputWASAPI::writeAudio(unsigned char *data, qint64 len)
{
    UINT32 frames = 0;
    BYTE *pData = 0;
    DWORD flags = 0;
    m_pAudioClient->GetCurrentPadding(&frames);

    UINT32 framesAvailable = m_bufferSize - frames;

    UINT32 to_write = qMin(framesAvailable, (UINT32)len / 4);

    m_pRenderClient->GetBuffer(to_write, &pData);
    memcpy(pData, data, to_write * 4);
    m_pRenderClient->ReleaseBuffer(to_write, flags);
    return to_write * 4;


    /*unsigned char *ptr = 0, *ptr2 = 0;
    DWORD size = 0, size2 = 0;

    DWORD available = bytesToWrite(); //available bytes
    if(available < 128)
    {
        usleep(5000);
        return 0;
    }
    DWORD lockSize = qMin((DWORD)len, available); //required size

    HRESULT result = m_dsBuffer->Lock(m_dsBufferAt, lockSize,
                                      (void**)&ptr, (DWORD*)&size,
                                      (void**)&ptr2, (DWORD*)&size2, 0);
    if(result == DSERR_BUFFERLOST)
    {
        m_dsBuffer->Restore();
        result = m_dsBuffer->Lock(m_dsBufferAt, lockSize,
                                  (void**)&ptr, (DWORD*)&size,
                                  (void**)&ptr2, (DWORD*)&size2, 0);
    }
    if(result != DS_OK)
    {
        qWarning("OutputDirectSound: unable to lock buffer, error = 0x%lx", result);
        return -1;
    }

    DWORD totalSize = size + size2; //total locked size

    if(format() == Qmmp::PCM_S24LE)
    {
        for(DWORD i = 0; i < totalSize / 4; ++i)
        {
            ((quint32*) data)[i] <<= 8;
        }
    }

    memmove(ptr, data, size);
    if(size2 > 0)
        memmove(ptr2, data + size, size2);

    m_dsBuffer->Unlock((void*)ptr, size, (void*)ptr2, size2);

    m_dsBufferAt += totalSize;
    m_dsBufferAt %= DS_BUFSIZE;

    return totalSize;*/
}

void OutputWASAPI::drain()
{
    /*DWORD dsCurrentPlayCursor = 0;
    m_dsBuffer->GetCurrentPosition((DWORD*)&dsCurrentPlayCursor, 0);

    while(dsCurrentPlayCursor >= m_dsBufferAt)
    {
        m_dsBuffer->GetCurrentPosition((DWORD*)&dsCurrentPlayCursor, 0);
    }
    while (dsCurrentPlayCursor <= m_dsBufferAt)
    {
        m_dsBuffer->GetCurrentPosition((DWORD*)&dsCurrentPlayCursor, 0);
    }*/
}

void OutputWASAPI::suspend()
{
    //m_dsBuffer->Stop();
}

void OutputWASAPI::resume()
{
    /*HRESULT result = m_dsBuffer->Play(0,0,DSBPLAY_LOOPING);
    if(result == DSERR_BUFFERLOST)
    {
        result = m_dsBuffer->Play(0,0,DSBPLAY_LOOPING);
        m_dsBuffer->Restore();
    }*/
}

void OutputWASAPI::reset()
{
    //m_dsBuffer->SetCurrentPosition(m_dsBufferAt-128);
}

/*IDirectSoundBuffer8 *OutputWASAPI::secondaryBuffer()
{
    return m_dsBuffer;
}*/

void OutputWASAPI::uninitialize()
{
    m_dsBufferAt = 0;
    /*if(m_dsBuffer)
    {
        m_dsBuffer->Stop();
        m_dsBuffer->Release();
        m_dsBuffer = 0;
    }
    if(m_primaryBuffer)
    {
        m_primaryBuffer->Stop();
        m_primaryBuffer->Release();
        m_primaryBuffer = 0;
    }
    if(m_ds)
    {
        m_ds->Release();
        m_ds = 0;
    }*/
}

DWORD OutputWASAPI::bytesToWrite()
{
    /*DWORD dsCurrentPlayCursor = 0;
    m_dsBuffer->GetCurrentPosition((DWORD*)&dsCurrentPlayCursor, 0);
    long available = dsCurrentPlayCursor - m_dsBufferAt; //available bytes

    if(available < 0)
    {
        available += DS_BUFSIZE;
    }
    return available;*/
}

/***** MIXER *****/
/*VolumeDirectSound::VolumeDirectSound()
{
    OutputWASAPI::volumeControl = this;
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    m_volume.left = settings.value("DirectSound/left_volume", 100).toInt();
    m_volume.right = settings.value("DirectSound/right_volume", 100).toInt();
}

VolumeDirectSound::~VolumeDirectSound()
{
    m_volume = volume();
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("DirectSound/left_volume", m_volume.left);
    settings.setValue("DirectSound/right_volume", m_volume.right);
    OutputWASAPI::volumeControl = 0;
}

void VolumeDirectSound::setVolume(const VolumeSettings &vol)
{
    if(OutputWASAPI::instance && OutputWASAPI::instance->secondaryBuffer())
    {
        int maxVol = qMax(vol.left, vol.right);
        double voldB = -100.0, pandB = 0;
        if(maxVol)
        {
            voldB = 20.0*log(maxVol/100.0)/log(10);
            int balance = (vol.right - vol.left)*100.0/maxVol;
            pandB = balance ? 20.0*log((100.0 - fabs(balance))/100.0)/log(10) : 0;
            if(balance > 0)
                pandB = -pandB;
        }
        OutputWASAPI::instance->secondaryBuffer()->SetVolume(voldB*100);
        OutputWASAPI::instance->secondaryBuffer()->SetPan(pandB*100);
    }
    m_volume = vol;
}

VolumeSettings VolumeDirectSound::volume() const
{
    VolumeSettings vol;
    if(OutputWASAPI::instance && OutputWASAPI::instance->secondaryBuffer())
    {
        long v = 0;
        double voldB = 0, pandB = 0;
        OutputWASAPI::instance->secondaryBuffer()->GetVolume(&v);
        voldB = v / 100.0;
        OutputWASAPI::instance->secondaryBuffer()->GetPan(&v);
        pandB = v / 100.0;
        int volume = 100*pow(10, voldB/20.0);
        int balance = 100 - 100*pow(10, abs(pandB)/20.0);
        if(pandB > 0)
            balance = -balance;
        vol.left = volume-qMax(balance,0)*volume/100.0;
        vol.right = volume+qMin(balance,0)*volume/100.0;
        return vol;
    }
    return m_volume;
}

void VolumeDirectSound::restore()
{
    setVolume(m_volume);
}*/
