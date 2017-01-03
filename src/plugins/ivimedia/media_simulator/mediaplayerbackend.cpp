/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtIvi module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-QTAS$
** Commercial License Usage
** Licensees holding valid commercial Qt Automotive Suite licenses may use
** this file in accordance with the commercial license agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and The Qt Company.  For
** licensing terms and conditions see https://www.qt.io/terms-conditions.
** For further information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
** SPDX-License-Identifier: LGPL-3.0
**
****************************************************************************/

#include "mediaplayerbackend.h"
#include "searchandbrowsebackend.h"

#include <QtConcurrent/QtConcurrent>
#include <QtMultimedia/QMediaPlayer>

#include <QFuture>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>

MediaPlayerBackend::MediaPlayerBackend(const QSqlDatabase &database, QObject *parent)
    : QIviMediaPlayerBackendInterface(parent)
    , m_count(0)
    , m_currentIndex(-1)
    , m_player(new QMediaPlayer(this))
{
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &MediaPlayerBackend::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &MediaPlayerBackend::positionChanged);

    m_db = database;
    m_db.open();

    QSqlQuery query = m_db.exec(QLatin1String("CREATE TABLE IF NOT EXISTS \"queue\" (\"id\" INTEGER PRIMARY KEY, \"qindex\" INTEGER, \"track_index\" INTEGER)"));
    if (query.lastError().isValid())
        qWarning() << query.lastError().text();
    m_db.commit();
}

void MediaPlayerBackend::initialize()
{
    m_count = 0;
    m_currentIndex = -1;
}

void MediaPlayerBackend::play()
{
    m_player->play();
}

void MediaPlayerBackend::pause()
{
    m_player->pause();
}

void MediaPlayerBackend::stop()
{
    m_player->stop();
}

void MediaPlayerBackend::seek(qint64 offset)
{
    m_player->setPosition(m_player->position() + offset);
}

void MediaPlayerBackend::next()
{
    setCurrentIndex(m_currentIndex + 1);
}

void MediaPlayerBackend::previous()
{
    setCurrentIndex(m_currentIndex - 1);
}

bool MediaPlayerBackend::canReportCount()
{
    return true;
}

void MediaPlayerBackend::fetchData(int start, int count)
{
    QString queryString = QString(QLatin1String("SELECT track.id, artistName, albumName, trackName, genre, number, file, coverArtUrl FROM track JOIN queue ON queue.track_index=track.id ORDER BY queue.qindex LIMIT %4, %5"))
            .arg(start)
            .arg(count);

    QStringList queries;
    queries.append(queryString);
    QtConcurrent::run(this,
                      &MediaPlayerBackend::doSqlOperation,
                      MediaPlayerBackend::Select,
                      queries,
                      start,
                      count);
}

void MediaPlayerBackend::insert(int index, const QIviPlayableItem *item)
{
    if (item->type() != "audiotrack")
        return;

    int track_index = item->id().toInt();

    QString queryString = QString(QLatin1String("UPDATE queue SET qindex = qindex + 1 WHERE qindex >= %1;"
                                                "INSERT INTO queue(qindex, track_index) VALUES( %1, %2);"
                                                "SELECT track.id, artistName, albumName, trackName, genre, number, file, coverArtUrl FROM track JOIN queue ON queue.track_index=track.id WHERE qindex=%1"))
            .arg(index)
            .arg(track_index);
    QStringList queries = queryString.split(';');

    QtConcurrent::run(this,
                      &MediaPlayerBackend::doSqlOperation,
                      MediaPlayerBackend::Insert,
                      queries, index, 0);
}

void MediaPlayerBackend::remove(int index)
{
    QString queryString = QString(QLatin1String("DELETE FROM queue WHERE qindex=%1;"
                                                "UPDATE queue SET qindex = qindex - 1 WHERE qindex >= %1"))
            .arg(index);
    QStringList queries = queryString.split(';');

    QtConcurrent::run(this,
                      &MediaPlayerBackend::doSqlOperation,
                      MediaPlayerBackend::Remove,
                      queries, index, 1);
}

