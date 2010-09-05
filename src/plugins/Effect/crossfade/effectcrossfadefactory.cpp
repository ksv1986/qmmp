/***************************************************************************
 *   Copyright (C) 2010 by Ilya Kotov                                      *
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
#include "effectcrossfadefactory.h"
#include "settingsdialog.h"
#include "crossfadeplugin.h"

const EffectProperties EffectCrossfadeFactory::properties() const
{
    EffectProperties properties;
    properties.name = tr("Crossfade Plugin");
    properties.shortName = "crossfade";
    properties.hasSettings = true;
    properties.hasAbout = true;
    properties.priority = EffectProperties::EFFECT_PRIORITY_LOW;
    return properties;
}

Effect *EffectCrossfadeFactory::create()
{
    return new CrossfadePlugin();
}

void EffectCrossfadeFactory::showSettings(QWidget *parent)
{
    SettingsDialog *dialog = new SettingsDialog(parent);
    dialog->show();
}

void EffectCrossfadeFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About Crossfade Plugin"),
                       tr("Qmmp Crossfade Plugin")+"\n"+
                       tr("Writen by: Ilya Kotov <forkotov02@hotmail.ru>"));
}

QTranslator *EffectCrossfadeFactory::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/crossfade_plugin_") + locale);
    return translator;
}

Q_EXPORT_PLUGIN2(crossfade,EffectCrossfadeFactory)
