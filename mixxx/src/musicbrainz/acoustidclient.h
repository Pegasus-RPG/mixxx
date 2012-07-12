#ifndef ACOUSTIDCLIENT_H
#define ACOUSTIDCLIENT_H

#include <QMap>
#include <QObject>

class NetworkTimeouts;

class QNetworkAccessManager;
class QNetworkReply;
class QXmlStreamReader;

class AcoustidClient : public QObject {
  Q_OBJECT

    // Gets a MBID from a Chromaprint fingerprint.
    // A fingerprint identifies one particular encoding of a song and is created
    // by Fingerprinter.  An MBID identifies the actual song and can be passed to
    // Musicbrainz to get metadata.
    // You can create one AcoustidClient and make multiple requests using it.
    // IDs are provided by the caller when a request is started and included in
    // the Finished signal - they have no meaning to AcoustidClient.

  public:
    AcoustidClient(QObject* parent = 0);

    // Network requests will be aborted after this interval.
    void SetTimeout(int msec);

    // Starts a request and returns immediately.  Finished() will be emitted
    // later with the same ID.
    void Start(int id, const QString& fingerprint, int duration);

    // Cancels the request with the given ID.  Finished() will never be emitted
    // for that ID.  Does nothing if there is no request with the given ID.
    void Cancel(int id);

    // Cancels all requests.  Finished() will never be emitted for any pending
    // requests.
    void CancelAll();

    QString ParseResult(QXmlStreamReader& reader);

  signals:
    void Finished(int id, const QString& mbid);

  private slots:
    void RequestFinished();

  private:
    static const char* m_pClientId;
    static const char* m_pUrl;
    static const int m_DefaultTimeout;

    QNetworkAccessManager* m_pnetwork;
    NetworkTimeouts* m_ptimeouts;
    QMap<QNetworkReply*, int> m_requests;
};

#endif // ACOUSTIDCLIENT_H
