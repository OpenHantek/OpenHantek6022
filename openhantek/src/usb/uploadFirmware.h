// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

class ScopeDevice;

/**
 * Extracts the firmware from the applications resources, and uploads the
 * firmware to the given device.
 */
class UploadFirmware {
  public:
    bool startUpload( ScopeDevice *scopeDevice );
    const QString &getErrorMessage() const;

  private:
    QString errorMessage;
};
