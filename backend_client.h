#ifndef BACKEND_CLIENT_H
#define BACKEND_CLIENT_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVector>

struct User {
    int id;
    QString name;
    QString email;
    int fingerCount;
    QString createdAt;
};

struct FingerprintTemplate {
    int userId;
    QString finger;
    QByteArray templateData;
    QString userName;
    QString userEmail;
    QString createdAt;
};

class BackendClient : public QObject {
    Q_OBJECT

public:
    explicit BackendClient(QObject* parent = nullptr);
    ~BackendClient();

    void setBaseUrl(const QString& url);
    QString getBaseUrl() const { return m_baseUrl; }
    bool isConfigured() const { return !m_baseUrl.isEmpty(); }

    // User operations
    void createUser(const QString& name, const QString& email = QString());
    void listUsers();
    void getUser(int userId);
    void getUserFingers(int userId);

    // Fingerprint operations
    void storeTemplate(int userId, const QByteArray& templateData, const QString& finger);
    void loadTemplate(int userId, const QString& finger = QString());
    void loadTemplates(const QString& scope = QString());
    void logAuth(int userId, bool success, float score);

signals:
    void userCreated(int userId);
    void usersListed(const QVector<User>& users);
    void userRetrieved(const User& user);
    void userFingersRetrieved(int userId, const QStringList& fingers);
    void templateStored(int userId, const QString& finger);
    void templateLoaded(const FingerprintTemplate& template);
    void templatesLoaded(const QVector<FingerprintTemplate>& templates);
    void authLogged();
    void error(const QString& errorMessage);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_baseUrl;
    QMap<QNetworkReply*, QString> m_requestTypes; // Track request types

    void handleUserCreated(QNetworkReply* reply);
    void handleUsersListed(QNetworkReply* reply);
    void handleUserRetrieved(QNetworkReply* reply);
    void handleUserFingersRetrieved(QNetworkReply* reply);
    void handleTemplateStored(QNetworkReply* reply);
    void handleTemplateLoaded(QNetworkReply* reply);
    void handleTemplatesLoaded(QNetworkReply* reply);
    void handleError(QNetworkReply* reply, const QString& context);
};

#endif // BACKEND_CLIENT_H

