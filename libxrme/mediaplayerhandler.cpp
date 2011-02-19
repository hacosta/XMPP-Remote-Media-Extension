#include "connection.h"
#include "mediaplayerhandler.h"
#include "mediaplayerinterface.h"
#include "remotecontrolhandler.h"

#include <QBuffer>
#include <QtDebug>

#include <gloox/client.h>
#include <gloox/disco.h>

MediaPlayerHandler::MediaPlayerHandler(MediaPlayerInterface* interface)
    : interface_(interface) {
  interface_->Attach(this);
}

void MediaPlayerHandler::StateChanged() {
  if (!connection_) {
    return;
  }

  State s = interface_->state();

  foreach (const Connection::Peer& peer, connection_->peers(Connection::Peer::RemoteControl)) {
    gloox::JID to(client_->jid().bareJID());
    to.setResource(peer.jid_resource_.toUtf8().constData());

    gloox::Tag* stanza = gloox::Stanza::createIqStanza(
          to, std::string(), gloox::StanzaIqSet, kXmlnsXrmeRemoteControl);
    gloox::Tag* state = new gloox::Tag(stanza, "state");
    state->addAttribute("xmlns", kXmlnsXrmeRemoteControl);
    state->addAttribute("playback_state", s.playback_state);
    state->addAttribute("position_millisec", s.position_millisec);
    state->addAttribute("volume", QString::number(s.volume, 'f').toUtf8().constData());
    state->addAttribute("can_go_next", s.can_go_next ? 1 : 0);
    state->addAttribute("can_go_previous", s.can_go_previous ? 1 : 0);
    state->addAttribute("can_seek", s.can_seek ? 1 : 0);

    gloox::Tag* metadata = new gloox::Tag(state, "metadata");
    metadata->addAttribute("title", s.metadata.title.toUtf8().constData());
    metadata->addAttribute("artist", s.metadata.artist.toUtf8().constData());
    metadata->addAttribute("album", s.metadata.album.toUtf8().constData());
    metadata->addAttribute("albumartist", s.metadata.albumartist.toUtf8().constData());
    metadata->addAttribute("composer", s.metadata.composer.toUtf8().constData());
    metadata->addAttribute("genre", s.metadata.genre.toUtf8().constData());
    metadata->addAttribute("track", s.metadata.track);
    metadata->addAttribute("disc", s.metadata.disc);
    metadata->addAttribute("year", s.metadata.year);
    metadata->addAttribute("length_millisec", s.metadata.length_millisec);
    metadata->addAttribute("rating", QString::number(s.metadata.rating, 'f').toUtf8().constData());

    client_->send(stanza);
  }
}

void MediaPlayerHandler::AlbumArtChanged() {
  if (!connection_) {
    return;
  }

  QImage image = interface_->album_art();

  // Scale the image down if it's too big
  if (!image.isNull() && (image.width() > kMaxAlbumArtSize ||
                          image.height() > kMaxAlbumArtSize)) {
    image = image.scaled(kMaxAlbumArtSize,kMaxAlbumArtSize,
                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }

  // Write the image data
  QByteArray image_data;
  if (!image.isNull()) {
    QBuffer buffer(&image_data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG");
  }

  // Convert to base64
  QByteArray image_data_base64 = image_data.toBase64();

  // Send the IQs
  foreach (const Connection::Peer& peer, connection_->peers(Connection::Peer::RemoteControl)) {
    gloox::JID to(client_->jid().bareJID());
    to.setResource(peer.jid_resource_.toUtf8().constData());

    gloox::Tag* stanza = gloox::Stanza::createIqStanza(
          to, std::string(), gloox::StanzaIqSet, kXmlnsXrmeRemoteControl);
    gloox::Tag* album_art = new gloox::Tag(stanza, "album_art");
    album_art->addAttribute("xmlns", kXmlnsXrmeRemoteControl);
    album_art->setCData(image_data_base64.constData());

    client_->send(stanza);
  }
}

void MediaPlayerHandler::Init(Connection* connection, gloox::Client* client) {
  Handler::Init(connection, client);

  client->registerIqHandler(this, kXmlnsXrmeMediaPlayer);
  client->disco()->addFeature(kXmlnsXrmeMediaPlayer);
}

bool MediaPlayerHandler::handleIq(gloox::Stanza* stanza) {
  // Ignore stanzas from anyone else
  if (stanza->from().bareJID() != client_->jid().bareJID()) {
    return false;
  }

  if (stanza->hasChild("playpause")) {
    interface_->PlayPause();
  } else if (stanza->hasChild("stop")) {
    interface_->Stop();
  } else if (stanza->hasChild("previous")) {
    interface_->Previous();
  } else if (stanza->hasChild("next")) {
    interface_->Next();
  } else if (stanza->hasChild("querystate")) {
    StateChanged();
    AlbumArtChanged();
  } else {
    qWarning() << "Unknown command received from"
               << stanza->from().resource().c_str()
               << stanza->xml().c_str();
    return false;
  }

  return true;
}
