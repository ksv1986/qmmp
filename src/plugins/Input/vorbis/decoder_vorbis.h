// Copyright (c) 2000-2001 Brad Hughes <bhughes@trolltech.com>
//
// Use, modification and distribution is allowed without limitation,
// warranty, or liability of any kind.
//

#ifndef DECODER_VORBIS_H
#define DECODER_VORBIS_H

#include <qmmp/decoder.h>

#include <vorbis/vorbisfile.h>


class DecoderVorbis : public Decoder
{
public:
    DecoderVorbis(QIODevice *i);
    virtual ~DecoderVorbis();

    // Standard Decoder API
    bool initialize() override;
    qint64 totalTime() const override;
    int bitrate() const override;

private:
    virtual qint64 read(unsigned char *data, qint64 maxSize) override;
    virtual void seek(qint64 time) override;

    // helper functions
    void deinit();
    void updateTags();
    ChannelMap findChannelMap(int channels);

    // OggVorbis Decoder
    OggVorbis_File oggfile;
    qint64 m_totalTime;
    long len;
    int m_last_section;
    int m_bitrate;
    bool m_inited;
};


#endif // __decoder_vorbis_h
