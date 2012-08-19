// library.h
// Created 8/23/2009 by RJ Ryan (rryan@mit.edu)

// A Library class is a container for all the model-side aspects of the library.
// A library widget can be attached to the Library object by calling bindWidget.

#ifndef LIBRARY_H
#define LIBRARY_H

#include <QList>
#include <QObject>
#include <QAbstractItemModel>

#include "configobject.h"
#include "trackinfoobject.h"
#include "recording/recordingmanager.h"
#include "mixxxlibraryfeature.h"
#include "preparefeature.h"
#include "library/dao/directorydao.h"
#include "library/mountwatcher.h"

class TrackModel;
class TrackCollection;
class SidebarModel;
class LibraryFeature;
class LibraryTableModel;
class WLibrarySidebar;
class WLibrary;
class WSearchLineEdit;
class PromoTracksFeature;
class PlaylistFeature;
class CrateFeature;
class LibraryControl;
class MixxxKeyboard;

class Library : public QObject {
    Q_OBJECT
public:
    Library(QObject* parent,
            ConfigObject<ConfigValue>* pConfig,
            bool firstRun, RecordingManager* pRecordingManager);
    virtual ~Library();

    void bindWidget(WLibrarySidebar* sidebarWidget,
                    WLibrary* libraryWidget,
                    MixxxKeyboard* pKeyboard);
    void addFeature(LibraryFeature* feature, bool config=false);
    QList<TrackPointer> getTracksToAutoLoad();
    MixxxLibraryFeature* getpMixxxLibraryFeature();
    QStringList getDirs();

    // TODO(rryan) Transitionary only -- the only reason this is here is so the
    // waveform widgets can signal to a player to load a track. This can be
    // fixed by moving the waveform renderers inside player and connecting the
    // signals directly.
    TrackCollection* getTrackCollection() {
        return m_pTrackCollection;
    }

    //static Library* buildDefaultLibrary();

public slots:
    void slotShowTrackModel(QAbstractItemModel* model);
    void slotSwitchToView(const QString& view);
    void slotLoadTrack(TrackPointer pTrack);
    void slotLoadTrackToPlayer(TrackPointer pTrack, QString group);
    void slotRestoreSearch(const QString& text);
    void slotRefreshLibraryModels();
    void slotCreatePlaylist();
    void slotCreateCrate();
    void slotDirsChanged(QString,QString);
    void slotFoundNewStorage(QStringList);
    void slotRemovedStorage(QStringList);
    void slotLoadTrackFailed(TrackPointer pTrack);
    
signals:
    void showTrackModel(QAbstractItemModel* model);
    void switchToView(const QString& view);
    void loadTrack(TrackPointer pTrack);
    void loadTrackToPlayer(TrackPointer pTrack, QString group);
    void restoreSearch(const QString&);
    void configChanged(QString, QString);
    void dirsChanged(QString,QString);
    void availableDirsChanged(QList<int>);
    void loadTrackFailed(TrackPointer);

private:
    void purgeTracks(const int dirId);

    ConfigObject<ConfigValue>* m_pConfig;
    SidebarModel* m_pSidebarModel;
    TrackCollection* m_pTrackCollection;
    QList<LibraryFeature*> m_features;
    const static QString m_sTrackViewName;
    const static QString m_sAutoDJViewName;
    MixxxLibraryFeature* m_pMixxxLibraryFeature;
    PlaylistFeature* m_pPlaylistFeature;
    CrateFeature* m_pCrateFeature;
    PromoTracksFeature* m_pPromoTracksFeature;
    PrepareFeature* m_pPrepareFeature;
    LibraryControl* m_pLibraryControl;
    RecordingManager* m_pRecordingManager;
    TrackModel* m_ptrackModel;
    DirectoryDAO m_directoryDAO;

    MountWatcher m_mountwatcher;
    QStringList m_availableDirs;
    QStringList m_unavailableDirs;
};

#endif /* LIBRARY_H */