void MediaPlayerBackend::move(int cur_index, int new_index)
{
    int delta = new_index - cur_index;
    if (delta == 0)
        return;

    QString queryString = QString(QLatin1String("UPDATE queue SET qindex = ( SELECT MAX(qindex) + 1 FROM queue) WHERE qindex=%1;"
                                                "UPDATE queue SET qindex = qindex %5 1 WHERE qindex >= %3 AND qindex <= %4;"
                                                "UPDATE queue SET qindex = %2 WHERE qindex= ( SELECT MAX(qindex) FROM queue);"
                                                "SELECT track.id, artistName, albumName, trackName, genre, number, file, coverArtUrl FROM track JOIN queue ON queue.track_index=track.id WHERE qindex >= %3 AND qindex <= %4 ORDER BY qindex"))
            .arg(cur_index)
            .arg(new_index)
            .arg(qMin(cur_index, new_index))
            .arg(qMax(cur_index, new_index))
            .arg(delta > 0 ? "-" : "+");
    QStringList queries = queryString.split(';');

    QtConcurrent::run(this,
                      &MediaPlayerBackend::doSqlOperation,
                      MediaPlayerBackend::Move,
                      queries, qMin(cur_index, new_index), qAbs(delta) + 1);
}

void MediaPlayerBackend::doSqlOperation(MediaPlayerBackend::OperationType type, const QStringList &queries, int start, int count)
{
    m_db.transaction();
    QSqlQuery query(m_db);
    QVariantList list;

    for (const QString& queryString : queries) {
        if (query.exec(queryString)) {
            while (query.next()) {
                QString id = query.value(0).toString();
                QString artist = query.value(1).toString();
                QString album = query.value(2).toString();

                //Creating the TrackItem in an factory with would make this more performant
                QIviAudioTrackItem item;
                item.setId(id);
                item.setTitle(query.value(3).toString());
                item.setArtist(artist);
                item.setAlbum(album);
                item.setUrl(QUrl::fromLocalFile(query.value(6).toString()));
                item.setCoverArtUrl(QUrl::fromLocalFile(query.value(7).toString()));
                list.append(QVariant::fromValue(item));
            }
        } else {
            qDebug() << queryString;
            qDebug() << query.lastError().text();
            m_db.rollback();
            break;
        }
    }

    query.clear();
    if (query.exec(QLatin1String("SELECT COUNT(*) FROM queue"))) {
        query.next();
        m_count = query.value(0).toInt();
        emit countChanged(m_count);
    } else {
        qWarning() << query.lastError().text();
    }

    if (type == MediaPlayerBackend::Select) {
        emit dataFetched(list, start, list.count() >= count);
    } else if (type == MediaPlayerBackend::SetIndex) {
        QIviAudioTrackItem item = list.at(0).value<QIviAudioTrackItem>();
        bool playing = m_player->state() == QMediaPlayer::PlayingState;
        m_player->setMedia(item.url());
        if (playing)
            m_player->play();
        emit currentTrackChanged(list.at(0));
    } else {
        emit dataChanged(list, start, count);
    }

    m_db.commit();
}

void MediaPlayerBackend::setCurrentIndex(int index)
{
    if (index >= m_count || index < 0)
        return;

    m_currentIndex = index;
    QString queryString = QString(QLatin1String("SELECT track.id, artistName, albumName, trackName, genre, number, file, coverArtUrl FROM track JOIN queue ON queue.track_index=track.id WHERE queue.qindex=%1 ORDER BY queue.qindex"))
            .arg(m_currentIndex);

    QStringList queries;
    queries.append(queryString);

    QtConcurrent::run(this,
                      &MediaPlayerBackend::doSqlOperation,
                      MediaPlayerBackend::SetIndex,
                      queries, m_currentIndex, 0);

    emit currentIndexChanged(m_currentIndex);
}
