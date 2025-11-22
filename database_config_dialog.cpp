#include "database_config_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

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
    }
    
    onTypeChanged(m_typeCombo->currentText());
}

void DatabaseConfigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"SQLITE", "POSTGRESQL"});
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, &DatabaseConfigDialog::onTypeChanged);
    formLayout->addRow("Database Type:", m_typeCombo);
    
    m_hostEdit = new QLineEdit("localhost");
    formLayout->addRow("Host:", m_hostEdit);
    
    m_portEdit = new QLineEdit("5432");
    formLayout->addRow("Port:", m_portEdit);
    
    QHBoxLayout* nameLayout = new QHBoxLayout();
    m_nameEdit = new QLineEdit("fingerprint.db");
    m_browseBtn = new QPushButton("Browse...");
    connect(m_browseBtn, &QPushButton::clicked, this, &DatabaseConfigDialog::onBrowseClicked);
    nameLayout->addWidget(m_nameEdit);
    nameLayout->addWidget(m_browseBtn);
    formLayout->addRow("Database Name/File:", nameLayout);
    
    m_userEdit = new QLineEdit("postgres");
    formLayout->addRow("Username:", m_userEdit);
    
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Password:", m_passEdit);
    
    mainLayout->addLayout(formLayout);
    
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
    bool isSqlite = (type == "SQLITE");
    m_hostEdit->setEnabled(!isSqlite);
    m_portEdit->setEnabled(!isSqlite);
    m_userEdit->setEnabled(!isSqlite);
    m_passEdit->setEnabled(!isSqlite);
    m_browseBtn->setVisible(isSqlite);
    
    if (isSqlite) {
        if (m_nameEdit->text().isEmpty() || !m_nameEdit->text().endsWith(".db")) {
            m_nameEdit->setText("fingerprint.db");
        }
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
        db.setDatabaseName(m_nameEdit->text());
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
    }
    QSqlDatabase::removeDatabase("TEST_CONN");
}

void DatabaseConfigDialog::onSaveClicked()
{
    Config cfg;
    cfg.type = m_typeCombo->currentText();
    cfg.host = m_hostEdit->text();
    cfg.port = m_portEdit->text().toInt();
    cfg.name = m_nameEdit->text();
    cfg.user = m_userEdit->text();
    cfg.password = m_passEdit->text();
    
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
    cfg.name = settings.value("DB/Name", "fingerprint.db").toString();
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

