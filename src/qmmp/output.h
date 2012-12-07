
// Copyright (c) 2000-2001 Brad Hughes <bhughes@trolltech.com>
//
// Use, modification and distribution is allowed without limitation,
// warranty, or liability of any kind.
//

#ifndef OUTPUT_H
#define OUTPUT_H

#include <QObject>
#include <QThread>
#include <QEvent>
#include <QList>
#include <QHash>
#include <QIODevice>
#include "visual.h"
#include "outputfactory.h"
#include "visualfactory.h"
#include "statehandler.h"
#include "audioparameters.h"

class QTimer;
class QmmpSettings;

/*! @brief The Output class provides the base interface class of audio outputs.
 * @author Brad Hughes <bhughes@trolltech.com>
 * @author Ilya Kotov <forkotov@hotmail.ru>
 */
class Output
{
public:
    /*!
     * Object contsructor.
     * @param parent Parent object.
     */
    Output();
    /*!
     * Destructor.
     */
    ~Output();
    /*!
     * Prepares object for usage and setups required audio parameters.
     * Subclass should reimplement this function.
     * @param freq Sample rate.
     * @param chan Number of channels.
     * @param format Audio format
     * @return initialization result (\b true - success, \b false - failure)
     */
    virtual bool initialize(quint32 freq, int chan, Qmmp::AudioFormat format) = 0;
    /*!
     * Returns output interface latency in milliseconds.
     */
    virtual qint64 latency() = 0;
    /*!
     * Requests playback to pause. If it was paused already, playback should resume.
     * Subclasses that reimplement this function must call the base implementation.
     */
    /*!
     * Writes up to \b maxSize bytes from \b data to the output interface device.
     * Returns the number of bytes written, or -1 if an error occurred.
     * Subclass should reimplement this function.
     */
    virtual qint64 writeAudio(unsigned char *data, qint64 maxSize) = 0;
    /*!
     * Writes all remaining plugin's internal data to audio output device.
     * Subclass should reimplement this function.
     */
    virtual void drain() = 0;
    /*!
     * Drops all plugin's internal data, resets audio device
     * Subclass should reimplement this function.
     */
    virtual void reset() = 0;
    /*!
     * Stops processing audio data, preserving buffered audio data.
     */
    virtual void suspend();
    /*!
     * Resumes processing audio data.
     */
    virtual void resume();
    /*!
     * Returns selected audio parameters.
     */
    AudioParameters audioParameters() const;
    /*!
     * Returns samplerate.
     */
    quint32 sampleRate();
    /*!
     * Returns channels number.
     */
    int channels();
    /*!
     * Returns selected audio format.
     */
    Qmmp::AudioFormat format() const;
    /*!
     * Returns sample size in bytes.
     */
    int sampleSize() const;
    /*!
     * Creates selected output.
     * @return Output subclass object.
     */
    static Output *create();
    /*!
     * Returns a list of output factories.
     */
    static QList<OutputFactory*> *factories();
    /*!
     * Returns plugin file path.
     * @param factory Output plugin factory.
     */
    static QString file(OutputFactory *factory);
    /*!
     * Selects current output \b factory.
     */
    static void setCurrentFactory(OutputFactory* factory);
    /*!
     * Returns selected output factory.
     */
    static OutputFactory *currentFactory();

protected:
    /*!
     * Use this function inside initialize() reimplementation to tell about accepted audio parameters.
     * @param freq Sample rate.
     * @param chan Number of channels.
     * @param format Audio format.
     */
    void configure(quint32 freq, int chan, Qmmp::AudioFormat format);

private:
    quint32 m_frequency;
    int m_channels;
    Qmmp::AudioFormat m_format;
    static void checkFactories();
    static QList<OutputFactory*> *m_factories;
    static QHash <OutputFactory*, QString> *m_files;
};


#endif // OUTPUT_H
