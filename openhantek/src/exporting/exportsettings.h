// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSize>

/// \brief Holds the export options of the program.
struct DsoSettingsExport {
    QSize imageSize = QSize( 800, 600 );         ///< Size of exported images in pixels
    bool screenshotDisplayOnly = false;          ///< Take a screenshot of the display area only
    unsigned exportSizeBytes = 1024 * 1024 * 10; ///< For exporters that save a continous stream. Default: 10 Megabytes
    bool useProcessedSamples = true;             ///< Export raw or processed samples
};
