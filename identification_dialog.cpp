#include "identification_dialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPainter>
#include <QRadialGradient>

IdentificationDialog::IdentificationDialog(FingerprintManager* fpManager, DatabaseManager* dbManager, QWidget *parent)
    : QDialog(parent)
    , m_fpManager(fpManager)
    , m_dbManager(dbManager)
    , m_isScanning(false)
    , m_cancelRequested(false)
{
    setupUI();
    setWindowTitle("Identify User");
    setFixedSize(500, 500);
}

IdentificationDialog::~IdentificationDialog()
{
    m_cancelRequested = true; // Ensure background threads stop
}

void IdentificationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Automatically perform a scan check or just wait for button
    updateStatus("Ready to Scan", "#333");
}

void IdentificationDialog::closeEvent(QCloseEvent *event)
{
    if (m_isScanning) {
        // Ideally cancel the scan, but libfprint sync calls are hard to cancel without thread termination
        // For now we just let the dialog close, but the background thread might still be running.
        // In a production app, we'd handle cancellation better.
    }
    QDialog::closeEvent(event);
}

void IdentificationDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Header / Status Area
    QVBoxLayout* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(10);

    m_statusLabel = new QLabel("Identification Mode");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #333; }");
    headerLayout->addWidget(m_statusLabel);

    m_instructionLabel = new QLabel("Click 'Scan Fingerprint' and place your finger on the reader.");
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; }");
    headerLayout->addWidget(m_instructionLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setVisible(false);
    headerLayout->addWidget(m_progressBar);

    mainLayout->addLayout(headerLayout);

    // User Info Card (Hidden by default)
    m_userGroup = new QGroupBox("Identified User");
    m_userGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 8px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    m_userGroup->setVisible(false); // Hide initially

    QVBoxLayout* cardLayout = new QVBoxLayout(m_userGroup);
    cardLayout->setSpacing(15);
    cardLayout->setContentsMargins(20, 25, 20, 20);

    QHBoxLayout* profileLayout = new QHBoxLayout();
    
    // Avatar (Placeholder)
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(80, 80);
    m_avatarLabel->setStyleSheet("QLabel { background-color: #e0e0e0; border-radius: 40px; border: 2px solid #bdbdbd; }");
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    
    // Draw a simple user icon
    QPixmap avatarPix(80, 80);
    avatarPix.fill(Qt::transparent);
    QPainter p(&avatarPix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#2196F3"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 80, 80);
    p.setBrush(Qt::white);
    p.drawEllipse(20, 20, 40, 40); // Head
    p.drawChord(10, 50, 60, 50, 0, 180 * 16); // Shoulders
    m_avatarLabel->setPixmap(avatarPix);

    profileLayout->addWidget(m_avatarLabel);

    // Details
    QVBoxLayout* detailsLayout = new QVBoxLayout();
    detailsLayout->setSpacing(5);

    m_nameLabel = new QLabel("Name: -");
    m_nameLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    detailsLayout->addWidget(m_nameLabel);

    m_emailLabel = new QLabel("Email: -");
    m_emailLabel->setStyleSheet("font-size: 13px; color: #555;");
    detailsLayout->addWidget(m_emailLabel);

    m_idLabel = new QLabel("ID: -");
    m_idLabel->setStyleSheet("font-size: 12px; color: #888;");
    detailsLayout->addWidget(m_idLabel);

    profileLayout->addLayout(detailsLayout);
    cardLayout->addLayout(profileLayout);

    // Score badge
    m_scoreLabel = new QLabel("Match Score: 95%");
    m_scoreLabel->setStyleSheet("QLabel { background-color: #c8e6c9; color: #2e7d32; padding: 5px 10px; border-radius: 4px; font-weight: bold; }");
    m_scoreLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_scoreLabel);

    mainLayout->addWidget(m_userGroup);
    
    // Spacer
    mainLayout->addStretch();

    // Buttons
    m_btnScan = new QPushButton("Scan Fingerprint");
    m_btnScan->setMinimumHeight(50);
    m_btnScan->setCursor(Qt::PointingHandCursor);
    m_btnScan->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; font-size: 16px; font-weight: bold; border-radius: 5px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:disabled { background-color: #BDBDBD; }"
    );
    mainLayout->addWidget(m_btnScan);

    m_btnCancel = new QPushButton("Cancel");
    m_btnCancel->setMinimumHeight(50);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; font-size: 16px; font-weight: bold; border-radius: 5px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
    );
    m_btnCancel->setVisible(false);
    mainLayout->addWidget(m_btnCancel);

    m_btnClose = new QPushButton("Close");
    m_btnClose->setCursor(Qt::PointingHandCursor);
    m_btnClose->setStyleSheet("QPushButton { border: none; color: #757575; font-size: 14px; text-decoration: underline; } QPushButton:hover { color: #424242; }");
    mainLayout->addWidget(m_btnClose);

    connect(m_btnScan, &QPushButton::clicked, this, &IdentificationDialog::onScanClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &IdentificationDialog::onCancelClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

void IdentificationDialog::updateStatus(const QString& text, const QString& color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString("QLabel { font-size: 24px; font-weight: bold; color: %1; }").arg(color));
}

void IdentificationDialog::showUserInfo(const User& user, int score)
{
    m_nameLabel->setText(user.name);
    m_emailLabel->setText(user.email.isEmpty() ? "No Email" : user.email);
    m_idLabel->setText(QString("User ID: %1").arg(user.id));
    m_scoreLabel->setText(QString("Match Confidence: %1%").arg(score));
    
    m_userGroup->setVisible(true);
}

