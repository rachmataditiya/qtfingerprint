#ifndef IDENTIFICATION_DIALOG_H
#define IDENTIFICATION_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTimer>

#include "database_manager.h"
#include "digitalpersonalib/include/fingerprint_manager.h"

class IdentificationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IdentificationDialog(FingerprintManager* fpManager, DatabaseManager* dbManager, QWidget *parent = nullptr);
    ~IdentificationDialog();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onScanClicked();

private:
    void setupUI();
    void updateStatus(const QString& text, const QString& color = "black");
    void showUserInfo(const User& user, int score);
    void clearUserInfo();

    FingerprintManager* m_fpManager;
    DatabaseManager* m_dbManager;

    // UI Elements
    QLabel* m_statusLabel;
    QLabel* m_instructionLabel;
    QPushButton* m_btnScan;
    QPushButton* m_btnClose;
    
    // User Info Section
    QGroupBox* m_userGroup;
    QLabel* m_nameLabel;
    QLabel* m_emailLabel;
    QLabel* m_idLabel;
    QLabel* m_scoreLabel;
    QLabel* m_avatarLabel;

    bool m_isScanning;
};

#endif // IDENTIFICATION_DIALOG_H

