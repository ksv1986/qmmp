// Copyright (c) 2000-2001 Brad Hughes <bhughes@trolltech.com>
//
// Use, modification and distribution is allowed without limitation,
// warranty, or liability of any kind.
//

#ifndef   __buffer_h
#define   __buffer_h

#include <QMap>
#include <QSharedPointer>
#include <stddef.h>
#include "qmmp.h"

#define QMMP_BLOCK_FRAMES 512

/*! @brief Audio buffer class.
 * @author Brad Hughes <bhughes@trolltech.com>
 */
class Buffer
{
public:
    /*!
     * Constructs an empty buffer object.
     * @param sz Size in samples;
     */
    Buffer(size_t sz)
    {
        data = new float[sz];
        samples = 0;
        rate = 0;
        size = sz;
    }
    /*!
     * Destructor.
     */
    ~Buffer()
    {
        delete[] data;
        data = 0;
        samples = 0;
        rate = 0;
        size = 0;
    }

    float *data;         /*!< Audio data */
    size_t samples;      /*!< Audio data size in samples */
    size_t size;         /*!< Buffer size in samples */
    unsigned int rate;   /*!< Buffer bitrate */
    QSharedPointer<QMap<Qmmp::MetaData, QString> > metaData; /*!< Indicates that new metadata is available */
};


#endif // __buffer_h
