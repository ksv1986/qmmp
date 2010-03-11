/**************************************************************************
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

#include "qmmpfiledialogimpl.h"

#include <QDirModel>
#include <QApplication>
#include <QFileInfo>
#include <QStyle>
#include <QSettings>
#include <QMessageBox>
#include <QHeaderView>

#include <qmmp/qmmp.h>

#define HISTORY_SIZE 8


/**
 *   This variable has been copied from Qt library
 */

const char *qt_file_dialog_filter_reg_exp =
    "([a-zA-Z0-9 -]*)\\(([a-zA-Z0-9_.*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

/**
 *   This function has been copied from Qt library
 */

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
static QStringList qt_clean_filter_list(const QString &filter)
{
    QRegExp regexp(QString::fromLatin1(qt_file_dialog_filter_reg_exp));
    QString f = filter;
    int i = regexp.indexIn(f);
    if (i >= 0)
        f = regexp.cap(2);
    return f.split(QLatin1Char(' '), QString::SkipEmptyParts);
}



QmmpFileDialogImpl::QmmpFileDialogImpl(QWidget * parent, Qt::WindowFlags f) : QDialog(parent,f)
{
    setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, FALSE);
#if QT_VERSION >= 0x040400
    m_model = new QFileSystemModel(this);
    m_model->setNameFilterDisables (FALSE);
    m_model->setReadOnly(FALSE);
#else
    m_model = new QDirModel(this);
    m_model->setSorting(QDir::Type);
#endif

    fileListView->setModel(m_model);
    treeView->setModel(m_model);
    treeView->setSortingEnabled(true);
    treeView->setItemsExpandable(FALSE);
    treeView->header()->setSortIndicator(0, Qt::AscendingOrder);
    treeView->header()->setStretchLastSection (FALSE);
    listToolButton->setChecked(true);
    upToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    listToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_FileDialogListView));
    closeOnAddToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOkButton));
    detailsToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(fileListView->selectionModel(),
            SIGNAL(selectionChanged (const QItemSelection&, const QItemSelection&)),
            SLOT(updateSelection ()));
    connect(treeView->selectionModel(),
            SIGNAL(selectionChanged (const QItemSelection&, const QItemSelection&)),
            SLOT(updateSelection ()));
    PathCompleter* completer = new PathCompleter (m_model, fileListView, this);
    fileNameLineEdit->setCompleter (completer);
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    closeOnAddToolButton->setChecked(settings.value("QMMPFileDialog/close_on_add", FALSE).toBool());
    restoreGeometry(settings.value("QMMPFileDialog/geometry").toByteArray());
    m_history = settings.value("QMMPFileDialog/history").toStringList();
    lookInComboBox->addItems(m_history);
    lookInComboBox->setMaxCount(HISTORY_SIZE);
    QCompleter* dir_completer = new QCompleter (m_model, this);
    lookInComboBox->setCompleter (dir_completer);
}

QmmpFileDialogImpl::~QmmpFileDialogImpl()
{
}

QStringList QmmpFileDialogImpl::selectedFiles ()
{
    QStringList l;
    if (m_mode == FileDialog::SaveFile)
    {
        l << m_model->filePath(fileListView->rootIndex()) + "/" + fileNameLineEdit->text();
        qDebug("%s",qPrintable(l[0]));
    }
    else
    {
        QModelIndexList ml = fileListView->selectionModel()->selectedIndexes();
        foreach(QModelIndex i,ml)
        l << m_model->filePath(i);
    }
    return l;
}

void QmmpFileDialogImpl::on_lookInComboBox_activated(const QString &path)
{
    if (QDir(path).exists ())
    {
        fileListView->setRootIndex(m_model->index(path));
        treeView->setRootIndex(m_model->index(path));
#if QT_VERSION >= 0x040400
        m_model->setRootPath(path);
#endif
    }
}

void QmmpFileDialogImpl::on_upToolButton_clicked()
{
    if (!m_model->parent(fileListView->rootIndex()).isValid())
        return;
    fileListView->setRootIndex(m_model->parent(fileListView->rootIndex()));
    treeView->setRootIndex(fileListView->rootIndex());
    lookInComboBox->setEditText(m_model->filePath(fileListView->rootIndex()));
    fileListView->selectionModel()->clear ();
#if QT_VERSION >= 0x040400
    m_model->setRootPath(m_model->filePath(treeView->rootIndex()));
#endif
}

