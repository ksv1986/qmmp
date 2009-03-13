/***************************************************************************
 *   Copyright (C) 2007-2009 by Ilya Kotov                                 *
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

#ifndef OUTPUTFACTORY_H
#define OUTPUTFACTORY_H

class QObject;
class QString;
class QIODevice;
class QWidget;
class QTranslator;
class VolumeControl;
class Decoder;
class Output;

/*! @brief Helper class to store output plugin properies.
 * @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class OutputProperties
{
public:
    /*!
     * Constructor
     */
    OutputProperties()
    {
        hasAbout = FALSE;
        hasSettings = FALSE;
    }
    QString name;      /*!< Effect plugin full name */
    QString shortName; /*!< Effect plugin short name for internal usage */
    bool hasAbout;     /*!< Should be \b true if plugin has about dialog, otherwise \b false */
    bool hasSettings;  /*!< Should be \b true if plugin has settings dialog, otherwise \b false */
};
/*! @brief Output plugin interface (output factory).
 * @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class OutputFactory
{
public:
    /*!
     * Destructor.
     */
    virtual ~OutputFactory() {}
    /*!
     * Returns output plugin properties.
     */
    virtual const OutputProperties properties() const = 0;
    /*!
     * Creates output provided by plugin.
     * @param parent Parent object.
     */
    virtual Output *create(QObject *parent) = 0;
    /*!
     * Creates volume control object provided by plugin.
     * Returns \b 0 if volume control is not supported by plugin.
     * @param parent Parent object.
     */
    virtual VolumeControl *createVolumeControl(QObject *) = 0;
    /*!
     * Shows settings dialog.
     * @param parent Parent widget.
     */
    virtual void showSettings(QWidget *parent) = 0;
    /*!
     * Shows about dialog.
     * @param parent Parent widget.
     */
    virtual void showAbout(QWidget *parent) = 0;
    /*!
     * Creates QTranslator object of the system locale. Should return 0 if translation doesn't exist.
     * @param parent Parent object.
     */
    virtual QTranslator *createTranslator(QObject *parent) = 0;
};

Q_DECLARE_INTERFACE(OutputFactory, "OutputFactory/1.0")

#endif
