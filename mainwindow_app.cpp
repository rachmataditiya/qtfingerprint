#include "mainwindow_app.h"
#include "database_config_dialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QScrollBar>
#include <QPainter>
#include <QPixmap>
#include <QMetaObject>
#include <QGridLayout>
#include <QRadialGradient>
#include <QtConcurrent>

MainWindowApp::MainWindowApp(QWidget *parent)
    : QMainWindow(parent)
    , m_fpManager(new FingerprintManager())
    , m_dbManager(new DatabaseManager(this))
    , m_enrollmentInProgress(false)
    , m_enrollmentSampleCount(0)
{
    setupUI();
    setWindowTitle("U.are.U 4500 Fingerprint Application - DigitalPersona");
    resize(1200, 750);

    // Setup enrollment progress callback
    m_fpManager->setProgressCallback([this](int current, int total, QString message) {
        // Use AutoConnection to ensure immediate UI updates on both Mac and Linux
        // since we are running synchronously on the main thread.
        // QueuedConnection would wait until the blocking call finishes, freezing the UI on Mac.
        QMetaObject::invokeMethod(this, [this, current, total, message]() {
            onEnrollmentProgress(current, total, message);
        }, Qt::AutoConnection);
    });

    // Connect watcher - REMOVED as we are running on main thread now
    // connect(&m_enrollWatcher, &QFutureWatcher<int>::finished, this, &MainWindowApp::onCaptureEnrollFinished);

    // Initialize database with configuration dialog
    if (!DatabaseConfigDialog::hasConfig()) {
        DatabaseConfigDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) {
            QMessageBox::warning(this, "Configuration", "Database configuration required. Exiting.");
            // We can't easily exit here in constructor without showing window first or using timer.
            // But we can disable functionality or close in showEvent.
            // For now, just let it be uninitialized and buttons disabled.
            updateStatus("Database not configured", true);
            return;
        }
    }
    
    DatabaseConfigDialog::Config dbConfig = DatabaseConfigDialog::loadConfig();
    if (!m_dbManager->initialize(dbConfig)) {
        QMessageBox::critical(this, "Database Error", 
            QString("Failed to initialize database: %1").arg(m_dbManager->getLastError()));
        
        // Offer to reconfigure
        if (QMessageBox::question(this, "Retry?", "Would you like to reconfigure database?") == QMessageBox::Yes) {
             DatabaseConfigDialog dlg(this);
             if (dlg.exec() == QDialog::Accepted) {
                 dbConfig = DatabaseConfigDialog::loadConfig();
                 if (m_dbManager->initialize(dbConfig)) {
                     goto db_success;
                 }
             }
        }
        updateStatus("Database initialization failed", true);
    } else {
db_success:
        log("Database initialized successfully");
        qDebug() << "Calling updateUserList()...";
        updateUserList();
        qDebug() << "updateUserList() returned.";
    }
}

MainWindowApp::~MainWindowApp()
{
    // Explicit cleanup in closeEvent is preferred, but just in case
    if (m_fpManager) {
        m_fpManager->cleanup();
        delete m_fpManager;
        m_fpManager = nullptr;
    }
}

void MainWindowApp::closeEvent(QCloseEvent *event)
{
    // Ensure thread is done
    /* 
    if (m_enrollWatcher.isRunning()) {
        m_enrollWatcher.waitForFinished();
    }
    */
    
    if (m_fpManager) {
        log("Application closing, cleaning up...");
        m_fpManager->cleanup();
    }
    
    QMainWindow::closeEvent(event);
}

