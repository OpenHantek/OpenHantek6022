#include <libusb-1.0/libusb.h>

#include "utils/printutils.h"
#include "selectdevice.h"
#include "usb/uploadFirmware.h"
#include "usb/finddevices.h"
#include "dsomodel.h"
#include <QApplication>
#include <QTimer>
#include <QAbstractTableModel>
#include <QHeaderView>
#include <QFile>

#include "devicelistentry.h"
#include "deviceslistmodel.h"

SelectDevice::SelectDevice() {
    btn = new QPushButton(this);
    w = new QTableView(this);
    label = new QLabel();
    label->setWordWrap(true);
    move(QApplication::desktop()->screen()->rect().center() - w->rect().center());
    setWindowTitle(tr("Select device"));
    setLayout(new QVBoxLayout());
    layout()->addWidget(w);
    layout()->addWidget(label);
    layout()->addWidget(btn);
    qRegisterMetaType<UniqueUSBid>();
    connect(btn, &QPushButton::clicked, [this]() {
        if (w->currentIndex().isValid()) {
            selectedDevice = w->currentIndex().data(Qt::UserRole).value<UniqueUSBid>();
        }
        QCoreApplication::instance()->quit();
    });
}

std::unique_ptr<USBDevice> SelectDevice::showSelectDeviceModal(libusb_context *&context)
{

    std::unique_ptr<FindDevices> findDevices = std::unique_ptr<FindDevices>(new FindDevices(context));
    std::unique_ptr<DevicesListModel> model = std::unique_ptr<DevicesListModel>(new DevicesListModel(findDevices.get()));
    w->setModel(model.get());
    w->verticalHeader()->hide();
    w->horizontalHeader()->hide();
    w->setSelectionBehavior(QAbstractItemView::SelectRows);
    // w->setColumnWidth(1,60);
    w->horizontalHeader()->setStretchLastSection(true);
    connect(w->selectionModel(), &QItemSelectionModel::currentChanged, this, &SelectDevice::currentChanged);

    QTimer timer;
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, [this, &model, &findDevices]() {
        if (findDevices->updateDeviceList())
            model->updateDeviceList();
        if (model->rowCount(QModelIndex())) {
            w->setVisible(true);
            label->setVisible(false);
            currentChanged(w->currentIndex(),QModelIndex());
        } else {
            QString generalMessage = tr("<p>OpenHantek did not find any compatible devices.</p>"
                                        "<p><img align='right' height='150' src='qrc:///switch.png'>"
                                        "Don't forget to switch your device into oscilloscope mode if it has multiple modes.</p>"
                                        );
#if defined(Q_OS_WIN)
            generalMessage = generalMessage.arg("Please make sure you have installed the windows usb driver correctly");
#elif defined(Q_OS_LINUX)
            QFile file("/lib/udev/rules.d/60-hantek.rules");
            if (!file.exists()) {
                generalMessage += tr("<p>Please make sure you have copied the udev rules file to <b>%1</b> for correct USB access permissions.</p>").arg(file.fileName());
            }
#else
#endif
            generalMessage += tr("<p>Visit the build and run instruction "
                              "<a href='https://github.com/OpenHantek/openhantek/blob/master/docs/build.md'>website</a> for help.</p>");
            makeErrorDialog(generalMessage);
        }
        if (!w->currentIndex().isValid()) {
            currentChanged(QModelIndex(),QModelIndex());
        }
    });
    timer.start();
    QCoreApplication::sendEvent(&timer, new QTimerEvent(timer.timerId())); // immediate timer event

    show();
    QCoreApplication::instance()->exec();
    timer.stop();
    close();

    return findDevices->takeDevice(selectedDevice);
}

void SelectDevice::showLibUSBFailedDialogModel(int error)
{
    makeErrorDialog(tr("Can't initalize USB: %1").arg(libUsbErrorString(error)));
    show();
    QCoreApplication::instance()->exec();
    close();
}

void SelectDevice::makeErrorDialog(const QString &message)
{
    w->setVisible(false);
    label->setText(message);
    label->setVisible(true);
}

void SelectDevice::currentChanged(const QModelIndex &current, const QModelIndex &)
{
    if (!current.isValid()) {
        btn->setText(tr("Quit application"));
        btn->setEnabled(true);
        return;
    }
    if (current.data(Qt::UserRole+1).toBool()) {
        btn->setText(tr("Use device %1").arg(current.sibling(current.row(),0).data(Qt::DisplayRole).toString()));
        btn->setEnabled(true);
    } else {
        btn->setEnabled(false);
        if (current.data(Qt::UserRole+2).toBool()) {
            btn->setText(tr("Upload in progress..."));
        } else {
            btn->setText(tr("Connection failed!"));
        }
    }
}
