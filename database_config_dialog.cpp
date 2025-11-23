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
#include <QApplication>
#include <QGroupBox>
#include <QStackedWidget>
#include <QStyle>

DatabaseConfigDialog::DatabaseConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Database Configuration - FingerprintApp");
    resize(500, 450);
    setupUI();
    
    // Load existing or defaults
    if (hasConfig()) {
        Config cfg = loadConfig();
        m_typeCombo->setCurrentText(cfg.type);
        m_hostEdit->setText(cfg.host);
        m_portEdit->setText(QString::number(cfg.port));
        m_nameEdit->setText(cfg.name);
        m_pgNameEdit->setText(cfg.name); // Ensure PG name is also set if type matches
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
            m_typeCombo->removeItem(index);
            m_statusLabel->setText("Note: PostgreSQL driver not installed.");
            m_statusLabel->setStyleSheet("color: orange; font-style: italic;");
        }
    }
}

void DatabaseConfigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(style()->standardIcon(QStyle::SP_DriveHDIcon).pixmap(48, 48));
    
    QVBoxLayout* headerTextLayout = new QVBoxLayout();
    QLabel* titleLabel = new QLabel("Database Setup");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    QLabel* descLabel = new QLabel("Configure your database connection settings.");
    descLabel->setStyleSheet("color: #666;");
    headerTextLayout->addWidget(titleLabel);
    headerTextLayout->addWidget(descLabel);
    
    headerLayout->addWidget(iconLabel);
    headerLayout->addLayout(headerTextLayout);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Divider
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // Connection Settings Group
    QGroupBox* settingsGroup = new QGroupBox("Connection Settings");
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);
    settingsLayout->setSpacing(15);
    
    // Database Type Selection
    QHBoxLayout* typeLayout = new QHBoxLayout();
    QLabel* typeLabel = new QLabel("Driver Type:");
    typeLabel->setFixedWidth(100);
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"SQLITE", "POSTGRESQL"});
    m_typeCombo->setStyleSheet("padding: 5px;");
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, &DatabaseConfigDialog::onTypeChanged);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(m_typeCombo);
    settingsLayout->addLayout(typeLayout);
    
    // Settings Stack
    m_settingsStack = new QStackedWidget();
    
    // Page 1: SQLite
    QWidget* sqlitePage = new QWidget();
    QFormLayout* sqliteLayout = new QFormLayout(sqlitePage);
    sqliteLayout->setLabelAlignment(Qt::AlignLeft);
    sqliteLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    QHBoxLayout* nameLayout = new QHBoxLayout();
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Path to .db file");
    m_nameEdit->setStyleSheet("padding: 5px;");
    m_browseBtn = new QPushButton("Browse...");
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_browseBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onBrowseClicked);
    nameLayout->addWidget(m_nameEdit);
    nameLayout->addWidget(m_browseBtn);
    
    sqliteLayout->addRow("Database File:", nameLayout);
    QLabel* sqliteHint = new QLabel("SQLite stores data in a local file. Recommended for single-user setup.");
    sqliteHint->setStyleSheet("color: #888; font-size: 11px; font-style: italic; margin-top: 5px;");
    sqliteHint->setWordWrap(true);
    sqliteLayout->addRow("", sqliteHint);
    
    m_settingsStack->addWidget(sqlitePage);
    
    // Page 2: PostgreSQL
    QWidget* pgPage = new QWidget();
    QFormLayout* pgLayout = new QFormLayout(pgPage);
    pgLayout->setLabelAlignment(Qt::AlignLeft);
    pgLayout->setSpacing(10);
    
    m_hostEdit = new QLineEdit("localhost");
    m_hostEdit->setStyleSheet("padding: 5px;");
    pgLayout->addRow("Host:", m_hostEdit);
    
    m_portEdit = new QLineEdit("5432");
    m_portEdit->setStyleSheet("padding: 5px;");
    pgLayout->addRow("Port:", m_portEdit);
    
    m_pgNameEdit = new QLineEdit("fingerprint_db");
    m_pgNameEdit->setStyleSheet("padding: 5px;");
    pgLayout->addRow("Database Name:", m_pgNameEdit);
    
    m_userEdit = new QLineEdit("postgres");
    m_userEdit->setStyleSheet("padding: 5px;");
    pgLayout->addRow("Username:", m_userEdit);
    
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setStyleSheet("padding: 5px;");
    pgLayout->addRow("Password:", m_passEdit);
    
    m_settingsStack->addWidget(pgPage);
    
    settingsLayout->addWidget(m_settingsStack);
    mainLayout->addWidget(settingsGroup);
    
    // Status & Buttons
    m_statusLabel = new QLabel("");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    
    QPushButton* testBtn = new QPushButton("Test Connection");
    testBtn->setCursor(Qt::PointingHandCursor);
    testBtn->setStyleSheet("QPushButton { padding: 8px 15px; border: 1px solid #ccc; border-radius: 4px; background-color: #f8f9fa; } QPushButton:hover { background-color: #e9ecef; }");
    connect(testBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onTestClicked);
    
    m_saveBtn = new QPushButton("Save & Continue");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setDefault(true);
    m_saveBtn->setStyleSheet("QPushButton { padding: 8px 20px; border: none; border-radius: 4px; background-color: #007bff; color: white; font-weight: bold; } QPushButton:hover { background-color: #0056b3; }");
    connect(m_saveBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onSaveClicked);
    
    m_migrateBtn = new QPushButton("Run Migrations");
    m_migrateBtn->setCursor(Qt::PointingHandCursor);
    m_migrateBtn->setStyleSheet("QPushButton { padding: 8px 15px; border: 1px solid #ccc; border-radius: 4px; background-color: #ffc107; color: #000; font-weight: bold; } QPushButton:hover { background-color: #e0a800; }");
    // Only show migration button if config exists, or maybe always show but warn?
    // Let's just connect it.
    connect(m_migrateBtn, &QPushButton::clicked, this, [this]() {
        emit runMigrationsRequested();
    });

    btnLayout->addWidget(m_migrateBtn);
    btnLayout->addStretch();
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
    m_statusLabel->clear();
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
    
    m_statusLabel->setText("Testing connection...");
    m_statusLabel->setStyleSheet("color: blue;");
    QCoreApplication::processEvents();
    
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
        m_statusLabel->setText("✓ Connection Successful!");
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        db.close();
    } else {
        m_statusLabel->setText("✗ Connection Failed: " + db.lastError().text());
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        
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
    
    QString storedName = settings.value("DB/Name", "fingerprint.db").toString();
    cfg.name = storedName;
    
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
