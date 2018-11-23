/***************************************************************************
 *   Copyright (C) 2018 by Ilya Kotov                                      *
 *   forkotov02@ya.ru                                                      *
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
#ifndef TASKBARFACTORY_H
#define TASKBARFACTORY_H

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
#include <QObject>
#include <QDialog>

#include <qmmpui/general.h>
#include <qmmpui/generalfactory.h>

class TaskbarFactory : public QObject, public GeneralFactory
{
Q_OBJECT
Q_PLUGIN_METADATA(IID "org.qmmp.qmmpui.GeneralFactoryInterface.1.0")
Q_INTERFACES(GeneralFactory)
public:
    GeneralProperties properties() const;
    QObject *create(QObject *parent);
    QDialog *createConfigDialog(QWidget *parent);
    void showAbout(QWidget *parent);
    QString translation() const;
};

#endif
