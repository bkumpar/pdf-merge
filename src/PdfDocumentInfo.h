// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QPixmap>
#include <QSize>
#include <QString>

/* Facts about one PDF file that the list needs to show. */
struct PdfDocumentInfo
{
    int pageCount = 0;
    bool digitallySigned = false;
    bool loadFailed = false;
    QString errorString;

    bool isValid() const { return !loadFailed && pageCount > 0; }
};

class PdfTools
{
public:
    /* Reads page count and looks for a digital signature. */
    static PdfDocumentInfo inspect(const QString &filePath);

    /* Renders page 0 scaled into targetSize (logical pixels), draws a thin
     * grey frame and, when requested, a red "digitally signed" band. */
    static QPixmap thumbnail(const QString &filePath, const QSize &targetSize,
                             qreal devicePixelRatio, const QString &badgeText);

    static bool isPdfFile(const QString &filePath);

private:
    static bool hasSignatureMarkers(const QByteArray &raw);
};
