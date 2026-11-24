// SPDX-License-Identifier: GPL-3.0-or-later
#include "PdfItemDelegate.h"

#include "PdfListWidget.h"

#include <QApplication>
#include <QFontMetrics>
#include <QListView>
#include <QPainter>
#include <QStyle>

namespace {
constexpr int kRowPadding = 8;
constexpr int kMinRowHeight = 36;
constexpr int kMargin = 8;
constexpr int kGap = 10;
constexpr int kPagesWidth = 92;
constexpr qreal kNameShare = 0.42;

bool isIconMode(const QStyleOptionViewItem &option)
{
    const auto *view = qobject_cast<const QListView *>(option.widget);
    return view && view->viewMode() == QListView::IconMode;
}

/* The details row follows the icon size the view is configured with, so the
 * thumbnail slider works in both display modes. */
QSize detailsIconSize(const QStyleOptionViewItem &option)
{
    const auto *view = qobject_cast<const QListView *>(option.widget);
    if (view && view->iconSize().isValid()) {
        return view->iconSize();
    }
    return QSize(32, 42);
}
} // namespace

PdfItemDelegate::PdfItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize PdfItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (isIconMode(option)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }
    const int height = qMax(kMinRowHeight, detailsIconSize(option).height() + kRowPadding);
    return QSize(320, height);
}

void PdfItemDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    if (isIconMode(option)) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    painter->save();
    painter->setClipRect(opt.rect);

    const QPalette &palette = opt.palette;
    const bool selected = opt.state & QStyle::State_Selected;
    const bool hovered = opt.state & QStyle::State_MouseOver;

    if (selected) {
        painter->fillRect(opt.rect, palette.brush(QPalette::Highlight));
    } else if (hovered) {
        QColor tint = palette.color(QPalette::Highlight);
        tint.setAlpha(28);
        painter->fillRect(opt.rect, tint);
    }

    const QColor textColor = selected ? palette.color(QPalette::HighlightedText)
                                      : palette.color(QPalette::Text);
    QColor pathColor = selected ? palette.color(QPalette::HighlightedText)
                                : palette.color(QPalette::Disabled, QPalette::Text);
    pathColor.setAlpha(selected ? 200 : 255);

    const QSize iconSize = detailsIconSize(option);
    const int left = opt.rect.left() + kMargin;
    const QRect iconRect(left,
                         opt.rect.top() + (opt.rect.height() - iconSize.height()) / 2,
                         iconSize.width(), iconSize.height());

    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        icon.paint(painter, iconRect, Qt::AlignCenter,
                   selected ? QIcon::Selected : QIcon::Normal);
    }

    const QFontMetrics metrics(opt.font);
    int textLeft = iconRect.right() + kGap;
    int textRight = opt.rect.right() - kMargin;

    const QString pages = index.data(PdfListWidget::PagesTextRole).toString();
    int pagesWidth = 0;
    if (!pages.isEmpty()) {
        pagesWidth = qMin(kPagesWidth, metrics.horizontalAdvance(pages) + kGap);
        textRight -= pagesWidth;
        painter->setPen(textColor);
        painter->setFont(opt.font);
        painter->drawText(QRect(textRight, opt.rect.top(), pagesWidth - kGap,
                                opt.rect.height()),
                          Qt::AlignRight | Qt::AlignVCenter, pages);
    }

    const QString displayPath = index.data(PdfListWidget::DisplayPathRole).toString();
    int pathWidth = 0;
    if (!displayPath.isEmpty()) {
        const int available = qMax(0, textRight - textLeft);
        pathWidth = qMin(static_cast<int>(available * (1.0 - kNameShare)), available / 2);
        textRight -= pathWidth + kGap;
    }

    painter->setPen(textColor);
    painter->setFont(opt.font);
    const QString name = index.data(Qt::DisplayRole).toString();
    const QRect nameRect(textLeft, opt.rect.top(), qMax(0, textRight - textLeft),
                         opt.rect.height());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                      metrics.elidedText(name, Qt::ElideRight, nameRect.width()));

    if (pathWidth > 0) {
        painter->setPen(pathColor);
        const QRect pathRect(textRight + kGap, opt.rect.top(), pathWidth,
                             opt.rect.height());
        const QFontMetrics pathMetrics(opt.font);
        painter->drawText(pathRect, Qt::AlignLeft | Qt::AlignVCenter,
                          pathMetrics.elidedText(displayPath, Qt::ElideMiddle,
                                                 pathRect.width()));
    }

    painter->restore();
}
