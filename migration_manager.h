#ifndef MIGRATION_MANAGER_H
#define MIGRATION_MANAGER_H

#include <QString>
#include <QStringList>
#include <QSqlDatabase>

class MigrationManager {
public:
    MigrationManager(QSqlDatabase& db, const QString& migrationsDir);
    
    bool migrate();
    QString getLastError() const { return m_lastError; }

private:
    void init();
    bool executeFile(const QString& filePath);
    
    QSqlDatabase& m_db;
    QString m_migrationsDir;
    QString m_lastError;
    QString m_lastFile;
};

#endif // MIGRATION_MANAGER_H

