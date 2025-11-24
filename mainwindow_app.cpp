#include "mainwindow_app.h"
#include "backend_config_dialog.h"
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
#include <QTimer>

MainWindowApp::MainWindowApp(QWidget *parent)
    : QMainWindow(parent)
    , m_fpManager(new FingerprintManager())
    , m_backendClient(new BackendClient(this))
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

    // Connect watcher - restored for macOS async handling
    connect(&m_enrollWatcher, &QFutureWatcher<int>::finished, this, &MainWindowApp::onCaptureEnrollFinished);

    // Initialize backend client
    if (!BackendConfigDialog::hasConfig()) {
        BackendConfigDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) {
            QMessageBox::warning(this, "Configuration", "Backend configuration required. Exiting.");
            updateStatus("Backend not configured", true);
            return;
        }
    }
    
    QString backendUrl = BackendConfigDialog::loadBackendUrl();
    m_backendClient->setBaseUrl(backendUrl);
    
    // Connect backend signals
    connect(m_backendClient, &BackendClient::usersListed, this, &MainWindowApp::onUsersListed);
    connect(m_backendClient, &BackendClient::userCreated, this, &MainWindowApp::onUserCreated);
    connect(m_backendClient, &BackendClient::templateStored, this, &MainWindowApp::onTemplateStored);
    connect(m_backendClient, &BackendClient::templateLoaded, this, &MainWindowApp::onTemplateLoaded);
    connect(m_backendClient, &BackendClient::templatesLoaded, this, &MainWindowApp::onTemplatesLoaded);
    connect(m_backendClient, &BackendClient::error, this, &MainWindowApp::onBackendError);
    
    log("Backend initialized successfully");
    m_backendClient->listUsers();
    updateStatus("Backend Connected", false);
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
    m_enrollGroup = new QGroupBox("2. Enrollment");
    QVBoxLayout* enrollLayout = new QVBoxLayout(m_enrollGroup);
    enrollLayout->setSpacing(8);
    enrollLayout->setContentsMargins(15, 15, 15, 15);
    
    // User and finger selection in a grid
    QGridLayout* inputGrid = new QGridLayout();
    inputGrid->setSpacing(10);
    
    QLabel* userLabel = new QLabel("User:");
    userLabel->setStyleSheet("QLabel { font-weight: bold; }");
    m_enrollUserSelect = new QComboBox();
    m_enrollUserSelect->setPlaceholderText("Select user");
    m_enrollUserSelect->setMinimumHeight(30);
    m_enrollUserSelect->setEditable(false);
    inputGrid->addWidget(userLabel, 0, 0);
    inputGrid->addWidget(m_enrollUserSelect, 0, 1);
    
    QLabel* fingerLabel = new QLabel("Finger:");
    fingerLabel->setStyleSheet("QLabel { font-weight: bold; }");
    m_enrollFingerSelect = new QComboBox();
    m_enrollFingerSelect->setMinimumHeight(30);
    m_enrollFingerSelect->setEditable(false);
    
    // Populate finger options
    QStringList fingers = {"Right Index", "Right Middle", "Right Ring", "Right Pinky", "Right Thumb",
                           "Left Index", "Left Middle", "Left Ring", "Left Pinky", "Left Thumb"};
    m_enrollFingerSelect->addItems(fingers);
    m_enrollFingerSelect->setCurrentText("Right Index"); // Default
    
    inputGrid->addWidget(fingerLabel, 1, 0);
    inputGrid->addWidget(m_enrollFingerSelect, 1, 1);
    
    enrollLayout->addLayout(inputGrid);
    
    // Connect user selection to update finger list
    connect(m_enrollUserSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0) {
            int userId = m_enrollUserSelect->itemData(index).toInt();
            m_backendClient->getUserFingers(userId);
        }
    });
    
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
    
    m_btnStartVerify = new QPushButton("Verify (1:1)");
    m_btnStartVerify->setStyleSheet("QPushButton { padding: 10px; font-size: 13px; background-color: #FF9800; color: white; font-weight: bold; } QPushButton:hover { background-color: #e68900; }");
    m_btnStartVerify->setEnabled(false);
    m_btnStartVerify->setMinimumHeight(40);
    verifyButtonsLayout->addWidget(m_btnStartVerify);
    
    m_btnIdentify = new QPushButton("Identify (1:N)");
    m_btnIdentify->setStyleSheet("QPushButton { padding: 10px; font-size: 13px; background-color: #673AB7; color: white; font-weight: bold; } QPushButton:hover { background-color: #5E35B1; }");
    m_btnIdentify->setEnabled(false);
    m_btnIdentify->setMinimumHeight(40);
    verifyButtonsLayout->addWidget(m_btnIdentify);
    
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
    
    m_btnConfig = new QPushButton("Database Config");
    m_btnConfig->setCursor(Qt::PointingHandCursor);
    m_btnConfig->setStyleSheet("QPushButton { padding: 6px; font-size: 11px; background-color: #607d8b; color: white; border-radius: 3px; } QPushButton:hover { background-color: #546e7a; }");
    logLayout->addWidget(m_btnConfig);
    
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
    connect(m_btnIdentify, &QPushButton::clicked, this, &MainWindowApp::onIdentifyClicked);
    connect(m_btnCaptureVerify, &QPushButton::clicked, this, &MainWindowApp::onCaptureVerifySample);
    connect(m_btnRefreshList, &QPushButton::clicked, this, &MainWindowApp::onRefreshUserList);
    connect(m_btnDeleteUser, &QPushButton::clicked, this, &MainWindowApp::onDeleteUserClicked);
    connect(m_btnClearLog, &QPushButton::clicked, this, &MainWindowApp::onClearLog);
    connect(m_btnConfig, &QPushButton::clicked, this, &MainWindowApp::onConfigClicked);
    connect(m_userList, &QListWidget::itemClicked, this, &MainWindowApp::onUserSelected);
}

