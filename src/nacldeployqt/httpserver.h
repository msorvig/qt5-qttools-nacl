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

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QtNetwork>

class HttpRequest
{
public:
    HttpRequest();
    HttpRequest(const QList<QByteArray> &text);
    HttpRequest(QTcpSocket *socket);
    QByteArray path();
    QByteArray cookies();
    QList<QNetworkCookie> parsedCookies();
    QByteArray hostName();
//    QHash<QByteArray, QByteArray> cookieValues();
//private:
    void readText();
    void parseText();
    QByteArray m_path;
    QByteArray m_cookies;
    QList<QNetworkCookie> m_parsedCookies;
    QByteArray m_hostName;
    QList<QByteArray> m_text;
    QTcpSocket *m_socket;
    QByteArray m_ifNoneMatch;
};

class HttpResponse
{
public:
    HttpResponse();
    void setBody(const QByteArray &body);
    void setCookie(const QByteArray &name, const QByteArray &value);
    void setContentType(const QByteArray &contentType);
    void seteTag(const QByteArray &eTag);
    void set304Response();
    void setNeverExpires();

    bool isHandled() const;
    QByteArray toText();

    QByteArray body;
    QByteArray cookie;
    QByteArray contentType;
    QByteArray eTag;
    bool response304;
    bool neverExpires;
};

class Server : public QTcpServer
{
Q_OBJECT
public:
    Server(quint16 port = 1818);
    ~Server();

    void setRootPath(const QString &path);
    void addSearchPath(const QString &path);
    QString findCanonicalPath(QString path);
    void serveResponseFromPath(HttpResponse *request, QString path);
private slots:
    void connectionAvailable();
    void dataOnSocket();
private:
    quint16 port;
    QString rootPath;
    QStringList searchPaths;
};


#endif // HTTPSERVER_H
