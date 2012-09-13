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
#include "qfunctional.h"
#include <QtCore/QString>

int logLevel = 4;

QString archx86_32(QStringLiteral("x86-32"));
QString archx86_64(QStringLiteral("x86-64"));

QByteArray readFile(const QString &filePath)
{
    QFile f(filePath);
    f.open(QIODevice::ReadOnly);
    return f.readAll();
}

void writeFile(const QString &filePath, const QByteArray &contents)
{
    QFile f(filePath);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(contents);
}

void copyFile(const QString &sourceFile, const QString &destinationFile)
{
    writeFile(destinationFile, readFile(sourceFile));
}

void deployResourceFile(const QString &fileName, const QString &outPath)
{
    QDir().mkpath(outPath);
    QString filePath = outPath + "/" + fileName;
    copyFile(":/" + fileName, outPath + "/" + fileName);
    qDebug() << "Created file" << filePath;
}

#define xstr(s) str(s)
#define str(s) #s

QString naclToolchainPath()
{
    return QString(QByteArrayLiteral(xstr(NACL_TOOLCHAIN_PATH)));
}

QString naclLibraryPath(const QString &arch)
{
    QString bits = (arch == archx86_32 ? QStringLiteral("32") : QStringLiteral("64"));
    QString path = naclToolchainPath() + QByteArray("../x86_64-nacl/lib") + bits;
    return QDir(path).absolutePath();
}

QStringList naclLibraryPath(QStringList archs)
{
    return map<QString, QString>(archs, naclLibraryPath);
}

QByteArray runFromNaclToolhainPath(const char *binaryName, const QStringList &arguments)
{
    QString binaryPath = naclToolchainPath() + QByteArray(binaryName);

    //qDebug() << "running"  << binaryName << binaryPath << arguments;

    if (binaryPath.isEmpty()) {
        qFatal("Not Found: %s", binaryName);
        return QByteArray();
    }

    QProcess p;
    p.start(binaryPath, arguments);
    p.waitForFinished();
   // qDebug() << p.errorString() << p.readAllStandardError() << p.readAllStandardOutput();

    return p.readAll();
}


QStringList operator +(const QString &prefix, const QStringList &strings)
{
    QStringList prefixedStrings;
    foreach (const QString &string, strings) {
        prefixedStrings.append(prefix + string);
    }

    return prefixedStrings;
}

QStringList findNexes(const QString &path)
{
    return path + QLatin1String("/") + QDir(path).entryList(QStringList() << "*.nexe");
}

void createSupportFilesForNexes(const QStringList &deployedNexePaths, const QList<Deployables> &deployables, const QStringList &archs, const QString &outPath)
{
    //qDebug() << "createSupportFilesForNexes" << outPath << nexes;

    // Create support files for multiple archs of the same nexe.

    QString appName = QFileInfo(deployedNexePaths.at(0)).baseName();
    if (appName.contains("-"))
        appName = appName.split("-").at(0);
    LogDebug() << "App name" << appName;

    QStringList deplyableNames = deployables.at(0).pluginNames + deployables.at(0).dynamicLibraries;

    // Create nmf file
    QByteArray nmfFileContents = generateDynamicNmf(appName, archs, deplyableNames);
    const QString &nmfFileName = outPath + QLatin1String("/") + appName + QLatin1String(".nmf");
    writeFile(nmfFileName, nmfFileContents);
    LogDebug() << "Created file" << nmfFileName;

    // Create html file
    QByteArray hmtlFileContents = readFile(QLatin1String(":fullwindowtemplate.html"));
    hmtlFileContents.replace("APPNAME", appName.toLatin1());
    const QString &htmlFileName = outPath + QLatin1String("/index.html");
    writeFile(htmlFileName, hmtlFileContents);
    LogDebug() << "Created file" << htmlFileName;

    // Copy javascript
    QFile::remove(QLatin1String("/check_browser.js"));
    QFile::copy(":check_browser.js", outPath +  QLatin1String("/check_browser.js"));
    LogDebug() << "Created file" << outPath +  QLatin1String("/check_browser.js");
    QFile::remove(QLatin1String("/qtnaclloader.js"));
    QFile::copy(":qtnaclloader.js", outPath +  QLatin1String("/qtnaclloader.js"));
    LogDebug() << "Created file" << outPath +  QLatin1String("/qtnaclloader.js");
}

