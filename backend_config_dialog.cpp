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
#include <QFrame>
#include <QGroupBox>
#include <QValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QIcon>
#include <QStyle>

BackendConfigDialog::BackendConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Backend API Configuration");
    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    resize(550, 350);
    setMinimumSize(500, 300);
    setupUI();
    
    // Load existing URL
    QString url = loadBackendUrl();
    if (url.isEmpty()) {
        url = "http://localhost:3000";
    }
    m_urlEdit->setText(url);
    
    // Validate URL format
    QRegularExpression urlRegex("^https?://[\\w\\-\\.]+(:\\d+)?(/.*)?$");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(urlRegex, this);
    m_urlEdit->setValidator(validator);
}

void BackendConfigDialog::setupUI()
{
    // Main layout with padding
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Header section with gradient-like background
    QFrame* headerFrame = new QFrame();
    headerFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #f8f9fa;"
        "  border-bottom: 2px solid #e0e0e0;"
        "  padding: 20px;"
        "}"
    );
    QVBoxLayout* headerLayout = new QVBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 20, 20, 20);
    headerLayout->setSpacing(8);
    
    QLabel* titleLabel = new QLabel("Backend API Configuration");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 20px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  padding: 0px;"
        "}"
    );
    headerLayout->addWidget(titleLabel);
    
    QLabel* descLabel = new QLabel("Configure the backend API server URL for fingerprint management");
    descLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 13px;"
        "  color: #7f8c8d;"
        "  padding: 0px;"
        "}"
    );
    headerLayout->addWidget(descLabel);
    
    mainLayout->addWidget(headerFrame);
    
    // Content section
    QFrame* contentFrame = new QFrame();
    contentFrame->setStyleSheet(
        "QFrame {"
        "  background-color: white;"
        "  padding: 30px;"
        "}"
    );
    QVBoxLayout* contentLayout = new QVBoxLayout(contentFrame);
    contentLayout->setContentsMargins(30, 30, 30, 30);
    contentLayout->setSpacing(20);
    
    // URL input group
    QGroupBox* urlGroup = new QGroupBox("Backend URL");
    urlGroup->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  color: #34495e;"
        "  border: 2px solid #e0e0e0;"
        "  border-radius: 8px;"
        "  margin-top: 12px;"
        "  padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 8px;"
        "}"
    );
    QVBoxLayout* urlGroupLayout = new QVBoxLayout(urlGroup);
    urlGroupLayout->setContentsMargins(15, 20, 15, 15);
    urlGroupLayout->setSpacing(10);
    
    QLabel* urlHintLabel = new QLabel("Enter the full URL of your backend API server:");
    urlHintLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 12px;"
        "  color: #7f8c8d;"
        "  padding: 0px;"
        "}"
    );
    urlGroupLayout->addWidget(urlHintLabel);
    
    m_urlEdit = new QLineEdit();
    m_urlEdit->setPlaceholderText("http://localhost:3000");
    m_urlEdit->setMinimumHeight(40);
    m_urlEdit->setStyleSheet(
        "QLineEdit {"
        "  font-size: 14px;"
        "  padding: 10px;"
        "  border: 2px solid #ddd;"
        "  border-radius: 6px;"
        "  background-color: #fafafa;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #3498db;"
        "  background-color: white;"
        "}"
        "QLineEdit:invalid {"
        "  border: 2px solid #e74c3c;"
        "}"
    );
    urlGroupLayout->addWidget(m_urlEdit);
    
    QLabel* exampleLabel = new QLabel("Example: http://localhost:3000 or http://192.168.1.100:3000");
    exampleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 11px;"
        "  color: #95a5a6;"
        "  font-style: italic;"
        "  padding: 0px;"
        "}"
    );
    urlGroupLayout->addWidget(exampleLabel);
    
    contentLayout->addWidget(urlGroup);
    
    // Status section
    QFrame* statusFrame = new QFrame();
    statusFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #f8f9fa;"
        "  border: 1px solid #e0e0e0;"
        "  border-radius: 6px;"
        "  padding: 12px;"
        "  min-height: 40px;"
        "}"
    );
    QHBoxLayout* statusLayout = new QHBoxLayout(statusFrame);
    statusLayout->setContentsMargins(12, 8, 12, 8);
    
    m_statusLabel = new QLabel("Ready to configure");
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 13px;"
        "  color: #7f8c8d;"
        "  padding: 0px;"
        "}"
    );
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    
    contentLayout->addWidget(statusFrame);
    
    mainLayout->addWidget(contentFrame);
    
    // Button section
    QFrame* buttonFrame = new QFrame();
    buttonFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #f8f9fa;"
        "  border-top: 1px solid #e0e0e0;"
        "  padding: 15px 20px;"
        "}"
    );
    QHBoxLayout* btnLayout = new QHBoxLayout(buttonFrame);
    btnLayout->setContentsMargins(20, 15, 20, 15);
    btnLayout->setSpacing(12);
    
    QPushButton* testBtn = new QPushButton("Test Connection");
    testBtn->setMinimumHeight(38);
    testBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  padding: 10px 20px;"
        "  background-color: #3498db;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2980b9;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #21618c;"
        "}"
    );
    connect(testBtn, &QPushButton::clicked, this, &BackendConfigDialog::onTestClicked);
    btnLayout->addWidget(testBtn);
    
    btnLayout->addStretch();
    
    QPushButton* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setMinimumHeight(38);
    cancelBtn->setMinimumWidth(100);
    cancelBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 13px;"
        "  padding: 10px 20px;"
        "  background-color: #ecf0f1;"
        "  color: #2c3e50;"
        "  border: 1px solid #bdc3c7;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #d5dbdb;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #aab7b8;"
        "}"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    m_saveBtn = new QPushButton("Save Configuration");
    m_saveBtn->setDefault(true);
    m_saveBtn->setMinimumHeight(38);
    m_saveBtn->setMinimumWidth(150);
    m_saveBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  padding: 10px 20px;"
        "  background-color: #27ae60;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #229954;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1e8449;"
        "}"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &BackendConfigDialog::onSaveClicked);
    btnLayout->addWidget(m_saveBtn);
    
    mainLayout->addWidget(buttonFrame);
    
    // Set dialog style
    setStyleSheet(
        "QDialog {"
        "  background-color: white;"
        "}"
    );
}

