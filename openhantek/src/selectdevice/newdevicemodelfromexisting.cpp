#include "newdevicemodelfromexisting.h"

#include "dsomodel.h"
#include "modelregistry.h"
#include "rawdeviceslistmodel.h"

#include <QDebug>
#include <QPushButton>
#include <QStringListModel>
#include <QMessageBox>

NewDeviceModelFromExisting::NewDeviceModelFromExisting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewDeviceModelFromExisting)
{
    ui->setupUi(this);
    connect(ui->checkBox, &QCheckBox::stateChanged,[this](int state) {
       ui->stackedWidget->setCurrentIndex(state==Qt::Checked ? 0: 1);
    });
    ui->checkBox->setCheckState(Qt::Checked);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QStringList supportedModelsList;
    for (const DSOModel* model: ModelRegistry::get()->models()) {
        supportedModelsList.append(QString::fromStdString(model->name));
    }

    QStringListModel* model = new QStringListModel(this);
    model->setStringList(supportedModelsList);
    ui->cmbTemplateModel->setModel(model);

    RawDevicesListModel* deviceListModel = new RawDevicesListModel(context, this);
    ui->cmbUSBdevices->setModel(deviceListModel);
    deviceListModel->updateDeviceList();

    connect(ui->btnRefresh, &QPushButton::clicked, [this,deviceListModel] {
        deviceListModel->updateDeviceList();
    });

    connect(ui->cmbUSBdevices, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {
        if (index == -1) {
            ui->stackedWidget->setCurrentIndex(2);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            ui->checkBox->setEnabled(false);
            return;
        }
        if (ui->cmbUSBdevices->currentData(RawDevicesListModel::AccessRole).toBool()) {
            ui->checkBox->setEnabled(true);
            ui->modelname->setText(ui->cmbUSBdevices->currentData(RawDevicesListModel::DeviceNameRole).toString());
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui->modelname->text().size());
            ui->stackedWidget->setCurrentIndex(ui->checkBox->isChecked() ? 0: 1);
        } else {
            ui->checkBox->setEnabled(false);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            ui->stackedWidget->setCurrentIndex(3);
        }
    });

    if (deviceListModel->rowCount(QModelIndex())) {
        ui->cmbUSBdevices->setCurrentIndex(0);
    }
}

void NewDeviceModelFromExisting::setUSBcontext(libusb_context *context)
{
    this->context = context;
}

RawDeviceListEntry *NewDeviceModelFromExisting::getSelectedEntry()
{
    return (RawDeviceListEntry*) ui->cmbUSBdevices->currentData(RawDevicesListModel::EntryPointerRole).value<void*>();
}

void NewDeviceModelFromExisting::accept()
{
    QMessageBox::information(this,tr("Sorry"),tr("This is not yet implemented!"));
    QDialog::accept();
}