QString deployNexe(const QString &nexePath, const QString &outPath)
{
    QString appName = QFileInfo(nexePath).baseName();
//    QByteArray arch = getNexeArch(nexePath);
//    QString nexeOutPath = outPath + QLatin1Char('/') + appName + QLatin1Char('-') + arch + QLatin1String(".nexe");

    QString nexeOutPath = outPath + QLatin1Char('/') + appName + QLatin1String(".nexe");

    LogDebug() << "Copying nexe to" << nexeOutPath;
    QFile::copy(nexePath, nexeOutPath);
    stripFile(nexeOutPath);
    return nexeOutPath;
}

void stripFile(const QString &filePath)
{
    runFromNaclToolhainPath("i686-nacl-strip", QStringList() << filePath);
}

QString findNexeArch(const QString &nexePath)
{
    QProcess p;
    p.start(QLatin1String("/usr/bin/file"), QStringList() << nexePath);
    p.waitForFinished();
    QByteArray fileOutput = p.readAll();

    if (fileOutput.contains("x86-64"))
        return archx86_64;
    else
        return archx86_32;
}

QStringList findNexeArch(const QStringList &nexePaths)
{
    return map<QString, QString>(nexePaths, findNexeArch);
}

bool isDynamicBuild(const QString &nexePath)
{
    QProcess p;
    p.start(QLatin1String("/usr/bin/file"), QStringList() << nexePath);
    p.waitForFinished();
    QByteArray fileOutput = p.readAll();
    return fileOutput.contains("uses shared libs");
}

QList<bool> isDynamicBuild(const QStringList &nexePaths)
{
    return map<bool, QString>(nexePaths, isDynamicBuild);
}

QString buildType(const bool &isDynamic)
{
    return isDynamic ? "Dynamic" : "Static";
}

QStringList getNexeBuildTypes(const QStringList &nexePaths)
{
    QList<bool> foo = isDynamicBuild(nexePaths);
    return map<QString, bool>(foo, buildType);
}

QString findBinary(const QString &name, const QStringList &searchPaths)
{
    foreach (const QString &searchPath, searchPaths) {
        QString foundFilePath = searchPath + "/" + name;
        if (QFile::exists(foundFilePath))
            return foundFilePath;
    }
    qDebug() << "Not found" << name;
    return QString();
}

QStringList findBinaries(const QStringList &names, const QStringList &searchPaths)
{
    QStringList binaryPaths;
    foreach (const QString &name, names) {
        QString binaryPath = findBinary(name, searchPaths);
        binaryPaths.append(binaryPath);
    }
    return binaryPaths;
}

QStringList findDynamicDependencies(const QStringList &binaryPaths, const QStringList &searchPaths)
{
    QSet<QString> toVisit; // binaries to visit (full paths)
    QSet<QString> visited; // already visited (names only), also the resulting dependency list

    toVisit = binaryPaths.toSet();

    while (!toVisit.isEmpty()) {
        QString binaryPath = *toVisit.begin();
        toVisit.remove(binaryPath);

        //qDebug() << "look at" << binaryPath;

        // Run objdump
        QByteArray objdumpOutput = runFromNaclToolhainPath("i686-nacl-objdump", QStringList() << "-x" << binaryPath);
        QList<QByteArray> lines = objdumpOutput.split('\n');

        // Find lines like "NEEDED   aLibrary.so"
        QStringList dynamicLibs;
        foreach (const QByteArray &line, lines) {
            if (line.contains("NEEDED")) {
                QList<QByteArray> parts = line.simplified().split(' ');
                if (parts.count() == 2) {
                    dynamicLibs.append(parts.at(1));
                }
            }
        }

        // "recurse"
        foreach (const QString &dynamicLib, dynamicLibs) {
            if (visited.contains(dynamicLib)) // don't search already visited binaries again
                continue;
            QString binaryPath = findBinary(dynamicLib, searchPaths);
            if (!binaryPath.isEmpty()) {
                toVisit.insert(binaryPath); // new binary, add path to todo list
                visited.insert(dynamicLib); // Add to results
            }
        }
    }

    QStringList sortedList = visited.toList();
    qSort(sortedList);
    return sortedList;
}

