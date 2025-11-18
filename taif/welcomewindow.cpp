#include "welcomewindow.h"
#include "Taif.h"
#include "TMenu.h"
#include <QtWidgets>


WelcomeWindow::WelcomeWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ===================================================================
    // الجزء الأول: القوائم وشريط الأدوات (لا تغيير هنا)
    // ===================================================================

    TMenuBar *mainMenuBar = new TMenuBar(this);
    this->setMenuBar(mainMenuBar);

    // ===================================================================
    // الجزء الثاني: بناء الواجهة المركزية (مع التعديلات المطلوبة)
    // ===================================================================

    // 1. الإعداد الأساسي
    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout *mainVLayout = new QVBoxLayout(centralWidget);

    // 2. بناء الجزء العلوي (Header)
    QHBoxLayout *headerContent = new QHBoxLayout();
    QLabel *logoLabel = new QLabel();
    logoLabel->setPixmap(QPixmap(":/icons/resources/TaifLogo.ico").scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QVBoxLayout *textVLayout = new QVBoxLayout();
    QLabel *titleLabel = new QLabel("Welcome to Taif");
    QFont titleFont;
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    QLabel *subtitleLabel = new QLabel("Taif - Advanced Text Editor");
    textVLayout->addWidget(titleLabel);
    textVLayout->addWidget(subtitleLabel);
    headerContent->addWidget(logoLabel);
    headerContent->addSpacing(15);
    headerContent->addLayout(textVLayout);
    headerContent->addWidget(logoLabel);
    headerContent->addSpacing(10);
    headerContent->addLayout(textVLayout);

    // 3. بناء الجزء الأوسط (المحتوى الرئيسي)
    QVBoxLayout *mainContentLayout = new QVBoxLayout();
    mainContentLayout->setSpacing(20);
    QSettings settings("Alif", "Taif");
    QStringList recentFiles = settings.value("RecentFiles").toStringList();
    // --- المجموعة الأولى: "Recent Documents and Projects" (ستكون في الأعلى) ---
    QHBoxLayout *filesGroup = new QHBoxLayout();
    QVBoxLayout *filesButtons = new QVBoxLayout();
    newFileButton = new QPushButton("ملف جديد");
    openFileButton = new QPushButton("فتح الملف");
    openFolderButton = new QPushButton("فتح المجلد");
    filesButtons->addWidget(newFileButton);
    filesButtons->addWidget(openFileButton);
    filesButtons->addWidget(openFolderButton);
    filesButtons->addStretch();
    recentFilesList = new QListWidget();
    recentFilesList->addItems(recentFiles);
    recentFilesList->setMinimumWidth(450);
    filesGroup->addLayout(filesButtons);
    filesGroup->addWidget(recentFilesList);

    // --- المجموعة الثانية: "Saved Sessions" (ستكون في الأسفل) ---
    QHBoxLayout *sessionsGroup = new QHBoxLayout();
    QVBoxLayout *sessionsButtons = new QVBoxLayout();
    newSessionButton = new QPushButton("جلسة جديدة");
    manageSessionsButton = new QPushButton("إدارة الجلسات");
    sessionsButtons->addWidget(newSessionButton);
    sessionsButtons->addWidget(manageSessionsButton);
    sessionsButtons->addStretch();
    QLabel *noSessionsLabel = new QLabel("لا يوجد جلسات محفوظة");
    noSessionsLabel->setAlignment(Qt::AlignCenter);
    noSessionsLabel->setMinimumSize(450, 100);
    noSessionsLabel->setObjectName("sessionsBoxLabel");
    sessionsGroup->addLayout(sessionsButtons);
    sessionsGroup->addWidget(noSessionsLabel);

    // ===================================================================
    // ✅ الجزء الجديد: توحيد أحجام المكونات
    // ===================================================================
    int uniformWidth = 450;
    recentFilesList->setFixedWidth(uniformWidth);
    noSessionsLabel->setFixedWidth(uniformWidth);

    // إضافة المجموعتين (فوق بعضهما) إلى تخطيط المحتوى الرئيسي
    mainContentLayout->addLayout(filesGroup);
    mainContentLayout->addLayout(sessionsGroup);

    // 4. بناء الجزء السفلي (Checkbox)
    showOnStartupCheck = new QCheckBox("Show welcome page for new window");
    showOnStartupCheck->setChecked(true);

    // ===================================================================
    // الجزء الثالث: التجميع النهائي والمحاذاة للمنتصف
    // ===================================================================
    mainVLayout->addStretch(1);
    QHBoxLayout *headerCenteringLayout = new QHBoxLayout();
    headerCenteringLayout->addStretch();
    headerCenteringLayout->addLayout(headerContent);
    headerCenteringLayout->addStretch();
    mainVLayout->addLayout(headerCenteringLayout);
    mainVLayout->addSpacing(30);
    QHBoxLayout *contentCenteringLayout = new QHBoxLayout();
    contentCenteringLayout->addStretch();
    contentCenteringLayout->addLayout(mainContentLayout);
    contentCenteringLayout->addStretch();
    mainVLayout->addLayout(contentCenteringLayout);
    mainVLayout->addSpacing(20);
    mainVLayout->addWidget(showOnStartupCheck, 0, Qt::AlignCenter);
    mainVLayout->addStretch(1);

    // ===================================================================
    // الجزء الرابع: تطبيق التصميم QSS (Styling)
    // ===================================================================


    QString styleSheet = R"(
        QWidget {
            font-size: 10pt;     /* ✅ أضف !important هنا */
            background-color: #141520; /* ✅ اللون الجديد للخلفية */
            color: #cccccc;
        }
        QLabel#titleLabel { color: #ffffff; }
        QLabel { color: #909090; }
        QPushButton {
            /* ✅ أضفنا حدًا أدنى للعرض لضمان عدم انضغاط الزر */
            min-width: 70px;
            background-color: #3a3d41; border: 1px solid #555555;
            padding: 4px 12px; border-radius: 4px;
        }
        QPushButton:hover { background-color: #4f5357; }
        QPushButton:pressed { background-color: #2a2d31; }
        QListWidget, QLabel#sessionsBoxLabel {
            background-color: #2d2d2d; border: 1px solid #555555;
            border-radius: 4px; color: #909090;
        }
        TMenuBar{
            background-color: #1e202e;
            spacing: 5px; /* spacing between menu bar items */
        }

        QListWidget::item:hover { background-color: #4f5357; }
    )";
    this->setStyleSheet(styleSheet);

    this->setWindowTitle("Taif Editor");
    this->resize(1024, 768);

    connect(mainMenuBar, &TMenuBar::newRequested, this, &WelcomeWindow::handleNewFileRequest);
    connect(mainMenuBar, &TMenuBar::openFileRequested, this, &WelcomeWindow::handleOpenFileRequest);
    connect(mainMenuBar, &TMenuBar::exitRequested, this, &QWidget::close);

    // ربط أزرار الواجهة المركزية
    connect(newFileButton, &QPushButton::clicked, this, &WelcomeWindow::handleNewFileRequest);
    connect(openFileButton, &QPushButton::clicked, this, &WelcomeWindow::handleOpenFileRequest);
    connect(openFolderButton, &QPushButton::clicked, this, &WelcomeWindow::handleOpenFolderRequest);
    connect(recentFilesList, &QListWidget::itemDoubleClicked, this, &WelcomeWindow::onRecentFileClicked);
}

void WelcomeWindow::onRecentFileClicked(QListWidgetItem *item)
{
    QString filePath = item->text();

    Taif *editor = new Taif(filePath);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->show();
    this->close();
}

void WelcomeWindow::handleNewFileRequest()
{
    Taif *editor = new Taif();
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->show();
    this->close();
}

void WelcomeWindow::handleOpenFileRequest()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open File");

    if (!filePath.isEmpty()) {
        Taif *editor = new Taif(filePath);
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->show();
        this->close();
    }
}

void WelcomeWindow::handleOpenFolderRequest()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Open Folder");

    if (!folderPath.isEmpty()) {
        Taif *editor = new Taif();
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->loadFolder(folderPath);

        editor->show();
        this->close();
    }
}

void WelcomeWindow::closeEvent(QCloseEvent *event)
{
    QGuiApplication::quitOnLastWindowClosed();
    // QApplication::quit();
    // QApplication::quitOnLastWindowClosed();
    QMainWindow::closeEvent(event);

    this->close();
}

WelcomeWindow::~WelcomeWindow()
{
}
