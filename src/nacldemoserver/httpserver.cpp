#include "httpserver.h"
#include "time.h"
#include <deploynexe.h>

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

    QFile f(":/htmltemplates/naclhtmltemplate.html");
    f.open(QIODevice::ReadOnly);
    naclHtmlContent = f.readAll();
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


void Server::addRootPath(const QString &path)
{
    rootPaths.append(QDir(path).canonicalPath());
}

void Server::serveResponseFromPath(HttpResponse *response, QString path)
{
//    qDebug() << "";
//    qDebug() << "htmlForPath" << path;
    QByteArray html;
    html += "<H2>Qt for Google Native Client</H2>";
    // special case for /: serve the root paths
    if (path == "/") {
        foreach (const QString &rootPath, rootPaths) {
            QDir dir(rootPath);
            QString name = dir.dirName();
            html.append("<a href=./" + name + "/>" + name + "</a><br>");
        }

        response->setBody(html);
    }

    // Serve resources. (Paths that starts with ':').
    int resourceIndex = path.indexOf(':');
    if (resourceIndex != -1) {
        QString fileName = path.mid(resourceIndex);
        QFile file(fileName);
        if (!file.exists())
            qDebug() << "Resource file not found:" << fileName;

        file.open(QIODevice::ReadOnly);
        response->setContentType("image");
        response->seteTag("1");

        QByteArray fileContents = file.readAll();
        if (fileContents.size() == 0)
            qDebug() << "Resource file has no contents" << fileName;
        response->setBody(fileContents);
        return;
    }

    // serve .nmf files generated from the template
    if (path.endsWith("nmf")) {
        qDebug() << "serve nmf" << path;

        QFile nmfFile("naclnmftemplate.nmf");
        nmfFile.open(QIODevice::ReadOnly);
        QByteArray fileContents = nmfFile.readAll();
        if (fileContents.size() == 0)
            qDebug() << "Resource file has no contents" << nmfFile.fileName();
        QString appName = QFileInfo(path).fileName();
        appName.chop(4);

        // Figure out which arch we are serving, set correct arch in nmf file
        QString nexePath = path;
        if (nexePath.startsWith("/"))
            nexePath.remove(0,1);
        nexePath.chop(3); // remove "nmf"
        nexePath.append("nexe");
        QProcess p;
        p.start(QLatin1String("/usr/bin/file"), QStringList() << findCanonicalPath(nexePath));
        p.waitForFinished();
        QByteArray fileOutput = p.readAll();
        if (fileOutput.contains("x86-64"))
            fileContents.replace("ARCH", "x86-64");
        else
            fileContents.replace("ARCH", "x86-32");

        // Set the nexe name
        fileContents.replace("APPNAME", appName.toAscii());
        qDebug() << "serve" << fileContents;

        response->setContentType("text/plain");
        response->setBody(fileContents);
        return;
    }
    
    if (path.startsWith("/"))
        path.remove(0,1);

    // a bit of security, check if path is a subpath of one of
    // the root paths.
    QString canonicalPath = findCanonicalPath(path);
    if (canonicalPath.isEmpty())
        return;

    // is the path pointing directly to a nacl file?
    bool isNaClFile = (QFile::exists(canonicalPath) && QFileInfo(canonicalPath).isDir() == false);

    // does the the path have a leaf directory containing an NaCl executable?
    bool isdir = QFileInfo(canonicalPath).isDir();
    QString leafName = QDir(canonicalPath).dirName();
    bool hasNaclFile = QFile::exists(canonicalPath + "/" + leafName + ".nexe");
    bool isNaClDir = isdir && hasNaclFile;

    //qDebug() << "canonical path" << canonicalPath;
    //qDebug() << "isNaClFile" << isNaClFile << "isNaClDir" << isNaClDir;

    // serve the file, a combined tree/nacl area view or a dir tree view.
    if (isNaClFile) {
        QFile nexeFile(canonicalPath);
        nexeFile.open(QIODevice::ReadOnly);
        response->setContentType("application/octet-stream");
        response->setBody(nexeFile.readAll());
        return;
    } else if (isNaClDir) {
        // chop of the last directory in the path
        QStringList pathParts = path.split('/', QString::SkipEmptyParts);
  //      qDebug() << "nacl dir" << path << pathParts;

        QString last = pathParts.takeLast();
        QString newPath = pathParts.join("/");

        html += "<div style=\"float: left; width: 30%\">";
        html += htmlLinksForPath(findCanonicalPath(newPath), newPath, QLatin1String("../"));
        html += "</div>";

        html += "<div style=\"float: left\">";
        html += instantiateNaclHtmlContent(last);
        html += "</div>";
    } else {
        html += "<div style=\"float: left; width: 50%\">";
        html += htmlLinksForPath(canonicalPath, path);
        html += "</div>";
    }

    response->setBody(html);
}

