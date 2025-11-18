#pragma once

//#include "TFolders.h"
#include "TEditor.h"
//#include "TTerminal.h"
#include "TMenu.h"
#include "TSettings.h"
#include "TConsole.h"

#include <QMainWindow>
#include <QStatusBar>

#include <QTreeView>
#include <QFileSystemModel>
#include <QSplitter>
#include <QLabel>


QT_BEGIN_NAMESPACE
namespace Ui { class Taif; }
QT_END_NAMESPACE

class Taif : public QMainWindow
{
    Q_OBJECT

public:
    Taif(const QString& filePath = "", QWidget* parent = nullptr);
    ~Taif();
    void loadFolder(const QString &folderPath);
protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
private slots:
    void newFile();
    void openFile(QString);
    void saveFile();
    void saveFileAs();
    void handleOpenFolderMenu();
    void openSettings();
    void exitApp();

    void runAlif();
    void aboutTaif();

    void updateWindowTitle();
    void onModificationChanged(bool modified);
    void onFileTreeDoubleClicked(const QModelIndex &index);
    void closeTab(int index);
    void toggleSidebar();

    void toggleConsole();

    void updateCursorPosition();
    void onCurrentTabChanged();

private:
    int needSave();
    TEditor* currentEditor();

private:
    QTabWidget *tabWidget{};
    TMenuBar* menuBar{};
    TSettings* settings{};
    QAction *toggleSidebarAction{};
    QString folderPath{};
    QAbstractItemModel* model{};

    QSplitter *mainSplitter{};
    QTreeView *fileTreeView{};
    QFileSystemModel *fileSystemModel{};


    QSplitter *editorSplitter;
    TConsole *console;
    QTabWidget *consoleTabWidget;

    TConsole *cmdConsole;     // ✅ أضف هذا المتغير العضو
    TConsole *alifConsole;    // ✅ أضف هذا المتغير العضو (اختياري لكن مفيد)

    QLabel *cursorPositionLabel;
    QLabel *encodingLabel;
    QProcess *alifProcess; // ✅ أضف هذا المتغير
    QProcess *currentAlifProcess = nullptr; // ✅ متغير لتتبع العملية الحالية

};
