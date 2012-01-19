/***************************************************************************
 *   Copyright (C) 2009-2012 by Ilya Kotov                                 *
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

#include <QSettings>
#include <QDir>
#include <QPluginLoader>
#include <QApplication>
#include "enginefactory.h"
#include "qmmpaudioengine_p.h"
#include "statehandler.h"
#include "qmmp.h"
#include "abstractengine.h"

AbstractEngine::AbstractEngine(QObject *parent) : QThread(parent)
{
}

QMutex *AbstractEngine::mutex()
{
    return &m_mutex;
}

// static methods
QList<EngineFactory*> *AbstractEngine::m_factories = 0;
QList<EngineFactory*> *AbstractEngine::m_disabledFactories = 0;
QHash <EngineFactory*, QString> *AbstractEngine::m_files = 0;

void AbstractEngine::checkFactories()
{
    if (!m_factories)
    {
        QSettings settings (Qmmp::configFile(), QSettings::IniFormat);
        QStringList disabledNames  = settings.value("Engine/disabled_plugins").toStringList ();
        m_files = new QHash <EngineFactory*, QString>;
        m_factories = new QList<EngineFactory *>;
        m_disabledFactories = new QList<EngineFactory *>;

        QDir pluginsDir (Qmmp::pluginsPath());
        pluginsDir.cd("Engines");
        foreach (QString fileName, pluginsDir.entryList(QDir::Files))
        {
            QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
            QObject *plugin = loader.instance();
            if (loader.isLoaded())
                qDebug("AbstractEngine: plugin loaded - %s", qPrintable(fileName));
            else
                qWarning("AbstractEngine: %s", qPrintable(loader.errorString ()));
            EngineFactory *factory = 0;
            if (plugin)
                factory = qobject_cast<EngineFactory *>(plugin);

            if (factory)
            {
                m_factories->append(factory);
                m_files->insert(factory, pluginsDir.absoluteFilePath(fileName));
                qApp->installTranslator(factory->createTranslator(qApp));
                if(disabledNames.contains(factory->properties().shortName))
                    m_disabledFactories->append(factory);
            }
        }
        //remove physically deleted plugins from disabled list
        QStringList names;
        foreach (EngineFactory *factory, *m_factories)
        {
            names.append(factory->properties().shortName);
        }
        foreach (QString name, disabledNames)
        {
            if (!names.contains(name))
                disabledNames.removeAll(name);
        }
        settings.setValue("Engine/disabled_plugins", disabledNames);
    }
}

AbstractEngine *AbstractEngine::create(InputSource *s, QObject *parent)
{
    AbstractEngine *engine = new QmmpAudioEngine(parent); //internal engine
    if(!engine->enqueue(s))
    {
        engine->deleteLater();
        engine = 0;
    }
    else
        return engine;


    checkFactories();
    foreach(EngineFactory *f, *m_factories)
    {
        if(!isEnabled(f))
            continue;
        engine = f->create(parent); //engine plugin
        if(!engine->enqueue(s))
        {
            engine->deleteLater();
            engine = 0;
        }
        else
            break;
    }
    return engine;
}

QList<EngineFactory*> *AbstractEngine::factories()
{
    checkFactories();
    return m_factories;
}

EngineFactory *AbstractEngine::findByPath(const QString& source)
{
    checkFactories();
    foreach(EngineFactory *fact, *m_factories)
    {
        if (fact->supports(source) && isEnabled(fact))
            return fact;
    }
    return 0;
}

void AbstractEngine::setEnabled(EngineFactory* factory, bool enable)
{
    checkFactories();
    if (!m_factories->contains(factory))
        return;

    if(enable == isEnabled(factory))
        return;

    if(enable)
        m_disabledFactories->removeAll(factory);
    else
        m_disabledFactories->append(factory);

    QStringList disabledNames;
    foreach(EngineFactory *f, *m_disabledFactories)
    {
        disabledNames << f->properties().shortName;
    }
    disabledNames.removeDuplicates();
    QSettings settings (Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("Engine/disabled_plugins", disabledNames);
}

bool AbstractEngine::isEnabled(EngineFactory* factory)
{
    checkFactories();
    return !m_disabledFactories->contains(factory);
}

QString AbstractEngine::file(EngineFactory *factory)
{
    checkFactories();
    return m_files->value(factory);
}

QStringList AbstractEngine::protocols()
{
    checkFactories();
    QStringList protocolsList;
    foreach(EngineFactory *f, *m_factories)
    {
        if(isEnabled(f))
            protocolsList << f->properties().protocols;
    }
    protocolsList.removeDuplicates();
    return protocolsList;
}

