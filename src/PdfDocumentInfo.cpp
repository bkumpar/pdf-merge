// SPDX-License-Identifier: GPL-3.0-or-later
#include "PdfDocumentInfo.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPdfDocument>
#include <QRegularExpression>

namespace {

constexpr int kFrameMargin = 1;
constexpr int kBadgeHeight = 18;
constexpr qint64 kMaxScanBytes = 64LL * 1024 * 1024;

QString describeError(QPdfDocument::Error error)
{
    // Enum members differ between Qt releases, so only the portable ones are
    // named here and everything else falls back to a generic message.
    switch (error) {
    case QPdfDocument::Error::None:
        return QString();
    case QPdfDocument::Error::FileNotFound:
        return QCoreApplication::translate("PdfTools", "File not found.");
    case QPdfDocument::Error::InvalidFileFormat:
        return QCoreApplication::translate("PdfTools", "The file is not a valid PDF document.");
    case QPdfDocument::Error::IncorrectPassword:
        return QCoreApplication::translate("PdfTools", "The document is password protected.");
    case QPdfDocument::Error::UnsupportedSecurityScheme:
        return QCoreApplication::translate("PdfTools",
                                           "The document uses an unsupported security scheme.");
    default:
        break;
    }
    return QCoreApplication::translate("PdfTools", "The document could not be read.");
}

} // namespace

bool PdfTools::isPdfFile(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        return false;
    }
    return info.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0;
}

bool PdfTools::hasSignatureMarkers(const QByteArray &raw)
{
    // A signed PDF always carries a signature dictionary with /ByteRange.
    // The dictionary itself is never compressed, so a byte scan is enough.
    if (!raw.contains("/ByteRange")) {
        return false;
    }
    const QString text = QString::fromLatin1(raw);
    static const QRegularExpression typeSig(QStringLiteral("/Type\\s*/Sig"));
    static const QRegularExpression filterSig(QStringLiteral("/FT\\s*/Sig"));
    return typeSig.match(text).hasMatch() || filterSig.match(text).hasMatch();
}

PdfDocumentInfo PdfTools::inspect(const QString &filePath)
{
    PdfDocumentInfo info;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray raw = file.read(kMaxScanBytes);
        file.close();
        info.digitallySigned = hasSignatureMarkers(raw);
    }

    QPdfDocument document;
    const QPdfDocument::Error error = document.load(filePath);
    if (error != QPdfDocument::Error::None) {
        info.loadFailed = true;
        info.errorString = describeError(error);
        return info;
    }
    info.pageCount = document.pageCount();
    return info;
}

QPixmap PdfTools::thumbnail(const QString &filePath, const QSize &targetSize,
                            qreal devicePixelRatio, const QString &badgeText)
{
    const qreal ratio = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    const QSize pixelSize(qRound(targetSize.width() * ratio),
                          qRound(targetSize.height() * ratio));

    QPixmap canvas(pixelSize);
    canvas.setDevicePixelRatio(ratio);
    canvas.fill(Qt::white);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPdfDocument document;
    const bool loaded = document.load(filePath) == QPdfDocument::Error::None;
    const int pageCount = loaded ? document.pageCount() : 0;

    if (pageCount > 0) {
        const QSizeF pointSize = document.pagePointSize(0);
        if (pointSize.isValid() && !pointSize.isEmpty()) {
            const qreal availableWidth = pixelSize.width() - 2 * kFrameMargin;
            const qreal availableHeight = pixelSize.height() - 2 * kFrameMargin;
            const qreal scale = qMin(availableWidth / pointSize.width(),
                                     availableHeight / pointSize.height());
            const QSize renderSize(qMax(1, qRound(pointSize.width() * scale)),
                                   qMax(1, qRound(pointSize.height() * scale)));

            const QImage image = document.render(0, renderSize);
            if (!image.isNull()) {
                const int x = (pixelSize.width() - image.width()) / 2;
                const int y = (pixelSize.height() - image.height()) / 2;
                painter.drawImage(QPoint(x, y), image);
            }
        }
    } else {
        // Placeholder so the item still shows up while the file is unreadable.
        painter.fillRect(QRect(QPoint(0, 0), pixelSize), QColor(0xF2, 0xF2, 0xF2));
        painter.setPen(QColor(0x88, 0x88, 0x88));
        QFont placeholderFont = painter.font();
        placeholderFont.setPointSizeF(placeholderFont.pointSizeF() * ratio);
        painter.setFont(placeholderFont);
        painter.drawText(QRect(QPoint(0, 0), pixelSize), Qt::AlignCenter,
                         QStringLiteral("PDF"));
    }

    // Thin grey frame: keeps white pages visible on a light background.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0xAA, 0xAA, 0xAA), 1));
    painter.drawRect(QRect(QPoint(0, 0), pixelSize).adjusted(0, 0, -1, -1));

    if (!badgeText.isEmpty()) {
        const int bandHeight = qRound(kBadgeHeight * ratio);
        const QRect band(0, pixelSize.height() - bandHeight, pixelSize.width(),
                         bandHeight);
        painter.fillRect(band, QColor(0xC6, 0x28, 0x28));

        QFont badgeFont = painter.font();
        badgeFont.setBold(true);
        badgeFont.setPixelSize(qRound(11 * ratio));
        painter.setFont(badgeFont);
        painter.setPen(Qt::white);
        painter.drawText(band, Qt::AlignCenter, badgeText);
    }

    painter.end();
    return canvas;
}
