#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H
#include <Arduino.h>
#include <Client.h>

void ircSetClient(Client &client);

void ircServer(const char * server, int port);

boolean ircConnect(const char * server, int port);

boolean ircConnect(Client &client, const char * server, int port, boolean reconnect);

boolean ircConnect(const char * server, int port, boolean reconnect);

void ircSetNick(const char * name);

const char * ircNick();

void ircSetVersion(const char * version);

void ircSetNickServPassword(const char * password);

void doIRC();

boolean parseIRCInput(boolean buffer_overflow);

void joinIRCChannel(const __FlashStringHelper *channel);

void joinIRCChannel(const char *channel);

void sendIRCMessage(const __FlashStringHelper *target, const __FlashStringHelper *message);

void sendIRCMessage(const char *target, const __FlashStringHelper *message);

void sendIRCMessage(const char *target, const char *message);

void sendIRCNotice(const __FlashStringHelper *target, const __FlashStringHelper *message);

void sendIRCNotice(const char *target, const __FlashStringHelper *message);

void sendIRCNotice(const char *target, const char *message);

void sendIRCCTCP(const __FlashStringHelper *target, const __FlashStringHelper *message);

void sendIRCCTCP(const char *target, const __FlashStringHelper *message);

void sendIRCCTCP(const char *target, const char *message);

void sendIRCAction(const __FlashStringHelper *target, const __FlashStringHelper *message);

void sendIRCAction(const char *target, const __FlashStringHelper *message);

void sendIRCAction(const char *target, const char *message);

void setAway();

void setAway(const char *message);

void setAway(const __FlashStringHelper *message);

void unAway();

boolean ircConnected();

boolean ircConnected(boolean in_progress);

void ircMode(const __FlashStringHelper *command);

void ircMode(const char *command);

void ircDisconnect();

void ircDisconnect(const __FlashStringHelper *quitmsg);

void ircDisconnect(const char *quitmsg);

void ircSetOnConnect(void (*fp)(void));

void ircOnConnect();

void ircSetOnDisconnect(void (*fp)(void));

void ircOnDisconnect();

void ircSetOnPrivateMessage(void (*fp)(const char *, const char*));

void ircOnPrivateMessage(const char * from, const char * message);

void ircSetOnAction(void (*fp)(const char *, const char *, const char *));

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

void ircSetOnNickChange(void (*fp)(const char *, const char *));

void ircOnNickChange(const char * oldnick, const char * newnick);

void ircSetOnRaw(void (*fp)(const char *));

void ircOnRaw(const char * line);

void ircSetNetworkLightOn(void (*fp)(void));

void ircNetworkLight();

void ircDebug (const __FlashStringHelper *line);

void ircDebug (const char *line);

void ircSetDebug(void (*fp)(const char *));

void stringRemoveNonPermitted(char *str);

#endif
