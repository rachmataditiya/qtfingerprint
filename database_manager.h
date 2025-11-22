#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVector>
#include <QByteArray>

struct User {
    int id;
    QString name;
    QString email;
    QByteArray fingerprintTemplate;
    QString createdAt;
    QString updatedAt;
};

class DatabaseManager : public QObject {
    Q_OBJECT

public:
    explicit DatabaseManager(const QString& dbPath = "fingerprint.db", QObject* parent = nullptr);
    ~DatabaseManager();

    bool initialize();
    bool isOpen() const;
    QString getLastError() const { return m_lastError; }

    // User operations
    bool addUser(const QString& name, const QString& email, const QByteArray& fingerprintTemplate, int& userId);
    bool updateUserFingerprint(int userId, const QByteArray& fingerprintTemplate);
    bool getUserById(int userId, User& user);
    bool getUserByName(const QString& name, User& user);
    QVector<User> getAllUsers();
    bool deleteUser(int userId);
    bool userExists(const QString& name);

    // Search operations
    QVector<User> searchUsers(const QString& searchTerm);

private:
    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_lastError;

    bool createTables();
    void setError(const QString& error);
};

#endif // DATABASE_MANAGER_H

