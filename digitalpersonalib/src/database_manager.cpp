#include "database_manager.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

DatabaseManager::DatabaseManager(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initialize()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        setError(QString("Failed to open database: %1").arg(m_db.lastError().text()));
        return false;
    }

    if (!createTables()) {
        return false;
    }

    qDebug() << "Database initialized successfully:" << m_dbPath;
    return true;
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // Create users table
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            email TEXT,
            fingerprint_template BLOB NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createUsersTable)) {
        setError(QString("Failed to create users table: %1").arg(query.lastError().text()));
        return false;
    }

    // Create index on name for faster searches
    QString createNameIndex = "CREATE INDEX IF NOT EXISTS idx_users_name ON users(name)";
    if (!query.exec(createNameIndex)) {
        setError(QString("Failed to create name index: %1").arg(query.lastError().text()));
        return false;
    }

    qDebug() << "Database tables created successfully";
    return true;
}

bool DatabaseManager::addUser(const QString& name, const QString& email, const QByteArray& fingerprintTemplate, int& userId)
{
    if (name.trimmed().isEmpty()) {
        setError("Name cannot be empty");
        return false;
    }

    if (fingerprintTemplate.isEmpty()) {
        setError("Fingerprint template cannot be empty");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (name, email, fingerprint_template) VALUES (:name, :email, :template)");
    query.bindValue(":name", name.trimmed());
    query.bindValue(":email", email.trimmed());
    query.bindValue(":template", fingerprintTemplate);

    if (!query.exec()) {
        setError(QString("Failed to add user: %1").arg(query.lastError().text()));
        return false;
    }

    userId = query.lastInsertId().toInt();
    qDebug() << "User added successfully. ID:" << userId;
    return true;
}

bool DatabaseManager::updateUserFingerprint(int userId, const QByteArray& fingerprintTemplate)
{
    if (fingerprintTemplate.isEmpty()) {
        setError("Fingerprint template cannot be empty");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET fingerprint_template = :template, updated_at = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":template", fingerprintTemplate);
    query.bindValue(":id", userId);

    if (!query.exec()) {
        setError(QString("Failed to update fingerprint: %1").arg(query.lastError().text()));
        return false;
    }

    if (query.numRowsAffected() == 0) {
        setError("User not found");
        return false;
    }

    qDebug() << "Fingerprint updated successfully for user ID:" << userId;
    return true;
}

bool DatabaseManager::getUserById(int userId, User& user)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec()) {
        setError(QString("Failed to get user: %1").arg(query.lastError().text()));
        return false;
    }

    if (!query.next()) {
        setError("User not found");
        return false;
    }

    user.id = query.value(0).toInt();
    user.name = query.value(1).toString();
    user.email = query.value(2).toString();
    user.fingerprintTemplate = query.value(3).toByteArray();
    user.createdAt = query.value(4).toString();
    user.updatedAt = query.value(5).toString();

    return true;
}

bool DatabaseManager::getUserByName(const QString& name, User& user)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE name = :name");
    query.bindValue(":name", name.trimmed());

    if (!query.exec()) {
        setError(QString("Failed to get user: %1").arg(query.lastError().text()));
        return false;
    }

    if (!query.next()) {
        setError("User not found");
        return false;
    }

    user.id = query.value(0).toInt();
    user.name = query.value(1).toString();
    user.email = query.value(2).toString();
    user.fingerprintTemplate = query.value(3).toByteArray();
    user.createdAt = query.value(4).toString();
    user.updatedAt = query.value(5).toString();

    return true;
}

QVector<User> DatabaseManager::getAllUsers()
{
    QVector<User> users;

    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users ORDER BY name")) {
        setError(QString("Failed to get users: %1").arg(query.lastError().text()));
        return users;
    }

    while (query.next()) {
        User user;
        user.id = query.value(0).toInt();
        user.name = query.value(1).toString();
        user.email = query.value(2).toString();
        user.fingerprintTemplate = query.value(3).toByteArray();
        user.createdAt = query.value(4).toString();
        user.updatedAt = query.value(5).toString();
        users.append(user);
    }

    qDebug() << "Retrieved" << users.size() << "users";
    return users;
}

bool DatabaseManager::deleteUser(int userId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec()) {
        setError(QString("Failed to delete user: %1").arg(query.lastError().text()));
        return false;
    }

    if (query.numRowsAffected() == 0) {
        setError("User not found");
        return false;
    }

    qDebug() << "User deleted successfully. ID:" << userId;
    return true;
}

bool DatabaseManager::userExists(const QString& name)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM users WHERE name = :name");
    query.bindValue(":name", name.trimmed());

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toInt() > 0;
}

QVector<User> DatabaseManager::searchUsers(const QString& searchTerm)
{
    QVector<User> users;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE name LIKE :term OR email LIKE :term ORDER BY name");
    query.bindValue(":term", QString("%%1%").arg(searchTerm.trimmed()));

    if (!query.exec()) {
        setError(QString("Failed to search users: %1").arg(query.lastError().text()));
        return users;
    }

    while (query.next()) {
        User user;
        user.id = query.value(0).toInt();
        user.name = query.value(1).toString();
        user.email = query.value(2).toString();
        user.fingerprintTemplate = query.value(3).toByteArray();
        user.createdAt = query.value(4).toString();
        user.updatedAt = query.value(5).toString();
        users.append(user);
    }

    return users;
}

void DatabaseManager::setError(const QString& error)
{
    m_lastError = error;
    qWarning() << "DatabaseManager Error:" << error;
}

