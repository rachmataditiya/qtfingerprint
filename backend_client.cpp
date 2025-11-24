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
        return;
    }

    QUrl url(m_baseUrl + QString("/users/%1/fingers").arg(userId));
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    m_requestTypes[reply] = "getUserFingers";
    qDebug() << "BackendClient: Getting fingers for user" << userId;
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

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit error(QString("JSON parse error: %1").arg(parseError.errorString()));
        reply->deleteLater();
        return;
    }

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
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();

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
    }

    emit usersListed(users);
    qDebug() << "BackendClient: Listed" << users.size() << "users";
}

void BackendClient::handleUserRetrieved(QNetworkReply* reply)
{
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
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();

    QStringList fingers;
    for (const QJsonValue& value : array) {
        fingers.append(value.toString());
    }

    // Extract userId from URL
    QString path = reply->url().path();
    int userId = path.section('/', -2, -2).toInt();

    emit userFingersRetrieved(userId, fingers);
    qDebug() << "BackendClient: Retrieved" << fingers.size() << "fingers for user" << userId;
}

void BackendClient::handleTemplateStored(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    int userId = obj["user_id"].toInt();
    QString finger = obj["finger"].toString();

    emit templateStored(userId, finger);
    qDebug() << "BackendClient: Template stored for user" << userId << "finger" << finger;
}

void BackendClient::handleTemplateLoaded(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    FingerprintTemplate template;
    template.userId = obj["user_id"].toInt();
    template.finger = obj["finger"].toString();
    QString base64Template = obj["template"].toString();
    template.templateData = QByteArray::fromBase64(base64Template.toUtf8());
    template.createdAt = obj["created_at"].toString();

    emit templateLoaded(template);
    qDebug() << "BackendClient: Template loaded for user" << template.userId << "finger" << template.finger;
}

void BackendClient::handleTemplatesLoaded(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();

    QVector<FingerprintTemplate> templates;
    for (const QJsonValue& value : array) {
        QJsonObject obj = value.toObject();
        FingerprintTemplate template;
        template.userId = obj["user_id"].toInt();
        template.finger = obj["finger"].toString();
        template.userName = obj["user_name"].toString();
        template.userEmail = obj["user_email"].toString();
        QString base64Template = obj["template"].toString();
        template.templateData = QByteArray::fromBase64(base64Template.toUtf8());
        template.createdAt = obj["created_at"].toString();
        templates.append(template);
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

