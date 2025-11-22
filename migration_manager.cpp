#include "migration_manager.h"
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

MigrationManager::MigrationManager(QSqlDatabase& db, const QString& migrationsDir)
    : m_db(db), m_migrationsDir(migrationsDir)
{
}

bool MigrationManager::migrate()
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    init();
    if (!m_lastError.isEmpty()) return false;

    QDir dir(m_migrationsDir);
    QStringList files = dir.entryList({"*.sql"}, QDir::Files, QDir::Name); // Sorted by name
    
    bool started = false;
    if (m_lastFile.isEmpty()) {
        started = true;
    }

    for (const QString& file : files) {
        if (!started) {
            if (file == m_lastFile) {
                started = true;
                continue;
            }
            continue;
        }

        qDebug() << "Executing migration:" << file;
        if (!executeFile(dir.filePath(file))) {
            return false;
        }

        // Update migration record
        QSqlQuery query(m_db);
        query.prepare("UPDATE migrations SET name = ?");
        query.addBindValue(file);
        if (!query.exec()) {
             // If update fails (e.g. multiple rows?), try insert if empty?
             // Actually init() ensures one row exists.
             // But if it was empty string, update works.
             m_lastError = "Failed to update migration record: " + query.lastError().text();
             return false;
        }
        m_lastFile = file;
    }
    
    return true;
}

void MigrationManager::init()
{
    // Check if table exists
    QSqlQuery query(m_db);
    if (!query.exec("SELECT 1 FROM migrations LIMIT 1")) {
        if (!query.exec("CREATE TABLE migrations (name VARCHAR(255) NOT NULL DEFAULT '')")) {
            m_lastError = "Failed to create migrations table: " + query.lastError().text();
            return;
        }
        if (!query.exec("INSERT INTO migrations (name) VALUES ('')")) {
            m_lastError = "Failed to init migrations table: " + query.lastError().text();
            return;
        }
        m_lastFile = "";
    } else {
        if (query.next()) {
             // Table exists, read last file
             QSqlQuery q2(m_db);
             if (q2.exec("SELECT name FROM migrations LIMIT 1")) {
                 if (q2.next()) {
                     m_lastFile = q2.value(0).toString();
                 }
             }
        }
    }
}

bool MigrationManager::executeFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Cannot open migration file: " + filePath;
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    QStringList statements;
    
    if (m_db.driverName() == "QSQLITE") {
        statements = content.split("-- separator", Qt::SkipEmptyParts);
    } else {
        // For Postgres, usually one block is fine, or use same separator logic
        // The example implies separator is for sqlite but let's support it generally
        statements = content.split("-- separator", Qt::SkipEmptyParts);
    }

    for (const QString& stmt : statements) {
        QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty()) continue;
        
        QSqlQuery query(m_db);
        if (!query.exec(trimmed)) {
            m_lastError = QString("Migration error in %1: %2").arg(filePath).arg(query.lastError().text());
            qCritical() << m_lastError;
            return false;
        }
    }
    
    return true;
}

