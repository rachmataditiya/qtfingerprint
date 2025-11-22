#include "mainwindow_app.h"
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QScrollBar>

MainWindowApp::MainWindowApp(QWidget *parent)
    : QMainWindow(parent)
    , m_fpManager(new FingerprintManager())
    , m_dbManager(new DatabaseManager("fingerprint.db", this))
    , m_enrollmentInProgress(false)
    , m_enrollmentSampleCount(0)
{
    setupUI();
    setWindowTitle("U.are.U 4500 Fingerprint Application");
    resize(1000, 700);

    // Initialize database
    if (!m_dbManager->initialize()) {
        QMessageBox::critical(this, "Database Error", 
            QString("Failed to initialize database: %1").arg(m_dbManager->getLastError()));
    } else {
        log("Database initialized successfully");
        updateUserList();
    }
}

MainWindowApp::~MainWindowApp()
{
    delete m_fpManager;
}

void MainWindowApp::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // Left panel - Controls
    QVBoxLayout* leftLayout = new QVBoxLayout();

    // Status section
    m_statusLabel = new QLabel("Status: Not initialized");
    m_statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; font-weight: bold; }");
    leftLayout->addWidget(m_statusLabel);

    // Reader initialization group
    m_readerGroup = new QGroupBox("1. Reader Initialization");
    QVBoxLayout* readerLayout = new QVBoxLayout(m_readerGroup);
    
    m_btnInitialize = new QPushButton("Initialize Reader");
    m_btnInitialize->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; }");
    readerLayout->addWidget(m_btnInitialize);
    
    m_readerStatusLabel = new QLabel("Reader: Not connected");
    m_readerStatusLabel->setStyleSheet("QLabel { color: red; }");
    readerLayout->addWidget(m_readerStatusLabel);
    
    leftLayout->addWidget(m_readerGroup);

    // Enrollment group
    m_enrollGroup = new QGroupBox("2. Enrollment (Registration)");
    QVBoxLayout* enrollLayout = new QVBoxLayout(m_enrollGroup);
    
    QLabel* nameLabel = new QLabel("Name:");
    enrollLayout->addWidget(nameLabel);
    m_editEnrollName = new QLineEdit();
    m_editEnrollName->setPlaceholderText("Enter user name");
    enrollLayout->addWidget(m_editEnrollName);
    
    QLabel* emailLabel = new QLabel("Email (optional):");
    enrollLayout->addWidget(emailLabel);
    m_editEnrollEmail = new QLineEdit();
    m_editEnrollEmail->setPlaceholderText("Enter email");
    enrollLayout->addWidget(m_editEnrollEmail);
    
    m_btnStartEnroll = new QPushButton("Start Enrollment");
    m_btnStartEnroll->setStyleSheet("QPushButton { padding: 8px; font-size: 13px; background-color: #4CAF50; color: white; }");
    m_btnStartEnroll->setEnabled(false);
    enrollLayout->addWidget(m_btnStartEnroll);
    
    m_btnCaptureEnroll = new QPushButton("Capture Fingerprint Sample");
    m_btnCaptureEnroll->setStyleSheet("QPushButton { padding: 8px; font-size: 13px; }");
    m_btnCaptureEnroll->setEnabled(false);
    enrollLayout->addWidget(m_btnCaptureEnroll);
    
    m_enrollProgress = new QProgressBar();
    m_enrollProgress->setRange(0, 5);
    m_enrollProgress->setValue(0);
    enrollLayout->addWidget(m_enrollProgress);
    
    m_enrollStatusLabel = new QLabel("Ready to enroll");
    enrollLayout->addWidget(m_enrollStatusLabel);
    
    leftLayout->addWidget(m_enrollGroup);

    // Verification group
    m_verifyGroup = new QGroupBox("3. Verification");
    QVBoxLayout* verifyLayout = new QVBoxLayout(m_verifyGroup);
    
    m_btnStartVerify = new QPushButton("Start Verification");
    m_btnStartVerify->setStyleSheet("QPushButton { padding: 8px; font-size: 13px; background-color: #2196F3; color: white; }");
    m_btnStartVerify->setEnabled(false);
    verifyLayout->addWidget(m_btnStartVerify);
    
    m_btnCaptureVerify = new QPushButton("Capture for Verification");
    m_btnCaptureVerify->setStyleSheet("QPushButton { padding: 8px; font-size: 13px; }");
    m_btnCaptureVerify->setEnabled(false);
    verifyLayout->addWidget(m_btnCaptureVerify);
    
    m_verifyResultLabel = new QLabel("Result: -");
    m_verifyResultLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; padding: 5px; }");
    verifyLayout->addWidget(m_verifyResultLabel);
    
    m_verifyScoreLabel = new QLabel("Score: -");
    verifyLayout->addWidget(m_verifyScoreLabel);
    
    leftLayout->addWidget(m_verifyGroup);

    leftLayout->addStretch();

    // Right panel - User list and log
    QVBoxLayout* rightLayout = new QVBoxLayout();

    // User list group
    m_userListGroup = new QGroupBox("Registered Users");
    QVBoxLayout* userListLayout = new QVBoxLayout(m_userListGroup);
    
    m_userList = new QListWidget();
    userListLayout->addWidget(m_userList);
    
    QHBoxLayout* userButtonsLayout = new QHBoxLayout();
    m_btnRefreshList = new QPushButton("Refresh");
    m_btnDeleteUser = new QPushButton("Delete User");
    m_btnDeleteUser->setStyleSheet("QPushButton { background-color: #f44336; color: white; }");
    m_btnDeleteUser->setEnabled(false);
    userButtonsLayout->addWidget(m_btnRefreshList);
    userButtonsLayout->addWidget(m_btnDeleteUser);
    userListLayout->addLayout(userButtonsLayout);
    
    m_userCountLabel = new QLabel("Total users: 0");
    userListLayout->addWidget(m_userCountLabel);
    
    rightLayout->addWidget(m_userListGroup);

    // Log section
    QGroupBox* logGroup = new QGroupBox("Activity Log");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_logText = new QTextEdit();
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(200);
    logLayout->addWidget(m_logText);
    
    m_btnClearLog = new QPushButton("Clear Log");
    logLayout->addWidget(m_btnClearLog);
    
    rightLayout->addWidget(logGroup);

    // Add panels to main layout
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 1);

    // Connect signals
    connect(m_btnInitialize, &QPushButton::clicked, this, &MainWindowApp::onInitializeClicked);
    connect(m_btnStartEnroll, &QPushButton::clicked, this, &MainWindowApp::onEnrollClicked);
    connect(m_btnCaptureEnroll, &QPushButton::clicked, this, &MainWindowApp::onCaptureEnrollSample);
    connect(m_btnStartVerify, &QPushButton::clicked, this, &MainWindowApp::onVerifyClicked);
    connect(m_btnCaptureVerify, &QPushButton::clicked, this, &MainWindowApp::onCaptureVerifySample);
    connect(m_btnRefreshList, &QPushButton::clicked, this, &MainWindowApp::onRefreshUserList);
    connect(m_btnDeleteUser, &QPushButton::clicked, this, &MainWindowApp::onDeleteUserClicked);
    connect(m_btnClearLog, &QPushButton::clicked, this, &MainWindowApp::onClearLog);
    connect(m_userList, &QListWidget::itemClicked, this, &MainWindowApp::onUserSelected);
}

