#include "library/playlisttablemodel.h"
#include "library/queryutil.h"

PlaylistTableModel::PlaylistTableModel(QObject* parent,
                                    TrackCollection* pTrackCollection,
                                    QString settingsNamespace,
                                    ConfigObject<ConfigValue>* pConfig,
                                    QList<int> availableDirIds,
                                    bool showAll)
        : BaseSqlTableModel(parent, pTrackCollection, pConfig,
                            availableDirIds, settingsNamespace),
                            m_playlistDao(m_pTrackCollection->getPlaylistDAO()),
                            m_iPlaylistId(-1),
                            m_showAll(showAll) {
}

PlaylistTableModel::~PlaylistTableModel() {
}

void PlaylistTableModel::setTableModel(int playlistId) {
    //qDebug() << "PlaylistTableModel::setPlaylist" << playlistId;

    if (playlistId == m_iPlaylistId) {
        qDebug() << "Already focused on playlist " << playlistId;
        return;
    } else if (playlistId == -1) {
        // calls from parent class use -1 as id then just set the current playlist
        playlistId = m_iPlaylistId;
    }

    m_iPlaylistId = playlistId;
    QString playlistTableName = "playlist_" + QString::number(m_iPlaylistId)+"_";

    QSqlQuery query(m_pTrackCollection->getDatabase());
    FieldEscaper escaper(m_pTrackCollection->getDatabase());

    QStringList columns;
    QStringList tableColumns;
    QString filter;
        columns << "PlaylistTracks."+PLAYLISTTRACKSTABLE_TRACKID
                << "PlaylistTracks."+PLAYLISTTRACKSTABLE_POSITION
                << "PlaylistTracks."+PLAYLISTTRACKSTABLE_DATETIMEADDED;
        tableColumns << PLAYLISTTRACKSTABLE_TRACKID
                    << PLAYLISTTRACKSTABLE_POSITION
                    << PLAYLISTTRACKSTABLE_DATETIMEADDED;
    bool showMissing = m_pConfig->getValueString(ConfigKey("[Library]","ShowMissingSongs"),"1").toInt();
    if (showMissing) {
        filter = "library.mixxx_deleted=0";
        playlistTableName.append("missing_");
    } else {
        filter = "library.mixxx_deleted=0 AND track_locations.fs_deleted=0";
    }
    QStringList ids;
    foreach (int id, m_availableDirIds) {
        ids << QString::number(id);
    }
    playlistTableName.append(ids.join(""));

    // We drop files that have been explicitly deleted from mixxx
    // (mixxx_deleted=0) from the view. There was a bug in <= 1.9.0 where
    // removed files were not removed from playlists, so some users will have
    // libraries where this is the case.
    QString queryString = QString(
        "CREATE TEMPORARY VIEW IF NOT EXISTS %1 AS "
        "SELECT %2 FROM PlaylistTracks "
        "INNER JOIN library ON library.id = PlaylistTracks.track_id "
        "INNER JOIN track_locations ON track_locations.id=PlaylistTracks.track_id "
        "WHERE PlaylistTracks.playlist_id = %3")
            .arg(escaper.escapeString(playlistTableName),
                 columns.join(","),
                 QString::number(playlistId));
    if (!m_showAll) {
        queryString.append(" AND " + filter);
    }
    queryString.append(" AND track_locations.maindir_id in ("+ids.join(",")+",0)");
    qDebug() << queryString;
    query.prepare(queryString);
    if (!query.exec()) {
        LOG_FAILED_QUERY(query);
    }

    setTable(playlistTableName, tableColumns[0], tableColumns,
             m_pTrackCollection->getTrackSource("default"));
    initHeaderData();
    setSearch("");
    setDefaultSort(fieldIndex("position"), Qt::AscendingOrder);
    setSort(defaultSortColumn(),defaultSortOrder());
}

int PlaylistTableModel::addTracks(const QModelIndex& index, QList<QString> locations) {
    const int positionColumn = fieldIndex(PLAYLISTTRACKSTABLE_POSITION);
    int position = index.sibling(index.row(), positionColumn).data().toInt();

    // Handle weird cases like a drag and drop to an invalid index
    if (position <= 0) {
        position = rowCount() + 1;
    }

    QList<QFileInfo> fileInfoList;
    foreach (QString fileLocation, locations) {
        fileInfoList.append(QFileInfo(fileLocation));
    }

    QList<int> trackIds = m_trackDAO.addTracks(fileInfoList, true);

    int tracksAdded = m_playlistDao.insertTracksIntoPlaylist(
        trackIds, m_iPlaylistId, position);

    if (tracksAdded > 0) {
        select();
    } else if (locations.size() - tracksAdded > 0) {
        qDebug() << "PlaylistTableModel::addTracks could not add"
                 << locations.size() - tracksAdded
                 << "to playlist" << m_iPlaylistId;
    }
    return tracksAdded;
}

