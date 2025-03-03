#ifndef EVENTS_H
#define EVENTS_H

// Códigos de mensaje del protocolo
enum MessageType {
    LIST_USERS = 1,
    GET_USER_INFO = 2,
    CHANGE_STATUS = 3,
    SEND_MESSAGE = 4,
    GET_HISTORY = 5,
    ERROR_MSG = 50,
    USER_LIST_RESPONSE = 51,
    USER_INFO_RESPONSE = 52,
    USER_REGISTERED = 53,
    STATUS_CHANGED = 54,
    MESSAGE_RECEIVED = 55,
    HISTORY_RESPONSE = 56
};

#endif