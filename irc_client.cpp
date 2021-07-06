#include "irc_client.h"
//#include <ETH.h>
#include <Client.h>

//#define DEBUG_IRC
//#define DEBUG_IRC_VERBOSE
#define IRC_BUFSIZE 200  //bytes (default 200)
#define IRC_RECONNECT_INTERVAL 60000  //milliseconds
#define IRC_TIMEOUT 120000  //milliseconds
#define NICK_RETRY 20000
#define NICKSERV_TIMEOUT 10000  //milliseconds

Client * _irc_ethClient = nullptr;
unsigned long _irc_last_connect_attempt = 0;
unsigned long _irc_last_line_from_server = 0;
#define WAITING_FOR_NICKSERV 0
#define NOT_IDENTIFIED 1
#define IDENTIFY_SENT 2
#define IDENTIFY_CONFIRMED 3
int _irc_identified = WAITING_FOR_NICKSERV;
boolean _irc_performed_on_connect = false;
boolean _irc_pinged = false;
boolean _irc_away_status = false;
char _irc_input_buffer[IRC_BUFSIZE];
int _irc_input_buffer_pointer = 0;
unsigned long _line_start_time = 0;
char _nickserv_password[20] = "";
char _irc_nick[20] = "irc_client";
char _version[32] = "irc client";
char _irc_server[32] = "";
int _irc_server_port = 6667;

void ircSetClient(Client &client) {
  _irc_ethClient = &client;
  #ifdef DEBUG_IRC_VERBOSE
  ircDebug(F("Network Client set."));
  #endif
}

void ircServer(const char * server, int port) {  //set server for implicit auto-reconnect
  char buf[100];
  snprintf_P(buf, sizeof(buf), PSTR("Server set to %s:%u"), server, port);
  ircDebug(buf);
  snprintf(_irc_server, sizeof(_irc_server), "%s", server);
  _irc_server_port = port;
}

boolean ircConnect(const char * server, int port) {
  return(ircConnect(server, port, false));
}

boolean ircConnect(Client &client, const char * server, int port, boolean reconnect) {
  _irc_ethClient = &client;
  return(ircConnect(server, port, reconnect));
}

boolean ircConnect(const char * server, int port, boolean reconnect) {
  if (reconnect) {
    snprintf(_irc_server, sizeof(_irc_server), "%s", server);
    _irc_server_port = port;
  } else {
    snprintf(_irc_server, sizeof(_irc_server), "");  //empty string means we don't attempt to reconnect
  }
  if (_irc_ethClient == NULL) {
    ircDebug(F("Client is not defined!"));
    return(false);
  }
  if (!_irc_ethClient->connected()) {
    char buf[100];
    snprintf_P(buf, sizeof(buf), PSTR("Connecting to %s:%u"), server, port);
    ircDebug(buf);
    _irc_last_connect_attempt = millis();
    _irc_away_status = false;
    _irc_performed_on_connect = false;
    _irc_pinged = false;
    _irc_identified = WAITING_FOR_NICKSERV;
    memset(_irc_input_buffer, 0, sizeof(_irc_input_buffer));
    _irc_input_buffer_pointer = 0;
    if (_irc_ethClient->connect(server, port)) {
      _irc_last_line_from_server = millis();
      ircNetworkLight();
      snprintf_P(buf, sizeof(buf), PSTR("Connected to %s:%u"), server, port);
      ircDebug(buf);
      _irc_ethClient->print(F("USER "));
	  //remove illegal characters from username
	  snprintf(buf, sizeof(buf), "%s", _irc_nick);
	  stringRemoveNonPermitted(buf);
      _irc_ethClient->print(buf);
      _irc_ethClient->print(F(" 8 * :"));
      _irc_ethClient->print(_version);
      _irc_ethClient->print(F("\r\nNICK "));
      _irc_ethClient->print(_irc_nick);
      _irc_ethClient->print(F("\r\n"));
      return(true);
    } else {
      snprintf_P(buf, sizeof(buf), PSTR("Connection failed!"));
      ircDebug(buf);
      return(false);
    }
  } else {
    ircDebug(F("Already connected!"));
    return(true);
  }
}