void QmmpFileDialogImpl::on_treeView_doubleClicked(const QModelIndex& ind)
{
    if (ind.isValid())
    {
        QFileInfo info = m_model->fileInfo(ind);
        if (info.isDir())
        {
            treeView->setRootIndex(ind);
            lookInComboBox->setEditText(m_model->filePath(ind));
            treeView->selectionModel()->clear ();
            fileListView->setRootIndex(ind);
            fileListView->selectionModel()->clear ();
#if QT_VERSION >= 0x040400
            m_model->setRootPath(m_model->filePath(ind));
#endif
        }
        else
        {
            QStringList l;
            l << m_model->filePath(ind);
            addToHistory(l[0]);
            addFiles(l);
        }
    }
}

void QmmpFileDialogImpl::on_fileListView_doubleClicked(const QModelIndex& ind)
{
    if (ind.isValid())
    {
        QFileInfo info = m_model->fileInfo(ind);
        if (info.isDir())
        {
            fileListView->setRootIndex(ind);
            lookInComboBox->setEditText(m_model->filePath(ind));
            fileListView->selectionModel()->clear ();
            treeView->setRootIndex(ind);
            treeView->selectionModel()->clear ();
#if QT_VERSION >= 0x040400
            m_model->setRootPath(m_model->filePath(ind));
#endif
        }
        else
        {
            QStringList l;
            l << m_model->filePath(ind);
            addToHistory(l[0]);
            addFiles(l);
        }
    }
}

void QmmpFileDialogImpl::on_fileNameLineEdit_returnPressed()
{
    qWarning("TODO: %s    %d", __FILE__, __LINE__);
}

void QmmpFileDialogImpl::on_fileNameLineEdit_textChanged (const QString &text)
{
    if (m_mode == FileDialog::SaveFile)
    {
        addPushButton->setEnabled(!text.isEmpty());
        return;
    }
    QModelIndex index;
    if (text.startsWith("/"))
        index = m_model->index(text);
    else
        index = m_model->index(m_model->filePath(fileListView->rootIndex()) + "/" + text);
    if (!index.isValid() || !fileNameLineEdit->hasFocus())
        return;
    fileListView->selectionModel()->clear();
    fileListView->selectionModel()->select(index, QItemSelectionModel::Select);
}

void QmmpFileDialogImpl::on_addPushButton_clicked()
{
    QStringList l;
    if (m_mode != FileDialog::SaveFile)
    {
        QModelIndexList ml;
        if (stackedWidget->currentIndex() == 0)
            ml = fileListView->selectionModel()->selectedIndexes();
        else
            ml = treeView->selectionModel()->selectedIndexes();
        foreach(QModelIndex i,ml)
        {
            if (!l.contains(m_model->filePath(i)))
                l << m_model->filePath(i);
        }
        if (!l.isEmpty())
        {
            addToHistory(l[0]);
            addFiles(l);
            return;
        }
    }
    else
    {
        l << m_model->filePath(fileListView->rootIndex()) + "/" + fileNameLineEdit->text();
        addFiles(l);
    }
}

void QmmpFileDialogImpl::setModeAndMask(const QString& d,FileDialog::Mode m, const QStringList& mask)
{
    m_mode = m;
    fileListView->clearSelection ();
    treeView->clearSelection ();
    fileTypeComboBox->clear();
    addPushButton->setEnabled(FALSE);
    addPushButton->setText(tr("Add"));

    QString fileName;
    QString path = d;

    if (m == FileDialog::SaveFile)
    {
        if (path.endsWith('/'))
            path.remove(path.size()-1, 1);
        path = path.left(path.lastIndexOf ('/'));
        fileName = d.section('/', -1);
        fileNameLineEdit->setText(fileName);
        addPushButton->setEnabled(!fileName.isEmpty());
        addPushButton->setText(tr("Save"));
    }
    if (!QFile::exists(path))
        path = QDir::home ().path ();
    if (m_model->filePath(fileListView->rootIndex()) != path)
    {
        fileListView->setRootIndex(m_model->index(path));
        treeView->setRootIndex(m_model->index(path));
#if QT_VERSION >= 0x040400
        m_model->setRootPath(path);
#endif
    }

    if (m == FileDialog::AddDirs || m == FileDialog::AddDir)
    {
        m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot); //dirs only
        fileTypeComboBox->addItem(tr("Directories"));
        fileTypeComboBox->setEnabled(FALSE);
    }
    else
    {
        m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        fileTypeComboBox->setEnabled(true);
        fileTypeComboBox->addItems(mask);
        on_fileTypeComboBox_activated(0);
    }

    //set selection mode
    if (m == FileDialog::AddDir ||  m == FileDialog::AddFile || m == FileDialog::SaveFile)
    {
        fileListView->setSelectionMode (QAbstractItemView::SingleSelection);
        treeView->setSelectionMode (QAbstractItemView::SingleSelection);
    }
    else
    {
        fileListView->setSelectionMode (QAbstractItemView::ExtendedSelection);
        treeView->setSelectionMode (QAbstractItemView::ExtendedSelection);
    }

    lookInComboBox->setEditText(QDir::cleanPath(path));
}

