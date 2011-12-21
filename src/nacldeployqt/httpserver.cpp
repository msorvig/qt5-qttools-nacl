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

#include "httpserver.h"
#include "time.h"

HttpRequest::HttpRequest()
{
}

HttpRequest::HttpRequest(const QList<QByteArray> &text)
:m_text(text) { parseText(); }

HttpRequest::HttpRequest(QTcpSocket *socket)
:m_socket(socket) { readText(); parseText(); }

QByteArray HttpRequest::path()
{
    return m_path;
}

QByteArray HttpRequest::cookies()
{
    return m_cookies;
}

QList<QNetworkCookie> HttpRequest::parsedCookies()
{
    return m_parsedCookies;
}

QByteArray HttpRequest::hostName()
{
    return m_hostName;
}

void HttpRequest::readText()
{
    // TODO fix denial-of-service attack
    while (m_socket->canReadLine()) {
        m_text.append(m_socket->readLine());
    }
//    DEBUG << "req" << m_text;
}

void HttpRequest::parseText()
{
    foreach (const QByteArray &line, m_text) {
       if (line.startsWith("GET")) {
             m_path = QUrl::fromPercentEncoding(line.mid(4).split(' ').at(0)).toAscii(); // ### assumes well-formed string
       } else if (line.startsWith("POST")) {
              m_path = QUrl::fromPercentEncoding(line.mid(5).split(' ').at(0)).toAscii(); // ### assumes well-formed string
       } else if (line.startsWith("Cookie:")) {
//            DEBUG << "cookie line" << line.simplified();
            m_cookies = line.mid(7).simplified(); // remove "Cookie:"
//            DEBUG << "cookies text" << m_cookies;
            foreach (const QByteArray cookieText, m_cookies.split(';')){
                if (cookieText.contains('=')) {
                    QList<QByteArray> cookieParts = cookieText.split('=');
                    QNetworkCookie cookie(cookieParts.at(0).simplified(), cookieParts.at(1).simplified());
                    m_parsedCookies.append(cookie);
                }
            }
        } else if (line.startsWith("Host")) {
            QByteArray hostline = line.split(' ').at(1); // ###
            hostline.chop(2); // remove newline
            m_hostName = hostline;
        } else if (line.startsWith("If-None-Match:")){
            m_ifNoneMatch = line.mid(18).simplified(); // remove "If-None-Match: vX-", where X is the integer version number
        }
    }
}

HttpResponse::HttpResponse()
: contentType("text/html"), response304(false), neverExpires(false) { }

void HttpResponse::setBody(const QByteArray &body)
{
    this->body = body;
}

void HttpResponse::setCookie(const QByteArray &name, const QByteArray &value)
{
    cookie = (name +"="+ value);
}

void HttpResponse::setContentType(const QByteArray &contentType)
{
    this->contentType = contentType;
}

void HttpResponse::seteTag(const QByteArray &eTag)
{
    if (eTag.isEmpty()) {
        this->eTag.clear();
        return;
    }
    this->eTag =  eTag;
}

void HttpResponse::set304Response()
{
    response304 = true;
}

void HttpResponse::setNeverExpires()
{
    neverExpires = true;
}

bool HttpResponse::isHandled() const
{
    return response304 || body.isEmpty() == false;
}

QByteArray HttpResponse::toText()
{
    time_t currentTime = time(0);

    QByteArray text;

    if (response304) {
        text += QByteArray("HTTP/1.1 304 Not Modified\r\n");
        text+= QByteArray("\r\n");
        return text;
    }

    text += QByteArray("HTTP/1.1 200 OK \r\n");
    text += QByteArray("Date: ") + QByteArray(asctime(gmtime(&currentTime))) + QByteArray("")
    + QByteArray("Content-Type: " + contentType + " \r\n")
    + QByteArray("Content-Length: " + QByteArray::number(body.length())  + "\r\n");

    if (cookie.isEmpty() == false) {
        text+= "Set-Cookie: " + cookie + "\r\n";
    }

    // Support three different caching strategies:
    // 1. Never-expires. Useful when content has a unique
    // name based on e.g. a hash of the content. Allows the
    // user agent to get the content once and then cache it
    // forever.
    // 2. Etag. The url for the content stays the same, the
    // etag changes with content changes. Allows the user agent
    // to ask if a spesific url as been updated, and then skip
    // the content download if not.
    // 3. no-cache. For dynamic content. Not cached by the user-agent
    // and is re-downloaded on each request.
    if (neverExpires) {
        text += QByteArray("Cache-control: max-age=9999999 \r\n"); // or -1?
        text += "eTag: 24 \r\n";
    } else if (eTag.isEmpty() == false) {
        text += "eTag: " + eTag + "\r\n";
    } else {
        text += QByteArray("Cache-control: no-cache \r\n");
        text += QByteArray("Cache-control: max-age=0 \r\n");
    }

    text+= QByteArray("\r\n")
    + body;
    return text;
}

Server::Server(quint16 port)
{
    this->port = port;
    connect(this, SIGNAL(newConnection()), SLOT(connectionAvailable()));
    listen(QHostAddress::Any, port);
    qDebug() << QString("Server running on: http://" + QHostInfo::localHostName() + ":" + QString::number(port) + "/");
}

Server::~Server()
{
    qDebug() << QString("Server stopped.");
}

void Server::connectionAvailable()
{
    QTcpSocket *socket = nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(dataOnSocket())); // ### race condition?
}

void Server::dataOnSocket()
{
    QTcpSocket * socket = static_cast<QTcpSocket *>(sender());

    //DEBUG << "";
    //DEBUG << "request";

    QList<QByteArray> lines;
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine();
        lines.append(line);
    }

    //DEBUG << lines;

    HttpRequest request(lines);

    HttpResponse response;
    serveResponseFromPath(&response, request.path());
    QByteArray responseText = response.toText();
    socket->write(responseText);
    socket->close();
}


void Server::setRootPath(const QString &path)
{
    rootPath = QDir(path).canonicalPath();
}

void Server::addSearchPath(const QString &path)
{
    if (searchPaths.contains(path))
        return;
    searchPaths.append(path);
}

/*
QString findFileRecursivly(const QString& rootPath, const QString &fileName)
{
    QString filePath;
    QDir rootDir(rootPath);
    if (rootDir.


    return filePath;
}
*/
void Server::serveResponseFromPath(HttpResponse *response, QString path)
{
    qDebug() << "GET" << path;

    if (path == QLatin1String("/"))
        path = QLatin1String("/index.html");

    if (path.contains(".."))
        return;


    QByteArray fileContents;
    QString rootFilePath = path;
    rootFilePath.prepend(rootPath);
    extern QByteArray readFile(const QString &filePath);
    if (QFile::exists(rootFilePath)) {
        fileContents = readFile(rootFilePath);
        qDebug() << "serve file" << path << fileContents.count() << "bytes";
    } else {
        foreach (const QString &searchPath, searchPaths) {
            //QString foundFilePath = findFileRecursivly(searchPath, path);
            QString foundFilePath = searchPath + path;
            if (!QFile::exists(foundFilePath))
                continue;
            fileContents = readFile(foundFilePath);
            qDebug() << "serve file" << foundFilePath << fileContents.count() << "bytes";
        }
    }

    response->setBody(fileContents);
}

