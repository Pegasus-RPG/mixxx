// mixxxlibraryfeature.cpp
// Created 8/23/2009 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>


#include "library/mixxxlibraryfeature.h"

#include "library/basetrackcache.h"
#include "library/librarytablemodel.h"
#include "library/hiddentablemodel.h"
#include "library/queryutil.h"
#include "library/trackcollection.h"
#include "treeitem.h"

MixxxLibraryFeature::MixxxLibraryFeature(QObject* parent,
                                         TrackCollection* pTrackCollection,
                                         ConfigObject<ConfigValue>* pConfig)
        : LibraryFeature(parent),
          kHiddenTitle(tr("Hidden Tracks")),
          m_directoryDAO(pTrackCollection->getDirectoryDAO()) {
    QStringList columns;
    columns << "library." + LIBRARYTABLE_ID
            << "library." + LIBRARYTABLE_PLAYED
            << "library." + LIBRARYTABLE_TIMESPLAYED
            << "library." + LIBRARYTABLE_ARTIST
            << "library." + LIBRARYTABLE_TITLE
            << "library." + LIBRARYTABLE_ALBUM
            << "library." + LIBRARYTABLE_YEAR
            << "library." + LIBRARYTABLE_DURATION
            << "library." + LIBRARYTABLE_RATING
            << "library." + LIBRARYTABLE_GENRE
            << "library." + LIBRARYTABLE_COMPOSER
            << "library." + LIBRARYTABLE_FILETYPE
            << "library." + LIBRARYTABLE_TRACKNUMBER
            << "library." + LIBRARYTABLE_KEY
            << "library." + LIBRARYTABLE_DATETIMEADDED
            << "library." + LIBRARYTABLE_BPM
            << "library." + LIBRARYTABLE_BPM_LOCK
            << "library." + LIBRARYTABLE_BITRATE
            << "track_locations.location"
            << "track_locations.fs_deleted"
            << "library." + LIBRARYTABLE_COMMENT
            << "library." + LIBRARYTABLE_MIXXXDELETED;

    QSqlQuery query(pTrackCollection->getDatabase());
    QString tableName = "library_cache_view";
    QString queryString = QString(
        "CREATE TEMPORARY VIEW IF NOT EXISTS %1 AS "
        "SELECT %2 FROM library "
        "INNER JOIN track_locations ON library.location = track_locations.id")
            .arg(tableName, columns.join(","));
    query.prepare(queryString);
    if (!query.exec()) {
        LOG_FAILED_QUERY(query);
    }

    // Strip out library. and track_locations.
    for (QStringList::iterator it = columns.begin();
         it != columns.end(); ++it) {
        if (it->startsWith("library.")) {
            *it = it->replace("library.", "");
        } else if (it->startsWith("track_locations.")) {
            *it = it->replace("track_locations.", "");
        }
    }

    BaseTrackCache* pBaseTrackCache = new BaseTrackCache(
        pTrackCollection, tableName, LIBRARYTABLE_ID, columns, true);
    connect(&pTrackCollection->getTrackDAO(), SIGNAL(trackDirty(int)),
            pBaseTrackCache, SLOT(slotTrackDirty(int)));
    connect(&pTrackCollection->getTrackDAO(), SIGNAL(trackClean(int)),
            pBaseTrackCache, SLOT(slotTrackClean(int)));
    connect(&pTrackCollection->getTrackDAO(), SIGNAL(trackChanged(int)),
            pBaseTrackCache, SLOT(slotTrackChanged(int)));
    connect(&pTrackCollection->getTrackDAO(), SIGNAL(tracksAdded(QSet<int>)),
            pBaseTrackCache, SLOT(slotTracksAdded(QSet<int>)));
    connect(&pTrackCollection->getTrackDAO(), SIGNAL(tracksRemoved(QSet<int>)),
            pBaseTrackCache, SLOT(slotTracksRemoved(QSet<int>)));
    connect(&pTrackCollection->getTrackDAO(), SIGNAL(dbTrackAdded(TrackPointer)),
            pBaseTrackCache, SLOT(slotDbTrackAdded(TrackPointer)));


    m_pBaseTrackCache = QSharedPointer<BaseTrackCache>(pBaseTrackCache);
    pTrackCollection->addTrackSource(QString("default"), m_pBaseTrackCache);

    // These rely on the 'default' track source being present.
    m_pLibraryTableModel = new LibraryTableModel(this, pTrackCollection,pConfig);
    connect(parent,SIGNAL(configChanged(QString,QString)),
            m_pLibraryTableModel, SLOT(slotConfigChanged(QString, QString)));
    connect(this, SIGNAL(loadTrackFailed(TrackPointer)),
            m_pLibraryTableModel, SLOT(slotLoadTrackFailed(TrackPointer)));
    m_pHiddenTableModel = new HiddenTableModel(this, pTrackCollection);


    TreeItem* pRootItem = new TreeItem();
    TreeItem* phiddenChildItem = new TreeItem(kHiddenTitle, kHiddenTitle,
                                       this, pRootItem);
    pRootItem->appendChild(phiddenChildItem);

    m_childModel.setRootItem(pRootItem);
}