QByteArray Server::htmlForPath(QString path)
{
    HttpResponse response;

    serveResponseFromPath(&response, path);

    return response.toText();
}

QString Server::findCanonicalPath(QString path)
{
    // case 1: a full path (C:\foo\bar\baz)
    QString canonicalPath = QDir(path).canonicalPath();
    foreach (const QString &rootPath, rootPaths) {
        if (canonicalPath.startsWith(rootPath))
            return canonicalPath;
    }

    // case 2: a partial path: rootPath/foo/bar
    canonicalPath.clear();
    if (path.startsWith("/"))
        path.remove(0,1);

   // qDebug() << "path" << path;

    // path should loook like rootPath/foo/bar
    int firstSlash = path.indexOf('/');
    if (firstSlash == -1)
        return canonicalPath;

   // qDebug() << "first slash at" << firstSlash;

    // for security, test that the subpath is a proper subpath
    // of one of the root paths.
    QString rootPathCandidate = path.mid(0, firstSlash);
    QString subPath = path.mid(firstSlash);

  //  qDebug() << "rootPathCandidate" << rootPathCandidate;
  //  qDebug() << "subPath" << subPath;
    foreach (const QString &rootPath, rootPaths) {
    //    qDebug() << "test" << rootPath << QDir(rootPath).dirName() << rootPathCandidate ;
        if (QDir(rootPath).dirName() == rootPathCandidate){
            QString canonicalPathCandidate = QDir(rootPath + subPath).canonicalPath();
     //      qDebug() << "canonicalPathCandidate" << canonicalPathCandidate;
            if (QDir().exists(canonicalPathCandidate))
                canonicalPath = canonicalPathCandidate;
        }
    }

 //   qDebug() << "canonical path" << canonicalPath;
    return canonicalPath;
}

QByteArray Server::htmlLinksForPath(QString canonicalPath, QString path, QString linkPrefix)
{
    QByteArray html;
    if (canonicalPath.isEmpty())
        return html;

    if (path.endsWith('/') == false)
        path.append("/");

    // qDebug() << "link path is" << path;
    html.append("<a href=" + linkPrefix + "../>..</a><br>");

    QDirIterator it(canonicalPath);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isDir()
            && fileInfo.isHidden() == false
            && fileInfo.fileName() != "."
            && fileInfo.fileName() != ".."
            && fileInfo.fileName() != "debug"
            && fileInfo.fileName() != "release"
            && hasDemoFileRecursive(canonicalPath + "/" + fileInfo.fileName())
            ) {
            QDir dir(it.filePath());
            QString name = dir.dirName();
      //      qDebug() << "payh is" << path;
            html.append("<a href=" + linkPrefix + name + "/>" + name + "</a><br>");
        }
    }

    return html;
}

QByteArray Server::instantiateNaclHtmlContent(QString appName, bool noResourceLoad)
{
//    qDebug() << "instantiateNaclHtmlContent" << appName;
    QByteArray html = naclHtmlContent;
    html.replace("APPNAME", appName.toUtf8());
    if (noResourceLoad) {
        html.replace("src=\":", "src=\"");
    }

    return html;
}

