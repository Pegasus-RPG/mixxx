
#ifndef DLGTRACKINFO_H
#define DLGTRACKINFO_H

#include <QDialog>
#include <QMutex>
#include <QHash>
#include <QList>

#include "ui_dlgtrackinfo.h"
#include "trackinfoobject.h"
#include "controlbeat.h"
#include "musicbrainz/tagfetcher.h"
#include "dlgtagfetcher.h"

class Cue;

class DlgTrackInfo : public QDialog, public Ui::DlgTrackInfo {
    Q_OBJECT
  public:
    DlgTrackInfo(QWidget* parent, TagFetcher& TagFetcher,
                 DlgTagFetcher& DlgTagFetcher);
    virtual ~DlgTrackInfo();

  public slots:
    // Not thread safe. Only invoke via AutoConnection or QueuedConnection, not
    // directly!
    void loadTrack(TrackPointer pTrack);

  signals:
    void next();
    void previous();

  private slots:
    void slotNext();
    void slotPrev();
    void apply();
    void cancel();
    void trackUpdated();
    void fetchTag();

    void cueActivate();
    void cueDelete();

    void slotBpmDouble();
    void slotBpmHalve();
    void slotBpmTap();

    void reloadTrackMetadata();

  private:
    void populateFields(TrackPointer pTrack);
    void populateCues(TrackPointer pTrack);
    void unloadTrack(bool save);
    void clear();
    void init();

    QHash<int, Cue*> m_cueMap;
    TrackPointer m_pLoadedTrack;

    CSAMPLE m_bpmTapFilter[filterLength];
    QTime m_bpmTapTimer;

    QMutex m_mutex;
    DlgTagFetcher& m_DlgTagFetcher;
    TagFetcher& m_TagFetcher;

};

#endif /* DLGTRACKINFO_H */

