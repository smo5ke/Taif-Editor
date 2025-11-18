#include "Taif.h"
#include "welcomewindow.h"
#include "TConsole.h"
#include "ProcessWorker.h"

#include <QThread>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QTextStream>
#include <QApplication>
#include <QToolBar>
#include <QHeaderView>
#include <QSettings>
#include <QProcess>
#include <QStyleFactory>
#include <QKeyEvent>
#include <QTimer>


Taif::Taif(const QString& filePath, QWidget *parent)
    : QMainWindow(parent)
{
    // ===================================================================
    // الخطوة 1: إنشاء المكونات الرئيسية
    // ===================================================================
    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("MainTabs");
    tabWidget->setDocumentMode(true);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    menuBar = new TMenuBar(this);
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    fileTreeView = new QTreeView(this);
    fileSystemModel = new QFileSystemModel(this);

    editorSplitter = new QSplitter(Qt::Vertical, this);

    // ===================================================================
    // الخطوة 2: إعداد النافذة وشريط القوائم
    // ===================================================================
    QScreen* screenSize = QGuiApplication::primaryScreen();
    this->setGeometry(screenSize->size().width() / 4, screenSize->size().height() / 5, 900, 700);
    this->setMenuBar(menuBar);
    // ===================================================================
    // ✅ الخطوة 3: إعداد شريط الأدوات وزر تبديل الشريط (مهم الترتيب هنا)
    // ===================================================================
    QToolBar *mainToolBar = new QToolBar("Main Toolbar", this); // أنشئ الكائن
    mainToolBar->setObjectName("mainToolBar");
    mainToolBar->setMovable(false);
    mainToolBar->setIconSize(QSize(30, 30));
    this->addToolBar(Qt::RightToolBarArea, mainToolBar); //

    toggleSidebarAction = new QAction(this);
    toggleSidebarAction->setIcon(QIcon(":/icons/resources/side-bar.png"));
    toggleSidebarAction->setCheckable(true);
    toggleSidebarAction->setChecked(false);
    mainToolBar->addAction(toggleSidebarAction);

    // ---  زر التشغيل ---
    QAction *runToolbarAction = new QAction(this);
    runToolbarAction->setIcon(QIcon(":/icons/resources/run.png"));
    runToolbarAction->setToolTip("تشغيل الملف الحالي (Run)");
    mainToolBar->addAction(runToolbarAction);
    connect(runToolbarAction, &QAction::triggered, this, &Taif::runAlif);

    // يمكنك إضافة أزرار أخرى هنا إذا أردت
    // mainToolBar->addSeparator();
    // mainToolBar->addAction(menuBar->newAction); // مثال لإضافة زر New


    // ===================================================================
    // الخطوة 4: إعداد الشريط الجانبي (يبقى مخفيًا وفارغًا)
    // ===================================================================
    fileTreeView->setModel(fileSystemModel);
    fileTreeView->header()->setVisible(false);
    fileTreeView->hideColumn(1);
    fileTreeView->hideColumn(2);
    fileTreeView->hideColumn(3);
    fileSystemModel->setRootPath(QDir::homePath());
    fileTreeView->setRootIndex(fileSystemModel->index(QDir::homePath()));
    fileTreeView->setVisible(false);

    // ===================================================================
    // الخطوة 5: تجميع الواجهة (الفاصل)
    // ===================================================================
    mainSplitter->addWidget(fileTreeView);
    mainSplitter->addWidget(tabWidget);
    mainSplitter->setSizes({200, 700}); // شريط جانبي أصغر، محرر أكبر
    this->setCentralWidget(mainSplitter);

    // ===================================================================

    consoleTabWidget = new QTabWidget(this);
    consoleTabWidget->setDocumentMode(true);

    TConsole *cmdConsole = new TConsole(this);
    cmdConsole->setObjectName("cmdConsole");
    consoleTabWidget->addTab(cmdConsole, "Terminal (CMD)");
    cmdConsole->setConsoleRTL();
    cmdConsole->startCmd();


    // ✅ 5. أضف التبويبات والكونسول إلى الفاصل العمودي
    editorSplitter->addWidget(tabWidget);
    editorSplitter->addWidget(consoleTabWidget);
    editorSplitter->setSizes({600, 200}); // 600 للمحرر, 200 للكونسول
    consoleTabWidget->hide(); // ابدأ مخفيًا

    // ✅ 6. أضف الشريط الجانبي والفاصل العمودي (الجديد) إلى الفاصل الأفقي (الرئيسي)
    mainSplitter->addWidget(fileTreeView);
    mainSplitter->addWidget(editorSplitter); // ✅ أضف الفاصل الجديد هنا
    mainSplitter->setSizes({200, 700}); // 200 للشريط الجانبي, 700 لباقي الواجهة
    this->setCentralWidget(mainSplitter); // ✅ الفاصل الرئيسي هو المكون المركزي

    // ===================================================================

    cursorPositionLabel = new QLabel(this);
    cursorPositionLabel->setText("UTF-8  السطر: 1  العمود: 1");
    statusBar()->addPermanentWidget(cursorPositionLabel);

    // ===================================================================
    // الخطوة 6: ربط الإشارات والمقابس
    // ===================================================================
    connect(fileTreeView, &QTreeView::doubleClicked, this, &Taif::onFileTreeDoubleClicked);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &Taif::closeTab);
    connect(toggleSidebarAction, &QAction::triggered, this, &Taif::toggleSidebar);
    QShortcut* saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &Taif::saveFile);
    connect(menuBar, &TMenuBar::newRequested, this, &Taif::newFile);
    connect(menuBar, &TMenuBar::openFileRequested, this, [this](){this->openFile("");});
    connect(menuBar, &TMenuBar::saveRequested, this, &Taif::saveFile);
    connect(menuBar, &TMenuBar::saveAsRequested, this, &Taif::saveFileAs);
    connect(menuBar, &TMenuBar::settingsRequest, this, &Taif::openSettings);
    connect(menuBar, &TMenuBar::exitRequested, this, &Taif::exitApp); // سيستدعي closeEvent بشكل غير مباشر
    connect(menuBar, &TMenuBar::runRequested, this, &Taif::runAlif);
    connect(menuBar, &TMenuBar::aboutRequested, this, &Taif::aboutTaif);
    connect(menuBar, &TMenuBar::openFolderRequested, this, &Taif::handleOpenFolderMenu);
    // ربط تغيير التبويب النشط لتحديث عنوان النافذة
    connect(tabWidget, &QTabWidget::currentChanged, this, &Taif::updateWindowTitle);
    connect(tabWidget, &QTabWidget::currentChanged, this, &Taif::onCurrentTabChanged);
    onCurrentTabChanged();

    // ===================================================================
    // ✅ الخطوة 7: تطبيق التصميم (QSS) - نسخة نهائية
    // ===================================================================
    QString styleSheet = R"(
        QMainWindow { background-color: #1e202e;font-size: 12pt;  }

        /* --- تصميم شريط الأدوات --- */
QToolBar {
            background-color: #1e202e;
            border: none;
            /* ✅ زيادة الحشو حول الشريط لجعله أعرض قليلاً */
            padding: 5px;
            spacing: 10px; /* مسافة بين كل زر والآخر */
        }

        /* تصميم أزرار شريط الأدوات */
        QToolBar QToolButton {
            background-color: transparent;
            border: none;
            border-radius: 6px; /* حواف دائرية ناعمة */

            /* ✅ أهم جزء: تحديد حجم مربع الزر ليكون كبيراً ومربعاً */
            min-width: 40px;
            max-width: 40px;
            min-height: 40px;
            max-height: 40px;

            /* ✅ ضبط الحشو لضمان توسط الأيقونة (30px) داخل الزر (40px) */
            /* 40 - 30 = 10، يعني 5 بكسل من كل جهة */
            padding: 0px;
            margin: 0px;
        }

        QToolBar QToolButton:hover {
            background-color: #4f5357;
        }

        QToolBar QToolButton:pressed {
            background-color: #2a2d31;
        }

        QToolBar QToolButton:checked {
            background-color: #0078d7; /* اللون الأزرق */
        }

        /* --- تصميم الشريط الجانبي --- */
        QTreeView { background-color: #232629; border: none; color: #cccccc;font-size: 10pt; }
        QTreeView::item { padding: 5px 3px; border-radius: 3px; }
        QTreeView::item:selected:active { background-color: #094771; color: #ffffff; }
        QTreeView::item:selected:!active { background-color: #3a3d41; }
        QTreeView::branch { background: transparent; }

        /* --- تصميم الفاصل --- */
        QSplitter::handle {
            background-color: #094771;
            width: 1px;
        }
        QSplitter::handle:horizontal {
            width: 1px;
        }
        QSplitter::handle:vertical {
            height: 1px;
        }

        /* --- تصميم التبويبات --- */
        QTabWidget#MainTabs::pane {
            border: none;
            background-color: #1e202e;
        }
        QTabWidget#MainTabs QTabBar { /* شريط التبويبات نفسه */
            font-size: 10pt !important;
            background-color: #1e202e;
            border: none;
            qproperty-drawBase: 0;
            margin: 0px;
            padding: 0px;
        }
       QTabWidget#MainTabs QTabBar::tab {
            background: #2d2d30;
            color: #909090;
            padding: 0px 0px;
            border: none;
            border-top: 1px solid #444444;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
       QTabWidget#MainTabs QTabBar::tab:selected {
            background: #1e1e1e;
            color: #ffffff;
            border-top: 2px solid #007acc;
            border-left: 1px solid #007acc;
            border-right: 1px solid #007acc;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        QTabWidget#MainTabs QTabBar::tab:hover:!selected {
            background: #3e3e42;
        }
        QTabWidget#MainTabs QTabBar::close-button {
            image: url(:/icons/resources/close.png);
            background: transparent;
            border: none;
            subcontrol-position: right;
            subcontrol-origin: padding;
            border-radius: 3px;
            padding: 1px;
            margin-right: 2px;
            min-width: 12px;
            min-height: 12px;
        }
        QTabWidget#MainTabs QTabBar::close-button:hover { background: #5a5a5f; }

        QStatusBar {
            background-color: #333333;
            color: #cccccc;
            // border-top: 1px solid #4f4f4f;
            font-size: 6pt;
        }
    )";
    tabWidget->setStyleSheet(styleSheet);

    tabWidget->setTabsClosable(true);
    tabWidget->setStyleSheet(R"(
    QTabWidget#MainTabs QTabWidget::pane {
        border: none;
        background-color: #1e202e;
    }
    QTabWidget#MainTabs QTabBar {
        font-size: 9pt;
        background-color: #1e202e;
        border: none;
        qproperty-drawBase: 0;
        margin: 0px;
        padding: 0px;
    }
    QTabWidget#MainTabs QTabBar::tab {
        background: #2d2d30;
        color: #909090;
        padding: 0px 8px;
        border: none;
        border-top: 1px solid #444444;
        border-top-left-radius: 6px;
        border-top-right-radius: 6px;
    }
    QTabWidget#MainTabs QTabBar::tab:selected {
        background: #1e1e1e;
        color: #ffffff;
        border-top: 2px solid #007acc;
        border-left: 1px solid #007acc;
        border-right: 1px solid #007acc;
    }
    QTabWidget#MainTabs QTabBar::tab:hover:!selected {
        background: #3e3e42;
    }
    QTabWidget#MainTabs QTabBar::close-button {
            image: url(:/icons/resources/close.png);
            background: transparent;
            border: none;
            subcontrol-position: right;
            subcontrol-origin: padding;
            border-radius: 3px;
            padding: 1px;
            margin-right: 2px;
            min-width: 6px;
            min-height: 6px;
        }
        QTabWidget#MainTabs QTabBar::close-button:hover { background: #5a5a5f; }

)");

    this->setStyleSheet(styleSheet);


    // ===================================================================
    // الخطوة 8: تحميل الملف المبدئي أو إنشاء تبويب جديد
    // ===================================================================
    installEventFilter(this);

    if (!filePath.isEmpty()) {
        openFile(filePath);
    } else {
        newFile();
    }
}