void MainWindowApp::onConfigClicked()
{
    BackendConfigDialog dlg(this);
    
    if (dlg.exec() == QDialog::Accepted) {
        // Config saved, re-initialize
        QString backendUrl = BackendConfigDialog::loadBackendUrl();
        m_backendClient->setBaseUrl(backendUrl);
        updateStatus("Backend Connected", false);
        m_backendClient->listUsers();
    }
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
    m_btnIdentify->setEnabled(true);
}

void MainWindowApp::onEnrollClicked()
{
    // Get selected user
    int userIndex = m_enrollUserSelect->currentIndex();
    if (userIndex < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select a user from the list");
        return;
    }
    
    int userId = m_enrollUserSelect->itemData(userIndex).toInt();
    QString userName = m_enrollUserSelect->currentText();
    QString selectedFinger = m_enrollFingerSelect->currentText();
    
    // Remove "(enrolled)" suffix if present
    selectedFinger = selectedFinger.replace(" (enrolled)", "");
    
    if (selectedFinger.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select a finger");
        return;
    }
    
    // Check if finger is already enrolled
    // Note: This will be checked when userFingersRetrieved is called
    
    if (!m_fpManager->startEnrollment()) {
        QMessageBox::critical(this, "Error", m_fpManager->getLastError());
        return;
    }
    
    m_enrollmentInProgress = true;
    m_enrollmentSampleCount = 0;
    m_enrollmentUserId = userId; // Store userId instead of name/email
    m_enrollmentUserName = userName;
    m_pendingEnrollmentFinger = selectedFinger;
    
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
    
    log(QString("Starting enrollment for user: %1, finger: %2").arg(userName).arg(selectedFinger));
    
    enableEnrollmentControls(false);
    m_btnCaptureEnroll->setEnabled(true);
}

