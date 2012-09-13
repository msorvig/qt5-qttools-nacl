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
QString naclToolchainPath();
QString naclLibraryPath(QString arch);
QStringList naclLibraryPath(QStringList archs);
bool isDynamicBuild(const QString &nexePath);
QList<bool> isDynamicBuilds(const QStringList &nexePaths);
QStringList getNexeBuildTypes(const QStringList &nexePaths);

QString findBinary(const QString &name, const QStringList &searchPaths);
QStringList findBinaries(const QStringList &names, const QStringList &searchPaths);
QStringList findDynamicDependencies(const QString &nexePath, const QStringList &searchPaths);
QStringList findDynamicDependencies(const QStringList &binaryPaths, const QStringList &searchPaths);
QStringList findPlugins(const QString & nexePath);
QString findNexeRPath(const QString &nexePath);
QString findNexeArch(const QString &nexePath);
QStringList findNexeArch(const QStringList &nexePaths);
QString findQtLibPath(const QString &nexePath);
QStringList findQtLibPath(const QStringList &nexePaths);
QString findQtPluginPath(const QString &nexePath);
QStringList findQtPluginPath(const QStringList &nexePaths);

struct Deployables {
    QString nexeName;
    QString nexePath;
    QStringList pluginNames;
    QStringList pluginPaths;
    QStringList dynamicLibraries;
    QStringList dynamicLibraryPaths;
};

QDebug operator<< (QDebug d, const Deployables &deployables);

Deployables getDeployables(const QString &nexePath, const QString &naclLibPaths, const QString &qtLibPaths, const QString &qtPluginPaths);
QList<Deployables> getDeployables(const QStringList &nexePath, const QStringList &naclLibPaths, const QStringList &qtLibPaths, const QStringList &qtPluginPaths);

void deployBinaries(const QStringList &binaries, const QString &outPath);
QByteArray generateDynamicNmf(const QString &appName, const QStringList &archs, const QStringList &libs);
void createSupportFilesForNexes(const QStringList &deployedNexePaths, const QList<Deployables> &deployables, const QStringList &archs, const QString &outPath);
QString deployNexe(const QString &nexePath, const QString &outPath);
void stripFile(const QString &filePath);

void deployNexes(const QList<Deployables> &deployables, QStringList archs, QString &outPath);

void createChromeWebStoreSupportFiles(const QString &appName, const QString &outpPath);

#endif
