#include "ExportHelper.h"

#include <algorithm>  // for max
#include <memory>     // for unique_ptr, allocator
#include <string>     // for string

#include <gio/gio.h>      // for g_file_new_for_commandlin...
#include <glib-object.h>  // for g_object_unref
#include <glib.h>         // for g_message, g_error

#include "control/jobs/ImageExport.h"       // for ImageExport, EXPORT_GRAPH...
#include "control/jobs/ProgressListener.h"  // for DummyProgressListener
#include "model/Document.h"                 // for Document
#include "pdf/base/XojPdfExport.h"          // for XojPdfExport
#include "util/ElementRange.h"              // for parse, PageRangeVector
#include "util/i18n.h"                      // for _

#include "filesystem.h"  // for operator==, path, u8path

namespace ExportHelper {

/**
 * @brief Export the input file as a bunch of image files (one per page)
 * @param input Path to the input file
 * @param output Path to the output file(s)
 * @param range Page range to be parsed. If range=nullptr, exports the whole file
 * @param pngDpi Set dpi for Png files. Non positive values are ignored
 * @param pngWidth Set the width for Png files. Non positive values are ignored
 * @param pngHeight Set the height for Png files. Non positive values are ignored
 * @param exportBackground If EXPORT_BACKGROUND_NONE, the exported image file has transparent background
 *
 *  The priority is: pngDpi overwrites pngWidth overwrites pngHeight
 *
 * @return 0 on success, -3 on export failure
 */
auto exportImg(Document* doc, const char* output, const char* range, const char* layerRange, int pngDpi, int pngWidth,
               int pngHeight, ExportBackgroundType exportBackground) -> int {

    fs::path const path(output);

    ExportGraphicsFormat format = EXPORT_GRAPHICS_PNG;

    if (path.extension() == ".svg") {
        format = EXPORT_GRAPHICS_SVG;
    }

    PageRangeVector exportRange;
    if (range) {
        exportRange = ElementRange::parse(range, doc->getPageCount());
    } else {
        exportRange.emplace_back(0, doc->getPageCount() - 1);
    }

    DummyProgressListener progress;

    ImageExport imgExport(doc, path, format, exportBackground, exportRange, &progress);

    if (format == EXPORT_GRAPHICS_PNG) {
        if (pngDpi > 0) {
            imgExport.setQualityParameter(EXPORT_QUALITY_DPI, pngDpi);
        } else if (pngWidth > 0) {
            imgExport.setQualityParameter(EXPORT_QUALITY_WIDTH, pngWidth);
        } else if (pngHeight > 0) {
            imgExport.setQualityParameter(EXPORT_QUALITY_HEIGHT, pngHeight);
        }
    }

    imgExport.setLayerRange(layerRange);

    imgExport.exportGraphics();

    std::string errorMsg = imgExport.getLastErrorMsg();
    if (!errorMsg.empty()) {
        g_message("Error exporting image: %s\n", errorMsg.c_str());
    }

    g_message("%s", _("Image file successfully created"));

    return 0;  // no error
}

/**
 * @brief Export the input file as pdf
 * @param input Path to the input file
 * @param output Path to the output file
 * @param range Page range to be parsed. If range=nullptr, exports the whole file
 * @param exportBackground If EXPORT_BACKGROUND_NONE, the exported pdf file has white background
 * @param progressiveMode If true, then for each xournalpp page, instead of rendering one PDF page, the page layers are
 * rendered one by one to produce as many pages as there are layers.
 *
 * @return 0 on success, -3 on export failure
 */
auto exportPdf(Document* doc, const char* output, const char* range, const char* layerRange,
               ExportBackgroundType exportBackground, bool progressiveMode) -> int {

    GFile* file = g_file_new_for_commandline_arg(output);

    auto path = fs::u8path(g_file_peek_path(file));
    g_object_unref(file);

    PageRangeVector exportRange = ElementRange::parse("1-" + std::to_string(doc->getPageCount()), doc->getPageCount());
    if (range) {
        // Parse the range
        exportRange = ElementRange::parse(range, doc->getPageCount());
    }
    XojPdfExport pdfe{doc, exportBackground, nullptr, path, exportRange};
    pdfe.setLayerRange(layerRange);

    // Do the export
    bool exportSuccess = false;  // Return of the export job
    exportSuccess = pdfe.createPdf(exportRange, progressiveMode);

    if (!exportSuccess) {
        g_error("%s", pdfe.getLastErrorMsg().c_str());
    }

    g_message("%s", _("PDF file successfully created"));

    return 0;  // no error
}

}  // namespace ExportHelper
