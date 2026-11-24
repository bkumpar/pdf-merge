// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QListWidget>
#include <QStringList>
#include <QUrl>

/* Icon-mode list of PDF documents with thumbnails, drag and drop and
 * reordering. Each item keeps its absolute path in Qt::UserRole. */
class PdfListWidget : public QListWidget
{
    Q_OBJECT

public:
    enum ItemRole {
        PathRole = Qt::UserRole,
        PagesRole,
        SignedRole,
        FailedRole,
    };

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
    static QStringList pdfPathsFromUrls(const QList<QUrl> &urls);

    QSize m_thumbnailSize;
};
