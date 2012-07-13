#ifndef MUSICBRAINZCLIENT_H
#define MUSICBRAINZCLIENT_H

#include <QHash>
#include <QMap>
#include <QObject>
#include <QXmlStreamReader>
#include <QtNetwork>

#include "network.h"

// class QNetworkReply;

class MusicBrainzClient : public QObject {
  Q_OBJECT

  // Gets metadata for a particular MBID.
  // An MBID is created from a fingerprint using MusicDnsClient.
  // You can create one MusicBrainzClient and make multiple requests using it.
  // IDs are provided by the caller when a request is started and included in
  // the Finished signal - they have no meaning to MusicBrainzClient.

  public:
    MusicBrainzClient(QObject* parent = 0);

    struct Result {
        Result() : m_duration(0), m_track(0), m_year(-1) {}

        bool operator <(const Result& other) const {
        #define cmp(field) \
            if (field < other.field) return true; \
            if (field > other.field) return false;

        cmp(m_track);
        cmp(m_year);
        cmp(m_title);
        cmp(m_artist);
        return false;

        #undef cmp
        }

        bool operator ==(const Result& other) const {
        return m_title == other.m_title &&
                m_artist == other.m_artist &&
                m_album == other.m_album &&
                m_duration == other.m_duration &&
                m_track == other.m_track &&
                m_year == other.m_year;
        }

        QString m_title;
        QString m_artist;
        QString m_album;
        int m_duration;
        int m_track;
        int m_year;
    };
    typedef QList<Result> ResultList;


    // Starts a request and returns immediately.  Finished() will be emitted
    // later with the same ID.
    void Start(int id, const QString& mbid);
    void StartDiscIdRequest(const QString& discid);
    static void ConsumeCurrentElement(QXmlStreamReader& reader);

    // Cancels the request with the given ID.  Finished() will never be emitted
    // for that ID.  Does nothing if there is no request with the given ID.
    void Cancel(int id);

    // Cancels all requests.  Finished() will never be emitted for any pending
    // requests.
    void CancelAll();

  signals:
    // Finished signal emitted when fechting songs tags
    void Finished(int id, const MusicBrainzClient::ResultList& result);
    // Finished signal emitted when fechting album's songs tags using DiscId
    void Finished(const QString& artist, const QString album,
                  const MusicBrainzClient::ResultList& result);

  private slots:
    void RequestFinished();
    void DiscIdRequestFinished();

  private:
    struct Release {
        Release() : m_track(0), m_year(0) {}

        Result CopyAndMergeInto(const Result& orig) const {
            Result ret(orig);
            ret.m_album = m_album;
            ret.m_track = m_track;
            ret.m_year = m_year;
            return ret;
        }

        QString m_album;
        int m_track;
        int m_year;
    };

    static ResultList ParseTrack(QXmlStreamReader& reader);
    static void ParseArtist(QXmlStreamReader& reader, QString& artist);
    static Release ParseRelease(QXmlStreamReader& reader);
    static ResultList UniqueResults(const ResultList& results);

  private:
    static const QString m_TrackUrl;
    static const QString m_DiscUrl;
    static const QString m_DateRegex;
    static const int m_DefaultTimeout;
    
    QNetworkAccessManager m_network;
    NetworkTimeouts m_timeouts;
    QMap<QNetworkReply*, int> m_requests;
};

inline uint qHash(const MusicBrainzClient::Result& result) {
  return qHash(result.m_album) ^
         qHash(result.m_artist) ^
         result.m_duration ^
         qHash(result.m_title) ^
         result.m_track ^
         result.m_year;
}

#endif // MUSICBRAINZCLIENT_H