void ircSetNick(const char * name) {  //fixme should this do a NICK after we're connected?
  snprintf(_irc_nick, sizeof(_irc_nick), "%s", name);
  char buf[100];
  snprintf_P(buf, sizeof(buf), PSTR("Set nick to %s"), _irc_nick);
  ircDebug(buf);
}

void ircSetVersion(const char * version) {  //fixme should this do a NICK after we're connected?
  snprintf(_version, sizeof(_version), "%s", version);
  char buf[100];
  snprintf_P(buf, sizeof(buf), PSTR("Set version to: %s"), _version);
  ircDebug(buf);
}

void ircSetNickServPassword(const char * password) {
  snprintf(_nickserv_password, sizeof(_nickserv_password), "%s", password);
  char buf[100];
  snprintf_P(buf, sizeof(buf), PSTR("Setting nickserv password to \"%s\""), _nickserv_password);
  ircDebug(buf);
}

void doIRC() {
  if (_irc_ethClient == NULL) {
    return;
  }
  if (!_irc_ethClient->connected() && strlen(_irc_server) > 0 && (!_irc_last_connect_attempt || millis() - _irc_last_connect_attempt >= IRC_RECONNECT_INTERVAL)) {  //(re)connect to irc server
    char buf[100];
    snprintf_P(buf, sizeof(buf), PSTR("Reconnecting to %s:%u..."), _irc_server, _irc_server_port);
    ircDebug(buf);
    _irc_last_connect_attempt = millis();
    _irc_away_status = false;
    _irc_performed_on_connect = false;
    _irc_pinged = false;
    _irc_identified = WAITING_FOR_NICKSERV;
    memset(_irc_input_buffer, 0, sizeof(_irc_input_buffer));
    _irc_input_buffer_pointer = 0;
    if (_irc_ethClient->connect(_irc_server, _irc_server_port)) {
      _irc_last_line_from_server = millis();
      ircNetworkLight();
      snprintf_P(buf, sizeof(buf), PSTR("Connected to %s:%u"), _irc_server, _irc_server_port);
      ircDebug(buf);
      _irc_ethClient->print(F("USER "));
	  //remove illegal characters from username
      snprintf(buf, sizeof(buf), "%s", _irc_nick);
      stringRemoveNonPermitted(buf);
      _irc_ethClient->print(buf);
      _irc_ethClient->print(F(" 8 * :"));
      _irc_ethClient->print(_version);
      _irc_ethClient->print(F("\r\nNICK "));
      _irc_ethClient->print(_irc_nick);
      _irc_ethClient->print(F("\r\n"));
    } else {
      snprintf_P(buf, sizeof(buf), PSTR("Connection failed!"));
      ircDebug(buf);
      ircOnDisconnect();
    }
  }
  if (_irc_ethClient->connected()) {
    if (strlen(_nickserv_password) > 0 && _irc_identified == NOT_IDENTIFIED && _irc_pinged) {  // we've pinged and nickserv wants auth, now identify with nickserv
      ircNetworkLight();
	  #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
      ircDebug(F("Identifying..."));
	  #endif
      char buf[50];
      snprintf_P(buf, sizeof(buf), PSTR("identify %s"), _nickserv_password);
      sendIRCMessage("nickserv", buf);
      _irc_identified = IDENTIFY_SENT;
    }
    if (!_irc_performed_on_connect && _irc_pinged && (_irc_identified == IDENTIFY_CONFIRMED || millis() - _irc_last_connect_attempt > NICKSERV_TIMEOUT)) {  //we're identified (or timed out), now join channels
      if (_irc_identified != IDENTIFY_CONFIRMED) {
        #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
		ircDebug(F("NickServ timed out!"));
		#endif
        _irc_identified = WAITING_FOR_NICKSERV;
      }
      #ifdef DEBUG_IRC
      ircDebug(F("Performing onConnect()"));
      #endif
      _irc_performed_on_connect = true;
      ircOnConnect();
    }
    while (_irc_ethClient->available()) {
      if (_irc_input_buffer_pointer == 0) {
        _line_start_time = millis();  //reset this on first char of line
      }
      char c = _irc_ethClient->read();
      if (_irc_input_buffer_pointer < sizeof(_irc_input_buffer)) {
        _irc_input_buffer[_irc_input_buffer_pointer] = c;
        _irc_input_buffer_pointer++;
      }
      if (c == '\n') {  //got end of line.
        _irc_last_line_from_server = millis();
        boolean buffer_overflow = false;
        _irc_input_buffer[_irc_input_buffer_pointer - 2] = '\0';
        #ifdef DEBUG_IRC_VERBOSE
        char buf[IRC_BUFSIZE+100];
        snprintf_P(buf, sizeof(buf), PSTR("Got EOL after %5ums.  Buffer contains %u bytes: %s"), _irc_last_line_from_server - _line_start_time, _irc_input_buffer_pointer, _irc_input_buffer);
        ircDebug(buf);
        #endif
        if (_irc_input_buffer_pointer >= sizeof(_irc_input_buffer)) {
          #ifdef DEBUG_IRC_VERBOSE
          ircDebug(F(" (OVERFLOW!)"));
          #endif
          buffer_overflow = true;
        }
        #ifdef DEBUG_IRC_VERBOSE
        ircDebug(F("\r\n"));
        #endif
        parseIRCInput(buffer_overflow);
        //reset buffer for next time
        memset(_irc_input_buffer, 0, sizeof(_irc_input_buffer));
        _irc_input_buffer_pointer = 0;
      }
    }
    if (millis() - _irc_last_line_from_server >= IRC_TIMEOUT) {
      ircDebug(F("Connection appears comotose, closing!"));
      _irc_ethClient->stop();
      _irc_pinged = false;
      _irc_performed_on_connect = false;
      ircOnDisconnect();
    }
  } else {  //client is not connected
    if (_irc_pinged) {
      ircDebug(F("Disconnected for some reason!"));
      _irc_last_connect_attempt = millis();
      _irc_pinged = false;
      _irc_performed_on_connect = false;
      ircOnDisconnect();
    }
  }
}

