#include "backend_config_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QCoreApplication>

BackendConfigDialog::BackendConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Backend Configuration - FingerprintApp");
    resize(500, 200);
    setupUI();
    
    // Load existing URL
    QString url = loadBackendUrl();
    if (url.isEmpty()) {
        url = "http://localhost:3000";
    }
    m_urlEdit->setText(url);
}

void BackendConfigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header
    QLabel* titleLabel = new QLabel("Backend API Configuration");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    mainLayout->addWidget(titleLabel);
    
    QLabel* descLabel = new QLabel("Enter the backend API URL (e.g., http://localhost:3000)");
    descLabel->setStyleSheet("color: #666;");
    mainLayout->addWidget(descLabel);
    
    // Divider
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // Form
    QFormLayout* formLayout = new QFormLayout();
    
    m_urlEdit = new QLineEdit();
    m_urlEdit->setPlaceholderText("http://localhost:3000");
    formLayout->addRow("Backend URL:", m_urlEdit);
    
    mainLayout->addLayout(formLayout);
    
    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: blue;");
    mainLayout->addWidget(m_statusLabel);
    
    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* testBtn = new QPushButton("Test Connection");
    connect(testBtn, &QPushButton::clicked, this, &BackendConfigDialog::onTestClicked);
    btnLayout->addWidget(testBtn);
    
    btnLayout->addStretch();
    
    QPushButton* cancelBtn = new QPushButton("Cancel");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    m_saveBtn = new QPushButton("Save");
    m_saveBtn->setDefault(true);
    connect(m_saveBtn, &QPushButton::clicked, this, &BackendConfigDialog::onSaveClicked);
    btnLayout->addWidget(m_saveBtn);
    
    mainLayout->addLayout(btnLayout);
}

void BackendConfigDialog::onTestClicked()
{
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        m_statusLabel->setText("Please enter a URL");
        m_statusLabel->setStyleSheet("color: red;");
        return;
    }
    
    m_statusLabel->setText("Testing connection...");
    m_statusLabel->setStyleSheet("color: blue;");
    QCoreApplication::processEvents();
    
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QUrl testUrl(url + "/users");
    QNetworkRequest request(testUrl);
    
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            m_statusLabel->setText("✓ Connection Successful!");
            m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            m_statusLabel->setText("✗ Connection Failed: " + reply->errorString());
            m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

void BackendConfigDialog::onSaveClicked()
{
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "Invalid URL", "Please enter a backend URL");
        return;
    }
    
    saveBackendUrl(url);
    accept();
}

QString BackendConfigDialog::loadBackendUrl()
{
    QSettings settings("Arkana", "FingerprintApp");
    return settings.value("Backend/Url", "").toString();
}

void BackendConfigDialog::saveBackendUrl(const QString& url)
{
    QSettings settings("Arkana", "FingerprintApp");
    settings.setValue("Backend/Url", url);
    settings.setValue("Backend/Configured", true);
}

bool BackendConfigDialog::hasConfig()
{
    QSettings settings("Arkana", "FingerprintApp");
    return settings.value("Backend/Configured", false).toBool();
}