void MainWindowApp::onInitializeClicked()
{
    log("Initializing fingerprint reader using DigitalPersona Library...");
    log(QString("Library version: %1").arg(DigitalPersona::version()));
    
    if (!m_fpManager->initialize()) {
        updateStatus("Initialization failed", true);
        log(QString("Error: %1").arg(m_fpManager->getLastError()));
        QMessageBox::critical(this, "Error", m_fpManager->getLastError());
        return;
    }
    
    if (!m_fpManager->openReader()) {
        updateStatus("Failed to open reader", true);
        log(QString("Error: %1").arg(m_fpManager->getLastError()));
        QMessageBox::critical(this, "Error", m_fpManager->getLastError());
        return;
    }
    
    updateStatus("Reader initialized successfully", false);
    log("✓ Reader opened successfully");
    m_readerStatusLabel->setText("Reader: Connected");
    m_readerStatusLabel->setStyleSheet("QLabel { color: green; }");
    
    m_btnInitialize->setEnabled(false);
    m_btnStartEnroll->setEnabled(true);
    m_btnStartVerify->setEnabled(true);
}

void MainWindowApp::onEnrollClicked()
{
    QString name = m_editEnrollName->text().trimmed();
    QString email = m_editEnrollEmail->text().trimmed();
    
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter a name");
        return;
    }
    
    if (m_dbManager->userExists(name)) {
        QMessageBox::warning(this, "User Exists", "A user with this name already exists");
        return;
    }
    
    if (!m_fpManager->startEnrollment()) {
        QMessageBox::critical(this, "Error", m_fpManager->getLastError());
        return;
    }
    
    m_enrollmentInProgress = true;
    m_enrollmentSampleCount = 0;
    m_enrollmentUserName = name;
    m_enrollmentUserEmail = email;
    
    m_enrollProgress->setValue(0);
    m_enrollStatusLabel->setText("Enrollment started. Ready to capture.");
    log(QString("Starting enrollment for: %1").arg(name));
    
    enableEnrollmentControls(false);
    m_btnCaptureEnroll->setEnabled(true);
}