Taif::~Taif() {

    if (TEditor* editor = currentEditor()) {
        QSettings settings("Alif", "Taif");
        settings.setValue("editorFontSize", editor->font().pointSize());
    }
}

void Taif::closeEvent(QCloseEvent *event) {
    int saveResult = needSave();

    if (saveResult == 1) {
        saveFile();
    } else if (saveResult == 0) {
        event->ignore();
        return;
    }

    event->accept();
}

bool Taif::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F6) {
            toggleConsole();
            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}


void Taif::toggleConsole()
{
    consoleTabWidget->setVisible(!consoleTabWidget->isVisible());

    if (consoleTabWidget->isVisible()) {
        QWidget* currentConsole = consoleTabWidget->currentWidget();
        if (currentConsole) {
            currentConsole->setFocus();
        }
    } else {
        if(TEditor* editor = currentEditor()) {
            editor->setFocus();
        }
    }
}

/* ----------------------------------- File Menu Button ----------------------------------- */

int Taif::needSave() {
    if (TEditor* editor = currentEditor()) {
        if (editor->document()->isModified()) {
       QMessageBox msgBox;
        msgBox.setWindowTitle("طيف");
        msgBox.setText("تم التعديل على الملف.\n"    \
                       "هل تريد حفظ التغييرات؟");
        QPushButton *saveButton = msgBox.addButton("حفظ", QMessageBox::AcceptRole);
        QPushButton *discardButton = msgBox.addButton("تجاهل", QMessageBox::DestructiveRole);
        QPushButton *cancelButton = msgBox.addButton("إلغاء", QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelButton);

        QFont msgFont = this->font();
        msgFont.setPointSize(10);
        saveButton->setFont(msgFont);
        discardButton->setFont(msgFont);
        cancelButton->setFont(msgFont);

        msgBox.exec();

        QAbstractButton *clickedButton = msgBox.clickedButton();
        if (clickedButton == saveButton) {
            return 1;
        } else if (clickedButton == discardButton) {
            return 2;
        } else if (clickedButton == cancelButton) {
            return 0;
        }
        }
    }

    return 2;
}

