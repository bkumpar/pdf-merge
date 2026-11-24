// SPDX-License-Identifier: GPL-3.0-or-later
#include "PdfListWidget.h"

#include "PdfDocumentInfo.h"

#include <QContextMenuEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QUrl>

namespace {
constexpr int kThumbWidth = 150;
constexpr int kThumbHeight = 196;
} // namespace

PdfListWidget::PdfListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_thumbnailSize(kThumbWidth, kThumbHeight)
{
    setViewMode(QListView::IconMode);
    setIconSize(m_thumbnailSize);
    setGridSize(QSize(kThumbWidth + 24, kThumbHeight + 46));
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setWordWrap(true);
    setTextElideMode(Qt::ElideMiddle);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setUniformItemSizes(true);
    setSpacing(6);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setToolTip(tr("Drag PDF files here or use \"Add PDFs...\""));
}

QStringList PdfListWidget::pdfPathsFromUrls(const QList<QUrl> &urls)
{
    QStringList result;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString path = url.toLocalFile();
        if (PdfTools::isPdfFile(path)) {
            result.append(path);
        }
    }
    return result;
}

int PdfListWidget::addFiles(const QStringList &paths, QString *firstDirectory)
{
    QStringList existing;
    existing.reserve(count());
    for (int row = 0; row < count(); ++row) {
        existing.append(item(row)->data(PathRole).toString());
    }

    int added = 0;
    QString firstDir;
    for (const QString &path : paths) {
        if (!PdfTools::isPdfFile(path)) {
            continue;
        }
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (existing.contains(absolute)) {
            continue;
        }
        appendItem(absolute);
        existing.append(absolute);
        if (firstDir.isEmpty()) {
            firstDir = QFileInfo(absolute).absolutePath();
        }
        ++added;
    }

    if (added > 0) {
        if (firstDirectory) {
            *firstDirectory = firstDir;
        }
        emit sourceDirectoryChanged(firstDir);
        emit filesChanged();
    }
    return added;
}

void PdfListWidget::appendItem(const QString &path)
{
    const PdfDocumentInfo info = PdfTools::inspect(path);

    auto *item = new QListWidgetItem();
    item->setData(PathRole, path);
    item->setData(PagesRole, info.pageCount);
    item->setData(SignedRole, info.digitallySigned);
    item->setData(FailedRole, info.loadFailed);
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
    addItem(item);

    refreshItem(item);
}

void PdfListWidget::refreshItem(QListWidgetItem *item)
{
    const QString path = item->data(PathRole).toString();
    const QString fileName = QFileInfo(path).fileName();
    const int pages = item->data(PagesRole).toInt();
    const bool failed = item->data(FailedRole).toBool();
    const bool signedFile = item->data(SignedRole).toBool();

    QString label;
    if (failed) {
        label = tr("%1 (unreadable)").arg(fileName);
    } else {
        label = tr("%1 (%n page(s))", nullptr, pages).arg(fileName);
    }
    item->setText(label);

    QString tooltip = path;
    if (signedFile) {
        tooltip += QLatin1Char('\n')
                   + tr("This document is digitally signed. Merging removes the signature.");
    }
    item->setToolTip(tooltip);

    const qreal ratio = devicePixelRatioF();
    const QString badge = signedFile ? tr("DIGITALLY SIGNED") : QString();
    item->setIcon(QIcon(PdfTools::thumbnail(path, m_thumbnailSize, ratio, badge)));
}

void PdfListWidget::retranslateItems()
{
    for (int row = 0; row < count(); ++row) {
        refreshItem(item(row));
    }
    setToolTip(tr("Drag PDF files here or use \"Add PDFs...\""));
}

void PdfListWidget::removeSelected()
{
    const QList<QListWidgetItem *> selected = selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    for (QListWidgetItem *item : selected) {
        delete takeItem(row(item));
    }
    emit filesChanged();
}

void PdfListWidget::clearAll()
{
    if (count() == 0) {
        return;
    }
    clear();
    emit filesChanged();
}

QStringList PdfListWidget::filePaths() const
{
    QStringList paths;
    paths.reserve(count());
    for (int row = 0; row < count(); ++row) {
        paths.append(item(row)->data(PathRole).toString());
    }
    return paths;
}

QStringList PdfListWidget::signedFilePaths() const
{
    QStringList paths;
    for (int row = 0; row < count(); ++row) {
        QListWidgetItem *entry = item(row);
        if (entry->data(SignedRole).toBool()) {
            paths.append(entry->data(PathRole).toString());
        }
    }
    return paths;
}

void PdfListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && !pdfPathsFromUrls(event->mimeData()->urls()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void PdfListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void PdfListWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QStringList paths = pdfPathsFromUrls(event->mimeData()->urls());
        if (paths.isEmpty()) {
            event->ignore();
            return;
        }
        addFiles(paths);
        event->acceptProposedAction();
        return;
    }
    QListWidget::dropEvent(event);
    emit filesChanged();
}

void PdfListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        removeSelected();
        event->accept();
        return;
    }
    QListWidget::keyPressEvent(event);
}

void PdfListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *clicked = itemAt(event->pos());
    if (clicked && !clicked->isSelected()) {
        clearSelection();
        clicked->setSelected(true);
    }

    QMenu menu(this);
    QAction *removeAction = menu.addAction(tr("Remove"));
    removeAction->setEnabled(!selectedItems().isEmpty());
    QAction *clearAction = menu.addAction(tr("Clear All"));
    clearAction->setEnabled(count() > 0);

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == removeAction) {
        removeSelected();
    } else if (chosen == clearAction) {
        clearAll();
    }
}
