#ifndef MUSICBRAINZCLIENT_H
#define MUSICBRAINZCLIENT_H

#include <QHash>
#include <QMap>
#include <QObject>
#include <QXmlStreamReader>

class QNetworkAccessManager;
class QNetworkReply;
class NetworkTimeouts;

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
        Result() : duration_msec_(0), track_(0), year_(-1) {}

        bool operator <(const Result& other) const {
        #define cmp(field) \
            if (field < other.field) return true; \
            if (field > other.field) return false;

        cmp(track_);
        cmp(year_);
        cmp(title_);
        cmp(artist_);
        return false;

        #undef cmp
        }

        bool operator ==(const Result& other) const {
        return title_ == other.title_ &&
                artist_ == other.artist_ &&
                album_ == other.album_ &&
                duration_msec_ == other.duration_msec_ &&
                track_ == other.track_ &&
                year_ == other.year_;
        }

        QString title_;
        QString artist_;
        QString album_;
        int duration_msec_;
        int track_;
        int year_;
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
        Release() : track_(0), year_(0) {}

        Result CopyAndMergeInto(const Result& orig) const {
        Result ret(orig);
        ret.album_ = album_;
        ret.track_ = track_;
        ret.year_ = year_;
        return ret;
        }

        QString album_;
        int track_;
        int year_;
    };

    static ResultList ParseTrack(QXmlStreamReader& reader);
    static void ParseArtist(QXmlStreamReader& reader, QString* artist);
    static Release ParseRelease(QXmlStreamReader& reader);
    static ResultList UniqueResults(const ResultList& results);

  private:
    static const char* m_pTrackUrl;
    static const char* m_pDiscUrl;
    static const char* m_pDateRegex;
    static const int m_DefaultTimeout;
    
    QNetworkAccessManager* m_network;
    NetworkTimeouts* m_ptimeouts;
    QMap<QNetworkReply*, int> m_requests;
};

inline uint qHash(const MusicBrainzClient::Result& result) {
  return qHash(result.album_) ^
         qHash(result.artist_) ^
         result.duration_msec_ ^
         qHash(result.title_) ^
         result.track_ ^
         result.year_;
}

#endif // MUSICBRAINZCLIENT_H
