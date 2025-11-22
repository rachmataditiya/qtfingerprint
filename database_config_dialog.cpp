#include "database_config_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QGroupBox>
#include <QStackedWidget>

DatabaseConfigDialog::DatabaseConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Database Configuration");
    setupUI();
    
    // Load existing or defaults
    if (hasConfig()) {
        Config cfg = loadConfig();
        m_typeCombo->setCurrentText(cfg.type);
        m_hostEdit->setText(cfg.host);
        m_portEdit->setText(QString::number(cfg.port));
        m_nameEdit->setText(cfg.name);
        m_userEdit->setText(cfg.user);
        m_passEdit->setText(cfg.password);
    } else {
        // Set default secure path for SQLite
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataPath);
        if (!dir.exists()) dir.mkpath(".");
        m_nameEdit->setText(dir.filePath("fingerprint.db"));
    }
    
    onTypeChanged(m_typeCombo->currentText());
    
    // Check available drivers
    QStringList drivers = QSqlDatabase::drivers();
    if (!drivers.contains("QPSQL")) {
        int index = m_typeCombo->findText("POSTGRESQL");
        if (index >= 0) {
            m_typeCombo->setItemData(index, 0, Qt::UserRole - 1); // Disable item in view (requires delegate or just remove)
            // Simpler: Remove and add Note
            m_typeCombo->removeItem(index);
            m_statusLabel->setText("PostgreSQL driver (QPSQL) not found.");
        }
    }
}

void DatabaseConfigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Database Type Selection
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Database Type:"));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"SQLITE", "POSTGRESQL"});
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, &DatabaseConfigDialog::onTypeChanged);
    typeLayout->addWidget(m_typeCombo);
    mainLayout->addLayout(typeLayout);
    
    // Settings Stack
    m_settingsStack = new QStackedWidget();
    
    // Page 1: SQLite
    QWidget* sqlitePage = new QWidget();
    QFormLayout* sqliteLayout = new QFormLayout(sqlitePage);
    
    QHBoxLayout* nameLayout = new QHBoxLayout();
    m_nameEdit = new QLineEdit(); 
    m_browseBtn = new QPushButton("Browse...");
    connect(m_browseBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onBrowseClicked);
    nameLayout->addWidget(m_nameEdit);
    nameLayout->addWidget(m_browseBtn);
    
    sqliteLayout->addRow("Database File:", nameLayout);
    m_settingsStack->addWidget(sqlitePage);
    
    // Page 2: PostgreSQL
    QWidget* pgPage = new QWidget();
    QFormLayout* pgLayout = new QFormLayout(pgPage);
    
    m_hostEdit = new QLineEdit("localhost");
    pgLayout->addRow("Host:", m_hostEdit);
    
    m_portEdit = new QLineEdit("5432");
    pgLayout->addRow("Port:", m_portEdit);
    
    m_pgNameEdit = new QLineEdit("fingerprint_db");
    pgLayout->addRow("Database Name:", m_pgNameEdit);
    
    m_userEdit = new QLineEdit("postgres");
    pgLayout->addRow("Username:", m_userEdit);
    
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    pgLayout->addRow("Password:", m_passEdit);
    
    m_settingsStack->addWidget(pgPage);
    
    mainLayout->addWidget(m_settingsStack);
    
    m_statusLabel = new QLabel("");
    m_statusLabel->setStyleSheet("color: red");
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* testBtn = new QPushButton("Test Connection");
    connect(testBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onTestClicked);
    
    m_saveBtn = new QPushButton("Save && Continue");
    m_saveBtn->setDefault(true);
    connect(m_saveBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onSaveClicked);
    
    btnLayout->addWidget(testBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

void DatabaseConfigDialog::onTypeChanged(const QString& type)
{
    if (type == "SQLITE") {
        m_settingsStack->setCurrentIndex(0);
        
        // Default path check
        if (m_nameEdit->text().isEmpty() || (!m_nameEdit->text().contains("/") && !m_nameEdit->text().contains("\\"))) {
             QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
             m_nameEdit->setText(QDir(dataPath).filePath("fingerprint.db"));
        }
    } else {
        m_settingsStack->setCurrentIndex(1);
    }
}

void DatabaseConfigDialog::onBrowseClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Select Database File", 
                                                  m_nameEdit->text(), 
                                                  "SQLite Database (*.db *.sqlite)");
    if (!fileName.isEmpty()) {
        m_nameEdit->setText(fileName);
    }
}

