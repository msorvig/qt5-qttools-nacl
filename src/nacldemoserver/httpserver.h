#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QtNetwork>
#include <QtGui>
#include <QtWidgets/QtWidgets>

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

    void addRootPath(const QString&path);
    QString findCanonicalPath(QString path);
    void serveResponseFromPath(HttpResponse *request, QString path);
    QByteArray htmlForPath(QString path);
    QByteArray htmlLinksForPath(QString canonicalPath, QString path, QString prefix = QString());
    QByteArray instantiateNaclHtmlContent(QString appName, bool noResourceLoad = false);
    QByteArray naclHtmlContent;
    void setSaveLocationLineEdit(QLineEdit *lineEdit) { saveLocationLineEdit = lineEdit; }
    void saveDirectory(QString sourcePath, QString destinationPath);
    bool hasDemoFileRecursive(QString path);
private slots:
    void saveDemoFiles();
    void connectionAvailable();
    void dataOnSocket();
private:
    quint16 port;
    QStringList rootPaths;
    QLineEdit *saveLocationLineEdit;
    QHash<QString, bool> hasDemoFilesCache;

};


#endif // HTTPSERVER_H