void MainWindowApp::onCaptureEnrollSample()
{
    if (!m_enrollmentInProgress) {
        return;
    }
    
    m_btnCaptureEnroll->setEnabled(false);
    m_enrollStatusLabel->setText("📌 Scan your finger 5 times...");
    log("=== ENROLLMENT: Scan your finger 5 times ===");
    
    QApplication::processEvents();
    
    QString message;
    int quality;
    int result = m_fpManager->addEnrollmentSample(message, quality);
    
    if (result < 0) {
        log(QString("❌ ERROR: %1").arg(m_fpManager->getLastError()));
        m_enrollStatusLabel->setText("Capture failed");
        m_enrollmentInProgress = false;
        m_enrollProgress->setValue(0);
        enableEnrollmentControls(true);
        QMessageBox::critical(this, "Enrollment Error", m_fpManager->getLastError());
        return;
    }
    
    m_enrollProgress->setValue(5);
    m_enrollStatusLabel->setText(message);
    log(message);
    
    if (result == 1) {
        log("Saving fingerprint template to database...");
        
        QByteArray templateData;
        if (!m_fpManager->createEnrollmentTemplate(templateData)) {
            QMessageBox::critical(this, "Error", "Failed to create fingerprint template");
            log("❌ Error creating template");
            m_fpManager->cancelEnrollment();
            m_enrollmentInProgress = false;
            m_enrollProgress->setValue(0);
            enableEnrollmentControls(true);
            return;
        }
        
        log(QString("Template created, size: %1 bytes").arg(templateData.size()));
        
        int userId;
        if (!m_dbManager->addUser(m_enrollmentUserName, m_enrollmentUserEmail, templateData, userId)) {
            QMessageBox::critical(this, "Database Error", 
                QString("Failed to save user:\n%1").arg(m_dbManager->getLastError()));
            log(QString("❌ Database error: %1").arg(m_dbManager->getLastError()));
        } else {
            log(QString("✓ User enrolled successfully: %1 (ID: %2)").arg(m_enrollmentUserName).arg(userId));
            QMessageBox::information(this, "Enrollment Complete", 
                QString("✓ User '%1' enrolled successfully!\n\nUser ID: %2\nTemplate size: %3 bytes")
                    .arg(m_enrollmentUserName)
                    .arg(userId)
                    .arg(templateData.size()));
            updateUserList();
            
            m_editEnrollName->clear();
            m_editEnrollEmail->clear();
        }
        
        log("Cleaning up enrollment session...");
        m_fpManager->cancelEnrollment();
        m_enrollmentInProgress = false;
        m_enrollProgress->setValue(0);
        m_enrollStatusLabel->setText("Ready to enroll next user");
        enableEnrollmentControls(true);
        log("=== ENROLLMENT SESSION COMPLETED ===");
    }
}

void MainWindowApp::onVerifyClicked()
{
    if (m_userList->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select a user from the list");
        return;
    }
    
    m_btnStartVerify->setEnabled(false);
    m_btnCaptureVerify->setEnabled(true);
    m_verifyResultLabel->setText("Result: Waiting for capture...");
    m_verifyScoreLabel->setText("Score: -");
    log("Verification started. Place finger on reader.");
}

