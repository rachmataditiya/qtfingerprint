#include "backend_client.h"
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageAuthenticationCode>
#include <QCryptographicHash>
#include <QByteArray>
#include <QBuffer>

BackendClient::BackendClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &BackendClient::onReplyFinished);
}

BackendClient::~BackendClient()
{
}

void BackendClient::setBaseUrl(const QString& url)
{
    m_baseUrl = url;
    if (m_baseUrl.endsWith("/")) {
        m_baseUrl.chop(1);
    }
    qDebug() << "BackendClient: Base URL set to" << m_baseUrl;
}

void BackendClient::createUser(const QString& name, const QString& email)
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        return;
    }

    QJsonObject json;
    json["name"] = name;
    if (!email.isEmpty()) {
        json["email"] = email;
    }

    QUrl url(m_baseUrl + "/users");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    m_requestTypes[reply] = "createUser";
    qDebug() << "BackendClient: Creating user" << name;
}

void BackendClient::listUsers()
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        return;
    }

    QUrl url(m_baseUrl + "/users");
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    m_requestTypes[reply] = "listUsers";
    qDebug() << "BackendClient: Listing users";
}

void BackendClient::getUser(int userId)
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        return;
    }

    QUrl url(m_baseUrl + QString("/users/%1").arg(userId));
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    m_requestTypes[reply] = "getUser";
    qDebug() << "BackendClient: Getting user" << userId;
}

void BackendClient::getUserFingers(int userId)
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        qDebug() << "BackendClient: ERROR - Base URL is empty!";
        return;
    }

    QUrl url(m_baseUrl + QString("/users/%1/fingers").arg(userId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);
    m_requestTypes[reply] = "getUserFingers";
    qDebug() << "BackendClient: Getting fingers for user" << userId << "from URL:" << url.toString();
    
    // Connect error signal for debugging
    connect(reply, &QNetworkReply::errorOccurred, this, [this, userId, url](QNetworkReply::NetworkError error) {
        qDebug() << "BackendClient: Network error getting fingers for user" << userId << ":" << error << "URL:" << url.toString();
    });
}

void BackendClient::storeTemplate(int userId, const QByteArray& templateData, const QString& finger)
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        return;
    }

    // Encode template as base64
    QByteArray base64Template = templateData.toBase64();

    QJsonObject json;
    json["template"] = QString::fromUtf8(base64Template);
    json["finger"] = finger;

    QUrl url(m_baseUrl + QString("/users/%1/fingerprint").arg(userId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    m_requestTypes[reply] = "storeTemplate";
    m_storeTemplateParams[reply] = qMakePair(userId, finger); // Store params
    qDebug() << "BackendClient: Storing template for user" << userId << "finger" << finger;
}

void BackendClient::loadTemplate(int userId, const QString& finger)
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        return;
    }

    QUrl url(m_baseUrl + QString("/users/%1/fingerprint").arg(userId));
    if (!finger.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem("finger", finger);
        url.setQuery(query);
    }
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    m_requestTypes[reply] = "loadTemplate";
    qDebug() << "BackendClient: Loading template for user" << userId << "finger" << finger;
}

void BackendClient::loadTemplates(const QString& scope)
{
    if (m_baseUrl.isEmpty()) {
        emit error("Backend URL not configured");
        return;
    }

    QUrl url(m_baseUrl + "/templates");
    if (!scope.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem("scope", scope);
        url.setQuery(query);
    }
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    m_requestTypes[reply] = "loadTemplates";
    qDebug() << "BackendClient: Loading templates (scope:" << scope << ")";
}

void BackendClient::logAuth(int userId, bool success, float score)
{
    if (m_baseUrl.isEmpty()) {
        return; // Silent fail for logging
    }

    QJsonObject json;
    json["user_id"] = userId;
    json["success"] = success;
    json["score"] = score;

    QUrl url(m_baseUrl + "/log_auth");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    m_requestTypes[reply] = "logAuth";
    // Don't wait for response, fire and forget
}

