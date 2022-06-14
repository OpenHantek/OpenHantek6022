// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>

class DsoConfigAnalysisPage;
class DsoConfigScopePage;
class DsoConfigColorsPage;
class DsoSettings;

class QHBoxLayout;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QShortcut;
class QStackedWidget;
class QVBoxLayout;

////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigDialog                                        configdialog.h
/// \brief The dialog for the configuration options.
class DsoConfigDialog : public QDialog {
    Q_OBJECT

  public:
    DsoConfigDialog( DsoSettings *settings, QWidget *parent = nullptr );
    ~DsoConfigDialog() override;

  public slots:
    void accept() override;
    void apply();

    void changePage( QListWidgetItem *current, QListWidgetItem *previous );

  private:
    void createIcons();

    DsoSettings *settings;

    QVBoxLayout *mainLayout;
    QHBoxLayout *sectionsLayout;
    QHBoxLayout *buttonsLayout;

    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;

    DsoConfigScopePage *scopePage;
    DsoConfigAnalysisPage *analysisPage;
    DsoConfigColorsPage *colorsPage;

    QPushButton *acceptButton, *applyButton, *rejectButton;
    QShortcut *rejectShortcut;
};