void parseIRCInput(boolean buffer_overflow) {  //_irc_input_buffer contains a line of IRC, do something with it
  char * pch;
  if (_irc_input_buffer[0] == ':') {  //if the line starts with a ':', as most lines do...
    //get the from field
    int start_pos = 1;
    int length = strcspn(_irc_input_buffer+start_pos, " ");
    char * from = &_irc_input_buffer[start_pos];
    _irc_input_buffer[start_pos+length] = '\0';
    //split the from string to nick and username, if applicable
    char * user = &from[0];
    int nick_length = strcspn(from, "!");
    if (nick_length != strlen(from)) {
      from[nick_length] = '\0';
      user = &from[nick_length + 1];
    }
    #ifdef DEBUG_IRC_VERBOSE
    char buf[IRC_BUFSIZE+100];
    if (user != NULL) {
      snprintf_P(buf, sizeof(buf), PSTR("Message from: %s (%s)"), from, user);
    } else {
      snprintf_P(buf, sizeof(buf), PSTR("Message from: %s"), from);
    }
    ircDebug(buf);
    #endif
    start_pos += length + 1;  //ready for next part of string
    //get the type
    length = strcspn(_irc_input_buffer+start_pos, " ");
    char * type = &_irc_input_buffer[start_pos];
    _irc_input_buffer[start_pos+length] = '\0';
    #ifdef DEBUG_IRC_VERBOSE
    snprintf_P(buf, sizeof(buf), PSTR("Message type: %s"), type);
    ircDebug(buf);
    #endif
    start_pos += length + 1;  //ready for next part of string
    //get the destination
    length = strcspn(_irc_input_buffer+start_pos, " ");
    char * to = &_irc_input_buffer[start_pos];
    _irc_input_buffer[start_pos+length] = '\0';
    #ifdef DEBUG_IRC_VERBOSE
    snprintf_P(buf, sizeof(buf), PSTR("Message   to: %s"), to);
    ircDebug(buf);
    #endif
    start_pos += length + 1;  //ready for next part of string
    //get the message body
    if (_irc_input_buffer[start_pos] == ':') {  //discard leading ':'
      start_pos++;
    }
    char * message = &_irc_input_buffer[start_pos];
    #ifdef DEBUG_IRC_VERBOSE
    snprintf_P(buf, sizeof(buf), PSTR("Message body: %s"), message);
    ircDebug(buf);
    #endif
    if (strcmp(type, "PRIVMSG") == 0) {  //we have a PRIVMSG
      if (strcmp(to, _irc_nick) == 0) { //this is a private /MSG
        ircNetworkLight();
        if (message[0] == '\1') {  //is CTCP
          memmove(message, message+1, strlen(message));  //move the string 1 character to the left, overwriting first delimiter
          length = strcspn(message, "\1");
          message[length] = '\0'; //terminate at '\1', if present.
          #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
          char buf[100];
          snprintf_P(buf, sizeof(buf), PSTR("<%s> CTCP: %s"), from, message);
          ircDebug(buf);
          #endif
          ircOnCTCP(from, to, message);
        } else {  //normal private /MSG
          #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
          char buf[100];
          snprintf_P(buf, sizeof(buf), PSTR("MSG <%s> %s"), from, message);
          ircDebug(buf);
          #endif
          ircOnPrivateMessage(from, message);
        }
      } else {  //channel message
        if (message[0] == '\1') {  //is CTCP
          memmove(message, message+1, strlen(message));  //move the string 1 character to the left, overwriting first delimiter
          length = strcspn(message, "\1");  //terminate at '\1', if present.
          message[length] = '\0';
          #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
          ircNetworkLight();
          char buf[100];
          snprintf_P(buf, sizeof(buf), PSTR("%s <%s> CTCP: %s"), to, from, message);
          ircDebug(buf);
          #endif
          ircOnCTCP(from, to, message);
        } else {  //normal message
          #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
          ircNetworkLight();;
          char buf[100];
          snprintf_P(buf, sizeof(buf), PSTR("%s <%s> %s"), to, from, message);
          ircDebug(buf);
          #endif
          ircOnChannelMessage(from, to, message);
        }
      }
      return;
    } else if (strcmp(type, "NOTICE") == 0) {  //we have a NOTICE
      if (strcmp(to, _irc_nick) == 0) { //this is a private NOTICE
        #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
        ircNetworkLight();
        char buf[100];
        snprintf_P(buf, sizeof(buf), PSTR("NOTICE <%s> %s"), from, message);
        ircDebug(buf);
        #endif
        if (strcmp(from, "NickServ") == 0) {  //notice from NickServ
          ircNetworkLight();
          pch = strstr_P(message, "Password accepted");  //
          if (pch != NULL) {
            _irc_identified = IDENTIFY_CONFIRMED;
            char buf[100];
            snprintf_P(buf, sizeof(buf), PSTR("Identified: %s"), message);
            ircDebug(buf);
            return;
          }
          pch = strstr_P(message, "This nickname is registered and protected.");  //
          if (pch != NULL) {
            _irc_identified = NOT_IDENTIFIED;
			#ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
            ircDebug(F("Nickserv wants auth."));
            if (strlen(_nickserv_password) == 0) {
              ircDebug(F("WARNING: No nickserv password is set!\r\n"));
            }
			#endif
            return;
          }
          char buf[100];
          snprintf_P(buf, sizeof(buf), PSTR("Ignoring NiceServ NOTICE: %s"), message);
          ircDebug(buf);
          return;
	    }
        //some other private notice
        #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
        ircNetworkLight();
        snprintf_P(buf, sizeof(buf), PSTR("Got private notice from <%s> %s"), from, message);
        ircDebug(buf);
        #endif
        ircOnPrivateNotice(from, message);
      } else {  //channel notice
        #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
        ircNetworkLight();
        char buf[100];
        snprintf_P(buf, sizeof(buf), PSTR("%s <%s> NOTICE: %s"), to, from, message);
        ircDebug(buf);
        #endif
        if (strcmp(to, "AUTH") == 0) { //this is an AUTH notice
          #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
          ircNetworkLight();
          char buf[100];
          snprintf_P(buf, sizeof(buf), PSTR("Ignoring AUTH NOTICE: %s"), message);
          ircDebug(buf);
          #endif
          return;
        } else {
          ircOnChannelNotice(from, to, message);
        }

      }
    } else if (strcmp(type, "JOIN") == 0) {  //we have a JOIN
      #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
      ircNetworkLight();
      char buf[100];
      snprintf_P(buf, sizeof(buf), PSTR("%s joined %s"), from, to+1);
      ircDebug(buf);
      #endif
      return;
    } else if (strcmp(type, "MODE") == 0) {  //we have a MODE
      #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
      ircNetworkLight();
      char buf[100];
      snprintf_P(buf, sizeof(buf), PSTR("%s set mode: %s on %s"), from, message, to);
      ircDebug(buf);
      #endif
      char test_string[20];
      strcpy(test_string, "+v ");
      strcat(test_string, _irc_nick);
      pch = strstr_P(message, test_string);
      if (pch != NULL) {
		#ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
        char buf[100];
        snprintf_P(buf, sizeof(buf), PSTR("Got voice on %s"), to);
        ircDebug(buf);
		#endif
        ircOnVoice(from, to);
        return;
      }
      strcpy(test_string, "+o ");
      strcat(test_string, _irc_nick);
      pch = strstr_P(message, test_string);
      if (pch != NULL) {
		#ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
        char buf[100];
        snprintf_P(buf, sizeof(buf), PSTR("Got ops on %s"), to);
        ircDebug(buf);
		#endif
        ircOnOp(from, to);
        return;
      }
      //handle other mode here?
      return;
    } else if (strcmp(type, "301") == 0) {  //user is AWAY
      #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
      char buf[100];
      snprintf_P(buf, sizeof(buf), PSTR("RPL_AWAY: %s"), message);
      ircDebug(buf);
      #endif
    } else if (strcmp(type, "305") == 0 && strcmp(to, _irc_nick) == 0) {  //we are no longer AWAY
      #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
      char buf[100];
      snprintf_P(buf, sizeof(buf), PSTR("RPL_UNAWAY: %s"), message);
      ircDebug(buf);
      #endif
    } else if (strcmp(type, "306") == 0 && strcmp(to, _irc_nick) == 0) {  //we are now AWAY
      #ifdef DEBUG_IRC || DEBUG_IRC_VERBOSE
      char buf[100];
      snprintf_P(buf, sizeof(buf), PSTR("RPL_NOWAWAY: %s"), message);
      ircDebug(buf);
      #endif
    }
    //test for error codes
    int err = atoi(type);
    if (err >= 400 && err <= 502) {
      ircNetworkLight();
      char error_string[100];
      if (err >= 401 && err <= 403) {
        snprintf_P(error_string, sizeof(error_string), PSTR("Target does not exist."));
      } if (err == 404) {
        snprintf_P(error_string, sizeof(error_string), PSTR("Cannot send to channel."));
      } else if (err == 422) {
        snprintf_P(error_string, sizeof(error_string), PSTR("No MOTD"));
      } else if (err >= 431 && err <= 436) {
        snprintf_P(error_string, sizeof(error_string), PSTR("Nick error."));
      } else {
        snprintf_P(error_string, sizeof(error_string), PSTR("Unknown error, continuing..."));
      }
	  char buf[100];
	  snprintf_P(buf, sizeof(buf), PSTR("Message is an error (%i): %s"), err, error_string);
      ircDebug(buf);
	  if (err >= 431 && err <= 436) {  //disconnect on nick error
		_irc_last_connect_attempt = millis() - IRC_RECONNECT_INTERVAL + NICK_RETRY;
        _irc_ethClient->stop();
        snprintf_P(buf, sizeof(buf), PSTR("Connection closed."));
        ircDebug(buf);
        return;
	  }
    }
    return;
  }
  //respond to server PING
  pch = strstr_P(_irc_input_buffer,"PING :");
  if (pch == &_irc_input_buffer[0]) {
    ircNetworkLight();
    _irc_last_line_from_server = millis();
    char ping_string[31];
    strlcpy (ping_string, pch+6, 30);
    #ifdef DEBUG_IRC
    char buf[100];
    snprintf_P(buf, sizeof(buf), PSTR("Got ping: %s, sending pong"), ping_string);
    ircDebug(buf);
    #endif
    _irc_ethClient->print("PONG ");
    _irc_ethClient->print(ping_string);
    _irc_ethClient->print("\r\n");
    _irc_pinged = true;
    return;
  }
  //now test for ERROR
  pch = strstr_P(_irc_input_buffer,"ERROR :");
  if (pch == &_irc_input_buffer[0]) {
    ircNetworkLight();
    char buf[IRC_BUFSIZE+20];
    snprintf_P(buf, sizeof(buf), PSTR("Got ERROR from server: %s"), pch+7);
    ircDebug(buf);
    _irc_last_connect_attempt = millis();  //prevent immediate reconnect for rate limiting
    _irc_ethClient->stop();
    snprintf_P(buf, sizeof(buf), PSTR("Connection closed."));
    ircDebug(buf);
    return;
  }
  //if we get here, we don't know how to parse the line.
  char buf[IRC_BUFSIZE+20];
  snprintf_P(buf, sizeof(buf), PSTR("Could not parse %s"), _irc_input_buffer);
  ircDebug(buf);
}

