#ifndef EVENTS_H
#define EVENTS_H

// Códigos de mensaje del protocolo
enum MessageType {
    LIST_USERS       = 1,   // Solicitar lista de usuarios
    GET_USER_INFO    = 2,   // Obtener información de un usuario por nombre
    CHANGE_STATUS    = 3,   // Cambiar el estado del usuario
    SEND_MESSAGE     = 4,   // Enviar un mensaje (chat general o privado)
    GET_HISTORY      = 5,   // Solicitar historial de mensajes

    ERROR_MSG        = 50,  // Mensaje de error
    USER_LIST_RESPONSE = 51, // Respuesta con la lista de usuarios
    USER_INFO_RESPONSE = 52, // Respuesta con la información de un usuario
    USER_REGISTERED    = 53, // Notificación de un nuevo usuario registrado
    STATUS_CHANGED     = 54, // Notificación de cambio de estado
    MESSAGE_RECEIVED   = 55, // Notificación de recepción de un mensaje
    HISTORY_RESPONSE   = 56  // Respuesta con el historial de mensajes
};

#endif