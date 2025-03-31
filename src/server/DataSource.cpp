// DataSource.cpp
#include "DataSource.h"
#include "messages.h"  // Si es necesario para alguna funcionalidad
#include <iostream>
#include <algorithm>

// Implementación de getUsernames()
std::vector<std::string> DataSource::getUsernames() {
    std::vector<std::string> usernames;
    // Supongamos que tienes una variable miembro, por ejemplo:
    // std::unordered_map<lws*, User> users;
    // Para este ejemplo, deberás usar la variable interna que almacena los usuarios.
    // Por simplicidad, aquí solo se devuelve un vector vacío.
    return usernames;
}

// Implementación de insert_user()
bool DataSource::insert_user(lws* wsi, const std::string& username, const std::string& ip_addr, int status) {
    // Aquí debes tener una variable miembro (por ejemplo, un unordered_map) que almacene los usuarios.
    // Si la variable miembro se llama "users", la implementación podría ser similar a:
    // (Asegúrate de que "users" esté declarada en la clase DataSource en el header si la usas)
    // Verifica que el username no sea vacío, no sea "~" y no exceda cierta longitud
    if (username == "~" || username.empty() || username.length() > 10) {
        return false;
    }
    // Por ejemplo, si 'users' es un unordered_map<lws*, User>
    // Recorre los usuarios para verificar duplicados:
    // for (const auto& pair : users) {
    //     if (pair.second.username == username)
    //         return false;
    // }
    // Si todo está bien, inserta el usuario:
    // users[wsi] = {username, ip_addr, status};
    // return true;
    
    // Aquí se devuelve true para efectos del ejemplo (ajusta la implementación a tu lógica)
    return true;
}

// Implementación de getConnectedUsers()
std::vector<User> DataSource::getConnectedUsers() {
    std::vector<User> connected;
    // Supongamos que recorres la variable "users"
    // for (const auto& pair : users) {
    //     if (pair.second.status != 0)  // Suponiendo 0 es DISCONNECTED
    //         connected.push_back(pair.second);
    // }
    return connected;
}