QStringList findDynamicDependencies(const QString &binaryPath, const QStringList &searchPaths)
{
    return findDynamicDependencies(QStringList() << binaryPath, searchPaths);
}

QStringList findPlugins(const QString &nexePath)
{
    Q_UNUSED(nexePath);

    QStringList plugins;
    plugins.append(QStringLiteral("libqtpepper.so")); // # hardcode platform plugin
    return plugins;
}

QString findNexeRPath(const QString &nexePath)
{
    QByteArray objdumpOutput = runFromNaclToolhainPath("i686-nacl-objdump", QStringList() << "-x" << nexePath);
    QList<QByteArray> lines = objdumpOutput.split('\n');
    QString rpath;
    // Find the RAPTH line
    foreach (const QByteArray &line, lines) {
        if (line.contains("RPATH")) {
            QList<QByteArray> parts = line.simplified().split(' ');
            // qDebug() << "parts" << parts;
            if (parts.count() == 2) {
                rpath =  parts.at(1);
            }
        }
    }

    return rpath;

}

QString findQtLibPath(const QString &nexePath)
{
    return findNexeRPath(nexePath);
}

QStringList findQtLibPath(const QStringList &nexePaths)
{
    return map<QString, QString>(nexePaths, findQtLibPath);
}

QString findQtPluginPath(const QString &nexePath)
{
    return QDir(findQtLibPath(nexePath) + "/../plugins/platforms").canonicalPath(); // ### platforms only
}

QStringList findQtPluginPath(const QStringList &nexePaths)
{
    return map<QString, QString>(nexePaths, findQtPluginPath);
}

QStringList findDynamicLibraries(const QStringList &libraryNames, const QStringList &searchPaths)
{
    QStringList resolvedDynamicLibraries;

    foreach (const QString &libraryName, libraryNames) {
        foreach (const QString &searchPath, searchPaths) {
            QString candidatePath = QDir(searchPath + "/" + libraryName).canonicalPath();
            if (QFileInfo(candidatePath).exists())
                resolvedDynamicLibraries.append(candidatePath);
        }
    }

    return resolvedDynamicLibraries;
}

Deployables getDeployables(const QString &nexePath, const QString &naclLibPath, const QString &qtLibPath, const QString &qtPluginPath)
{
    Deployables deployables;

    QStringList searchPaths = QStringList() << naclLibPath << qtLibPath << qtPluginPath;
    deployables.nexePath = nexePath;
    deployables.nexeName = QFileInfo(nexePath).baseName();
    deployables.pluginNames = findPlugins(nexePath);
    deployables.pluginPaths = findBinaries(deployables.pluginNames, searchPaths);
    QStringList rootBinaries = deployables.pluginPaths;
    rootBinaries.append(nexePath);
    rootBinaries.append(deployables.pluginPaths);
    rootBinaries.append(findBinary(QStringLiteral("runnable-ld.so"), searchPaths));

    deployables.dynamicLibraries = findDynamicDependencies(rootBinaries, searchPaths);
    deployables.dynamicLibraryPaths = findBinaries(deployables.dynamicLibraries, searchPaths);

    return deployables;
}

QList<Deployables> getDeployables(const QStringList &nexePaths, const QStringList &naclLibPaths, const QStringList &qtLibPaths, const QStringList &qtPluginPaths)
{
    return map<Deployables>(nexePaths, naclLibPaths, qtLibPaths, qtPluginPaths, getDeployables);
}

void deployBinaries(const QStringList &binaries, const QString &outPath)
{
   foreach (const QString &binaryPath, binaries) {
        QString targetPath = outPath + "/" + QFileInfo(binaryPath).fileName();
        if (QFileInfo(targetPath).exists()) {
            //qDebug() << "skip" << targetPath;
            //continue;
        }
        qDebug() << "copy" << binaryPath << "to" << targetPath;
        QFile::copy(binaryPath, targetPath);
        stripFile(targetPath);
    }
}

