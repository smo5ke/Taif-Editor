#ifndef WELCOMEWINDOW_H
#define WELCOMEWINDOW_H
#include <QMainWindow>

// قم بتضمين forward declarations لتسريع عملية البناء
class QPushButton;
class QListWidget;
class QCheckBox;
class QListWidgetItem;

class WelcomeWindow : public QMainWindow
{
    Q_OBJECT

public:
    WelcomeWindow(QWidget *parent = nullptr);
    ~WelcomeWindow();
protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QMenu *fileMenu;
    QMenu *editMenu;
    QPushButton *newFileButton;
    QPushButton *openFileButton;
    QPushButton *openFolderButton;
    QListWidget *recentFilesList;

    QPushButton *newSessionButton;
    QPushButton *manageSessionsButton;
    QListWidget *savedSessionsList;

    QCheckBox *showOnStartupCheck;

private slots:
    void handleNewFileRequest();
    void handleOpenFileRequest();
    void handleOpenFolderRequest();
    void onRecentFileClicked(QListWidgetItem *); // ✅ دالة جديدة
};

#endif // WELCOMEWINDOW_H
