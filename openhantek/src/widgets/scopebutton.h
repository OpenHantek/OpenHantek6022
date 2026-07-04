// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

class QButtonGroup;
class QHBoxLayout;

/// \brief A checkable panel key like on a HW scope front panel.
/// When an accent color is set the checked button lights up in this color
/// (e.g. the yellow / blue channel keys of a typical two channel scope).
class ScopeButton : public QPushButton {
    Q_OBJECT

  public:
    explicit ScopeButton( const QString &text, QWidget *parent = nullptr );
    void setAccentColor( const QColor &color );
    QSize minimumSizeHint() const override; ///< never squeeze the key label

  private:
    void updateStyle();
    QColor accent;
};

/// \brief A panel key that cycles through a small list of values on each click
/// (like the acquisition mode keys of a HW scope), replaces a small QComboBox.
class ScopeCycleButton : public ScopeButton {
    Q_OBJECT

  public:
    explicit ScopeCycleButton( QWidget *parent = nullptr );
    void addItems( const QStringList &items );
    int currentIndex() const;
    void setCurrentIndex( int index ); ///< does not emit currentIndexChanged

  signals:
    void currentIndexChanged( int index ); ///< emitted on user interaction only

  private:
    QStringList items;
    int index = 0;
};

/// \brief A row of mutually exclusive panel keys, drop-in replacement for a small QComboBox.
class ScopeButtonGroup : public QWidget {
    Q_OBJECT

  public:
    explicit ScopeButtonGroup( QWidget *parent = nullptr );
    void addItems( const QStringList &items );
    void addItem( const QString &item, const QColor &accent = QColor() );
    int currentIndex() const;
    void setCurrentIndex( int index ); ///< does not emit currentIndexChanged
    void setItemEnabled( int index, bool enabled );
    int count() const;

  signals:
    void currentIndexChanged( int index ); ///< emitted on user interaction only

  private:
    QButtonGroup *group;
    QHBoxLayout *layout;
    int items = 0;
};