void joinIRCChannel(const __FlashStringHelper *channel) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("JOIN "));
    _irc_ethClient->print(FPSTR((PGM_P)channel));
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void joinIRCChannel(const char *channel) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("JOIN "));
    _irc_ethClient->print(channel);
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void sendIRCMessage(const __FlashStringHelper *target, const __FlashStringHelper *message) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("PRIVMSG "));
    _irc_ethClient->print(FPSTR((PGM_P)target));
    _irc_ethClient->print(F(" :"));
    _irc_ethClient->print(FPSTR((PGM_P)message));
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void sendIRCMessage(const char *target, const __FlashStringHelper *message) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("PRIVMSG "));
    _irc_ethClient->print(target);
    _irc_ethClient->print(F(" :"));
    _irc_ethClient->print(FPSTR((PGM_P)message));
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void sendIRCMessage(const char *target, const char *message) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("PRIVMSG "));
    _irc_ethClient->print(target);
    _irc_ethClient->print(F(" :"));
    _irc_ethClient->print(message);
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void sendIRCNotice(const __FlashStringHelper *target, const __FlashStringHelper *message) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("NOTICE "));
    _irc_ethClient->print(FPSTR((PGM_P)target));
    _irc_ethClient->print(F(" :"));
    _irc_ethClient->print(FPSTR((PGM_P)message));
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void sendIRCNotice(const char *target, const __FlashStringHelper *message) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("NOTICE "));
    _irc_ethClient->print(target);
    _irc_ethClient->print(F(" :"));
    _irc_ethClient->print(FPSTR((PGM_P)message));
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void sendIRCNotice(const char *target, const char *message) {
  if (_irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    _irc_ethClient->print(F("NOTICE "));
    _irc_ethClient->print(target);
    _irc_ethClient->print(F(" :"));
    _irc_ethClient->print(message);
    _irc_ethClient->print(F("\r\n"));
    _irc_last_line_from_server = millis();
  }
}