void BackendClient::onReplyFinished(QNetworkReply* reply)
{
    if (!m_requestTypes.contains(reply)) {
        reply->deleteLater();
        return;
    }

    QString requestType = m_requestTypes.take(reply);

    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply, requestType);
        reply->deleteLater();
        return;
    }

    // Route to appropriate handler - handlers will read data themselves
    if (requestType == "createUser") {
        handleUserCreated(reply);
    } else if (requestType == "listUsers") {
        handleUsersListed(reply);
    } else if (requestType == "getUser") {
        handleUserRetrieved(reply);
    } else if (requestType == "getUserFingers") {
        handleUserFingersRetrieved(reply);
    } else if (requestType == "storeTemplate") {
        handleTemplateStored(reply);
    } else if (requestType == "loadTemplate") {
        handleTemplateLoaded(reply);
    } else if (requestType == "loadTemplates") {
        handleTemplatesLoaded(reply);
    } else if (requestType == "logAuth") {
        // Fire and forget, no handling needed
    }

    reply->deleteLater();
}

void BackendClient::handleUserCreated(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj.contains("id")) {
        int userId = obj["id"].toInt();
        emit userCreated(userId);
        qDebug() << "BackendClient: User created with ID" << userId;
    } else {
        emit error("Invalid response: missing user ID");
    }
}

void BackendClient::handleUsersListed(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Failed to list users: %1").arg(reply->errorString()));
        qDebug() << "BackendClient: Error listing users:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "BackendClient: Received response:" << data;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit error(QString("JSON parse error: %1").arg(parseError.errorString()));
        qDebug() << "BackendClient: JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonArray array = doc.array();
    qDebug() << "BackendClient: Parsed array with" << array.size() << "users";

    QVector<User> users;
    for (const QJsonValue& value : array) {
        QJsonObject obj = value.toObject();
        User user;
        user.id = obj["id"].toInt();
        user.name = obj["name"].toString();
        user.email = obj["email"].toString();
        user.fingerCount = obj["finger_count"].toInt();
        user.createdAt = obj["created_at"].toString();
        users.append(user);
        qDebug() << "BackendClient: Parsed user:" << user.id << user.name << user.email << "fingers:" << user.fingerCount;
    }

    emit usersListed(users);
    qDebug() << "BackendClient: Emitted usersListed signal with" << users.size() << "users";
}

void BackendClient::handleUserRetrieved(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Failed to retrieve user: %1").arg(reply->errorString()));
        qDebug() << "BackendClient: Error retrieving user:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    User user;
    user.id = obj["id"].toInt();
    user.name = obj["name"].toString();
    user.email = obj["email"].toString();
    user.fingerCount = obj["finger_count"].toInt();
    user.createdAt = obj["created_at"].toString();

    emit userRetrieved(user);
    qDebug() << "BackendClient: Retrieved user" << user.id << user.name;
}

void BackendClient::handleUserFingersRetrieved(QNetworkReply* reply)
{
    QString path = reply->url().path();
    int userId = path.section('/', -2, -2).toInt();
    QString fullUrl = reply->url().toString();
    
    qDebug() << "BackendClient: handleUserFingersRetrieved called for user" << userId << "URL:" << fullUrl;
    qDebug() << "BackendClient: Reply error:" << reply->error() << "Error string:" << reply->errorString();
    qDebug() << "BackendClient: HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Failed to get user fingers: %1").arg(reply->errorString()));
        qDebug() << "BackendClient: Error getting user fingers:" << reply->errorString() << "URL:" << fullUrl;
        
        // Emit empty list so UI can handle it
        emit userFingersRetrieved(userId, QStringList());
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "BackendClient: Received data size:" << data.size() << "bytes";
    qDebug() << "BackendClient: Received data:" << data;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit error(QString("JSON parse error: %1").arg(parseError.errorString()));
        qDebug() << "BackendClient: JSON parse error:" << parseError.errorString() << "at offset" << parseError.offset;
        qDebug() << "BackendClient: Raw data:" << data;
        
        // Emit empty list so UI can handle it
        emit userFingersRetrieved(userId, QStringList());
        return;
    }
    
    QJsonArray array = doc.array();
    qDebug() << "BackendClient: Parsed JSON array with" << array.size() << "items";

    QStringList fingers;
    for (const QJsonValue& value : array) {
        QString finger = value.toString();
        fingers.append(finger);
        qDebug() << "BackendClient: Found finger:" << finger;
    }

    emit userFingersRetrieved(userId, fingers);
    qDebug() << "BackendClient: Successfully retrieved" << fingers.size() << "fingers for user" << userId << ":" << fingers;
}