void Taif::newFile() {

    TEditor* editor = currentEditor();
    if (editor) {
        int isNeedSave = needSave();
        if (!isNeedSave) return;
        if (isNeedSave == 1) this->saveFile();
    }

    TEditor *newEditor = new TEditor(this);
    tabWidget->addTab(newEditor, "غير معنون");
    tabWidget->setCurrentWidget(newEditor);

    connect(newEditor, &TEditor::openRequest, this, [this](QString filePath){this->openFile(filePath);});
    connect(newEditor->document(), &QTextDocument::modificationChanged, this, &Taif::onModificationChanged);
    updateWindowTitle();
}

void Taif::openFile(QString filePath) {
    if (TEditor* current = currentEditor()) {
        int isNeedSave = needSave();
        if (!isNeedSave) return;
        if (isNeedSave == 1) this->saveFile();
    }

    if (filePath.isEmpty()) {
        filePath = QFileDialog::getOpenFileName(this, "فتح ملف", "", "ملف ألف (*.alif *.aliflib);;All Files (*)");
    }

    if (!filePath.isEmpty()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            TEditor* editor = qobject_cast<TEditor*>(tabWidget->widget(i));
            if (editor && editor->property("filePath").toString() == filePath) {
                tabWidget->setCurrentIndex(i);
                return;
            }
        }
        // -----------------------------------------

        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            TEditor *newEditor = new TEditor(this);
            connect(newEditor->document(), &QTextDocument::modificationChanged, this, &Taif::onModificationChanged);
            newEditor->setPlainText(content);
            newEditor->setProperty("filePath", filePath);

            connect(newEditor->document(), &QTextDocument::modificationChanged, this, &Taif::onModificationChanged);
            connect(newEditor, &QPlainTextEdit::cursorPositionChanged, this, &Taif::updateCursorPosition);

            QFileInfo fileInfo(filePath);
            tabWidget->addTab(newEditor, fileInfo.fileName());
            tabWidget->setCurrentWidget(newEditor);
            updateWindowTitle();


            QSettings settings("Alif", "Taif");
            QStringList recentFiles = settings.value("RecentFiles").toStringList();
            recentFiles.removeAll(filePath);
            recentFiles.prepend(filePath);
            while (recentFiles.size() > 10) {
                recentFiles.removeLast();
            }
            settings.setValue("RecentFiles", recentFiles);

        } else {
            QMessageBox::warning(this, "خطأ", "لا يمكن فتح الملف");
        }
    }
}

