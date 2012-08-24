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

int logLevel = 4;

QByteArray readFile(const QString &filePath)
{
    QFile f(filePath);
    f.open(QIODevice::ReadOnly);
    return f.readAll();
}

void writeFile(const QString &filePath, const QByteArray &contents)
{
    QFile f(filePath);
    f.open(QIODevice::WriteOnly);
    f.write(contents);
}

#define xstr(s) str(s)
#define str(s) #s

QString naclToolchainPath()
{
    return QString(QByteArrayLiteral(xstr(NACL_TOOLCHAIN_PATH)));
}

QString naclLibraryPath(const char *bits)
{
    QString path = naclToolchainPath() + QByteArray("../x86_64-nacl/lib") + QByteArray(bits);
    return QDir(path).absolutePath();
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

QByteArray getNexeArch(const QString &nexePath)
{
    QProcess p;
    p.start(QLatin1String("/usr/bin/file"), QStringList() << nexePath);
    p.waitForFinished();
    QByteArray fileOutput = p.readAll();
    if (fileOutput.contains("x86-64"))
        return QByteArray("x86-64");
    if (fileOutput.contains("80386"))
        return QByteArray("x86-32");
    LogError() << "Unnsupported nexe architechtre" << fileOutput;
    exit(1);
    return QByteArray();
}

void createSupportFilesForNexe(const QString &outPath, const QString &nexe, const QStringList &searchPaths)
{
   //qDebug() << "createSupportFilesForNexes" << outPath << nexes;

    QString appName = QFileInfo(nexe).baseName();
    if (appName.contains("-"))
        appName = appName.split("-").at(0);
    LogDebug() << "App name" << appName;

    // Create nmf file
    QByteArray arch = getNexeArch(nexe);

    QStringList deps;
    QStringList plugins = findPlugins(nexe);
    QStringList pluginPaths = findBinaries(plugins, searchPaths);
    QStringList allBinaries;
    allBinaries.append(nexe);
    allBinaries += pluginPaths;

    deps = findDynamicDependencies(allBinaries, searchPaths);
    deps += plugins;

    QByteArray nmfFileContents = generateDynamicNmf(appName, arch, deps, "/");
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
    QFile::copy(":check_browser.js", outPath +  QLatin1String("/check_browser.js"));
    LogDebug() << "Created file" << outPath +  QLatin1String("/check_browser.js");
    QFile::copy(":qtnaclloader.js", outPath +  QLatin1String("/qtnaclloader.js"));
    LogDebug() << "Created file" << outPath +  QLatin1String("/qtnaclloader.js");
}

QString deployNexe(const QString &nexePath, const QString &outPath)
{
    QString appName = QFileInfo(nexePath).baseName();
    QByteArray arch = getNexeArch(nexePath);

    QString nexeOutPath = outPath + QLatin1Char('/') + appName + QLatin1Char('-') + arch + QLatin1String(".nexe");

    LogDebug() << "Copying nexe to" << nexeOutPath;
    QFile::copy(nexePath, nexeOutPath);
    return nexeOutPath;
}

void stripNexe(const QString &nexePath)
{
    LogDebug() << "Stripping nexe" << nexePath;
    QProcess p;
    p.start(QLatin1String("i686-nacl-strip"), QStringList() << nexePath);
    p.waitForFinished();
}

QByteArray findNexeArch(const QString &nexePath)
{
    QProcess p;
    p.start(QLatin1String("/usr/bin/file"), QStringList() << nexePath);
    p.waitForFinished();
    QByteArray fileOutput = p.readAll();

    if (fileOutput.contains("x86-64"))
        return "x86-64";
    else
        return "x86-32";
}


bool isDynamicBuild(const QString &nexePath)
{
    QProcess p;
    p.start(QLatin1String("/usr/bin/file"), QStringList() << nexePath);
    p.waitForFinished();
    QByteArray fileOutput = p.readAll();
    return fileOutput.contains("dynamically linked");
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

QString findQtPluginPath(const QString &nexePath)
{
    return QDir(findQtLibPath(nexePath) + "/../plugins/platforms").canonicalPath(); // ### platforms only
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

void deployDynamicLibraries(const QString &qtLibPath, const QStringList &libraryPaths)
{
    foreach (const QString &libraryPath ,libraryPaths) {
        QString targetPath = qtLibPath + "/" + QFileInfo(libraryPath).fileName();
        if (QFileInfo(targetPath).exists()) {
            qDebug() << "skip" << targetPath;
            continue;
        }
        qDebug() << "copy" << libraryPath << "to" << targetPath;
        QFile::copy(libraryPath, targetPath);

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
QByteArray generateDynamicNmf(const QString &appName, const QByteArray &arch, const QStringList &libs, const QString &libPath)
{
    QByteArray nmf;
    // open brace
    nmf += "{\n";

    // files
    nmf += "  \"files\": {\n";

    // nexe
    nmf += "    \"main.nexe\": {\n";
    nmf += "      \"" + arch + "\": {\n";
    nmf += "         \"url\": \"" + appName.toLatin1() + ".nexe\"\n";
    nmf += "       }\n";
    nmf += "    },\n";

    //     dynamic libraries
    foreach (const QString &lib, libs) {
        nmf += "    \"" + lib.toLatin1() + "\": {\n";
        nmf += "      \"" + arch + "\": {\n";
        nmf += "         \"url\": \"" + libPath.toLatin1() + lib.toLatin1() + "\"\n";
        nmf += "       }\n";
        nmf += "    },\n";
    }
    nmf.chop(2); // remove last ",\n"
    nmf += "\n";

    // close "files"
    nmf += "  },\n";

    // program - runnable-ld.so
    nmf += "  \"program\": {\n";
    nmf += "    \"" + arch + "\": {\n";
    nmf += "       \"url\": \"" + libPath.toLatin1() + "runnable-ld.so\"\n";
    nmf += "     }\n";
    nmf += "  }\n";

    // close brace
    nmf += "}\n";
    return nmf;
}