void BackendClient::handleTemplateStored(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Failed to store template: %1").arg(reply->errorString()));
        qDebug() << "BackendClient: Error storing template:" << reply->errorString();
        m_storeTemplateParams.remove(reply);
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    // Get userId and finger from stored params (backend response doesn't include them)
    QPair<int, QString> params = m_storeTemplateParams.take(reply);
    int userId = params.first;
    QString finger = params.second;

    if (obj.contains("success") && obj["success"].toBool()) {
        emit templateStored(userId, finger);
        qDebug() << "BackendClient: Template stored for user" << userId << "finger" << finger;
    } else {
        QString errorMsg = obj.contains("error") ? obj["error"].toString() : "Unknown error";
        emit error(QString("Failed to store template: %1").arg(errorMsg));
    }
}

void BackendClient::handleTemplateLoaded(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        // Check for HTTP error codes
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 404) {
            emit error("Template not found. Please enroll a fingerprint for this user first.");
        } else {
            emit error(QString("Failed to load template: %1").arg(reply->errorString()));
        }
        qDebug() << "BackendClient: Error loading template:" << reply->errorString() << "Status:" << statusCode;
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit error(QString("JSON parse error: %1").arg(parseError.errorString()));
        qDebug() << "BackendClient: JSON parse error:" << parseError.errorString();
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // Check for error response
    if (obj.contains("error")) {
        QString errorMsg = obj["error"].toString();
        emit error(QString("Template not found: %1").arg(errorMsg));
        qDebug() << "BackendClient: Template not found:" << errorMsg;
        return;
    }

    BackendFingerprintTemplate tmpl;
    tmpl.userId = obj["user_id"].toInt();
    tmpl.finger = obj["finger"].toString();
    QString base64Template = obj["template"].toString();
    tmpl.templateData = QByteArray::fromBase64(base64Template.toUtf8());
    tmpl.createdAt = obj["created_at"].toString();

    if (tmpl.templateData.isEmpty()) {
        emit error("Template data is empty");
        qDebug() << "BackendClient: Template data is empty";
        return;
    }

    emit templateLoaded(tmpl);
    qDebug() << "BackendClient: Template loaded for user" << tmpl.userId << "finger" << tmpl.finger;
}

void BackendClient::handleTemplatesLoaded(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Failed to load templates: %1").arg(reply->errorString()));
        qDebug() << "BackendClient: Error loading templates:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();

    QVector<BackendFingerprintTemplate> templates;
    for (const QJsonValue& value : array) {
        QJsonObject obj = value.toObject();
        BackendFingerprintTemplate tmpl;
        tmpl.userId = obj["user_id"].toInt();
        tmpl.finger = obj["finger"].toString();
        tmpl.userName = obj["user_name"].toString();
        tmpl.userEmail = obj["user_email"].toString();
        QString base64Template = obj["template"].toString();
        tmpl.templateData = QByteArray::fromBase64(base64Template.toUtf8());
        tmpl.createdAt = obj["created_at"].toString();
        templates.append(tmpl);
    }

    emit templatesLoaded(templates);
    qDebug() << "BackendClient: Loaded" << templates.size() << "templates";
}

void BackendClient::handleError(QNetworkReply* reply, const QString& context)
{
    QString errorMsg = QString("Network error in %1: %2").arg(context).arg(reply->errorString());
    
    // Try to parse error response
    QByteArray data = reply->readAll();
    if (!data.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            QJsonObject obj = doc.object();
            if (obj.contains("error")) {
                errorMsg = obj["error"].toString();
            }
        }
    }

    emit error(errorMsg);
    qDebug() << "BackendClient: Error in" << context << ":" << errorMsg;
}

