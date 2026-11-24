// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QListWidget>
#include <QSize>
#include <QStringList>
#include <QUrl>

class QTimer;

/* List of PDF documents with thumbnails, drag and drop and reordering.
 *
 * The widget has two presentations of the same model: an icon grid with page
 * thumbnails, and a compact details list that also shows the document path.
 * Each item keeps its absolute path in PathRole. */
class PdfListWidget : public QListWidget
{
    Q_OBJECT

public:
    enum ItemRole {
        PathRole = Qt::UserRole,
        PagesRole,
        SignedRole,
        FailedRole,
        DisplayPathRole,  /* normalised or full path, see setShowFullPath() */
        PagesTextRole,    /* formatted page count, painted by the delegate */
    };

    enum DisplayMode { Icons, Details };

    explicit PdfListWidget(QWidget *parent = nullptr);

    /* Adds the PDF files among paths. Returns how many were added and, when
     * firstDirectory is given, stores the folder of the first added file. */
    int addFiles(const QStringList &paths, QString *firstDirectory = nullptr);

    void removeSelected();
    void clearAll();

    QStringList filePaths() const;
    QStringList signedFilePaths() const;

    /* Rebuilds item labels and thumbnails after a language change. */
    void retranslateItems();

    /* Moves the selected items so that they are inserted before the given row.
     * Returns true when the order actually changed. */
    bool moveSelectionToRow(int row);

    void setDisplayMode(DisplayMode mode);
    DisplayMode displayMode() const { return m_displayMode; }

    /* Width of the page thumbnail in icon mode, clamped to a usable range. */
    void setThumbnailWidth(int width);
    int thumbnailWidth() const { return m_thumbnailSize.width(); }

    /* Width of the thumbnail in details mode; the row height follows it. */
    void setDetailsIconWidth(int width);
    int detailsIconWidth() const { return m_detailsIconSize.width(); }

    /* Details mode shows the part of the path that differs between the
     * documents. With full paths enabled the whole path is shown instead. */
    void setShowFullPath(bool full);
    bool showFullPath() const { return m_showFullPath; }

    /* Longest directory prefix shared by all paths, with the trailing
     * separator included. Empty when there is nothing in common. */
    static QString commonDirectoryPrefix(const QStringList &paths);

signals:
    void filesChanged();
    void sourceDirectoryChanged(const QString &directory);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void appendItem(const QString &path);
    void refreshItem(QListWidgetItem *item);
    void applyMode();
    QSize iconSizeForMode() const;
    QString displayPathFor(const QString &path) const;
    void refreshTooltip(QListWidgetItem *item);
    void updateDisplayPaths();
    void refreshThumbnails();
    int dropRowAt(const QPoint &pos) const;
    static QStringList pdfPathsFromUrls(const QList<QUrl> &urls);

    QSize m_thumbnailSize;
    QSize m_detailsIconSize;
    DisplayMode m_displayMode = Icons;
    bool m_showFullPath = false;
    QTimer *m_thumbnailTimer = nullptr;
    QString m_pathPrefix;
    bool m_pathsAreFlat = false;
};
