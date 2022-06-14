// SPDX-License-Identifier: GPL-2.0-or-later


#define CONFIG_LIST_WIDTH 128      ///< The width of the page selection widget
#define CONFIG_LIST_ITEMHEIGHT 100 ///< The height of one item in the page selection widget
#define CONFIG_LIST_ICONWIDTH 80   ///< The icon size in the page selection widget
#define CONFIG_LIST_ICONHEIGHT 64  ///< The icon size in the page selection widget

#include <QDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "configdialog.h"

#include "DsoConfigAnalysisPage.h"
#include "DsoConfigColorsPage.h"
#include "DsoConfigScopePage.h"

#include "dsosettings.h"

////////////////////////////////////////////////////////////////////////////////
// class DsoConfigDialog
/// \brief Creates the configuration dialog and sets initial values.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
DsoConfigDialog::DsoConfigDialog( DsoSettings *settings, QWidget *parent ) : QDialog( parent ), settings( settings ) {
    setWindowTitle( tr( "Settings" ) );

    contentsWidget = new QListWidget;
    contentsWidget->setViewMode( QListView::IconMode );
    contentsWidget->setIconSize( QSize( CONFIG_LIST_ICONWIDTH, CONFIG_LIST_ICONHEIGHT ) );
    contentsWidget->setMovement( QListView::Static );
    contentsWidget->setGridSize( QSize( CONFIG_LIST_WIDTH - 2 * contentsWidget->frameWidth(), CONFIG_LIST_ITEMHEIGHT ) );
    contentsWidget->setMaximumWidth( CONFIG_LIST_WIDTH );
    contentsWidget->setMinimumWidth( CONFIG_LIST_WIDTH );
    contentsWidget->setMinimumHeight( CONFIG_LIST_ITEMHEIGHT * 3 + 2 * ( contentsWidget->frameWidth() ) );

    pagesWidget = new QStackedWidget;
    scopePage = new DsoConfigScopePage( settings );
    pagesWidget->addWidget( scopePage );
    analysisPage = new DsoConfigAnalysisPage( settings );
    pagesWidget->addWidget( analysisPage );
    colorsPage = new DsoConfigColorsPage( settings );
    pagesWidget->addWidget( colorsPage );

    acceptButton = new QPushButton( tr( "&Ok" ) );
    acceptButton->setDefault( true );
    applyButton = new QPushButton( tr( "&Apply" ) );
    rejectButton = new QPushButton( tr( "&Cancel" ) );
    rejectShortcut = new QShortcut( QKeySequence( "Ctrl+Q" ), rejectButton );

    createIcons();
    contentsWidget->setCurrentRow( 0 );

    sectionsLayout = new QHBoxLayout;
    sectionsLayout->addWidget( contentsWidget );
    sectionsLayout->addWidget( pagesWidget, 1 );

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing( 8 );
    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( acceptButton );
    buttonsLayout->addWidget( applyButton );
    buttonsLayout->addWidget( rejectButton );

    mainLayout = new QVBoxLayout;
    mainLayout->addSpacing( 8 );
    mainLayout->addLayout( sectionsLayout );
    mainLayout->addStretch( 1 );
    mainLayout->addSpacing( 8 );
    mainLayout->addLayout( buttonsLayout );
    setLayout( mainLayout );

    connect( acceptButton, &QAbstractButton::clicked, this, &DsoConfigDialog::accept );
    connect( applyButton, &QAbstractButton::clicked, this, &DsoConfigDialog::apply );
    connect( rejectButton, &QAbstractButton::clicked, this, &QDialog::reject );
    connect( rejectShortcut, &QShortcut::activated, this, &QDialog::reject );
}


/// \brief Cleans up the dialog.
DsoConfigDialog::~DsoConfigDialog() {}


/// \brief Create the icons for the pages.
void DsoConfigDialog::createIcons() {
    QListWidgetItem *scopeButton = new QListWidgetItem( contentsWidget );
    scopeButton->setIcon( QIcon( ":config/scope.png" ) );
    scopeButton->setText( tr( "Scope" ) );
    if ( settings->scope.toolTipVisible )
        scopeButton->setToolTip( tr( "Timing, display settings, and HW configuration" ) );

    QListWidgetItem *analysisButton = new QListWidgetItem( contentsWidget );
    analysisButton->setIcon( QIcon( ":config/spectrum.png" ) );
    analysisButton->setText( tr( "Analysis" ) );
    if ( settings->scope.toolTipVisible )
        analysisButton->setToolTip( tr( "FFT settings, power and THD calculation, musical note detection" ) );

    QListWidgetItem *colorsButton = new QListWidgetItem( contentsWidget );
    colorsButton->setIcon( QIcon( ":config/colors.png" ) );
    colorsButton->setText( tr( "Colors" ) );
    if ( settings->scope.toolTipVisible )
        colorsButton->setToolTip( tr( "Screen and printer colors, theme and style settings" ) );

    connect( contentsWidget, &QListWidget::currentItemChanged, this, &DsoConfigDialog::changePage );
}


/// \brief Saves the settings and closes the dialog.
void DsoConfigDialog::accept() {
    apply();
    QDialog::accept();
}


/// \brief Saves the settings.
void DsoConfigDialog::apply() {
    scopePage->saveSettings();
    analysisPage->saveSettings();
    colorsPage->saveSettings();
}


/// \brief Change the config page.
/// \param current The page that has been selected.
/// \param previous The page that was selected before.
void DsoConfigDialog::changePage( QListWidgetItem *current, QListWidgetItem *previous ) {
    if ( !current )
        current = previous;
    pagesWidget->setCurrentIndex( contentsWidget->row( current ) );
}