void Server::saveDirectory(QString sourcePath, QString destinationPath)
{
    if (hasDemoFileRecursive(sourcePath) == false)
        return;

    qDebug() << "savedirectory" << sourcePath << destinationPath;
    qDebug() << "nexes" << findNexes(sourcePath);

    // deploy the nexe:
    QStringList nexes = findNexes(sourcePath);
    if (!nexes.isEmpty()) {
        QString deployedNexePath = deployNexe(nexes.at(0), destinationPath);
        stripNexe(deployedNexePath);
        createSupportFilesForNexes(destinationPath);
    }

    // write html:
    QByteArray html;

    if (!nexes.isEmpty()) {
        // This is a leaf directory containing a nexe. Display a directory listing
        // for one level up. Load the nexe in this directory.
        QStringList pathParts = sourcePath.split('/', QString::SkipEmptyParts);
        QString last = pathParts.takeLast();
        QString upLevelPath = pathParts.join("/");

        qDebug() << "last path element" << last;
        qDebug() << "up-one path" << upLevelPath;

        html += "<div style=\"float: left; width: 30%\">";
        html += htmlLinksForPath(findCanonicalPath(upLevelPath), upLevelPath, QLatin1String("../"));
        html += "</div>";

        html += "<div style=\"float: left\">";
        html += instantiateNaclHtmlContent(last, true);
        html += "</div>";
    } else {
        html += "<div style=\"float: left; width: 50%\">";
        html += htmlLinksForPath(sourcePath, sourcePath);
        html += "</div>";
    }

    QFile f(destinationPath + "/index.html");
    f.open(QIODevice::WriteOnly);
    f.write(html);
    f.close();



    QDir sourceDir(sourcePath);
    QStringList subDirectories = sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QDir destinationDir(destinationPath);
    foreach (QString subDirectory, subDirectories) {
        QString subDirectoryPath = sourcePath + "/" + subDirectory;
        if (hasDemoFileRecursive(subDirectoryPath)) {
            destinationDir.mkdir(subDirectory);
        }

        saveDirectory(sourcePath + "/" + subDirectory, destinationPath + "/" + subDirectory);
    }
}

void Server::saveDemoFiles()
{
    QString savePath = QDir::fromNativeSeparators(saveLocationLineEdit->text());
    QDir().mkpath(savePath);

    qDebug() << "Server::saveDemoFiles";

    QByteArray html = htmlForPath("/"); //root
    QFile f(savePath + "/index.html");
    f.open(QIODevice::WriteOnly);
    f.write(html);
    f.close();


    foreach(QString rootPath, rootPaths) {
        QString saveRootPath = savePath + "/" + QDir(rootPath).dirName();
        qDebug() << "save" << rootPath << saveRootPath;
        QDir().mkpath(saveRootPath);
        saveDirectory(rootPath, saveRootPath);
    }
}

// Search for NaCl examples in path. Recurses into subdirectories.
// For a given path examples/widgets/wiggly, returns true if
// examples/widgets/wiggly/wigly.nexe exists.
bool Server::hasDemoFileRecursive(QString path)
{
    QString canonicalPath = QDir(path).canonicalPath();
    if (hasDemoFilesCache.contains(canonicalPath))
        return hasDemoFilesCache.value(canonicalPath);

    QDir dir(path);
    // qDebug() << "check" << path + "/" + dir.dirName() << QFile::exists(path + "/" + dir.dirName());
    if (QFile::exists(path + "/" + dir.dirName() + ".nexe")) {
        hasDemoFilesCache.insert(path, true);
        return true;
    }

    QStringList subDirectories = QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QString subDirectory, subDirectories) {
        QString subDirectoryPath = path + "/" + subDirectory;
        if (hasDemoFileRecursive(subDirectoryPath)) {
            hasDemoFilesCache.insert(subDirectoryPath, true);
            return true;
        }
    }
    hasDemoFilesCache.insert(path, false);
    return false;
}