void MainWindowApp::onCaptureEnrollSample()
{
    if (!m_enrollmentInProgress) {
        return;
    }
    
    // Check if device is still open
    if (!m_fpManager->isReaderOpen()) {
        QMessageBox::warning(this, "Device Not Ready", "Device is not open. Please initialize the reader first.");
        m_enrollmentInProgress = false;
        enableEnrollmentControls(true);
        return;
    }
    
    m_btnCaptureEnroll->setEnabled(false);
    m_enrollStatusLabel->setText("Place your finger on the reader. You will scan 5 times...");
    log("=== ENROLLMENT: Starting capture sequence ===");
    
#ifdef Q_OS_MACOS
    // On macOS, running synchronously blocks the UI too much despite processEvents.
    // We run in a background thread to keep UI responsive.
    QFuture<int> future = QtConcurrent::run([this]() {
        int quality;
        // We write to members here, which is safe because we read them only after future finishes
        return m_fpManager->addEnrollmentSample(m_tempEnrollMessage, quality, &m_tempEnrollImage);
    });
    m_enrollWatcher.setFuture(future);
#else
    // Linux implementation (Main Thread Synchronous)
    // This prevents "scan hang" issues seen on Linux with threading.
    QApplication::processEvents();
    
    int quality;
    int result = m_fpManager->addEnrollmentSample(m_tempEnrollMessage, quality, &m_tempEnrollImage);
    
    // Handle result immediately
    processEnrollmentResult(result);
#endif
}

void MainWindowApp::processEnrollmentResult(int result)
{
    QString message = m_tempEnrollMessage;
    
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
        
        // Store template for user (user already exists, just enroll finger)
        m_pendingEnrollmentTemplate = templateData;
        // m_pendingEnrollmentFinger already set in onEnrollClicked
        // m_enrollmentUserId already set in onEnrollClicked
        
        // Store template directly (user already exists)
        log(QString("Storing template for user ID: %1, finger: %2").arg(m_enrollmentUserId).arg(m_pendingEnrollmentFinger));
        m_backendClient->storeTemplate(m_enrollmentUserId, templateData, m_pendingEnrollmentFinger);
        
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
    processEnrollmentResult(m_enrollWatcher.result());
}

void MainWindowApp::onIdentifyClicked()
{
    if (!m_fpManager->isReaderOpen()) {
        QMessageBox::warning(this, "Reader Not Ready", "Please initialize the reader first.");
        return;
    }
    
    // Check if device is still open and ready
    if (!m_fpManager->isReaderOpen()) {
        QMessageBox::warning(this, "Device Not Ready", "Device is not open. Please initialize the reader first.");
        return;
    }
    
    m_btnIdentify->setEnabled(false);
    log("=== IDENTIFICATION STARTED ===");
    log("Loading all templates from backend...");
    
    // Load all templates from backend for identification
    m_backendClient->loadTemplates();
}

void MainWindowApp::onVerifyClicked()
{
    if (m_userList->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select a user from the list");
        return;
    }
    
    // Check if device is still open
    if (!m_fpManager->isReaderOpen()) {
        QMessageBox::warning(this, "Device Not Ready", "Device is not open. Please initialize the reader first.");
        return;
    }
    
    // Store selected user info
    QListWidgetItem* item = m_userList->selectedItems().first();
    m_verificationUserId = item->data(Qt::UserRole).toInt();
    QStringList parts = item->text().split(" - ");
    if (parts.size() >= 1) {
        m_verificationUserName = parts.at(0);
    }
    if (parts.size() >= 2) {
        QString emailPart = parts.at(1);
        if (emailPart.contains("(")) {
            m_verificationUserEmail = emailPart.section(" (", 0, 0);
        } else {
            m_verificationUserEmail = "";
        }
    }
    
    m_btnStartVerify->setEnabled(false);
    m_verifyResultLabel->setText("Loading templates...");
    m_verifyScoreLabel->setText("Please wait...");
    log(QString("Verification started for user ID: %1").arg(m_verificationUserId));
    
    // Reset verification state
    m_verificationTemplates.clear();
    m_remainingVerificationFingers.clear();
    
    // Load all templates for this user (all fingers)
    // We'll verify against all of them
    log("Getting user fingers...");
    m_backendClient->getUserFingers(m_verificationUserId);
    
    // Fallback: if getUserFingers doesn't respond in 2 seconds, load template directly
    QTimer::singleShot(2000, this, [this]() {
        if (m_verifyResultLabel->text() == "Loading templates..." && m_verificationTemplates.isEmpty()) {
            log("getUserFingers timeout, loading template directly (most recent)");
            m_backendClient->loadTemplate(m_verificationUserId, ""); // Empty = most recent
        }
    });
}

