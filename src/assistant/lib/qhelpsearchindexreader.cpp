/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhelpsearchindexreader_p.h"

QT_BEGIN_NAMESPACE

namespace fulltextsearch {

QHelpSearchIndexReader::QHelpSearchIndexReader()
    : QThread()
    , m_cancel(false)
{
    // nothing todo
}

QHelpSearchIndexReader::~QHelpSearchIndexReader()
{
    mutex.lock();
    this->m_cancel = true;
    mutex.unlock();

    wait();
}

void QHelpSearchIndexReader::cancelSearching()
{
    mutex.lock();
    this->m_cancel = true;
    mutex.unlock();
}

void QHelpSearchIndexReader::search(const QString &collectionFile, const QString &indexFilesFolder,
    const QList<QHelpSearchQuery> &queryList)
{
    wait();

    this->hitList.clear();
    this->m_cancel = false;
    this->m_query = queryList;
    this->m_collectionFile = collectionFile;
    this->m_indexFilesFolder = indexFilesFolder;

    start(QThread::NormalPriority);
}

int QHelpSearchIndexReader::hitCount() const
{
    QMutexLocker lock(&mutex);
    return hitList.count();
}

QList<QHelpSearchEngine::SearchHit> QHelpSearchIndexReader::hits(int start,
                                                                 int end) const
{
    QList<QHelpSearchEngine::SearchHit> hits;
    QMutexLocker lock(&mutex);
    for (int i = start; i < end && i < hitList.count(); ++i)
        hits.append(hitList.at(i));
    return hits;
}


}   // namespace fulltextsearch

QT_END_NAMESPACE