MixxxLibraryFeature::~MixxxLibraryFeature() {
    delete m_pLibraryTableModel;
    delete m_pHiddenTableModel;
}

QVariant MixxxLibraryFeature::title() {
    return tr("Library");
}

QIcon MixxxLibraryFeature::getIcon() {
    return QIcon(":/images/library/ic_library_library.png");
}

TreeItemModel* MixxxLibraryFeature::getChildModel() {
    return &m_childModel;
}

void MixxxLibraryFeature::refreshLibraryModels()
{
    if (m_pLibraryTableModel) {
        m_pLibraryTableModel->select();
    }
    if (m_pHiddenTableModel) {
        m_pHiddenTableModel->select();
    }
}

void MixxxLibraryFeature::activate() {
    qDebug() << "MixxxLibraryFeature::activate()";
    emit(showTrackModel(m_pLibraryTableModel));
}

void MixxxLibraryFeature::activateChild(const QModelIndex& index) {
    QString itemName = index.data().toString();

    /*if (itemName == m_childModel.stringList().at(0))
        emit(showTrackModel(m_pDeletedTableModel));
     */
    if (itemName == kHiddenTitle) {
        emit(showTrackModel(m_pHiddenTableModel));
    }
}

void MixxxLibraryFeature::onRightClick(const QPoint& globalPos) {
    Q_UNUSED(globalPos);
}

void MixxxLibraryFeature::onRightClickChild(const QPoint& globalPos,
                                            QModelIndex index) {
    Q_UNUSED(globalPos);
    Q_UNUSED(index);
}

bool MixxxLibraryFeature::dropAccept(QList<QUrl> urls) {
    Q_UNUSED(urls);
    return false;
}

bool MixxxLibraryFeature::dropAcceptChild(const QModelIndex& index, QList<QUrl> urls) {
    Q_UNUSED(urls);
    Q_UNUSED(index);
    return false;
}

bool MixxxLibraryFeature::dragMoveAccept(QUrl url) {
    Q_UNUSED(url);
    return false;
}

bool MixxxLibraryFeature::dragMoveAcceptChild(const QModelIndex& index,
                                              QUrl url) {
    Q_UNUSED(url);
    Q_UNUSED(index);
    return false;
}

void MixxxLibraryFeature::onLazyChildExpandation(const QModelIndex &index){
    Q_UNUSED(index);
    // Nothing to do because the childmodel is not of lazy nature.
}

void MixxxLibraryFeature::slotDirsChanged(QString op, QString dir){
    if (op=="added") {
        m_directoryDAO.addDirectory(dir);
    } else if (op=="removed") {
        m_pHiddenTableModel->purgeTracks(m_directoryDAO.getDirId(dir));
        m_directoryDAO.purgeDirectory(dir);
        // this should signal to library to do a select on the currenty active
        // trackmodel
        emit(dirsChanged(op,dir));
    } else if (op=="update"){
        // this will be signaled from the library scanner if the db needs to be 
        // updated
        m_directoryDAO.addDirectory(dir);
        m_directoryDAO.updateTrackLocations(dir);
    } else if (op=="relocate") {
        QStringList dirs = dir.split("!(~)!");
        QString newFolder = dirs[0];
        QString oldFolder = dirs[1];
        m_directoryDAO.relocateDirectory(oldFolder,newFolder);
    } else {
        qDebug() << "MixxxLibraryFeature::slotDirsChanged "
                    "op not recognised";
    }
}

QStringList MixxxLibraryFeature::getDirs(){
    return m_directoryDAO.getDirs();
}
