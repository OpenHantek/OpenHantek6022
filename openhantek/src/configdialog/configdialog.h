// SPDX-License-Identifier: GPL-2.0+

#include <QDialog>

class DsoConfigAnalysisPage;
class DsoConfigColorsPage;
class DsoConfigFilesPage;
class DsoConfigScopePage;
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
  DsoConfigDialog(DsoSettings *settings, QWidget *parent = 0,
                  Qt::WindowFlags flags = 0);
  ~DsoConfigDialog();

public slots:
  void accept();
  void apply();

  void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
  void createIcons();

  DsoSettings *settings;

  QVBoxLayout *mainLayout;
  QHBoxLayout *horizontalLayout;
  QHBoxLayout *buttonsLayout;

  QListWidget *contentsWidget;
  QStackedWidget *pagesWidget;

  DsoConfigAnalysisPage *analysisPage;
  DsoConfigColorsPage *colorsPage;
  DsoConfigFilesPage *filesPage;
  DsoConfigScopePage *scopePage;

  QPushButton *acceptButton, *applyButton, *rejectButton;
};
