// SPDX-License-Identifier: GPL-2.0+

/*#if defined(OS_UNIX)
#define CONFIG_PATH QDir::homePath() + "/.config/paranoiacs.net/openhantek"
#define CONFIG_FILE CONFIG_PATH "/openhantek.conf"
#elif defined(OS_DARWIN)
#define CONFIG_PATH QDir::homePath() + "/Library/Application Support/OpenHantek"
#define CONFIG_FILE CONFIG_PATH "/openhantek.plist"
#elif defined(OS_WINDOWS)
//#define CONFIG_PATH QDir::homePath() + "" // Too hard to get and this OS sucks
anyway, ignore it
#define CONFIG_FILE "HKEY_CURRENT_USER\\Software\\paranoiacs.net\\OpenHantek"
#endif*/

#define CONFIG_LIST_WIDTH 128     ///< The width of the page selection widget
#define CONFIG_LIST_ITEMHEIGHT 80 ///< The height of one item in the page selection widget
#define CONFIG_LIST_ICONSIZE 48   ///< The icon size in the page selection widget

#include <QDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "configdialog.h"

#include "DsoConfigAnalysisPage.h"
#include "DsoConfigColorsPage.h"
#include "DsoConfigFilesPage.h"
#include "DsoConfigScopePage.h"

#include "settings.h"

////////////////////////////////////////////////////////////////////////////////
// class DsoConfigDialog
/// \brief Creates the configuration dialog and sets initial values.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
DsoConfigDialog::DsoConfigDialog(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags), settings(settings) {

    this->setWindowTitle(tr("Settings"));

    this->contentsWidget = new QListWidget;
    this->contentsWidget->setViewMode(QListView::IconMode);
    this->contentsWidget->setIconSize(QSize(CONFIG_LIST_ICONSIZE, CONFIG_LIST_ICONSIZE));
    this->contentsWidget->setMovement(QListView::Static);
    this->contentsWidget->setGridSize(
        QSize(CONFIG_LIST_WIDTH - 2 * this->contentsWidget->frameWidth(), CONFIG_LIST_ITEMHEIGHT));
    this->contentsWidget->setMaximumWidth(CONFIG_LIST_WIDTH);
    this->contentsWidget->setMinimumWidth(CONFIG_LIST_WIDTH);
    this->contentsWidget->setMinimumHeight(CONFIG_LIST_ITEMHEIGHT * 3 + 2 * (this->contentsWidget->frameWidth()));

    this->analysisPage = new DsoConfigAnalysisPage(settings);
    this->colorsPage = new DsoConfigColorsPage(settings);
    this->filesPage = new DsoConfigFilesPage(settings);
    this->scopePage = new DsoConfigScopePage(settings);
    this->pagesWidget = new QStackedWidget;
    this->pagesWidget->addWidget(this->analysisPage);
    this->pagesWidget->addWidget(this->colorsPage);
    this->pagesWidget->addWidget(this->filesPage);
    this->pagesWidget->addWidget(this->scopePage);

    this->acceptButton = new QPushButton(tr("&Ok"));
    this->acceptButton->setDefault(true);
    this->applyButton = new QPushButton(tr("&Apply"));
    this->rejectButton = new QPushButton(tr("&Cancel"));

    this->createIcons();
    this->contentsWidget->setCurrentRow(0);

    this->horizontalLayout = new QHBoxLayout;
    this->horizontalLayout->addWidget(this->contentsWidget);
    this->horizontalLayout->addWidget(this->pagesWidget, 1);

    this->buttonsLayout = new QHBoxLayout;
    this->buttonsLayout->setSpacing(8);
    this->buttonsLayout->addStretch(1);
    this->buttonsLayout->addWidget(this->acceptButton);
    this->buttonsLayout->addWidget(this->applyButton);
    this->buttonsLayout->addWidget(this->rejectButton);

    this->mainLayout = new QVBoxLayout;
    this->mainLayout->addLayout(this->horizontalLayout);
    this->mainLayout->addStretch(1);
    this->mainLayout->addSpacing(8);
    this->mainLayout->addLayout(this->buttonsLayout);
    this->setLayout(this->mainLayout);

    connect(this->acceptButton, &QAbstractButton::clicked, this, &DsoConfigDialog::accept);
    connect(this->applyButton, &QAbstractButton::clicked, this, &DsoConfigDialog::apply);
    connect(this->rejectButton, &QAbstractButton::clicked, this, &QDialog::reject);
}

/// \brief Cleans up the dialog.
DsoConfigDialog::~DsoConfigDialog() {}

/// \brief Create the icons for the pages.
void DsoConfigDialog::createIcons() {
    QListWidgetItem *analysisButton = new QListWidgetItem(contentsWidget);
    analysisButton->setIcon(QIcon(":config/analysis.png"));
    analysisButton->setText(tr("Analysis"));

    QListWidgetItem *colorsButton = new QListWidgetItem(contentsWidget);
    colorsButton->setIcon(QIcon(":config/colors.png"));
    colorsButton->setText(tr("Colors"));

    QListWidgetItem *filesButton = new QListWidgetItem(contentsWidget);
    filesButton->setIcon(QIcon(":config/files.png"));
    filesButton->setText(tr("Files"));

    QListWidgetItem *scopeButton = new QListWidgetItem(contentsWidget);
    scopeButton->setIcon(QIcon(":config/scope.png"));
    scopeButton->setText(tr("Scope"));

    connect(contentsWidget, &QListWidget::currentItemChanged, this,
            &DsoConfigDialog::changePage);
}

/// \brief Saves the settings and closes the dialog.
void DsoConfigDialog::accept() {
    this->apply();

    QDialog::accept();
}

/// \brief Saves the settings.
void DsoConfigDialog::apply() {
    this->analysisPage->saveSettings();
    this->colorsPage->saveSettings();
    this->filesPage->saveSettings();
    this->scopePage->saveSettings();
}

/// \brief Change the config page.
/// \param current The page that has been selected.
/// \param previous The page that was selected before.
void DsoConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
    if (!current) current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}