void setAway() {
  if (!_irc_away_status && _irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    ircNetworkLight();
    _irc_ethClient->println(F("AWAY :Idle"));
    _irc_last_line_from_server = millis();
    _irc_away_status = true;
  }
}

void unAway() {
  if (_irc_away_status && _irc_ethClient != NULL && _irc_ethClient->connected() && _irc_pinged) {
    ircNetworkLight();
    _irc_ethClient->println(F("AWAY"));
    _irc_last_line_from_server = millis();
    _irc_away_status = false;
  }
}

boolean ircConnected() {
  return(ircConnected(false));
}

boolean ircConnected(boolean in_progress) {
  if (_irc_ethClient == NULL) {
	return(false);
  }
  if (!in_progress) {
    return(_irc_ethClient->connected() && _irc_pinged && _irc_performed_on_connect);
  } else {
    return(_irc_ethClient->connected());
  }
}

void ircDisconnect() {
  if (_irc_ethClient != NULL) {
    _irc_ethClient->stop();
  }
  snprintf(_irc_server, sizeof(_irc_server), "");  //empty string means we don't attempt to reconnect
}

void (*fpOnConnect)();
void ircSetOnConnect(void (*fp)(void) ) {
  fpOnConnect = fp; 
}
void ircOnConnect() {
  if( 0 != fpOnConnect ) {
    (*fpOnConnect)();
  } else {
    //do nothing
  }
}

