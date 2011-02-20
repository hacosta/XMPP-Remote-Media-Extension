package com.purplehatstands.libxrme;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.jivesoftware.smack.ConnectionConfiguration;
import org.jivesoftware.smack.Roster;
import org.jivesoftware.smack.RosterListener;
import org.jivesoftware.smack.XMPPConnection;
import org.jivesoftware.smack.XMPPException;
import org.jivesoftware.smack.packet.Presence;
import org.jivesoftware.smack.provider.ProviderManager;
import org.jivesoftware.smackx.ServiceDiscoveryManager;
import org.jivesoftware.smackx.packet.DiscoverInfo;
import org.jivesoftware.smackx.packet.DiscoverItems;

import android.util.Log;

public final class Connection {
  public class Peer {
    public static final int CAP_NONE          = 0x00;
    public static final int CAP_MEDIAPLAYER   = 0x01;
    public static final int CAP_REMOTECONTROL = 0x02;
    public static final int CAP_MEDIASTORAGE  = 0x04;
    
    public String agent_name_;
    public String jid_resource_;
    public int caps_;
    
    public Peer() {
      caps_ = CAP_NONE;
    }
  }
  
  public String username() {
    return username_;
  }
  
  public void set_username(String username) {
    username_ = username;
  }
  
  public String password() {
    return password_;
  }
  
  public void set_password(String password) {
    password_ = password;
  }
  
  public String agent_name() {
    return agent_name_;
  }
  
  public void set_agent_name(String agent_name) {
    agent_name_ = agent_name;
  }
  
  public String server() {
    return server_;
  }
  
  public void set_server(String server) {
    server_ = server;
  }
  
  public String jid_resource() {
    return jid_resource_;
  }
  
  public void set_jid_resource(String jid_resource) {
    jid_resource_ = jid_resource;
  }
  
  public String jid_host() {
    return jid_host_;
  }
  
  public void set_jid_host(String jid_host) {
    jid_host_ = jid_host;
  }
  
  public boolean is_verbose() {
    return verbose_;
  }
  
  public void set_verbose(boolean verbose) {
    verbose_ = verbose;
  }
  
  public void SetMediaPlayer(MediaPlayerInterface iface) {
    if (media_player_ != null) {
      Log.w(TAG, "Connection::SetMediaPlayer: this connection already has a MediaPlayerInterface set");
      return;
    }
    
    if (iface == null) {
      Log.w(TAG, "Connection::SetMediaPlayer: called with null interface");
      return;
    }
    
    media_player_ = iface;
    handlers_.add(new MediaPlayerHandler(media_player_));
  }
  
  public void SetRemoteControl(RemoteControlInterface iface) {
    if (remote_control_ != null) {
      Log.w(TAG, "Connection::SetRemoteControl: this connection already has a RemoteControlInterface set");
      return;
    }
    
    if (iface == null) {
      Log.w(TAG, "Connection::SetRemoteControl: called with null interface");
      return;
    }
    
    remote_control_ = iface;
    handlers_.add(new RemoteControlHandler(remote_control_));
  }
  
  public void Connect() throws XMPPException {
    if (username_ == null || username_.length() == 0 ||
        password_ == null || password_.length() == 0 ||
        agent_name_ == null || agent_name_.length() == 0) {
      throw new XMPPException("Connection::Connect: A required field (username/password/agent_name) was empty");
    }
    
    ConnectionConfiguration config = new ConnectionConfiguration(server_, 5222, jid_host_);
    config.setDebuggerEnabled(true);
    
    xmpp_ = new XMPPConnection(config);
    
    // Set providers.  Maybe this should be done statically?
    ProviderManager.getInstance().addIQProvider("query", "http://jabber.org/protocol/disco#items", DiscoverItems.class);
    ProviderManager.getInstance().addIQProvider("query", "http://jabber.org/protocol/disco#info", DiscoverInfo.class);
    ProviderManager.getInstance().addIQProvider("state", Common.XMLNS_XRME_REMOTECONTROL, new State.Parser());
    ProviderManager.getInstance().addIQProvider("album_art", Common.XMLNS_XRME_REMOTECONTROL, new AlbumArt.Parser());
    
    // Connect
    xmpp_.connect();
    xmpp_.login(username_, password_, jid_resource_);
    
    // Set discovery information
    ServiceDiscoveryManager.setIdentityName(agent_name_);
    ServiceDiscoveryManager.setIdentityType("bot");
    ServiceDiscoveryManager disco_manager = ServiceDiscoveryManager.getInstanceFor(xmpp_);
    if (disco_manager == null) {
      disco_manager = new ServiceDiscoveryManager(xmpp_);
    }
    disco_manager.addFeature(Common.XMLNS_XRME);
    
    // Add a roster listener
    Roster roster = xmpp_.getRoster();
    roster.addRosterListener(new RosterListener() {
      // Ignored events public void entriesAdded(Collection<String> addresses) {}
      public void entriesDeleted(Collection<String> addresses) {}
      public void entriesUpdated(Collection<String> addresses) {}
      public void entriesAdded(Collection<String> addresses) {}
      public void presenceChanged(Presence presence) {
        Log.i(TAG, "Presence changed: " + presence.getFrom() + " " + presence);
      }
    });
    
    // Initialise the handlers
    for (Handler handler : handlers_) {
      handler.Init(this, xmpp_);
    }
    
    // Set presence
    Presence presence = new Presence(Presence.Type.available);
    presence.setPriority(-128);
    xmpp_.sendPacket(presence);
  }
  
  private static final String TAG = "libxrme";
  
  private String username_;
  private String password_;
  private String agent_name_;
  private String server_ = "talk.google.com";
  private String jid_resource_ = "xrmeagent";
  private String jid_host_ = "gmail.com";
  private boolean verbose_ = false;
  
  private MediaPlayerInterface media_player_;
  private RemoteControlInterface remote_control_;
  private List<Handler> handlers_ = new ArrayList<Handler>();

  private XMPPConnection xmpp_;
}
