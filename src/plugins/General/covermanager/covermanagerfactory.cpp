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

#include <QtGui>
#include <qmmp/qmmp.h>
#include "covermanager.h"
#include "covermanagerfactory.h"

const GeneralProperties CoverManagerFactory::properties() const
{
    GeneralProperties properties;
    properties.name = tr("Cover Manager Plugin");
    properties.shortName = "cover_manager";
    properties.hasAbout = true;
    properties.hasSettings = false;
    properties.visibilityControl = false;
    return properties;
}

General *CoverManagerFactory::create(QObject *parent)
{
    return new CoverManager(parent);
}

QDialog *CoverManagerFactory::createConfigDialog(QWidget *)
{
    return 0;
}

void CoverManagerFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About Cover Manager Plugin"),
                        tr("Qmmp Cover Manager Plugin")+"\n"+
                        tr("Writen by: Ilya Kotov <forkotov02@hotmail.ru>"));
}

QTranslator *CoverManagerFactory::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/covermanager_plugin_") + locale);
    return translator;
}

Q_EXPORT_PLUGIN2(covermanager, CoverManagerFactory)