void Taif::loadFolder(const QString &folderPath)
{

    if (!folderPath.isEmpty() && QDir(folderPath).exists()) {
        fileTreeView->setVisible(true);

        fileTreeView->setRootIndex(fileSystemModel->index(folderPath));
    } else {
        fileTreeView->setVisible(false);
    }
}

void Taif::handleOpenFolderMenu()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "اختر مجلد", QDir::homePath());
    if (folderPath.isEmpty()) return;

    QFileSystemModel *model = new QFileSystemModel(this);
    model->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    model->setRootPath(folderPath);
    loadFolder(folderPath);

}

void Taif::toggleSidebar()
{
    bool shouldBeVisible = !fileTreeView->isVisible();
    fileTreeView->setVisible(shouldBeVisible);
    toggleSidebarAction->setChecked(shouldBeVisible);

    if (shouldBeVisible && fileTreeView->rootIndex() == QModelIndex()) {
        QString homePath = QDir::homePath();
        fileTreeView->setRootIndex(fileSystemModel->index(homePath));
    }
}

void Taif::onFileTreeDoubleClicked(const QModelIndex &index)
{
    const QString filePath = fileSystemModel->filePath(index);

    if (!fileSystemModel->isDir(index)) {
        openFile(filePath);
    }
}

void Taif::saveFile() {
    TEditor *editor = currentEditor();
    if (!editor) return;

    QString filePath = editor->property("filePath").toString();
    // --------------------------------------------------------

    QString content = editor->toPlainText();

    if (filePath.isEmpty()) {
        saveFileAs();
        return;
    } else {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << content;
            file.close();
            editor->document()->setModified(false);

            int index = tabWidget->indexOf(editor);
            if (index != -1) {
                QFileInfo fileInfo(filePath);
                tabWidget->setTabText(index, fileInfo.fileName());
            }

            updateWindowTitle();
            return ;
        } else {
            QMessageBox::warning(this, "خطأ", "لا يمكن حفظ الملف");
            return;
        }
    }
}

