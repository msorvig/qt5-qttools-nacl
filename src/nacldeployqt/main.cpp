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
#include "deploynexe.h"
#include "httpserver.h"

#include <QtCore/QtCore>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString nexePath;
    QString outPath;
    bool deployToBuildDir = true;

    if (argc > 1) {
        nexePath = QString::fromLocal8Bit(argv[1]);
        outPath = QFileInfo(nexePath).absolutePath();
    }

    if (argc > 2 && !QString::fromLocal8Bit(argv[2]).startsWith("-")) {
        outPath = QString::fromLocal8Bit(argv[2]);
        deployToBuildDir = false;
    }
    
    if (argc < 2 || nexePath.startsWith("-")) {
        qDebug() << "Usage: nacldeployqt path/to/nexe [path/to/output] [options]";
        qDebug() << "";
        qDebug() << "Options:";
        qDebug() << "   -verbose=<0-3>  : 0 = no output, 1 = error/warning (default), 2 = normal, 3 = debug";
        qDebug() << "   -strip          : Strip the nexe" ;
        qDebug() << "   -startserver    : Start a http server after deploying the nexe. Useful for testing" ;
        qDebug() << "";
        qDebug() << "nacldeployqt creates server-ready Qt NaCl applications from a compiled nexe.";
        qDebug() << "";
        qDebug() << "This includes a native client manifest file (.nmf) in addition to html and javascript";
        qDebug() << "loader code. The output can be served as-is from a web server, or be used as a base for";
        qDebug() << "further modifications, for example by replacing the html but keeping the loader code.";
        qDebug() << "";
        qDebug() << "If the the output directory already contains a nexe nacldeployqt will";
        qDebug() << "do one of two things:";
        qDebug() << "1) If the new nexe is of the same CPU architechure the old one will be overwriten";
        qDebug() << "2) If the new nexe is of a different CPU architechure it will be deployed alongside the old nexe.";
        qDebug() << "   Both nexes will be renambed and have the architechture appended (wiggly-x86-nexe, wiggly-x86_64.nexe).";
        qDebug() << "   The .nmf manifest file will be updated to list both nexes.";

        return 0;
    }

    if (nexePath.endsWith("/"))
        nexePath.chop(1);
    if (outPath.endsWith("/"))
        outPath.chop(1);
    
    if (QDir().exists(nexePath) == false) {
        qDebug() << "Error: Could not find nexe" << nexePath;
        return 0;
    }

    bool staticNexe = false;
    bool server = false;
    bool strip = false;

    int logLevel = 4;
    for (int i = 2; i < argc; ++i) {
        QByteArray argument = QByteArray(argv[i]);
        if (argument.startsWith(QByteArray("-verbose"))) {
            qDebug() << "Argument found:" << argument;
            int index = argument.indexOf("=");
            bool ok = false;
            int number = argument.mid(index+1).toInt(&ok);
            if (!ok)
                qDebug() << "Could not parse verbose level";
            else
                logLevel = number;
        } else if (argument.startsWith(QByteArray("-server"))) {
            server = true;
        } else if (argument.startsWith(QByteArray("-strip"))) {
            strip = true;
        } else if (argument.startsWith("-")) {
            qDebug() << "Unknown argument" << argument << "\n";
            return 0;
        }
     }

    if (staticNexe) {
        qDebug() << "static nexes are not supported";
        return 0;
    }

    QDir().mkpath(outPath);
    nexePath = QDir(nexePath).canonicalPath();
    outPath = QDir(outPath).canonicalPath();

    LogDebug() << "";
    LogDebug() << "Source file     :" << nexePath;
    LogDebug() << "Destination dir :" << outPath;
    LogDebug() << "";
    LogDebug() << "Arch        :" << getNexeArch(nexePath);
    LogDebug() << "Build Type  :" << (isDynamicBuild(nexePath) ? "Dynamic" : "Static");
    LogDebug() << "";
    LogDebug() << "NaCl toolchain path :" << naclToolchainPath();
    QString naclLibPath = naclLibraryPath(getNexeArch(nexePath).contains("64") ? "64" : "32");
    LogDebug() << "NaCl lib path       :" << naclLibPath;
    QString qtLibPath = findQtLibPath(nexePath);
    LogDebug() << "Qt lib path         :" << qtLibPath;
    QString qtPluginPath = findQtPluginPath(nexePath);
    LogDebug() << "Qt plugin path      :" << qtPluginPath;

    QStringList searchPaths = QStringList()
            << naclLibPath << qtLibPath << qtPluginPath;

    QStringList plugins = findPlugins(nexePath);
    QStringList pluginPaths = findBinaries(plugins, searchPaths);
    QStringList allBinaries = pluginPaths;
    allBinaries.append(nexePath);

    LogDebug() << "";
    LogDebug() << "Plugins      :" << plugins;
    LogDebug() << "Plugin paths      :" << pluginPaths;
    LogDebug() << "";
    LogDebug() << "Dependencies :" << findDynamicDependencies(allBinaries, searchPaths);
    LogDebug() << "";

    QString deployedNexePath;
    if (deployToBuildDir) {
        deployedNexePath = nexePath;
    } else {
        deployedNexePath = deployNexe(nexePath, outPath);
    }

    createSupportFilesForNexe(outPath, deployedNexePath, searchPaths);

    if (strip) {
        stripNexe(deployedNexePath);
    }

    if (server) {
        LogDebug() << "";
        LogDebug() << "starting server on localhost:5103";
        Server httpServer(5103);
        httpServer.setRootPath(outPath);
        foreach(const QString &searchPath, searchPaths) {
            httpServer.addSearchPath(searchPath);
        }
        app.exec();
    }

    return 0;
}

