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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef INPUTSOURCEFACTORY_H
#define INPUTSOURCEFACTORY_H

#include <QObject>

class QTranslator;
class InputSource;

/*! @brief Helper class to store transport plugin properies.
 * @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class InputSourceProperties
{
public:
    /*!
     * Constructor
     */
    InputSourceProperties()
    {
        hasSettings = false;
        hasAbout = false;
    }

    QString name;      /*!< Transport plugin full name */
    QString shortName; /*!< Transport plugin name for internal usage */
    QString protocols; /*!< Supported protocols. */
    bool hasAbout;     /*!< Should be \b true if plugin has about dialog, otherwise \b false */
    bool hasSettings;  /*!< Should be \b true if plugin has settings dialog, otherwise \b false */
};


/*! @brief Transport plugin interface.
 * @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class InputSourceFactory
{
public:
    /*!
     * Returns transport plugin properties.
     */
    virtual const InputSourceProperties properties() const = 0;
    /*!
     * Creates transport provided by plugin.
     * @param url URL of the stream.
     * @param parent Parent object.
     */
    virtual InputSource *create(const QString &url, QObject *parent = 0) = 0;
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

Q_DECLARE_INTERFACE(InputSourceFactory, "InputSourceFactory/1.0");

#endif // INPUTSOURCEFACTORY_H