void PlaylistTableModel::removeTrack(const QModelIndex& index) {
    if (m_playlistDao.isPlaylistLocked(m_iPlaylistId)) {
        return;
    }

    const int positionColumnIndex = fieldIndex(PLAYLISTTRACKSTABLE_POSITION);
    int position = index.sibling(index.row(), positionColumnIndex).data().toInt();
    m_playlistDao.removeTrackFromPlaylist(m_iPlaylistId, position);
    select(); //Repopulate the data model.
}

void PlaylistTableModel::removeTracks(const QModelIndexList& indices) {
    bool locked = m_playlistDao.isPlaylistLocked(m_iPlaylistId);

    if (locked) {
        return;
    }

    const int positionColumnIndex = fieldIndex(PLAYLISTTRACKSTABLE_POSITION);

    QList<int> trackPositions;
    foreach (QModelIndex index, indices) {
        int trackPosition = index.sibling(index.row(), positionColumnIndex).data().toInt();
        trackPositions.append(trackPosition);
    }

    m_playlistDao.removeTracksFromPlaylist(m_iPlaylistId,trackPositions);

    // Have to re-lookup every track b/c their playlist ranks might have changed
    select();
}

void PlaylistTableModel::moveTrack(const QModelIndex& sourceIndex,
                                   const QModelIndex& destIndex) {
    //QSqlRecord sourceRecord = this->record(sourceIndex.row());
    //sourceRecord.setValue("position", destIndex.row());
    //this->removeRows(sourceIndex.row(), 1);

    //this->insertRecord(destIndex.row(), sourceRecord);

    //TODO: execute a real query to DELETE the sourceIndex.row() row from the PlaylistTracks table.
    //int newPosition = destIndex.row();
    //int oldPosition = sourceIndex.row();
    //const int positionColumnIndex = this->fieldIndex(PLAYLISTTRACKSTABLE_POSITION);
    //int newPosition = index.sibling(destIndex.row(), positionColumnIndex).data().toInt();
    //int oldPosition = index.sibling(sourceIndex.row(), positionColumnIndex).data().toInt();


    int playlistPositionColumn = fieldIndex(PLAYLISTTRACKSTABLE_POSITION);

    // this->record(destIndex.row()).value(PLAYLISTTRACKSTABLE_POSITION).toInt();
    int newPosition = destIndex.sibling(destIndex.row(), playlistPositionColumn).data().toInt();
    // this->record(sourceIndex.row()).value(PLAYLISTTRACKSTABLE_POSITION).toInt();
    int oldPosition = sourceIndex.sibling(sourceIndex.row(), playlistPositionColumn).data().toInt();


    //qDebug() << "old pos" << oldPosition << "new pos" << newPosition;

    //Invalid for the position to be 0 or less.
    if (newPosition < 0)
        return;
    else if (newPosition == 0) //Dragged out of bounds, which is past the end of the rows...
        newPosition = rowCount();

    //Start the transaction
    ScopedTransaction transaction(m_pTrackCollection->getDatabase());

    //Find out the highest position existing in the playlist so we know what
    //position this track should have.
    QSqlQuery query(m_pTrackCollection->getDatabase());

    //Insert the song into the PlaylistTracks table

    // ALGORITHM for code below
    // Case 1: destination < source (newPos < oldPos)
    //    1) Set position = -1 where pos=source -- Gives that track a dummy index to keep stuff simple.
    //    2) Decrement position where pos > source
    //    3) increment position where pos > dest
    //    4) Set position = dest where pos=-1 -- Move track from dummy pos to final destination.

     // Case 2: destination > source (newPos > oldPos)
     //   1) Set position=-1 where pos=source -- Give track a dummy index again.
     //   2) Decrement position where pos > source AND pos <= dest
     //   3) Set postion=dest where pos=-1 -- Move that track from dummy pos to final destination

    QString queryString;
    if (newPosition < oldPosition) {
        queryString =
            QString("UPDATE PlaylistTracks SET position=-1 "
                    "WHERE position=%1 AND "
                    "playlist_id=%2").arg(QString::number(oldPosition),
                                          QString::number(m_iPlaylistId));
        query.exec(queryString);
        //qDebug() << queryString;

        queryString = QString("UPDATE PlaylistTracks SET position=position-1 "
                              "WHERE position > %1 AND "
                              "playlist_id=%2").arg(QString::number(oldPosition),
                                                    QString::number(m_iPlaylistId));
        query.exec(queryString);

        queryString = QString("UPDATE PlaylistTracks SET position=position+1 "
                              "WHERE position >= %1 AND " //position < %2 AND "
                              "playlist_id=%3").arg(QString::number(newPosition),
                                                    QString::number(m_iPlaylistId));
        query.exec(queryString);

        queryString = QString("UPDATE PlaylistTracks SET position=%1 "
                              "WHERE position=-1 AND "
                              "playlist_id=%2").arg(QString::number(newPosition),
                                                    QString::number(m_iPlaylistId));
        query.exec(queryString);
    }
    else if (newPosition > oldPosition)
    {
        queryString = QString("UPDATE PlaylistTracks SET position=-1 "
                              "WHERE position = %1 AND "
                              "playlist_id=%2").arg(QString::number(oldPosition),
                                                    QString::number(m_iPlaylistId));
        //qDebug() << queryString;
        query.exec(queryString);

        queryString = QString("UPDATE PlaylistTracks SET position=position-1 "
                              "WHERE position > %1 AND position <= %2 AND "
                              "playlist_id=%3").arg(QString::number(oldPosition),
                                                    QString::number(newPosition),
                                                    QString::number(m_iPlaylistId));
        query.exec(queryString);

        queryString = QString("UPDATE PlaylistTracks SET position=%1 "
                              "WHERE position=-1 AND "
                              "playlist_id=%2").arg(QString::number(newPosition),
                                                    QString::number(m_iPlaylistId));
        query.exec(queryString);
    }

    transaction.commit();

    //Print out any SQL error, if there was one.
    if (query.lastError().isValid()) {
        qDebug() << query.lastError();
    }

    select();
}

