#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H
#include <Arduino.h>

void ircServer(const char * server, int port);

boolean ircConnect(const char * server, int port, boolean reconnect);

void ircSetNick(const char * name);

void ircSetVersion(const char * version);

void ircSetNickServPassword(const char * password);

void doIRC();

void parseIRCInput(boolean buffer_overflow);

void joinIRCChannel(const __FlashStringHelper *channel);

void joinIRCChannel(const char *channel);

void sendIRCMessage(const __FlashStringHelper *target, const __FlashStringHelper *message);

void sendIRCMessage(const char *target, const __FlashStringHelper *message);

void sendIRCMessage(const char *target, const char *message);

void sendIRCNotice(const __FlashStringHelper *target, const __FlashStringHelper *message);

void sendIRCNotice(const char *target, const __FlashStringHelper *message);

void sendIRCNotice(const char *target, const char *message);

void setAway();

void unAway();

boolean ircConnected(boolean in_progress);

void ircDisconnect();

void ircSetOnConnect(void (*fp)(void));

void ircOnConnect();

void ircSetOnDisconnect(void (*fp)(void));

void ircOnDisconnect();

void ircSetOnPrivateMessage(void (*fp)(const char *, const char*));

void ircOnPrivateMessage(const char * from, const char * message);

void ircSetOnCTCP(void (*fp)(const char *, const char *, const char *));

void ircOnCTCP(const char * from, const char * to, const char * message);

void ircSetOnChannelMessage(void (*fp)(const char *, const char *, const char *));

void ircOnChannelMessage(const char * from, const char * channel, const char * message);

void ircSetOnPrivateNotice(void (*fp)(const char *, const char*));

void ircOnPrivateNotice(const char * from, const char * message);

void ircSetOnChannelNotice(void (*fp)(const char *, const char *, const char *));

void ircOnChannelNotice(const char * from, const char * channel, const char * message);

void ircSetOnVoice(void (*fp)(const char *, const char *));

void ircOnVoice(const char * from, const char * channel);

void ircSetOnOp(void (*fp)(const char *, const char *));

void ircOnOp(const char * from, const char * channel);

void ircSetNetworkLightOn(void (*fp)(void));

void ircNetworkLight();

void ircDebug (const __FlashStringHelper *line);

void ircDebug (const char *line);

void ircSetDebug(void (*fp)(const char *));

#endif
