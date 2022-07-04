#include "XojPdfExport.h"

XojPdfExport::XojPdfExport() = default;

XojPdfExport::~XojPdfExport() = default;

/**
 * Export without background
 */
void XojPdfExport::setExportBackground(ExportBackgroundType exportBackground) {
    // Does nothing in the base class
}

/**
 * Crop export to the drawing content
 */
void XojPdfExport::setCropToContent(bool cropToContent) {
    // Does nothing in the base class
}