void PlaylistTableModel::shuffleTracks(const QModelIndex& shuffleStartIndex) {
    int numOfTracks = rowCount();
    int seed = QDateTime::currentDateTime().toTime_t();
    qsrand(seed);
    QSqlQuery query(m_pTrackCollection->getDatabase());
    const int positionColumnIndex = fieldIndex(PLAYLISTTRACKSTABLE_POSITION);
    int shuffleStartRow = shuffleStartIndex.row();

    ScopedTransaction transaction(m_pTrackCollection->getDatabase());

    // This is a simple Fisher-Yates shuffling algorithm
    for (int i=numOfTracks-1; i >= shuffleStartRow; i--)
    {
        int oldPosition = index(i, positionColumnIndex).data().toInt();
        int random = int(qrand() / (RAND_MAX + 1.0) * (numOfTracks - shuffleStartRow) + shuffleStartRow + 1);
        qDebug() << "Swapping tracks " << oldPosition << " and " << random;
        QString swapQuery = "UPDATE PlaylistTracks SET position=%1 WHERE position=%2 AND playlist_id=%3";
        query.exec(swapQuery.arg(QString::number(-1),
                                 QString::number(oldPosition),
                                 QString::number(m_iPlaylistId)));
        query.exec(swapQuery.arg(QString::number(oldPosition),
                                 QString::number(random),
                                 QString::number(m_iPlaylistId)));
        query.exec(swapQuery.arg(QString::number(random),
                                 QString::number(-1),
                                 QString::number(m_iPlaylistId)));

        if (query.lastError().isValid())
            qDebug() << query.lastError();
    }

    transaction.commit();
    // TODO(XXX) set dirty because someday select() will only do work on dirty.
    select();
}

bool PlaylistTableModel::isColumnInternal(int column) {
    if (column == fieldIndex(PLAYLISTTRACKSTABLE_TRACKID) ||
        column == fieldIndex(LIBRARYTABLE_PLAYED) ||
        column == fieldIndex(LIBRARYTABLE_MIXXXDELETED) ||
        column == fieldIndex(LIBRARYTABLE_BPM_LOCK) ||
        column == fieldIndex(TRACKLOCATIONSTABLE_FSDELETED)) {
        return true;
    }
    return false;
}
bool PlaylistTableModel::isColumnHiddenByDefault(int column) {
    if (column == fieldIndex(LIBRARYTABLE_KEY)) {
        return true;
    }
    if (column == fieldIndex(PLAYLISTTRACKSTABLE_DATETIMEADDED)) {
        return true;
    }
    return false;
}

TrackModel::CapabilitiesFlags PlaylistTableModel::getCapabilities() const {
    TrackModel::CapabilitiesFlags caps = TRACKMODELCAPS_NONE
            | TRACKMODELCAPS_RECEIVEDROPS
            | TRACKMODELCAPS_REORDER
            | TRACKMODELCAPS_ADDTOCRATE
            | TRACKMODELCAPS_ADDTOPLAYLIST
            | TRACKMODELCAPS_RELOADMETADATA
            | TRACKMODELCAPS_LOADTODECK
            | TRACKMODELCAPS_LOADTOSAMPLER
            | TRACKMODELCAPS_REMOVE
            | TRACKMODELCAPS_RELOCATE
            | TRACKMODELCAPS_BPMLOCK
            | TRACKMODELCAPS_CLEAR_BEATS
            | TRACKMODELCAPS_RESETPLAYED;

    // Only allow Add to AutoDJ if we aren't currently showing the AutoDJ queue.
    if (m_iPlaylistId != m_playlistDao.getPlaylistIdFromName(AUTODJ_TABLE)) {
        caps |= TRACKMODELCAPS_ADDTOAUTODJ;
    }

    bool locked = m_playlistDao.isPlaylistLocked(m_iPlaylistId);

    if (locked) {
        caps |= TRACKMODELCAPS_LOCKED;
    }

    return caps;
}