void MainWindowApp::onCaptureVerifySample()
{
    if (m_userList->selectedItems().isEmpty()) {
        return;
    }
    
    m_btnCaptureVerify->setEnabled(false);
    log("=== VERIFICATION: Place finger on reader NOW ===");
    m_verifyResultLabel->setText("Capturing...");
    m_verifyScoreLabel->setText("Please wait...");
    
    QApplication::processEvents();
    
    QListWidgetItem* item = m_userList->selectedItems().first();
    int userId = item->data(Qt::UserRole).toInt();
    
    User user;
    if (!m_dbManager->getUserById(userId, user)) {
        QMessageBox::critical(this, "Error", "Failed to load user data");
        log("❌ Failed to load user data");
        m_btnStartVerify->setEnabled(true);
        return;
    }
    
    log(QString("Verifying against: %1").arg(user.name));
    
    int score = 0;
    bool matched = m_fpManager->verifyFingerprint(user.fingerprintTemplate, score);
    
    if (!matched && score == 0) {
        QString error = m_fpManager->getLastError();
        log(QString("❌ Verification error: %1").arg(error));
        m_verifyResultLabel->setText("Result: ERROR");
        m_verifyResultLabel->setStyleSheet("QLabel { background-color: #ffcccc; color: red; padding: 5px; font-weight: bold; }");
        m_verifyScoreLabel->setText("Score: -");
        QMessageBox::critical(this, "Verification Error", error);
    } else {
        m_verifyScoreLabel->setText(QString("Match Score: %1%").arg(score));
        
        if (score >= 60) {
            m_verifyResultLabel->setText(QString("✓ MATCH: %1").arg(user.name));
            m_verifyResultLabel->setStyleSheet("QLabel { background-color: #c8e6c9; color: green; padding: 10px; font-weight: bold; font-size: 14px; }");
            log(QString("✓ VERIFICATION SUCCESS: %1 (score: %2%)").arg(user.name).arg(score));
            QMessageBox::information(this, "Verification Success", 
                QString("✓ Fingerprint MATCHED!\n\nUser: %1\nScore: %2%").arg(user.name).arg(score));
        } else {
            m_verifyResultLabel->setText(QString("✗ NO MATCH"));
            m_verifyResultLabel->setStyleSheet("QLabel { background-color: #ffcdd2; color: red; padding: 10px; font-weight: bold; font-size: 14px; }");
            log(QString("✗ VERIFICATION FAILED (score: %1%)").arg(score));
            QMessageBox::warning(this, "Verification Failed", 
                QString("✗ Fingerprint does NOT match!\n\nExpected: %1\nScore: %2%")
                    .arg(user.name).arg(score));
        }
    }
    
    m_btnStartVerify->setEnabled(true);
    log("=== VERIFICATION COMPLETED ===");
}

void MainWindowApp::onRefreshUserList()
{
    updateUserList();
}

void MainWindowApp::onUserSelected(QListWidgetItem* item)
{
    m_btnDeleteUser->setEnabled(item != nullptr);
}

void MainWindowApp::onDeleteUserClicked()
{
    if (m_userList->selectedItems().isEmpty()) {
        return;
    }
    
    QListWidgetItem* item = m_userList->selectedItems().first();
    int userId = item->data(Qt::UserRole).toInt();
    QString userName = item->text().section(" - ", 0, 0);
    
    auto reply = QMessageBox::question(this, "Confirm Delete", 
        QString("Are you sure you want to delete user '%1'?").arg(userName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (m_dbManager->deleteUser(userId)) {
            log(QString("User deleted: %1").arg(userName));
            updateUserList();
        } else {
            QMessageBox::critical(this, "Error", 
                QString("Failed to delete user: %1").arg(m_dbManager->getLastError()));
        }
    }
}

void MainWindowApp::onClearLog()
{
    m_logText->clear();
}

void MainWindowApp::updateStatus(const QString& status, bool isError)
{
    if (isError) {
        m_statusLabel->setText(QString("Status: ERROR - %1").arg(status));
        m_statusLabel->setStyleSheet("QLabel { background-color: #ffcccc; color: red; padding: 10px; font-weight: bold; }");
    } else {
        m_statusLabel->setText(QString("Status: %1").arg(status));
        m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: green; padding: 10px; font-weight: bold; }");
    }
}

void MainWindowApp::log(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logText->append(QString("[%1] %2").arg(timestamp).arg(message));
    m_logText->verticalScrollBar()->setValue(m_logText->verticalScrollBar()->maximum());
}

void MainWindowApp::updateUserList()
{
    m_userList->clear();
    
    QVector<User> users = m_dbManager->getAllUsers();
    
    for (const User& user : users) {
        QString displayText = QString("%1 - %2").arg(user.name).arg(user.email.isEmpty() ? "No email" : user.email);
        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, user.id);
        m_userList->addItem(item);
    }
    
    m_userCountLabel->setText(QString("Total users: %1").arg(users.size()));
    log(QString("User list updated: %1 users").arg(users.size()));
}

void MainWindowApp::enableEnrollmentControls(bool enable)
{
    m_editEnrollName->setEnabled(enable);
    m_editEnrollEmail->setEnabled(enable);
    m_btnStartEnroll->setEnabled(enable && m_fpManager->isReaderOpen());
    m_btnCaptureEnroll->setEnabled(!enable);
}

void MainWindowApp::enableVerificationControls(bool enable)
{
    m_btnStartVerify->setEnabled(enable && m_fpManager->isReaderOpen());
    m_btnCaptureVerify->setEnabled(!enable);
}