void Taif::saveFileAs() {
    TEditor *editor = currentEditor();
    if (!editor) return ;

    QString content = editor->toPlainText();
    QString currentPath = editor->property("filePath").toString();
    QString currentName = currentPath.isEmpty() ? "ملف جديد.alif" : QFileInfo(currentPath).fileName();
    QString fileName = QFileDialog::getSaveFileName(this, "حفظ الملف", currentName, "ملف ألف (*.alif);;مكتبة ألف(*.aliflib);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << content;
            file.close();

            editor->setProperty("filePath", fileName);
            // ---------------------------------------------------

            editor->document()->setModified(false);

            int index = tabWidget->indexOf(editor);
            if (index != -1) {
                QFileInfo fileInfo(fileName);
                tabWidget->setTabText(index, fileInfo.fileName());
            }

            updateWindowTitle();
            return ;
        } else {
            QMessageBox::warning(this, "خطأ", "لا يمكن حفظ الملف");
            return ;
        }
    }
    return ;
}

void Taif::openSettings() {
    if (settings and settings->isVisible()) return;

    settings = new TSettings(this);
    connect(settings, &TSettings::fontSizeChanged, this, [this](int size){
        if (TEditor* editor = currentEditor()) {
            editor->updateFontSize(size);
        }
    });
    settings->show();
}


