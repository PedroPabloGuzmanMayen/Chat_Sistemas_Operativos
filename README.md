# Chat_Sistemas_Operativos

Este proyecto implementa un servicio de chat en tiempo real usando websocktes y threads en c++. Consta de dos partes:

- Cliente: tiene una interfaz de usuario para enviar mensajes a otros usuarios
- Servidor: gestiona las conexiones de los usuarios y almacena la información de los chats 

## ¿Cómo compilar y ejecutar el cliente?

Ir a la carpeta src

 ```bash
  cd src
```

Luego ir a la carpeta cliente 

 ```bash
  cd client
```

Ejecutar este comando: 
 ```bash
  make
```

Eso generara un archivo ejecutable, ejecutelo con este comando:

 ```bash
  ./client_gui
```

## ¿Cómo compilar y ejecutar el servidor?

 ```bash
  cd src
```

Luego ir a la carpeta cliente 

 ```bash
  cd server
```

Ejecutar este comando: 
 ```bash
  make
```

Eso generara un archivo ejecutable, ejecutelo con este comando:

 ```bash
  ./websocket_servert
```

