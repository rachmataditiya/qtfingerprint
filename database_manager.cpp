#include "database_manager.h"
#include "migration_manager.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    
    QString connName = m_db.connectionName();
    m_db = QSqlDatabase(); // Release internal pointer
    
    if (!connName.isEmpty() && QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }
}

bool DatabaseManager::initialize(const DatabaseConfigDialog::Config& config)
{
    // Close existing connection if any
    close();

    // Re-establish connection
    if (config.type == "SQLITE") {
        // Robust Path Logic:
        // 1. If absolute, use it.
        // 2. If relative, prepend AppDataLocation.
        QString dbPath = config.name;
        QFileInfo fi(dbPath);
        if (fi.isRelative()) {
             QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
             QDir dir(dataPath);
             if (!dir.exists()) dir.mkpath(".");
             dbPath = dir.filePath(dbPath);
             qDebug() << "Resolved relative SQLite path to:" << dbPath;
        }

        // Create directory if needed
        QFileInfo finalFi(dbPath);
        QDir finalDir = finalFi.absoluteDir();
        if (!finalDir.exists()) {
            if (!finalDir.mkpath(".")) {
                setError(QString("Failed to create database directory: %1").arg(finalDir.path()));
                return false;
            }
        }
        
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setDatabaseName(dbPath);
    } else {
        m_db = QSqlDatabase::addDatabase("QPSQL");
        m_db.setHostName(config.host);
        m_db.setPort(config.port);
        m_db.setDatabaseName(config.name);
        m_db.setUserName(config.user);
        m_db.setPassword(config.password);
    }

    if (!m_db.open()) {
        setError(QString("Failed to open database: %1").arg(m_db.lastError().text()));
        return false;
    }

    // Run Migrations automatically
    if (!runMigrations()) {
        return false;
    }

    qDebug() << "Database initialized successfully";
    return true;
}

bool DatabaseManager::runMigrations()
{
    if (!isOpen()) {
        setError("Database not open");
        return false;
    }

    // Determine migration dir based on driver
    QString driver = m_db.driverName();
    QString migrationDir;
    
    if (driver == "QSQLITE") {
        migrationDir = ":/migrations/sqlite";
    } else {
        migrationDir = ":/migrations/postgresql";
    }
    
    MigrationManager migrator(m_db, migrationDir);
    if (!migrator.migrate()) {
        setError(QString("Migration failed: %1").arg(migrator.getLastError()));
        return false;
    }
    
    qDebug() << "Migrations executed successfully";
    return true;
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
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

