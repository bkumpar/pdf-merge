// SPDX-License-Identifier: GPL-3.0-or-later
#include "PdfListWidget.h"

#include "PdfDocumentInfo.h"
#include "PdfItemDelegate.h"

#include <algorithm>

#include <QContextMenuEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QPoint>
#include <QTimer>
#include <QUrl>

namespace {
constexpr int kThumbWidth = 150;
constexpr int kThumbHeight = 196;
} // namespace

PdfListWidget::PdfListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_thumbnailSize(kThumbWidth, kThumbHeight)
{
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setItemDelegate(new PdfItemDelegate(this));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragEnabled(true);
    setAcceptDrops(true);
    /* QListView::setMovement(Static) switches acceptDrops off on the viewport,
     * and only the viewport receives drop events. Without this line an internal
     * drag never reaches dropEvent, so the list cannot be reordered. */
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setUniformItemSizes(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setToolTip(tr("Drag PDF files here or use \"Add PDFs...\""));

    /* Re-rendering thumbnails on every slider step is too slow, so the crisp
     * redraw waits until the user stops moving the slider. */
    m_thumbnailTimer = new QTimer(this);
    m_thumbnailTimer->setSingleShot(true);
    m_thumbnailTimer->setInterval(180);
    connect(m_thumbnailTimer, &QTimer::timeout, this, &PdfListWidget::refreshThumbnails);

    applyMode();
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
        updateDisplayPaths();
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
    item->setData(PagesTextRole,
                  failed ? QString() : tr("%n page(s)", nullptr, pages));
    item->setData(DisplayPathRole, displayPathFor(path));
    refreshTooltip(item);

    const qreal ratio = devicePixelRatioF();
    const QString badge = signedFile ? tr("DIGITALLY SIGNED") : QString();
    item->setIcon(QIcon(PdfTools::thumbnail(path, m_thumbnailSize, ratio, badge)));
}

void PdfListWidget::setDisplayMode(DisplayMode mode)
{
    if (m_displayMode == mode) {
        return;
    }
    m_displayMode = mode;
    applyMode();
    viewport()->update();
}

void PdfListWidget::applyMode()
{
    if (m_displayMode == Icons) {
        setViewMode(QListView::IconMode);
        setIconSize(m_thumbnailSize);
        setGridSize(QSize(m_thumbnailSize.width() + 24, m_thumbnailSize.height() + 46));
        setWordWrap(true);
        setTextElideMode(Qt::ElideMiddle);
        setSpacing(6);
    } else {
        setViewMode(QListView::ListMode);
        setIconSize(QSize(26, 34));
        setGridSize(QSize());
        setWordWrap(false);
        setTextElideMode(Qt::ElideRight);
        setSpacing(0);
    }
    /* setMovement() and the mode switches touch the viewport drop state. */
    viewport()->setAcceptDrops(true);
    doItemsLayout();
}

void PdfListWidget::setThumbnailWidth(int width)
{
    const int clamped = qBound(80, width, 280);
    const int height = qRound(clamped * static_cast<qreal>(kThumbHeight) / kThumbWidth);
    const QSize size(clamped, height);
    if (size == m_thumbnailSize) {
        return;
    }
    m_thumbnailSize = size;
    if (m_displayMode == Icons) {
        setIconSize(m_thumbnailSize);
        setGridSize(QSize(m_thumbnailSize.width() + 24, m_thumbnailSize.height() + 46));
        doItemsLayout();
    }
    m_thumbnailTimer->start();
}

void PdfListWidget::refreshThumbnails()
{
    for (int row = 0; row < count(); ++row) {
        QListWidgetItem *it = item(row);
        const QString path = it->data(PathRole).toString();
        const bool signedFile = it->data(SignedRole).toBool();
        const QString badge = signedFile ? tr("DIGITALLY SIGNED") : QString();
        it->setIcon(QIcon(PdfTools::thumbnail(path, m_thumbnailSize,
                                              devicePixelRatioF(), badge)));
    }
    viewport()->update();
}

void PdfListWidget::setShowFullPath(bool full)
{
    if (m_showFullPath == full) {
        return;
    }
    m_showFullPath = full;
    updateDisplayPaths();
}

QString PdfListWidget::commonDirectoryPrefix(const QStringList &paths)
{
    if (paths.size() < 2) {
        return QString();
    }
    QStringList common = QFileInfo(paths.first()).absolutePath()
                             .split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString &path : paths) {
        const QStringList parts = QFileInfo(path).absolutePath()
                                      .split(QLatin1Char('/'), Qt::SkipEmptyParts);
        int shared = 0;
        while (shared < common.size() && shared < parts.size()
               && common.at(shared) == parts.at(shared)) {
            ++shared;
        }
        common = common.mid(0, shared);
        if (common.isEmpty()) {
            return QString();
        }
    }
    return QLatin1Char('/') + common.join(QLatin1Char('/')) + QLatin1Char('/');
}