void IdentificationDialog::clearUserInfo()
{
    m_userGroup->setVisible(false);
}

void IdentificationDialog::onCancelClicked()
{
    m_cancelRequested = true;
    m_btnCancel->setEnabled(false);
    m_btnCancel->setText("Stopping...");
}

void IdentificationDialog::onScanClicked()
{
    if (m_isScanning) return;

    m_btnScan->setEnabled(false);
    m_btnScan->setVisible(false);
    m_btnClose->setEnabled(false);
    
    m_btnCancel->setVisible(true);
    m_btnCancel->setEnabled(true);
    m_btnCancel->setText("Cancel");
    m_cancelRequested = false;

    clearUserInfo();
    updateStatus("Preparing...", "#2196F3");
    m_instructionLabel->setText("Loading user templates...");
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    // Fetch all templates from DB
    QVector<User> users = m_dbManager->getAllUsers();
    if (users.isEmpty()) {
        updateStatus("No Users", "red");
        m_instructionLabel->setText("No users found in database to match against.");
        m_btnScan->setEnabled(true);
        m_btnScan->setVisible(true);
        m_btnCancel->setVisible(false);
        m_btnClose->setEnabled(true);
        m_progressBar->setVisible(false);
        return;
    }

    QMap<int, QByteArray> templates;
    for (const User& u : users) {
        if (!u.fingerprintTemplate.isEmpty()) {
            templates.insert(u.id, u.fingerprintTemplate);
        }
    }

    if (templates.isEmpty()) {
        updateStatus("No Templates", "red");
        m_instructionLabel->setText("Users found but no fingerprint data available.");
        m_btnScan->setEnabled(true);
        m_btnScan->setVisible(true);
        m_btnCancel->setVisible(false);
        m_btnClose->setEnabled(true);
        m_progressBar->setVisible(false);
        return;
    }

    // Run identification
    m_isScanning = true;
    updateStatus("Scanning...", "#2196F3");
    m_instructionLabel->setText("Place your finger on the reader now...");

    auto progressCb = [this](int current, int total) {
        QMetaObject::invokeMethod(this, [this, current, total]() {
            m_progressBar->setMaximum(total);
            m_progressBar->setValue(current);
            if (current < total) {
                m_statusLabel->setText(QString("Loading Gallery: %1/%2").arg(current).arg(total));
            } else {
                m_statusLabel->setText("Identifying...");
            }
        }, Qt::QueuedConnection);
    };

    auto cancelCb = [this]() -> bool {
        return m_cancelRequested.load();
    };
    
    // Linux: Run synchronously on main thread to avoid threading issues with libfprint
    // macOS: Run in background thread to keep UI responsive
#ifdef Q_OS_LINUX
    QApplication::processEvents(); // Process any pending events before blocking
    
    int score = 0;
    // identifyUser handles processEvents internally now for gallery loading
    int userId = m_fpManager->identifyUser(templates, score, progressCb, cancelCb);
    
    // Handle result immediately
    if (m_cancelRequested) {
        updateStatus("Cancelled", "#FF9800");
        m_instructionLabel->setText("Identification cancelled by user.");
    } else if (userId != -1) {
        // Match found!
        User user;
        if (m_dbManager->getUserById(userId, user)) {
            updateStatus("Match Found!", "#4CAF50");
            m_instructionLabel->setText("User identified successfully.");
            showUserInfo(user, score);
        } else {
            updateStatus("User Error", "#F44336");
            m_instructionLabel->setText("Match found but failed to load user details.");
        }
    } else {
        // No match
        updateStatus("No Match", "#F44336");
        m_instructionLabel->setText("Fingerprint scan successful, but no matching user found.");
    }

    m_isScanning = false;
    m_btnScan->setEnabled(true);
    m_btnScan->setVisible(true);
    m_btnCancel->setVisible(false);
    m_btnClose->setEnabled(true);
    m_progressBar->setVisible(false);
#else
    QFutureWatcher<QPair<int, int>>* watcher = new QFutureWatcher<QPair<int, int>>(this);
    connect(watcher, &QFutureWatcher<QPair<int, int>>::finished, this, [this, watcher]() {
        QPair<int, int> result = watcher->result();
        int userId = result.first;
        int score = result.second;
        
        if (m_cancelRequested) {
            updateStatus("Cancelled", "#FF9800");
            m_instructionLabel->setText("Identification cancelled by user.");
        } else if (userId != -1) {
            // Match found!
            User user;
            if (m_dbManager->getUserById(userId, user)) {
                updateStatus("Match Found!", "#4CAF50");
                m_instructionLabel->setText("User identified successfully.");
                showUserInfo(user, score);
            } else {
                updateStatus("User Error", "#F44336");
                m_instructionLabel->setText("Match found but failed to load user details.");
            }
        } else {
            // No match
            updateStatus("No Match", "#F44336");
            m_instructionLabel->setText("Fingerprint scan successful, but no matching user found.");
        }

        m_isScanning = false;
        m_btnScan->setEnabled(true);
        m_btnScan->setVisible(true);
        m_btnCancel->setVisible(false);
        m_btnClose->setEnabled(true);
        m_progressBar->setVisible(false);
        watcher->deleteLater();
    });

    QFuture<QPair<int, int>> future = QtConcurrent::run([this, templates, progressCb, cancelCb]() {
        int score = 0;
        int userId = m_fpManager->identifyUser(templates, score, progressCb, cancelCb);
        return QPair<int, int>(userId, score);
    });

    watcher->setFuture(future);
#endif
}

