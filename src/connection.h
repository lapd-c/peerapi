#ifndef __THROUGHNET_CONNECTION_H__
#define __THROUGHNET_CONNECTION_H__
#pragma once

#include <map>
#include <string>

#include "webrtc/base/nethelpers.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"

typedef std::map<int, std::string> Peers;

struct ConnectionObserver {
  virtual void OnSignedIn() = 0;  // Called when we're logged on.
  virtual void OnDisconnected() = 0;
  virtual void OnPeerConnected(int id, const std::string& name) = 0;
  virtual void OnPeerDisconnected(int peer_id) = 0;
  virtual void OnMessageFromPeer(int peer_id, const std::string& message) = 0;
  virtual void OnMessageSent(int err) = 0;
  virtual void OnServerConnectionFailure() = 0;

 protected:
  virtual ~ConnectionObserver() {}
};

class Connection : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
 public:
  enum State {
    NOT_CONNECTED,
    RESOLVING,
    SIGNING_IN,
    CONNECTED,
    SIGNING_OUT_WAITING,
    SIGNING_OUT,
  };

  Connection();
  ~Connection();

  int id() const;
  bool is_connected() const;
  const Peers& peers() const;

  void RegisterObserver(ConnectionObserver* callback);

  void Connect(const std::string& server, int port,
               const std::string& client_name);

  bool SendToPeer(int peer_id, const std::string& message);
  bool SendHangUp(int peer_id);
  bool IsSendingMessage();

  bool SignOut();

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

 protected:
  void DoConnect();
  void Close();
  void InitSocketSignals();
  bool ConnectControlSocket();
  void OnConnect(rtc::AsyncSocket* socket);
  void OnHangingGetConnect(rtc::AsyncSocket* socket);
  void OnMessageFromPeer(int peer_id, const std::string& message);

  // Quick and dirty support for parsing HTTP header values.
  bool GetHeaderValue(const std::string& data, size_t eoh,
                      const char* header_pattern, size_t* value);

  bool GetHeaderValue(const std::string& data, size_t eoh,
                      const char* header_pattern, std::string* value);

  // Returns true if the whole response has been read.
  bool ReadIntoBuffer(rtc::AsyncSocket* socket, std::string* data,
                      size_t* content_length);

  void OnRead(rtc::AsyncSocket* socket);

  void OnHangingGetRead(rtc::AsyncSocket* socket);

  // Parses a single line entry in the form "<name>,<id>,<connected>"
  bool ParseEntry(const std::string& entry, std::string* name, int* id,
                  bool* connected);

  int GetResponseStatus(const std::string& response);

  bool ParseServerResponse(const std::string& response, size_t content_length,
                           size_t* peer_id, size_t* eoh);

  void OnClose(rtc::AsyncSocket* socket, int err);

  void OnResolveResult(rtc::AsyncResolverInterface* resolver);

  ConnectionObserver* callback_;
  rtc::SocketAddress server_address_;
  rtc::AsyncResolver* resolver_;
  rtc::scoped_ptr<rtc::AsyncSocket> control_socket_;
  rtc::scoped_ptr<rtc::AsyncSocket> hanging_get_;
  std::string onconnect_data_;
  std::string control_data_;
  std::string notification_data_;
  std::string client_name_;
  Peers peers_;
  State state_;
  int my_id_;
};

#endif  // __THROUGHNET_CONNECTION_H__