/*
{
  nmf format:
  "files": {
    "libimc_syscalls.so": {
      "x86-32": {
        "url": "lib32/libimc_syscalls.so"
      }
    },
    "main.nexe": {
      "x86-32": {
        "url": "wiggly.nexe"
      }
    }
  }, // <- "files" close
  "program": {
    "x86-32": {
      "url": "lib32/runnable-ld.so"
    }
  }
}

*/
QByteArray generateDynamicNmf(const QString &appName, const QStringList &archs, const QStringList &libs)
{
    QByteArray nmf;
    // open brace
    nmf += "{\n";

    // files
    nmf += "  \"files\": {\n";

    // nexe
    nmf += "    \"main.nexe\": {\n";

    foreach (const QString &arch, archs) {
        nmf += "      \"" + arch + "\": {\n";
        nmf += "         \"url\": \"" + arch + "/" + appName.toLatin1() + ".nexe\"\n";
        nmf += "       },\n";
    }
        nmf.chop(2); // remove last ",\n"
        nmf += "\n";

    nmf += "    },\n";

    //     dynamic libraries
    foreach (const QString &lib, libs) {
        nmf += "    \"" + lib.toLatin1() + "\": {\n";
        foreach (const QString &arch, archs) {
            nmf += "      \"" + arch + "\": {\n";
            nmf += "         \"url\": \"" + arch + "/" + lib.toLatin1() + "\"\n";
            nmf += "       },\n";
        }
            nmf.chop(2); // remove last ",\n"
            nmf += "\n";

        nmf += "    },\n";
    }
        nmf.chop(2); // remove last ",\n"
        nmf += "\n";

    // close "files"
    nmf += "  },\n";

    // program - runnable-ld.so
    nmf += "  \"program\": {\n";
    foreach (const QString &arch, archs) {
        nmf += "    \"" + arch + "\": {\n";
        nmf += "       \"url\": \"" + arch + "/" + "runnable-ld.so\"\n";
        nmf += "     },\n";
    }
        nmf.chop(2); // remove last ",\n"
        nmf += "\n";

    nmf += "  }\n";

    // close brace
    nmf += "}\n";
    return nmf;
}

QString deployNexe(const Deployables &deployables, QString arch, QString outPath)
{
    QString archedBinaryPath = outPath + "/" + arch;
    QDir().mkpath(archedBinaryPath);
    QString deployedNexePath = deployNexe(deployables.nexePath, archedBinaryPath);
    deployBinaries(deployables.dynamicLibraryPaths, archedBinaryPath);
    return deployedNexePath;
}

void deployNexes(const QList<Deployables> &deployables, QStringList archs, QString &outPath)
{
    QStringList deployedNexePaths;
    for (int i = 0; i < deployables.count(); ++i) {
        deployedNexePaths.append(deployNexe(deployables.at(i), archs.at(i), outPath));
    }

    createSupportFilesForNexes(deployedNexePaths, deployables, archs, outPath);
}


void createChromeWebStoreSupportFiles(const QString &appName, const QString &outPath)
{
    qDebug() << "";
    qDebug() << "Creating placeholder Chrome Web Store support files. Replace these";
    qDebug() << "files with acutal content, for example in a post-processing script.";
    qDebug() << "";

    // Manifest file
    deployResourceFile("manifest.json", outPath);
    // icon
    deployResourceFile("icon_128.png", outPath);

    // screnshoot1.png
    //deployResourceFile("screnshoot1.png", outPath + "/images");
    // promo_small.png
    //deployResourceFile("promo_small.png", outPath + "/images");

    QString zipFileName = (appName + QStringLiteral(".zip"));
    qDebug() << "";
    qDebug() << "To create a zip file for upload you may run:";
    qDebug() << "/usr/bin/zip -r" << zipFileName << outPath;
    qDebug() << "";

    /*QStringList arguments = QStringList() << "-r" << zipFileName << outPath;
    QProcess p;
    p.start("/usr/bin/zip", arguments);
    p.waitForFinished();
*/
}

