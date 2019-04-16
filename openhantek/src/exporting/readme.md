# Content
This directory contains exporting functionality and exporters, namely

* Export to comma separated value file (CSV): Write to a user selected file, 
use localisation for data and decimal separator
* Export to an image/pdf: Writes an image/pdf to a user selected file,
* Print exporter: Creates a printable document and opens the print dialog.

All export classes (exportcsv, exportimage, exportprint) implement the
ExporterInterface and are registered to the ExporterRegistry in the main.cpp.

Some export classes are still using the legacyExportDrawer class to
draw the grid and paint all the labels, values and graphs.

# Dependency
* Files in this directory depend on the result class of the post processing directory.
* Classes in here depend on the user settings (../viewsetting.h, ../scopesetting.h)
