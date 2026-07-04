// SPDX-License-Identifier: GPL-2.0-or-later

#include "scopebutton.h"

#include <QButtonGroup>
#include <QHBoxLayout>

ScopeButton::ScopeButton( const QString &text, QWidget *parent ) : QPushButton( text, parent ) {
    setCheckable( true );
    setFocusPolicy( Qt::TabFocus ); // do not grab <space>, it is the run/stop shortcut
    updateStyle();
}


void ScopeButton::setAccentColor( const QColor &color ) {
    accent = color;
    updateStyle();
}


QSize ScopeButton::minimumSizeHint() const { return sizeHint(); }


void ScopeButton::updateStyle() {
    // embossed front panel key on the fixed dark instrument panel;
    // the checked key lights up in the accent (channel) color
    const QString checkedColor = accent.isValid() ? accent.name() : "#3d7ae0";
    const QString checkedText = accent.isValid() && accent.lightness() > 128 ? "black" : "white";
    setStyleSheet(
        QString( "QPushButton { color: #d8dade; border: 1px solid #15171a; border-bottom: 2px solid #101214;"
                 " border-radius: 0.3em; padding: 0.15em 0.45em; min-height: 1.8em;"
                 " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4b5157, stop:0.08 #3e4349, stop:1 #2a2d32); }"
                 "QPushButton:hover { border-color: %1; }"
                 "QPushButton:pressed { background: #202328; border-bottom-width: 1px; }"
                 "QPushButton:checked { background: %1; color: %2; border-color: %1; font-weight: bold; }"
                 "QPushButton:disabled { color: #63676c; }"
                 // a disabled but checked key stays lit, e.g. fixed DC coupling without the AC hardware mod
                 "QPushButton:checked:disabled { background: %1; color: %2; border-color: %1; }" )
            .arg( checkedColor, checkedText ) );
}


ScopeCycleButton::ScopeCycleButton( QWidget *parent ) : ScopeButton( QString(), parent ) {
    setCheckable( false );
    connect( this, &QPushButton::clicked, this, [ this ]() {
        if ( items.isEmpty() )
            return;
        index = ( index + 1 ) % items.size();
        setText( items[ index ] );
        emit currentIndexChanged( index );
    } );
}


void ScopeCycleButton::addItems( const QStringList &texts ) {
    items << texts;
    if ( !items.isEmpty() )
        setText( items[ index ] );
}


int ScopeCycleButton::currentIndex() const { return index; }


void ScopeCycleButton::setCurrentIndex( int newIndex ) {
    if ( newIndex < 0 || newIndex >= items.size() )
        return;
    index = newIndex;
    setText( items[ index ] ); // does not emit currentIndexChanged
}


ScopeButtonGroup::ScopeButtonGroup( QWidget *parent ) : QWidget( parent ), group( new QButtonGroup( this ) ) {
    layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 2 );
    group->setExclusive( true );
    connect( group, &QButtonGroup::idClicked, this, &ScopeButtonGroup::currentIndexChanged );
}


void ScopeButtonGroup::addItems( const QStringList &texts ) {
    for ( const QString &text : texts )
        addItem( text );
}


void ScopeButtonGroup::addItem( const QString &text, const QColor &accent ) {
    ScopeButton *button = new ScopeButton( text, this );
    if ( accent.isValid() )
        button->setAccentColor( accent );
    group->addButton( button, items++ );
    layout->addWidget( button );
}


int ScopeButtonGroup::currentIndex() const { return group->checkedId(); }


void ScopeButtonGroup::setCurrentIndex( int index ) {
    if ( QAbstractButton *button = group->button( index ) )
        button->setChecked( true ); // QButtonGroup::idClicked is not emitted for programmatic changes
}


void ScopeButtonGroup::setItemEnabled( int index, bool enabled ) {
    if ( QAbstractButton *button = group->button( index ) )
        button->setEnabled( enabled );
}


int ScopeButtonGroup::count() const { return items; }