void MainWindowApp::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Left panel - Controls
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

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
    enrollLayout->setSpacing(8);
    enrollLayout->setContentsMargins(15, 15, 15, 15);
    
    // User info inputs in a grid
    QGridLayout* inputGrid = new QGridLayout();
    inputGrid->setSpacing(5);
    
    QLabel* nameLabel = new QLabel("Name:");
    nameLabel->setStyleSheet("QLabel { font-weight: bold; }");
    inputGrid->addWidget(nameLabel, 0, 0);
    m_editEnrollName = new QLineEdit();
    m_editEnrollName->setPlaceholderText("Enter user name");
    m_editEnrollName->setMinimumHeight(30);
    inputGrid->addWidget(m_editEnrollName, 0, 1);
    
    QLabel* emailLabel = new QLabel("Email:");
    emailLabel->setStyleSheet("QLabel { font-weight: bold; }");
    inputGrid->addWidget(emailLabel, 1, 0);
    m_editEnrollEmail = new QLineEdit();
    m_editEnrollEmail->setPlaceholderText("Enter email (optional)");
    m_editEnrollEmail->setMinimumHeight(30);
    inputGrid->addWidget(m_editEnrollEmail, 1, 1);
    
    enrollLayout->addLayout(inputGrid);
    
    // Buttons
    QHBoxLayout* enrollButtonsLayout = new QHBoxLayout();
    enrollButtonsLayout->setSpacing(10);
    
    m_btnStartEnroll = new QPushButton("Start Enrollment");
    m_btnStartEnroll->setStyleSheet("QPushButton { padding: 10px; font-size: 13px; background-color: #4CAF50; color: white; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");
    m_btnStartEnroll->setEnabled(false);
    m_btnStartEnroll->setMinimumHeight(40);
    enrollButtonsLayout->addWidget(m_btnStartEnroll);
    
    m_btnCaptureEnroll = new QPushButton("Capture Fingerprint");
    m_btnCaptureEnroll->setStyleSheet("QPushButton { padding: 10px; font-size: 13px; background-color: #2196F3; color: white; font-weight: bold; } QPushButton:hover { background-color: #0b7dda; }");
    m_btnCaptureEnroll->setEnabled(false);
    m_btnCaptureEnroll->setMinimumHeight(40);
    enrollButtonsLayout->addWidget(m_btnCaptureEnroll);
    
    enrollLayout->addLayout(enrollButtonsLayout);
    
    // Progress section with image preview side by side
    QHBoxLayout* progressImageLayout = new QHBoxLayout();
    progressImageLayout->setSpacing(15);
    
    // Left: Progress info
    QVBoxLayout* progressInfoLayout = new QVBoxLayout();
    progressInfoLayout->setSpacing(8);
    
    QLabel* progressLabel = new QLabel("Progress:");
    progressLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
    progressInfoLayout->addWidget(progressLabel);
    
    m_enrollProgress = new QProgressBar();
    m_enrollProgress->setRange(0, 5);
    m_enrollProgress->setValue(0);
    m_enrollProgress->setFormat("%v/5 scans (%p%)");
    m_enrollProgress->setMinimumHeight(30);
    m_enrollProgress->setStyleSheet("QProgressBar { border: 2px solid #ccc; border-radius: 5px; text-align: center; font-weight: bold; } QProgressBar::chunk { background-color: #4CAF50; }");
    progressInfoLayout->addWidget(m_enrollProgress);
    
    m_enrollStatusLabel = new QLabel("Ready to enroll");
    m_enrollStatusLabel->setWordWrap(true);
    m_enrollStatusLabel->setStyleSheet("QLabel { color: #555; font-size: 11px; padding: 5px; background-color: #f0f0f0; border-radius: 3px; }");
    m_enrollStatusLabel->setMinimumHeight(60);
    m_enrollStatusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    progressInfoLayout->addWidget(m_enrollStatusLabel);
    
    progressInfoLayout->addStretch();
    progressImageLayout->addLayout(progressInfoLayout, 1);
    
    // Right: Image preview
    QVBoxLayout* previewLayout = new QVBoxLayout();
    previewLayout->setSpacing(5);
    
    QLabel* previewLabel = new QLabel("Fingerprint Preview:");
    previewLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(previewLabel);
    
    m_enrollImagePreview = new QLabel();
    m_enrollImagePreview->setFixedSize(180, 180);
    m_enrollImagePreview->setScaledContents(false);
    m_enrollImagePreview->setStyleSheet("QLabel { border: 3px solid #2196F3; background-color: #fafafa; border-radius: 5px; }");
    m_enrollImagePreview->setAlignment(Qt::AlignCenter);
    m_enrollImagePreview->setText("No scan yet");
    previewLayout->addWidget(m_enrollImagePreview);
    
    QLabel* noteLabel = new QLabel("Note: Preview is simulated\n(device doesn't provide raw image)");
    noteLabel->setStyleSheet("QLabel { font-size: 9px; color: #888; font-style: italic; }");
    noteLabel->setAlignment(Qt::AlignCenter);
    noteLabel->setWordWrap(true);
    previewLayout->addWidget(noteLabel);
    
    progressImageLayout->addLayout(previewLayout, 0);
    
    enrollLayout->addLayout(progressImageLayout);
    
    leftLayout->addWidget(m_enrollGroup);

    // Verification group
    m_verifyGroup = new QGroupBox("3. Verification");
    QVBoxLayout* verifyLayout = new QVBoxLayout(m_verifyGroup);
    verifyLayout->setSpacing(10);
    verifyLayout->setContentsMargins(15, 15, 15, 15);
    
    QHBoxLayout* verifyButtonsLayout = new QHBoxLayout();
    verifyButtonsLayout->setSpacing(10);
    
    m_btnStartVerify = new QPushButton("Start Verification");
    m_btnStartVerify->setStyleSheet("QPushButton { padding: 10px; font-size: 13px; background-color: #FF9800; color: white; font-weight: bold; } QPushButton:hover { background-color: #e68900; }");
    m_btnStartVerify->setEnabled(false);
    m_btnStartVerify->setMinimumHeight(40);
    verifyButtonsLayout->addWidget(m_btnStartVerify);
    
    m_btnCaptureVerify = new QPushButton("Capture & Verify");
    m_btnCaptureVerify->setStyleSheet("QPushButton { padding: 10px; font-size: 13px; background-color: #2196F3; color: white; font-weight: bold; } QPushButton:hover { background-color: #0b7dda; }");
    m_btnCaptureVerify->setEnabled(false);
    m_btnCaptureVerify->setMinimumHeight(40);
    verifyButtonsLayout->addWidget(m_btnCaptureVerify);
    
    verifyLayout->addLayout(verifyButtonsLayout);
    
    // Result display
    QVBoxLayout* resultLayout = new QVBoxLayout();
    resultLayout->setSpacing(5);
    
    QLabel* resultTitleLabel = new QLabel("Verification Result:");
    resultTitleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
    resultLayout->addWidget(resultTitleLabel);
    
    m_verifyResultLabel = new QLabel("Result: -");
    m_verifyResultLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; padding: 10px; background-color: #f0f0f0; border-radius: 5px; }");
    m_verifyResultLabel->setAlignment(Qt::AlignCenter);
    m_verifyResultLabel->setMinimumHeight(50);
    resultLayout->addWidget(m_verifyResultLabel);
    
    m_verifyScoreLabel = new QLabel("Score: -");
    m_verifyScoreLabel->setStyleSheet("QLabel { font-size: 13px; padding: 5px; color: #555; }");
    m_verifyScoreLabel->setAlignment(Qt::AlignCenter);
    resultLayout->addWidget(m_verifyScoreLabel);
    
    verifyLayout->addLayout(resultLayout);
    
    leftLayout->addWidget(m_verifyGroup);

    leftLayout->addStretch();

    // Right panel - User list and log
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);

    // User list group
    m_userListGroup = new QGroupBox("Registered Users");
    QVBoxLayout* userListLayout = new QVBoxLayout(m_userListGroup);
    userListLayout->setSpacing(8);
    userListLayout->setContentsMargins(15, 15, 15, 15);
    
    m_userCountLabel = new QLabel("Total users: 0");
    m_userCountLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; color: #2196F3; padding: 5px; }");
    userListLayout->addWidget(m_userCountLabel);
    
    m_userList = new QListWidget();
    m_userList->setStyleSheet("QListWidget { border: 2px solid #ccc; border-radius: 5px; padding: 5px; } QListWidget::item { padding: 8px; border-bottom: 1px solid #eee; } QListWidget::item:selected { background-color: #e3f2fd; color: black; }");
    m_userList->setMinimumHeight(250);
    userListLayout->addWidget(m_userList);
    
    QHBoxLayout* userButtonsLayout = new QHBoxLayout();
    userButtonsLayout->setSpacing(8);
    
    m_btnRefreshList = new QPushButton("Refresh");
    m_btnRefreshList->setStyleSheet("QPushButton { padding: 8px; font-size: 12px; background-color: #4CAF50; color: white; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");
    m_btnRefreshList->setMinimumHeight(35);
    userButtonsLayout->addWidget(m_btnRefreshList);
    
    m_btnDeleteUser = new QPushButton("Delete User");
    m_btnDeleteUser->setStyleSheet("QPushButton { padding: 8px; font-size: 12px; background-color: #f44336; color: white; font-weight: bold; } QPushButton:hover { background-color: #da190b; } QPushButton:disabled { background-color: #ccc; }");
    m_btnDeleteUser->setEnabled(false);
    m_btnDeleteUser->setMinimumHeight(35);
    userButtonsLayout->addWidget(m_btnDeleteUser);
    
    userListLayout->addLayout(userButtonsLayout);
    
    rightLayout->addWidget(m_userListGroup);

    // Log section
    QGroupBox* logGroup = new QGroupBox("Activity Log");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    logLayout->setSpacing(8);
    logLayout->setContentsMargins(15, 15, 15, 15);
    
    m_logText = new QTextEdit();
    m_logText->setReadOnly(true);
    m_logText->setMinimumHeight(180);
    m_logText->setMaximumHeight(220);
    m_logText->setStyleSheet("QTextEdit { border: 2px solid #ccc; border-radius: 5px; padding: 8px; font-family: 'Courier New', monospace; font-size: 10px; background-color: #fafafa; }");
    logLayout->addWidget(m_logText);
    
    m_btnClearLog = new QPushButton("Clear Log");
    m_btnClearLog->setStyleSheet("QPushButton { padding: 6px; font-size: 11px; background-color: #757575; color: white; } QPushButton:hover { background-color: #616161; }");
    m_btnClearLog->setMinimumHeight(30);
    logLayout->addWidget(m_btnClearLog);
    
    rightLayout->addWidget(logGroup);

    // Add panels to main layout
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 2);

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
    log("Reader opened successfully");
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
    
    // Reset progress bar and preview
    m_enrollProgress->setValue(0);
    m_enrollProgress->setFormat("0/5 scans (0%)");
    m_enrollStatusLabel->setText("Enrollment started. Click 'Capture Fingerprint' to begin scanning.");
    
    // Create initial "Ready to scan" preview
    QImage readyImage(180, 180, QImage::Format_RGB888);
    readyImage.fill(QColor(250, 250, 250));
    QPainter painter(&readyImage);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    QRadialGradient gradient(90, 90, 80);
    gradient.setColorAt(0, QColor(245, 245, 250));
    gradient.setColorAt(1, QColor(230, 230, 240));
    painter.fillRect(readyImage.rect(), gradient);
    
    // Draw ready indicator
    painter.setPen(QPen(QColor(150, 150, 150), 2));
    painter.drawEllipse(QPoint(90, 90), 60, 60);
    painter.drawEllipse(QPoint(90, 90), 40, 40);
    
    painter.setPen(QColor(100, 100, 100));
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(readyImage.rect(), Qt::AlignCenter, "Ready\nto Scan");
    
    m_enrollImagePreview->setPixmap(QPixmap::fromImage(readyImage));
    
    log(QString("Starting enrollment for: %1 %2").arg(name).arg(email.isEmpty() ? "" : "(" + email + ")"));
    
    enableEnrollmentControls(false);
    m_btnCaptureEnroll->setEnabled(true);
}

