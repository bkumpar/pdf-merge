// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QStyledItemDelegate>

/* Paints the document rows.
 *
 * In icon mode the base class draws the thumbnail with the label underneath.
 * In details mode this delegate draws a compact row: small thumbnail, file
 * name, page count and the document path. The path comes from the item's
 * display-path role, so the list decides whether it is normalised or full. */
class PdfItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PdfItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};