QString PdfListWidget::displayPathFor(const QString &path) const
{
    if (m_showFullPath) {
        return path;
    }
    if (m_pathPrefix.isEmpty()) {
        /* Nothing in common: the folder is the part that tells them apart. */
        return QFileInfo(path).absolutePath();
    }
    if (m_pathsAreFlat) {
        /* Every document sits directly in the shared folder, so a path column
         * would only repeat the file name. */
        return QString();
    }
    return path.startsWith(m_pathPrefix) ? path.mid(m_pathPrefix.size())
                                         : QFileInfo(path).fileName();
}

void PdfListWidget::updateDisplayPaths()
{
    m_pathPrefix = commonDirectoryPrefix(filePaths());

    bool flat = !m_pathPrefix.isEmpty();
    for (int row = 0; flat && row < count(); ++row) {
        const QString path = item(row)->data(PathRole).toString();
        flat = path.startsWith(m_pathPrefix)
               && path.mid(m_pathPrefix.size()) == QFileInfo(path).fileName();
    }
    m_pathsAreFlat = flat;

    for (int row = 0; row < count(); ++row) {
        QListWidgetItem *it = item(row);
        it->setData(DisplayPathRole, displayPathFor(it->data(PathRole).toString()));
        refreshTooltip(it);
    }
    viewport()->update();
}

void PdfListWidget::refreshTooltip(QListWidgetItem *item)
{
    const QString path = item->data(PathRole).toString();
    const QString shown = displayPathFor(path);
    QString tooltip = shown;
    if (shown.isEmpty()) {
        tooltip = path;
    } else if (shown != path) {
        tooltip += QLatin1Char('\n') + path;
    }
    if (item->data(SignedRole).toBool()) {
        tooltip += QLatin1Char('\n')
                   + tr("This document is digitally signed. Merging removes the signature.");
    }
    item->setToolTip(tooltip);
}

void PdfListWidget::retranslateItems()
{
    for (int row = 0; row < count(); ++row) {
        refreshItem(item(row));
    }
    viewport()->update();
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
    updateDisplayPaths();
    emit filesChanged();
}

void PdfListWidget::clearAll()
{
    if (count() == 0) {
        return;
    }
    clear();
    updateDisplayPaths();
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
    // A drag that started in this list reorders it, it never adds files.
    if (event->source() == this) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    if (event->mimeData()->hasUrls() && !pdfPathsFromUrls(event->mimeData()->urls()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void PdfListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->source() == this) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void PdfListWidget::dropEvent(QDropEvent *event)
{
    if (event->source() == this) {
        const bool moved = moveSelectionToRow(dropRowAt(event->position().toPoint()));
        /* The items were already moved in the model, so the drop is reported
         * as a copy: a MoveAction would make QAbstractItemView delete the
         * source rows afterwards and the documents would disappear. */
        if (moved)
            event->setDropAction(Qt::CopyAction);
        else
            event->setDropAction(Qt::IgnoreAction);
        event->accept();
        return;
    }

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

int PdfListWidget::dropRowAt(const QPoint &pos) const
{
    const QModelIndex index = indexAt(pos);
    if (!index.isValid())
        return count();  // dropped below or beside the last item
    const QRect rect = visualRect(index);
    const bool after = viewMode() == QListView::IconMode
                           ? pos.x() > rect.center().x()
                           : pos.y() > rect.center().y();
    return after ? index.row() + 1 : index.row();
}

bool PdfListWidget::moveSelectionToRow(int row)
{
    QList<QListWidgetItem *> selected = selectedItems();
    if (selected.isEmpty())
        return false;

    QList<int> rows;
    rows.reserve(selected.size());
    for (QListWidgetItem *item : selected)
        rows.append(this->row(item));
    std::sort(rows.begin(), rows.end());

    const int target = qBound(0, row, count());
    // Dropping inside the moved block leaves the order unchanged.
    if (target >= rows.first() && target <= rows.last() + 1)
        return false;

    // Take from the bottom up so the remaining rows stay valid.
    QList<QListWidgetItem *> moved;
    moved.reserve(rows.size());
    for (int i = rows.size() - 1; i >= 0; --i)
        moved.prepend(takeItem(rows.at(i)));

    int insertAt = target > rows.last() ? target - moved.size() : target;
    for (QListWidgetItem *item : moved)
        insertItem(insertAt++, item);

    clearSelection();
    for (QListWidgetItem *item : moved)
        item->setSelected(true);
    scrollToItem(moved.first(), QAbstractItemView::EnsureVisible);
    setCurrentItem(moved.first());

    emit filesChanged();
    return true;
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
