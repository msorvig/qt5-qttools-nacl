/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef DEPLOYNEXE_H
#define DEPLOYNEXE_H

#include <QtCore/QtCore>

//extern int logLevel; // extern no working
#define LogError()      if (logLevel < 1) {} else qDebug() << "ERROR:"
#define LogWarning()    if (logLevel < 1) {} else qDebug() << "WARNING:"
#define LogNormal()     if (logLevel < 2) {} else qDebug() << "Log:"
#define LogDebug()      if (logLevel < 3) {} else qDebug() << "Log:"

QStringList findNexes(const QString &path);
QByteArray getNexeArch(const QString &nexePath);
QString naclToolchainPath();
QString naclLibraryPath(const char *bits);
bool isDynamicBuild(const QString &nexePath);
QStringList findDynamicDependencies(const QString &nexePath);
QStringList findDynamicLibraries(const QStringList &libraryNames, const QStringList &searchPaths);
void deployDynamicLibraries(const QString &qtLibPath, const QStringList &libraryPaths);
QByteArray generateDynamicNmf(const QString &appName, const QByteArray &arch, const QStringList &libs, const QString &libPath);
void createSupportFilesForNexes(const QString &outPath, const QStringList &nexes);
QString deployNexe(const QString &nexePath, const QString &outPath);
void stripNexe(const QString &nexePath);

#endif