void QmmpFileDialogImpl::on_listToolButton_toggled(bool yes)
{
    if (yes)
    {
        stackedWidget->setCurrentIndex(0);
    }
}

void QmmpFileDialogImpl::on_detailsToolButton_toggled(bool yes)
{
    if (yes)
    {
        stackedWidget->setCurrentIndex(1);
    }
}

void QmmpFileDialogImpl::on_fileTypeComboBox_activated(int index)
{
    m_model->setNameFilters(qt_clean_filter_list(fileTypeComboBox->itemText(index)));
}

void QmmpFileDialogImpl::hideEvent (QHideEvent *event)
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("QMMPFileDialog/close_on_add", closeOnAddToolButton->isChecked());
    settings.setValue("QMMPFileDialog/geometry", saveGeometry());
    settings.setValue("QMMPFileDialog/history", m_history);
    QWidget::hideEvent(event);
}

void QmmpFileDialogImpl::updateSelection ()
{
    QModelIndexList ml;
    if (stackedWidget->currentIndex() == 0)
        ml = fileListView->selectionModel()->selectedIndexes();
    else
        ml = treeView->selectionModel()->selectedIndexes();
    QStringList l;
    QStringList files;
    foreach(QModelIndex i,ml)
    {
        if (!l.contains(m_model->filePath(i).section("/", -1)))
        {
            files << m_model->filePath(i);
            l << m_model->filePath(i).section("/", -1);
        }
    }

    if (!l.isEmpty())
    {
        QString str;
        if (l.size() == 1)
            str = l.at(0);
        else
        {
            str = l.join ("\" \"");
            str.append("\"");
            str.prepend("\"");
        }
        if (!fileNameLineEdit->hasFocus())
            fileNameLineEdit->setText(str);
        if (m_mode == FileDialog::AddFiles || m_mode == FileDialog::AddFile/* || FileDialog::SaveFile*/)
        {
            addPushButton->setEnabled(true);
            foreach(str, files)
            {
                if (QFileInfo(str).isDir())
                {
                    addPushButton->setEnabled(FALSE);
                    break;
                }
            }
        }
        else
            addPushButton->setEnabled(true);
    }
    else
    {
        fileNameLineEdit->clear();
        addPushButton->setEnabled(FALSE);
    }
}

void QmmpFileDialogImpl::addToHistory(const QString &path)
{
    QString path_copy = path;
    if (path_copy.endsWith('/'))
        path_copy.remove(path.size()-1, 1);
    QString dir_path = path.left(path_copy.lastIndexOf ('/'));

    m_history.removeAll(dir_path);
    m_history.prepend(dir_path);

    while ( m_history.size() > HISTORY_SIZE)
        m_history.removeLast();

    lookInComboBox->clear();
    lookInComboBox->addItems(m_history);
}

void QmmpFileDialogImpl::addFiles(const QStringList &list)
{
    if (list.isEmpty())
        return;
    if (!isModal())
    {
        emit filesAdded(list);
        if (closeOnAddToolButton->isChecked())
            reject();
    }
    else if (m_mode == FileDialog::SaveFile)
    {
        //check file extension
        QString f_name = fileNameLineEdit->text();
        bool contains = FALSE;
        foreach(QString str, qt_clean_filter_list(fileTypeComboBox->currentText()))
        {
            QRegExp regExp(str);
            regExp.setPatternSyntax(QRegExp::Wildcard);
            if (f_name.contains(regExp))
            {
                contains = true;
                break;
            }
        }
        //add extensio to file name
        if (!contains)
        {
            QString ext = qt_clean_filter_list(fileTypeComboBox->currentText())[0];
            ext.remove("*");
            if (!ext.isEmpty() && ext != ".")
            {
                f_name.append(ext);
                qDebug("QmmpFileDialogImpl: added file extension");
                fileNameLineEdit->setText(f_name);
                return;
            }
        }
        QFileInfo info(list[0]);

        if (info.exists())
        {
            if (QMessageBox::question (this, windowTitle (),
                                       tr("%1 already exists.\nDo you want to replace it?")
                                       .arg(fileNameLineEdit->text()),
                                       QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
                accept();
            else
                return;

        }
        accept();
    }
    else
        accept();
}
