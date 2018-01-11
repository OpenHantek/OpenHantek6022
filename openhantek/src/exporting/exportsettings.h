// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSize>

/// \brief Holds the general options of the program.
struct DsoSettingsExport {
    QSize imageSize = QSize(640, 480); ///< Size of exported images in pixels
};

/// \brief Possible file formats for the export.
enum ExportFormat { EXPORT_FORMAT_PRINTER, EXPORT_FORMAT_PDF, EXPORT_FORMAT_IMAGE, EXPORT_FORMAT_CSV };