void Taif::exitApp() {
    int isNeedSave = needSave();
    if (!isNeedSave) {
        return;
    }
    else if (isNeedSave == 1) {
        this->saveFile();
        return;
    }

    WelcomeWindow *welcome = new WelcomeWindow();
    welcome->show();
    this->close();
}

void Taif::onCurrentTabChanged()
{
    updateWindowTitle();
    updateCursorPosition();

    TEditor* editor = currentEditor();
    if (editor) {
        connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &Taif::updateCursorPosition);
    }
}

void Taif::updateCursorPosition()
{
    TEditor* editor = currentEditor();
    if (editor) {
        const QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int column = cursor.columnNumber() + 1;

        cursorPositionLabel->setText(QString("UTF-8    السطر: %1   العمود: %2 ").arg(line).arg(column));
    } else {
        cursorPositionLabel->setText("");
    }
}


/* ----------------------------------- Run Menu Button ----------------------------------- */

// void Taif::runAlif() {
//     QString program{};
//     QStringList args{};
//     QString command{};
//     TEditor *editor = currentEditor(); // ✅ احصل على المحرر النشط
//     QStringList arguments{editor->filePath};
//     QString workingDirectory = QCoreApplication::applicationDirPath();

//     if (editor->filePath.isEmpty() or (currentEditor() && currentEditor()->document()->isModified())) {
//         QMessageBox::warning(nullptr, "تنبيه", "قم بحفظ الملف لتشغيله");
//         return;
//     }

// #if defined(Q_OS_WINDOWS)
//     // Windows: Start cmd.exe with /K to keep the window open
//     program = "cmd.exe";
//     command = "alif\\alif.exe";
//     args << "/C" << "start" << program << "/K" << command << arguments;
// #elif defined(Q_OS_LINUX)
//     // Linux: Use x-terminal-emulator with -e to execute the command
//     program = "x-terminal-emulator";
//     command = "./alif/alif";
//     if (!arguments.isEmpty()) {
//         command += " " + arguments.join(" ");
//     }
//     command += "; exec bash";
//     args << "-e" << "bash" << "-c" << command;
// #elif defined(Q_OS_MACOS)
//     // macOS: Use AppleScript to run the command in Terminal.app
//     program = "osascript";
//     command = "./alif/alif";

//     // Escape each part for shell execution
//     QStringList allParts = QStringList() << command << arguments;
//     QStringList escapedShellParts;
//     for (const QString &part : allParts) {
//         QString escaped = part;
//         escaped.replace("'", "'\"'\"'"); // Escape single quotes for AppleScript
//         escapedShellParts << "'" + escaped + "'";
//     }
//     QString shellCommand = escapedShellParts.join(" ");

//     // Escape double quotes for AppleScript
//     QString escapedAppleScriptCommand = shellCommand.replace("\"", "\\\"");

//     // Construct AppleScript
//     QString script = QString(
//                          "tell application \"Terminal\"\n"
//                          "    activate\n"
//                          "    do script \"cd '%1' && %2\"\n"
//                          "end tell"
//                          ).arg(workingDirectory, escapedAppleScriptCommand);

//     args << "-e" << script;
// #endif

//     QProcess* process = new QProcess(this);
//     process->setWorkingDirectory(workingDirectory);

//     process->start(program, args);
// }

//----------------