void (*fpOnDisconnect)();
void ircSetOnDisconnect(void (*fp)(void) ) {
  fpOnDisconnect = fp; 
}
void ircOnDisconnect() {
  if( 0 != fpOnDisconnect ) {
    (*fpOnDisconnect)();
  } else {
    //do nothing
  }
}

void (*fpOnPrivateMessage)(const char *, const char *);
void ircSetOnPrivateMessage(void (*fp)(const char *, const char *)) {
  fpOnPrivateMessage = fp;
}
void ircOnPrivateMessage(const char * from, const char * message) {
  if( 0 != fpOnPrivateMessage ) {
    (*fpOnPrivateMessage)(from, message);
  } else {
    //do nothing
  }
}

void (*fpOnChannelMessage)(const char *, const char *, const char *);
void ircSetOnChannelMessage(void (*fp)(const char *, const char *, const char *)) {
  fpOnChannelMessage = fp;
}
void ircOnChannelMessage(const char * from, const char * channel, const char * message) {
  if( 0 != fpOnChannelMessage ) {
    (*fpOnChannelMessage)(from, channel, message);
  } else {
    //do nothing
  }
}

void (*fpOnPrivateNotice)(const char *, const char *);
void ircSetOnPrivateNotice(void (*fp)(const char *, const char *)) {
  fpOnPrivateNotice = fp;
}
void ircOnPrivateNotice(const char * from, const char * message) {
  if( 0 != fpOnPrivateNotice ) {
    (*fpOnPrivateNotice)(from, message);
  } else {
    //do nothing
  }
}