void MainWindowApp::onCaptureEnrollSample()
{
    if (!m_enrollmentInProgress) {
        return;
    }
    
    m_btnCaptureEnroll->setEnabled(false);
    m_enrollStatusLabel->setText("Place your finger on the reader. You will scan 5 times...");
    log("=== ENROLLMENT: Starting capture sequence ===");
    
    QApplication::processEvents();
    
    QString message;
    int quality;
    QImage image;
    int result = m_fpManager->addEnrollmentSample(message, quality, &image);
    
    if (result < 0) {
        log(QString("ERROR: %1").arg(m_fpManager->getLastError()));
        m_enrollStatusLabel->setText("Capture failed");
        m_enrollmentInProgress = false;
        m_enrollProgress->setValue(0);
        m_enrollProgress->setFormat("0/5 scans (0%)");
        m_enrollImagePreview->clear();
        m_enrollImagePreview->setText("No scan yet");
        enableEnrollmentControls(true);
        QMessageBox::critical(this, "Enrollment Error", m_fpManager->getLastError());
        return;
    }
    
    // Update final status (progress bar already updated by callback)
    m_enrollStatusLabel->setText(message);
    log(message);
    
    if (result == 1) {
        log("All scans completed! Saving fingerprint template to database...");
        
        QByteArray templateData;
        if (!m_fpManager->createEnrollmentTemplate(templateData)) {
            QMessageBox::critical(this, "Error", "Failed to create fingerprint template");
            log("Error creating template");
            m_fpManager->cancelEnrollment();
            m_enrollmentInProgress = false;
            m_enrollProgress->setValue(0);
            m_enrollProgress->setFormat("0/5 scans (0%)");
            m_enrollImagePreview->clear();
            m_enrollImagePreview->setText("No scan yet");
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
            log(QString("User enrolled successfully: %1 (ID: %2)").arg(m_enrollmentUserName).arg(userId));
            QMessageBox::information(this, "Enrollment Complete", 
                QString("User '%1' enrolled successfully!\n\nUser ID: %2\nTemplate size: %3 bytes\nScans completed: 5")
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
        m_enrollProgress->setFormat("0/5 scans (0%)");
        m_enrollStatusLabel->setText("Ready to enroll next user");
        
        // Reset preview to empty state
        QImage emptyImage(180, 180, QImage::Format_RGB888);
        emptyImage.fill(QColor(250, 250, 250));
        QPainter painter(&emptyImage);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QRadialGradient gradient(90, 90, 80);
        gradient.setColorAt(0, QColor(245, 245, 250));
        gradient.setColorAt(1, QColor(230, 230, 240));
        painter.fillRect(emptyImage.rect(), gradient);
        
        painter.setPen(QColor(150, 150, 150));
        painter.setFont(QFont("Arial", 11));
        painter.drawText(emptyImage.rect(), Qt::AlignCenter, "No scan yet");
        
        m_enrollImagePreview->setPixmap(QPixmap::fromImage(emptyImage));
        
        enableEnrollmentControls(true);
        log("=== ENROLLMENT SESSION COMPLETED ===");
    } else {
        // If result is 0 (more scans needed), re-enable the capture button
         m_btnCaptureEnroll->setEnabled(true);
    }
}

void MainWindowApp::onCaptureEnrollFinished()
{
    // Unused now
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
        log(QString("Verification error: %1").arg(error));
        m_verifyResultLabel->setText("Result: ERROR");
        m_verifyResultLabel->setStyleSheet("QLabel { background-color: #ffcccc; color: red; padding: 5px; font-weight: bold; }");
        m_verifyScoreLabel->setText("Score: -");
        QMessageBox::critical(this, "Verification Error", error);
    } else {
        m_verifyScoreLabel->setText(QString("Match Score: %1%").arg(score));
        
        if (score >= 60) {
            m_verifyResultLabel->setText(QString("MATCH: %1").arg(user.name));
            m_verifyResultLabel->setStyleSheet("QLabel { background-color: #c8e6c9; color: green; padding: 10px; font-weight: bold; font-size: 14px; }");
            log(QString("VERIFICATION SUCCESS: %1 (score: %2%)").arg(user.name).arg(score));
            QMessageBox::information(this, "Verification Success", 
                QString("Fingerprint MATCHED!\n\nUser: %1\nScore: %2%").arg(user.name).arg(score));
        } else {
            m_verifyResultLabel->setText(QString("NO MATCH"));
            m_verifyResultLabel->setStyleSheet("QLabel { background-color: #ffcdd2; color: red; padding: 10px; font-weight: bold; font-size: 14px; }");
            log(QString("VERIFICATION FAILED (score: %1%)").arg(score));
            QMessageBox::warning(this, "Verification Failed", 
                QString("Fingerprint does NOT match!\n\nExpected: %1\nScore: %2%")
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
    qDebug() << "Updating user count label done.";
    log(QString("User list updated: %1 users").arg(users.size()));
    qDebug() << "Log called.";
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

void MainWindowApp::onEnrollmentProgress(int current, int total, QString message)
{
    // Update progress bar
    m_enrollProgress->setValue(current);
    m_enrollProgress->setFormat(QString("%1/%2 scans (%p%)").arg(current).arg(total));
    
    // Update status label
    m_enrollStatusLabel->setText(message);
    
    // Log progress
    log(QString("Enrollment: %1/%2 - %3").arg(current).arg(total).arg(message));
    
    // Create a more realistic fingerprint visualization
    // Note: This is a simulation since U.are.U doesn't provide raw images via libfprint
    QImage previewImage(180, 180, QImage::Format_RGB888);
    previewImage.fill(QColor(250, 250, 250));
    
    QPainter painter(&previewImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    int centerX = 90;
    int centerY = 90;
    
    // Draw background gradient for depth
    QRadialGradient gradient(centerX, centerY, 80);
    gradient.setColorAt(0, QColor(240, 245, 250));
    gradient.setColorAt(1, QColor(220, 230, 240));
    painter.fillRect(previewImage.rect(), gradient);
    
    // Draw fingerprint ridges with varying opacity based on progress
    for (int i = 0; i < current; ++i) {
        int alpha = 150 + (i * 20);
        if (alpha > 255) alpha = 255;
        
        painter.setPen(QPen(QColor(60, 100, 180, alpha), 2.5));
        
        // Draw multiple curved lines to simulate fingerprint ridges
        for (int j = 0; j < 8; ++j) {
            int radius = 15 + (i * 12) + (j * 3);
            QRect ellipseRect(centerX - radius, centerY - radius, radius * 2, radius * 2);
            
            // Draw partial arcs for more realistic fingerprint pattern
            int startAngle = (j * 15 + i * 10) * 16; // Qt uses 1/16th of a degree
            int spanAngle = (120 + j * 10) * 16;
            painter.drawArc(ellipseRect, startAngle, spanAngle);
        }
    }
    
    // Draw center point
    if (current > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(33, 150, 243, 200));
        painter.drawEllipse(QPoint(centerX, centerY), 5, 5);
    }
    
    // Draw scan indicator overlay
    QColor overlayColor;
    QString statusText;
    
    if (current == 0) {
        overlayColor = QColor(200, 200, 200, 230);
        statusText = "Ready to Scan";
    } else if (current < total) {
        overlayColor = QColor(255, 152, 0, 200);
        statusText = QString("Scan %1/%2").arg(current).arg(total);
    } else {
        overlayColor = QColor(76, 175, 80, 200);
        statusText = "Complete!";
    }
    
    // Draw status badge
    painter.setPen(Qt::NoPen);
    painter.setBrush(overlayColor);
    QRect badgeRect(10, 10, 160, 35);
    painter.drawRoundedRect(badgeRect, 5, 5);
    
    // Draw status text
    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(badgeRect, Qt::AlignCenter, statusText);
    
    // Draw progress indicator at bottom
    painter.setPen(Qt::NoPen);
    int progressWidth = (180 * current) / total;
    QRect progressRect(0, 170, progressWidth, 10);
    painter.setBrush(QColor(33, 150, 243, 220));
    painter.drawRect(progressRect);
    
    // Show in preview label (center the image)
    m_enrollImagePreview->setPixmap(QPixmap::fromImage(previewImage));
    
    // Process events to update UI
    QApplication::processEvents();
}

