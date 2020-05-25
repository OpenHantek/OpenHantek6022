// SPDX-License-Identifier: GPL-2.0+

#include <QDialog>

class DsoConfigSpectrumPage;
class DsoConfigScopePage;
class DsoConfigColorsPage;
class DsoSettings;

class QHBoxLayout;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigDialog                                        configdialog.h
/// \brief The dialog for the configuration options.
class DsoConfigDialog : public QDialog {
    Q_OBJECT

  public:
    DsoConfigDialog( DsoSettings *settings, QWidget *parent = nullptr, Qt::WindowFlags flags = nullptr );
    ~DsoConfigDialog();

  public slots:
    void accept();
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
    DsoConfigSpectrumPage *spectrumPage;
    DsoConfigColorsPage *colorsPage;

    QPushButton *acceptButton, *applyButton, *rejectButton;
};