void (*fpOnChannelNotice)(const char *, const char *, const char *);
void ircSetOnChannelNotice(void (*fp)(const char *, const char *, const char *)) {
  fpOnChannelNotice = fp;
}
void ircOnChannelNotice(const char * from, const char * channel, const char * message) {
  if( 0 != fpOnChannelNotice ) {
    (*fpOnChannelNotice)(from, channel, message);
  } else {
    //do nothing
  }
}

void (*fpOnCTCP)(const char *, const char *, const char *);
void ircSetOnCTCP(void (*fp)(const char *, const char *, const char *)) {
  fpOnCTCP = fp;
}
void ircOnCTCP(const char * from, const char * to, const char * message) {
  char * pch;
  pch = strstr_P(message,"VERSION");
  if (pch == &message[0]) {  //respond to CTCP VERSION request
    char buf[100];
    snprintf_P(buf, sizeof(buf), PSTR("Got CTCP VERSION request from <%s>"), from);
    ircDebug(buf);
    ircNetworkLight();
    sendIRCNotice(from, _version);
  } else if( 0 != fpOnCTCP ) {
    (*fpOnCTCP)(from, to, message);
  } else {
    //do nothing
  }
}

void (*fpOnVoice)(const char *, const char *);
void ircSetOnVoice(void (*fp)(const char *, const char *)) {
  fpOnVoice = fp;
}
void ircOnVoice(const char * from, const char * channel) {
  if( 0 != fpOnVoice ) {
    (*fpOnVoice)(from, channel);
  } else {
    //do nothing
  }
}

void (*fpOnOp)(const char *, const char *);
void ircSetOnOp(void (*fp)(const char *, const char *)) {
  fpOnOp = fp;
}
void ircOnOp(const char * from, const char * channel) {
  if( 0 != fpOnOp ) {
    (*fpOnOp)(from, channel);
  } else {
    //do nothing
  }
}

void (*fpNetworkLightOn)();
void ircSetNetworkLightOn(void (*fp)(void)) {
  fpNetworkLightOn = fp;
}
void ircNetworkLight(){
  if( 0 != fpNetworkLightOn ) {
    (*fpNetworkLightOn)();
  } else {
    //do nothing
  }
}

void ircDebug (const __FlashStringHelper *line) {
  char buf[508] = "";  //maximum safe UDP payload is 508 bytes
  strcpy_P(buf, (PGM_P)line);
  ircDebug(buf);
}

void (*fpDebug)(const char *);
void ircSetDebug(void (*fp)(const char *)) {
  fpDebug = fp;
}

void ircDebug(const char *line) {
  if( 0 != fpDebug ) {
    (*fpDebug)(line);
  } else {
    //do nothing
  }
}

void stringRemoveNonPermitted(char *str) {
    unsigned long i = 0;
    unsigned long j = 0;
    char c;
    while ((c = str[i++]) != '\0') {
        if (isalnum(c) || c == '-' || c == '.' | c == '_') {
            str[j++] = c;
        }
    }
    str[j] = '\0';
}