void DatabaseConfigDialog::onTestClicked()
{
    QSqlDatabase db;
    QString type = m_typeCombo->currentText();
    
    if (type == "SQLITE") {
        db = QSqlDatabase::addDatabase("QSQLITE", "TEST_CONN");
        db.setDatabaseName(m_nameEdit->text());
    } else {
        db = QSqlDatabase::addDatabase("QPSQL", "TEST_CONN");
        db.setHostName(m_hostEdit->text());
        db.setPort(m_portEdit->text().toInt());
        db.setDatabaseName(m_pgNameEdit->text());
        db.setUserName(m_userEdit->text());
        db.setPassword(m_passEdit->text());
    }
    
    if (db.open()) {
        m_statusLabel->setStyleSheet("color: green");
        m_statusLabel->setText("Connection Successful!");
        db.close();
    } else {
        m_statusLabel->setStyleSheet("color: red");
        m_statusLabel->setText("Connection Failed: " + db.lastError().text());
        
        // Hint about missing driver
        if (db.lastError().type() == QSqlError::ConnectionError && type == "POSTGRESQL") {
             if (!QSqlDatabase::drivers().contains("QPSQL")) {
                 m_statusLabel->setText("Error: PostgreSQL driver (QPSQL) not installed.");
             }
        }
    }
    QSqlDatabase::removeDatabase("TEST_CONN");
}

void DatabaseConfigDialog::onSaveClicked()
{
    Config cfg;
    cfg.type = m_typeCombo->currentText();
    if (cfg.type == "SQLITE") {
        cfg.name = m_nameEdit->text();
    } else {
        cfg.host = m_hostEdit->text();
        cfg.port = m_portEdit->text().toInt();
        cfg.name = m_pgNameEdit->text();
        cfg.user = m_userEdit->text();
        cfg.password = m_passEdit->text();
    }
    
    saveConfig(cfg);
    accept();
}

DatabaseConfigDialog::Config DatabaseConfigDialog::loadConfig()
{
    QSettings settings("Arkana", "FingerprintApp");
    Config cfg;
    cfg.type = settings.value("DB/Type", "SQLITE").toString();
    cfg.host = settings.value("DB/Host", "localhost").toString();
    cfg.port = settings.value("DB/Port", 5432).toInt();
    
    // Handle legacy config which might store sqlite filename in same key as postgres db name
    // Ideally we should have separated them, but let's infer
    QString storedName = settings.value("DB/Name", "fingerprint.db").toString();
    
    if (cfg.type == "SQLITE") {
        cfg.name = storedName;
    } else {
        cfg.name = storedName; // For postgres this is dbname
    }
    
    cfg.user = settings.value("DB/User", "postgres").toString();
    cfg.password = settings.value("DB/Password", "").toString();
    return cfg;
}

void DatabaseConfigDialog::saveConfig(const Config& config)
{
    QSettings settings("Arkana", "FingerprintApp");
    settings.setValue("DB/Type", config.type);
    settings.setValue("DB/Host", config.host);
    settings.setValue("DB/Port", config.port);
    settings.setValue("DB/Name", config.name);
    settings.setValue("DB/User", config.user);
    settings.setValue("DB/Password", config.password);
    settings.setValue("DB/Configured", true);
}

bool DatabaseConfigDialog::hasConfig()
{
    QSettings settings("Arkana", "FingerprintApp");
    return settings.value("DB/Configured", false).toBool();
}