void BackendConfigDialog::onTestClicked()
{
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        m_statusLabel->setText("⚠ Please enter a backend URL");
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "  font-size: 13px;"
            "  color: #e67e22;"
            "  font-weight: bold;"
            "  padding: 0px;"
            "}"
        );
        m_urlEdit->setFocus();
        return;
    }
    
    // Validate URL format
    QUrl testUrl(url);
    if (!testUrl.isValid() || testUrl.scheme().isEmpty()) {
        m_statusLabel->setText("⚠ Invalid URL format. Please use http:// or https://");
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "  font-size: 13px;"
            "  color: #e74c3c;"
            "  font-weight: bold;"
            "  padding: 0px;"
            "}"
        );
        m_urlEdit->setFocus();
        return;
    }
    
    m_statusLabel->setText("🔄 Testing connection...");
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 13px;"
        "  color: #3498db;"
        "  font-weight: bold;"
        "  padding: 0px;"
        "}"
    );
    QCoreApplication::processEvents();
    
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QUrl testEndpoint(url);
    if (!testEndpoint.path().endsWith("/users")) {
        testEndpoint.setPath("/users");
    }
    QNetworkRequest request(testEndpoint);
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(5000); // 5 second timeout
    
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply, manager, url]() {
        if (reply->error() == QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 200 && statusCode < 300) {
                m_statusLabel->setText("✓ Connection successful! Backend is reachable.");
                m_statusLabel->setStyleSheet(
                    "QLabel {"
                    "  font-size: 13px;"
                    "  color: #27ae60;"
                    "  font-weight: bold;"
                    "  padding: 0px;"
                    "}"
                );
            } else {
                m_statusLabel->setText(QString("⚠ Backend responded with status %1").arg(statusCode));
                m_statusLabel->setStyleSheet(
                    "QLabel {"
                    "  font-size: 13px;"
                    "  color: #f39c12;"
                    "  font-weight: bold;"
                    "  padding: 0px;"
                    "}"
                );
            }
        } else {
            QString errorMsg = reply->errorString();
            if (errorMsg.contains("timeout", Qt::CaseInsensitive)) {
                m_statusLabel->setText("✗ Connection timeout. Check if backend is running.");
            } else if (errorMsg.contains("refused", Qt::CaseInsensitive)) {
                m_statusLabel->setText("✗ Connection refused. Backend may not be running.");
            } else {
                m_statusLabel->setText(QString("✗ Connection failed: %1").arg(errorMsg));
            }
            m_statusLabel->setStyleSheet(
                "QLabel {"
                "  font-size: 13px;"
                "  color: #e74c3c;"
                "  font-weight: bold;"
                "  padding: 0px;"
                "}"
            );
        }
        reply->deleteLater();
        manager->deleteLater();
    });
    
    // Handle timeout
    connect(reply, &QNetworkReply::errorOccurred, [this, reply](QNetworkReply::NetworkError error) {
        if (error == QNetworkReply::TimeoutError) {
            m_statusLabel->setText("✗ Connection timeout. Check if backend is running.");
            m_statusLabel->setStyleSheet(
                "QLabel {"
                "  font-size: 13px;"
                "  color: #e74c3c;"
                "  font-weight: bold;"
                "  padding: 0px;"
                "}"
            );
        }
    });
}

void BackendConfigDialog::onSaveClicked()
{
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "Invalid URL", 
            "Please enter a backend URL.\n\nExample: http://localhost:3000");
        m_urlEdit->setFocus();
        return;
    }
    
    // Validate URL format
    QUrl testUrl(url);
    if (!testUrl.isValid() || testUrl.scheme().isEmpty()) {
        QMessageBox::warning(this, "Invalid URL Format", 
            "Please enter a valid URL with protocol (http:// or https://).\n\n"
            "Example: http://localhost:3000");
        m_urlEdit->setFocus();
        return;
    }
    
    // Remove trailing slash
    if (url.endsWith("/")) {
        url.chop(1);
        m_urlEdit->setText(url);
    }
    
    saveBackendUrl(url);
    
    QMessageBox::information(this, "Configuration Saved", 
        QString("Backend URL has been saved successfully.\n\nURL: %1\n\n"
                "The application will now use this URL for all API requests.")
        .arg(url));
    
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