void Taif::runAlif() {
    TEditor *editor = currentEditor();
    if (!editor) return;

    QString filePath = editor->property("filePath").toString();

    TConsole *console = nullptr;
    for (int i = 0; i < consoleTabWidget->count(); i++) {
        auto *tab = consoleTabWidget->widget(i);
        if (tab->objectName() == "interactiveConsole")
            console = qobject_cast<TConsole*>(tab);
    }

    if (!console) {
        console = new TConsole(this);
        console->setObjectName("interactiveConsole");
        consoleTabWidget->addTab(console, "طرفية ألف");
        console->setLayoutDirection(Qt::RightToLeft);
        console->setConsoleRTL();

    }

    consoleTabWidget->setCurrentWidget(console);
    console->clear();
    consoleTabWidget->setVisible(true);

    QString program = "C:/alif/alif.exe";
    QStringList args = { filePath };
    QString workingDir = QFileInfo(filePath).absolutePath();

    QThread *thread = new QThread();
    ProcessWorker *worker = new ProcessWorker(program, args, workingDir);

    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ProcessWorker::start);
    connect(worker, &ProcessWorker::outputReady,
            console, &TConsole::appendPlainTextThreadSafe);
    connect(worker, &ProcessWorker::errorReady,
            console, &TConsole::appendPlainTextThreadSafe);

    connect(worker, &ProcessWorker::finished, this, [=](int code){
        console->appendPlainTextThreadSafe(
            "──────────────────────────────\n✅ انتهى التنفيذ (Exit code = "
            + QString::number(code) + ")\n"
            );
        thread->quit();
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    connect(console, &TConsole::commandEntered,
            worker, &ProcessWorker::sendInput);

    thread->start();
}

//----------------

TEditor* Taif::currentEditor() {
    return qobject_cast<TEditor*>(tabWidget->currentWidget());
}

void Taif::closeTab(int index)
{

    if (tabWidget->count() <= 1) {
        return;
    }

    QWidget *tab = tabWidget->widget(index);

    if (!tab) return;

    TEditor* editor = qobject_cast<TEditor*>(tabWidget->widget(index));
    if (!editor) return;

    if (editor && editor->document()->isModified()) {
        int saveResult = needSave();

        if (!saveResult) {
            return;
        }
        else if (saveResult == 1) {
            this->saveFile();
            return;
        }

    }
     tabWidget->removeTab(index);

}

/* ----------------------------------- Help Menu Button ----------------------------------- */

void Taif::aboutTaif() {
    QMessageBox::information(nullptr,
                             "عن المحرر"
                             ,R"(
محرر طيف (نـ3) 1445-1446

© الحقوق محفوظة لصالح
برمجيات ألف - عبدالرحمن ومحمد الخطيب

محرر نصي خاص بلغة ألف نـ5
يعمل على جميع المنصات "ويندوز - لينكس - ماك"
ـــــــــــــــــــــــــــــــــــــــــــــــــــــ
المحرر لا يزال تحت التطوير وقد يحتوي بعض الاخطاء
نرجو تبليغ مجتمع ألف في حال وجود أي خطأ
https://t.me/aliflang
ـــــــــــــــــــــــــــــــــــــــــــــــــــــ
فريق التطوير لا يمتلك أي ضمانات وغير مسؤول
عن أي خطأ او خلل قد يحدث بسبب المحرر.

المحرر يخضع لرخصة برمجيات ألف
يجب قراءة الرخصة جيداً قبل البدأ بإستخدام المحرر
                            )");
}

/* ----------------------------------- Other Functions ----------------------------------- */

void Taif::updateWindowTitle() {
    TEditor* editor = currentEditor();
    QString title;

    if (!editor) {
        title = "طيف";
    } else {
       QString filePath = editor->property("filePath").toString();
        // --------------------------------------------------------

        if (filePath.isEmpty()) {
            title = "غير معنون";
        } else {
            title = QFileInfo(filePath).fileName();
        }
        if (editor->document()->isModified()) {
            title += "[*]";
        }
        title += " - طيف";
    }
    setWindowTitle(title);
    setWindowModified(editor && editor->document()->isModified()); // تحديث علامة التعديل للنافذة
}

void Taif::onModificationChanged(bool modified) {
    updateWindowTitle(); // استدعِ الدالة لتحديث علامة [*]
    // قد تحتاج أيضًا لتحديث اسم التبويب نفسه لإضافة [*]
    TEditor* editor = currentEditor(); // الحصول على المحرر المرتبط بالإشارة
    if (editor) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            QString currentText = tabWidget->tabText(index);
            if (modified && !currentText.endsWith("[*]")) {
                tabWidget->setTabText(index, currentText + "[*]");
            } else if (!modified && currentText.endsWith("[*]")) {
                tabWidget->setTabText(index, currentText.left(currentText.length() - 3));
            }
        }
    }
}

