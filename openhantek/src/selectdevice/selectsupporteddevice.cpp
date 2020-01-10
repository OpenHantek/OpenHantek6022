// SPDX-License-Identifier: GPL-2.0+

#include "selectsupporteddevice.h"

#include <QTimer>
#include <QDesktopServices>
#include <QFile>
#include <QUrl>

#include "usb/uploadFirmware.h"
#include "usb/finddevices.h"
#include "dsomodel.h"
#include "devicelistentry.h"
#include "deviceslistmodel.h"
#include "newdevicemodelfromexisting.h"
#include "modelregistry.h"

SelectSupportedDevice::SelectSupportedDevice( QWidget *parent ) :
    QDialog( parent ), ui( new Ui::SelectSupportedDevice )
{
    ui->setupUi(this);
#ifdef NEW_DEVICE_FROM_EXISTING_DIALOG
    newDeviceFromExistingDialog = new NewDeviceModelFromExisting(this);
#endif
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    qRegisterMetaType<UniqueUSBid>("UniqueUSBid");
    connect(ui->buttonBox, &QDialogButtonBox::accepted, [this]() {
        if (ui->cmbDevices->currentIndex()!=-1) {
            selectedDevice = ui->cmbDevices->currentData(Qt::UserRole).value<UniqueUSBid>();
        }
        QCoreApplication::instance()->quit();
    });
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, []() {
        QString usrManualPath( "/usr/share/doc/OpenHantek/OpenHantek6022_User_Manual.pdf" );
        QFile userManual( usrManualPath );
        if ( userManual.exists() )
            QDesktopServices::openUrl( QUrl( "file://" + usrManualPath ) );
        else
            QDesktopServices::openUrl( QUrl( "https://github.com/OpenHantek/OpenHantek6022/blob/master/docs/OpenHantek6022_User_Manual.pdf" ) );
    });
#ifdef NEW_DEVICE_FROM_EXISTING_DIALOG
    connect(ui->btnAddDevice, &QPushButton::clicked, [this]() {
        newDeviceFromExistingDialog->setModal(true);
        newDeviceFromExistingDialog->show();
    });
#endif
}

std::unique_ptr<USBDevice> SelectSupportedDevice::showSelectDeviceModal(libusb_context *context)
{
//     newDeviceFromExistingDialog->setUSBcontext(context);
    std::unique_ptr<FindDevices> findDevices = std::unique_ptr<FindDevices>(new FindDevices(context));
    std::unique_ptr<DevicesListModel> model = std::unique_ptr<DevicesListModel>(new DevicesListModel(findDevices.get()));
    ui->cmbDevices->setModel(model.get());
    connect(ui->cmbDevices, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {
        if (index == -1) {
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            return;
        }
        if (ui->cmbDevices->currentData( Qt::UserRole + 1 ).toBool()) { // canConnect
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            ui->labelReadyState->setText(
                tr("<p><br/><b>The device is ready for use.</b></p><p>Please observe the "
                   "<a href='https://github.com/OpenHantek/OpenHantek6022/blob/master/docs/OpenHantek6022_User_Manual.pdf'>"
                   "user manual</a> for safe operation.</p>"));
        } else {
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            if (ui->cmbDevices->currentData( Qt::UserRole + 2 ).toBool()) { // needFirmware
                ui->labelReadyState->setText(tr("<p>Upload in progress ...</p>"
                "<p><b>If the upload takes more than 30 s, please close this window <br/>and restart the program!</b></p>"
                ));
            } else { // something went wrong, inform user
                ui->labelReadyState->setText( tr( "<p><br/><b>Connection failed!</b></p>" )
                    + ui->cmbDevices->currentData( Qt::UserRole + 3 ).toString()
                );
            }
        }
    });

    QString messageNoDevices = tr("<p>OpenHantek6022 is searching for compatible devices ...</p>"
                                "<p><img align='right' height='200' src='qrc:///switch_6022BL.png'>"
                                "Don't forget to switch your device into oscilloscope mode if it has multiple modes.</p>"
                                );
    #if defined(Q_OS_WIN)
        messageNoDevices += tr("<p>Please make sure you have installed the windows usb driver correctly</p>");
    #elif defined(Q_OS_LINUX)
        QFile libRules("/lib/udev/rules.d/60-hantek.rules");
        QFile etcRules("/etc/udev/rules.d/60-hantek.rules");
        if ( !libRules.exists() && !etcRules.exists() ) {
            messageNoDevices += tr("<p>Please make sure you have copied the udev rules file to <b>%1</b> for correct USB access permissions.</p>").arg(libRules.fileName());
        }
    #endif
        messageNoDevices += tr("<p>Visit the build and run instruction "
                          "<a href='https://github.com/OpenHantek/OpenHantek6022/blob/master/docs/build.md'>website</a> for help.</p>");

    updateSupportedDevices();

    QTimer timer;
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, [this, &model, &findDevices, &messageNoDevices]() {
        if ( findDevices->updateDeviceList() ) { // searching...
            model->updateDeviceList();
        }
        if ( model->rowCount( QModelIndex() ) ) { // device ready
            ui->cmbDevices->setCurrentIndex( 0 );
            // HACK: "click()" the "OK" button (if enabled) to start the scope automatically
            if ( ui->buttonBox->button( QDialogButtonBox::Ok )->isEnabled() ) { // if scope is ready to run
                ui->buttonBox->button( QDialogButtonBox::Ok )->click(); // start it without user activity
            }
        } else {
            ui->labelReadyState->setText(messageNoDevices);
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

void SelectSupportedDevice::showLibUSBFailedDialogModel(int error)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
#ifdef NEW_DEVICE_FROM_EXISTING_DIALOG
    ui->btnAddDevice->setEnabled(false);
#endif
    ui->labelReadyState->setText(tr("Can't initalize USB: %1").arg(libUsbErrorString(error)));
    show();
    QCoreApplication::instance()->exec();
    close();
}

void SelectSupportedDevice::updateSupportedDevices()
{
    QString devices;
    for (const DSOModel* model: ModelRegistry::get()->models()) {
        devices.append(QString::fromStdString(model->name)).append(" ");
    }
    ui->labelSupportedDevices->setText(devices);
}
