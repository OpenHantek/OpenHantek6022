// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QString>

class USBDevice;

/**
 * Extracts the firmware from the applications resources, and uploads the
 * firmware to the given device.
 */
class UploadFirmware {
  public:
    bool startUpload(USBDevice *device);
    const QString &getErrorMessage() const;

  private:
    QString errorMessage;
};
