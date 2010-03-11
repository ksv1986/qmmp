/***************************************************************************
 *   Copyright (C) 2008-2009 by Ilya Kotov                                 *
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

#ifndef VISUALFACTORY_H
#define VISUALFACTORY_H

class QObject;
class QWidget;
class QTranslator;
class QDialog;
class Visual;

/*! @brief Helper class to store visual plugin properies.
 *  @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class VisualProperties
{
public:
    /*!
     * Constructor
     */
    VisualProperties()
    {
        hasAbout = false;
        hasSettings = false;
    }
    QString name;      /*!< Effect plugin full name */
    QString shortName; /*!< Effect plugin short name for internal usage */
    bool hasAbout;     /*!< Should be \b true if plugin has about dialog, otherwise returns \b false */
    bool hasSettings;  /*!< Should be \b true if plugin has settings dialog, otherwise returns \b false */
};
/*! @brief %Visual plugin interface (visual factory).
 * @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class VisualFactory
{
public:
    /*!
     * Destructor.
     */
    virtual ~VisualFactory() {}
     /*!
     * Returns visual plugin properties.
     */
    virtual const VisualProperties properties() const = 0;
    /*!
     * Creates visualization provided by plugin.
     * @param parent Parent object.
     */
    virtual Visual *create(QWidget *parent) = 0;
    /*!
     * Creates configuration dialog.
     * @param parent Parent widget.
     * @return Configuration dialog pointer.
     */
    virtual QDialog *createConfigDialog(QWidget *parent) = 0;
    /*!
     * Shows about dialog.
     * @param parent Parent widget.
     */
    virtual void showAbout(QWidget *parent) = 0;
    /*!
     * Creates QTranslator object of the system locale. Should return \b 0 if translation doesn't exist.
     * @param parent Parent object.
     */
    virtual QTranslator *createTranslator(QObject *parent) = 0;
};

Q_DECLARE_INTERFACE(VisualFactory, "VisualFactory/1.0");

#endif
