#ifndef REMOTECONTROLHANDLER_H
#define REMOTECONTROLHANDLER_H

#include "handler.h"

#include <QString>

#include <gloox/iqhandler.h>

class RemoteControlInterface;

class RemoteControlHandler : public Handler,
                             public gloox::IqHandler {
public:
  RemoteControlHandler(RemoteControlInterface* interface);

  static const char* kXmlNs;

  void PlayPause(const QString& jid_resource);
  void Stop(const QString& jid_resource);
  void Next(const QString& jid_resource);
  void Previous(const QString& jid_resource);

  void QueryState(const QString& jid_resource);

  // Handler
  void Init(gloox::Client* client);

  // gloox::IqHandler
  bool handleIq(gloox::Stanza* stanza);
  bool handleIqID(gloox::Stanza* stanza, int context);

private:
  void SendIQ(const QString& jid_resource, gloox::StanzaSubType type,
              const QString& command);

private:
  RemoteControlInterface* interface_;
};

#endif // REMOTECONTROLHANDLER_H