void MainWindowApp::onCaptureVerifySample()
{
    // Verification now happens directly in onVerifyClicked
    // This method is kept for UI compatibility but redirects to onVerifyClicked
    if (m_userList->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select a user from the list");
        return;
    }
    onVerifyClicked();
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
    QString userName = item->text().section(" - ", 0, 0);
    
    auto reply = QMessageBox::question(this, "Confirm Delete", 
        QString("Are you sure you want to delete user '%1'?").arg(userName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // User deletion needs backend API endpoint
        QMessageBox::information(this, "Not Implemented", "User deletion needs backend API endpoint");
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
    // Request users from backend
    m_backendClient->listUsers();
}

void MainWindowApp::onUsersListed(const QVector<User>& users)
{
    m_userList->clear();
    
    for (const User& user : users) {
        QString displayText = QString("%1 - %2 (%3 fingers)").arg(user.name).arg(user.email.isEmpty() ? "No email" : user.email).arg(user.fingerCount);
        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, user.id);
        m_userList->addItem(item);
    }
    
    m_userCountLabel->setText(QString("Total users: %1").arg(users.size()));
    log(QString("User list updated: %1 users").arg(users.size()));
}

void MainWindowApp::onUserCreated(int userId)
{
    log(QString("User created: ID %1").arg(userId));
    // User creation is now separate from enrollment
    // Enrollment happens for existing users only
    m_backendClient->listUsers(); // Refresh user list
}

void MainWindowApp::onTemplateStored(int userId, const QString& finger)
{
    log(QString("Template stored for user %1, finger %2").arg(userId).arg(finger));
    QMessageBox::information(this, "Enrollment Complete", 
        QString("User enrolled successfully!\n\nUser ID: %1\nFinger: %2").arg(userId).arg(finger));
    m_backendClient->listUsers();
}

void MainWindowApp::onTemplateLoaded(const BackendFingerprintTemplate& tmpl)
{
    // Handle template loaded for verification
    log(QString("Template loaded for user %1, finger %2").arg(tmpl.userId).arg(tmpl.finger));
    
    // Add to verification templates list
    m_verificationTemplates.append(tmpl);
    
    // Check if we need to load more templates
    if (!m_remainingVerificationFingers.isEmpty()) {
        // Load next finger template
        QString nextFinger = m_remainingVerificationFingers.takeFirst();
        log(QString("Loading template for finger: %1").arg(nextFinger));
        m_backendClient->loadTemplate(m_verificationUserId, nextFinger);
        return; // Wait for next template to load
    }
    
    // All templates loaded, now verify against all of them
    log(QString("All %1 template(s) loaded. Starting verification...").arg(m_verificationTemplates.size()));
    
    // Check if device is still open
    if (!m_fpManager->isReaderOpen()) {
        QMessageBox::critical(this, "Device Not Ready", "Device is not open. Please initialize the reader first.");
        m_btnStartVerify->setEnabled(true);
        m_btnCaptureVerify->setEnabled(false);
        m_verificationTemplates.clear();
        m_remainingVerificationFingers.clear();
        return;
    }
    
    m_verifyResultLabel->setText("Capturing...");
    m_verifyScoreLabel->setText("Please wait...");
    QApplication::processEvents();
    
    // Try to verify against all templates
    // Capture fingerprint once and verify against all templates
    bool foundMatch = false;
    int bestScore = 0;
    BackendFingerprintTemplate matchedTemplate;
    
    for (const BackendFingerprintTemplate& tmpl : m_verificationTemplates) {
        int score = 0;
        bool matched = m_fpManager->verifyFingerprint(tmpl.templateData, score);
        
        log(QString("Verifying against finger %1: score=%2, matched=%3").arg(tmpl.finger).arg(score).arg(matched));
        
        if (matched && score >= 60) {
            // Found a match!
            foundMatch = true;
            matchedTemplate = tmpl;
            bestScore = score;
            break; // Stop at first match
        } else if (score > bestScore) {
            // Keep track of best score even if not matched
            bestScore = score;
        }
    }
    
    // Use stored user info
    QString userName = m_verificationUserName.isEmpty() ? 
        QString("User %1").arg(m_verificationUserId) : 
        m_verificationUserName;
    
    if (foundMatch) {
        m_verifyScoreLabel->setText(QString("Match Score: %1%").arg(bestScore));
        m_verifyResultLabel->setText(QString("MATCH: %1").arg(userName));
        m_verifyResultLabel->setStyleSheet("QLabel { background-color: #c8e6c9; color: green; padding: 10px; font-weight: bold; font-size: 14px; }");
        log(QString("VERIFICATION SUCCESS: %1 (finger: %2, score: %3%)").arg(userName).arg(matchedTemplate.finger).arg(bestScore));
        QMessageBox::information(this, "Verification Success", 
            QString("Fingerprint MATCHED!\n\nUser: %1\nFinger: %2\nScore: %3%").arg(userName).arg(matchedTemplate.finger).arg(bestScore));
    } else {
        m_verifyScoreLabel->setText(QString("Best Score: %1%").arg(bestScore));
        m_verifyResultLabel->setText(QString("NO MATCH"));
        m_verifyResultLabel->setStyleSheet("QLabel { background-color: #ffcdd2; color: red; padding: 10px; font-weight: bold; font-size: 14px; }");
        log(QString("VERIFICATION FAILED: Tried %1 finger(s), best score: %2%").arg(m_verificationTemplates.size()).arg(bestScore));
        QMessageBox::warning(this, "Verification Failed", 
            QString("Fingerprint does NOT match any registered finger!\n\nUser: %1\nTried: %2 finger(s)\nBest Score: %3%")
                .arg(userName).arg(m_verificationTemplates.size()).arg(bestScore));
    }
    
    // Cleanup
    m_verificationTemplates.clear();
    m_remainingVerificationFingers.clear();
    m_btnStartVerify->setEnabled(true);
    m_btnCaptureVerify->setEnabled(false);
    log("=== VERIFICATION COMPLETED ===");
}

void MainWindowApp::onUserFingersRetrieved(int userId, const QStringList& fingers)
{
    // Check if this is for enrollment (user selected in enrollment dropdown)
    if (m_enrollUserSelect->currentData().toInt() == userId) {
        // Update finger selection - disable already enrolled fingers
        log(QString("User %1 has %2 enrolled finger(s): %3").arg(userId).arg(fingers.size()).arg(fingers.join(", ")));
        
        // Check if currently selected finger is already enrolled
        QString currentFinger = m_enrollFingerSelect->currentText().replace(" (enrolled)", "");
        if (fingers.contains(currentFinger)) {
            // Select first non-enrolled finger
            QStringList allFingers = {"Right Index", "Right Middle", "Right Ring", "Right Pinky", "Right Thumb",
                                      "Left Index", "Left Middle", "Left Ring", "Left Pinky", "Left Thumb"};
            for (const QString& finger : allFingers) {
                if (!fingers.contains(finger)) {
                    m_enrollFingerSelect->setCurrentText(finger);
                    break;
                }
            }
        }
    }
    
    // Check if this is for verification
    if (userId == m_verificationUserId) {
        log(QString("User %1 has %2 registered finger(s)").arg(userId).arg(fingers.size()));
        
        if (fingers.isEmpty()) {
            QMessageBox::warning(this, "No Fingerprints", 
                QString("User %1 has no registered fingerprints. Please enroll a fingerprint first.")
                .arg(m_verificationUserName.isEmpty() ? QString::number(userId) : m_verificationUserName));
            m_btnStartVerify->setEnabled(true);
            m_verifyResultLabel->setText("Result: No fingerprints");
            m_verifyScoreLabel->setText("Score: -");
            m_verificationTemplates.clear();
            m_remainingVerificationFingers.clear();
            return;
        }
        
        // Load all templates for all fingers
        // We'll verify against all of them
        m_verificationTemplates.clear();
        
        log(QString("Loading templates for %1 finger(s)...").arg(fingers.size()));
        m_verifyResultLabel->setText(QString("Loading %1 template(s)...").arg(fingers.size()));
        QApplication::processEvents();
        
        // Load first template to start verification process
        // We'll load others as needed
        QString firstFinger = fingers.first();
        log(QString("Loading template for finger: %1").arg(firstFinger));
        m_backendClient->loadTemplate(m_verificationUserId, firstFinger);
        
        // Store remaining fingers to load
        m_remainingVerificationFingers = fingers;
        m_remainingVerificationFingers.removeFirst(); // Remove first one as we're loading it now
    }
}

void MainWindowApp::onTemplatesLoaded(const QVector<BackendFingerprintTemplate>& templates)
{
    // Handle templates loaded for identification
    log(QString("Loaded %1 templates for identification").arg(templates.size()));
    
    if (templates.isEmpty()) {
        QMessageBox::warning(this, "No Templates", "No fingerprint templates found. Please enroll users first.");
        m_btnIdentify->setEnabled(true);
        return;
    }
    
    // Check if device is still open
    if (!m_fpManager->isReaderOpen()) {
        QMessageBox::critical(this, "Device Not Ready", "Device is not open. Please initialize the reader first.");
        m_btnIdentify->setEnabled(true);
        return;
    }
    
    // Prepare templates vector for identifyUser (like Android implementation)
    // IMPORTANT: Use ALL templates (all fingers) - this matches fingerprint-sdk implementation
    QVector<QPair<int, QByteArray>> templatePairs;
    for (const BackendFingerprintTemplate& tmpl : templates) {
        templatePairs.append(qMakePair(tmpl.userId, tmpl.templateData));
    }
    
    log(QString("Prepared %1 templates for identification (all fingers)").arg(templatePairs.size()));
    log("Place your finger on the reader...");
    
    // Perform identification - returns matchedIndex, not userId (like Android)
    int score = 0;
    int matchedIndex = -1;
    bool success = m_fpManager->identifyUser(templatePairs, matchedIndex, score, nullptr, nullptr);
    
    if (!success) {
        QString error = m_fpManager->getLastError();
        log(QString("Identification error: %1").arg(error));
        QMessageBox::critical(this, "Identification Error", error);
        m_btnIdentify->setEnabled(true);
        return;
    }
    
    if (matchedIndex >= 0 && matchedIndex < templates.size()) {
        // Get the actual matched template using the index (this gives us the correct finger!)
        const BackendFingerprintTemplate& matchedTemplate = templates[matchedIndex];
        int matchedUserId = matchedTemplate.userId;
        QString matchedUserName = matchedTemplate.userName.isEmpty() ? 
            QString("User %1").arg(matchedUserId) : matchedTemplate.userName;
        QString matchedFinger = matchedTemplate.finger;
        
        log(QString("✓ IDENTIFICATION SUCCESS: %1 (User ID: %2, Finger: %3, Score: %4%)")
            .arg(matchedUserName).arg(matchedUserId).arg(matchedFinger).arg(score));
        
        QMessageBox::information(this, "Identification Success", 
            QString("User IDENTIFIED!\n\nUser: %1\nUser ID: %2\nFinger: %3\nScore: %4%")
            .arg(matchedUserName).arg(matchedUserId).arg(matchedFinger).arg(score));
    } else {
        log(QString("✗ IDENTIFICATION FAILED: No match found (score: %1%)").arg(score));
        QMessageBox::warning(this, "Identification Failed", 
            QString("No matching fingerprint found!\n\nScore: %1%").arg(score));
    }
    
    m_btnIdentify->setEnabled(true);
    log("=== IDENTIFICATION COMPLETED ===");
}

void MainWindowApp::onBackendError(const QString& errorMessage)
{
    log(QString("Backend error: %1").arg(errorMessage));
    QMessageBox::warning(this, "Backend Error", errorMessage);
}

void MainWindowApp::enableEnrollmentControls(bool enable)
{
    m_enrollUserSelect->setEnabled(enable);
    m_enrollFingerSelect->setEnabled(enable);
    m_btnStartEnroll->setEnabled(enable && m_fpManager->isReaderOpen());
    m_btnCaptureEnroll->setEnabled(!enable);
}

void MainWindowApp::enableVerificationControls(bool enable)
{
    m_btnStartVerify->setEnabled(enable && m_fpManager->isReaderOpen());
    m_btnIdentify->setEnabled(enable && m_fpManager->isReaderOpen());
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
    
    // Force immediate repaint of widgets to ensure visual responsiveness on macOS
    m_enrollProgress->repaint();
    m_enrollStatusLabel->repaint();
    m_enrollImagePreview->repaint();

    // Process events to update UI
    QApplication::processEvents();
}

