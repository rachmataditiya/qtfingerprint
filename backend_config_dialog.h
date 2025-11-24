#ifndef BACKEND_CONFIG_DIALOG_H
#define BACKEND_CONFIG_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSettings>

class BackendConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackendConfigDialog(QWidget *parent = nullptr);
    
    static QString loadBackendUrl();
    static void saveBackendUrl(const QString& url);
    static bool hasConfig();

private slots:
    void onTestClicked();
    void onSaveClicked();

private:
    QLineEdit* m_urlEdit;
    QLabel* m_statusLabel;
    QPushButton* m_saveBtn;
    
    void setupUI();
};

#endif // BACKEND_CONFIG_DIALOG_H

