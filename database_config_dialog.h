#ifndef DATABASE_CONFIG_DIALOG_H
#define DATABASE_CONFIG_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSettings>

class DatabaseConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit DatabaseConfigDialog(QWidget *parent = nullptr);
    
    struct Config {
        QString type; // "SQLITE" or "POSTGRESQL"
        QString host;
        int port;
        QString name; // Database name or file path
        QString user;
        QString password;
    };
    
    static Config loadConfig();
    static void saveConfig(const Config& config);
    static bool hasConfig();

private slots:
    void onTypeChanged(const QString& type);
    void onBrowseClicked();
    void onTestClicked();
    void onSaveClicked();

private:
    QComboBox* m_typeCombo;
    QLineEdit* m_hostEdit;
    QLineEdit* m_portEdit;
    QLineEdit* m_nameEdit;
    QLineEdit* m_userEdit;
    QLineEdit* m_passEdit;
    QPushButton* m_browseBtn;
    QLabel* m_statusLabel;
    QPushButton* m_saveBtn;
    class QStackedWidget* m_settingsStack;
    QLineEdit* m_pgNameEdit; // Separate edit for PG database name
    
    void setupUI();
};

#endif // DATABASE_CONFIG_DIALOG_H

