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

bool isServerDeployment;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString nexePath;
    QStringList nexePaths;
    QString outPath;
    int logLevel = 4;

    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        qDebug() << "arg" << i << arg;
        if (arg.endsWith(".nexe")) {
            nexePaths.append(QFileInfo(arg).absoluteFilePath());
        } else if (arg.startsWith("-")) {
            if (arg.startsWith(QByteArray("-verbose"))) {
                qDebug() << "Argument found:" << arg;
                int index = arg.indexOf("=");
                bool ok = false;
                int number = arg.mid(index+1).toInt(&ok);
                if (!ok)
                    qDebug() << "Could not parse verbose level";
                else
                    logLevel = number;
            } else if (arg.startsWith("-")) {
                qDebug() << "Unknown argument" << arg << "\n";
                return 0;
            }
        } else {
            outPath = arg;
        }
    }

    bool inPlaceServe = outPath.isEmpty();

    if (inPlaceServe && nexePaths.count() > 1) {
        qDebug() << "Error: nacldeployqt can only serve one nexe using the built-in server";
        qDebug() << "Either specify one nexe or specify an out path";
    }

    if (argc < 2) {
        qDebug() << "Usage: nacldeployqt path/to/nexe [path/to/output] [options]";
        qDebug() << "";
        qDebug() << "Options:";
        qDebug() << "   -verbose=<0-3>  : 0 = no output, 1 = error/warning (default), 2 = normal, 3 = debug";
        qDebug() << "";
        qDebug() << "nacldeployqt creates server-ready Qt NaCl applications from a compiled nexe.";
        qDebug() << "";
        qDebug() << "This includes a native client manifest file (.nmf) in addition to html and javascript";
        qDebug() << "loader code. The output can be served as-is from a web server, or be used as a base for";
        qDebug() << "further modifications, for example by replacing the html but keeping the loader code.";
        qDebug() << "";

        return 0;
    }



    foreach (QString nexePath, nexePaths) {
        if (nexePath.endsWith("/"))
            nexePath.chop(1);

        if (QDir().exists(nexePath) == false) {
            qDebug() << "Error: Could not find nexe" << nexePath;
            return 0;
        }
    }

    if (outPath.endsWith("/"))
        outPath.chop(1);

    for (int i = 2; i < argc; ++i) {
     }

    QDir().mkpath(outPath);
    nexePath = QDir(nexePath).canonicalPath();
    outPath = QDir(outPath).canonicalPath();

    LogDebug() << "";
    LogDebug() << "Source file(s)     :" << nexePaths;
    LogDebug() << "Destination dir :" << outPath;
    LogDebug() << "";
    QStringList archs = findNexeArch(nexePaths);
    LogDebug() << "Arch(s)        :" << archs;
    QStringList buildTypes = getNexeBuildTypes(nexePaths);
    LogDebug() << "Build Type(s)  :" << buildTypes;
    LogDebug() << "";
    LogDebug() << "NaCl toolchain path :" << naclToolchainPath();
    QStringList naclLibPaths = naclLibraryPath(archs);
    LogDebug() << "NaCl lib path(s)       :" << naclLibPaths;
    QStringList qtLibPaths = findQtLibPath(nexePaths);
    LogDebug() << "Qt lib path(s)         :" << qtLibPaths;
    QStringList qtPluginPaths = findQtPluginPath(nexePaths);
    LogDebug() << "Qt plugin path(s)      :" << qtPluginPaths;


    QList<Deployables> deployables = getDeployables(nexePaths, naclLibPaths, qtLibPaths, qtPluginPaths);

    foreach (const Deployables &deployable, deployables) {
        LogDebug() << "";
        LogDebug() << "Deployables for nexe" << deployable.nexePath;
        LogDebug() << "Plugins           :" << deployable.pluginNames;
        LogDebug() << "Plugin paths      :" << deployable.pluginPaths;
        LogDebug() << "";
        LogDebug() << "Dependencies :" << deployable.dynamicLibraries;
        LogDebug() << "";
//        LogDebug() << "Dependencies Paths :" << deployable.dynamicLibraryPaths;
//        LogDebug() << "";
    }

    if (inPlaceServe) {
        isServerDeployment = true;
        QString deployedNexePath;
        deployedNexePath = deployables.at(0).nexePath;
        createSupportFilesForNexes(QStringList() << deployedNexePath, deployables, archs, outPath);

        LogDebug() << "";
        LogDebug() << "starting server on localhost:5103";
        Server httpServer(5103);
        httpServer.setRootPath(outPath);
        httpServer.addSearchPath(naclLibPaths.at(0));
        httpServer.addSearchPath(qtLibPaths.at(0));
        httpServer.addSearchPath(qtPluginPaths.at(0));
        app.exec();
    } else {
        isServerDeployment = false;
        // Copy binaries for each nexe/arch
        deployNexes(deployables, archs, outPath);
        QString appName = deployables.at(0).nexeName;
        createChromeWebStoreSupportFiles(appName, outPath);
    }

    return 0;
}

