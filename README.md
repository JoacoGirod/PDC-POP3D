# Trabajo Practico - Protocolos de Comunicacion - POP3D

> [!Integrantes]
> Bengolea, Iñaki
> Girod, Joaquin
> Ijjas, Christian
> Menardi, Felix Lopez

# Información del proyecto

## Servidor POP3

En la carpeta `/src` se encuentran todos los archivos de código del proyecto. Por un lado tenemos los parsers de comandos en la carpeta `/src/parser` en donde se define el tipo de dato abstracto "parser" y luego las correspondientes implementaciones y definiciones de los autómatas (`/src/parser/parser_automaton`) que conforman el parser.

En la carpera `/src/bash_script` se encuentran algunos de los scripts de bash usados para testear diferentes aspectos del servidor como lo son la velocidad, el throughput, la concurrencia y también un script de población para popular el directorio de mail con archivos de texto que simulan una los archivos que se encontrarían en la casilla de mails.

## Comandos útiles

### Compilado

Para compilar, desde el directorio raíz del proyecto, se debe correr el siguiente comando

```bash
make clean ; make all
```

Esto genera una archivo ejecutable llamado `pop3d` . En el mismo directorio en el cual está parado.

### Ejecución

#### Inicialización del servidor POP3D

Una vez compilado, se puede ejecutar el proyecto en la misma carpeta del paso previo (de compilación).

```bash
./pop3d -u testuser:testuser
```

Donde puede utilizar algunos argumentos que permiten establecer variables de entorno que definen el comportamiento inicial del servidor.
Para más información de los posibles comandos a usar como argumentos utilice.

```bash
./pop3d -h
```

Que mostrará una lista de argumentos aceptados.

Algunos de los valores predeterminados son:

- Dirección servidor pop3 : 0.0.0.0
- Puerto pop3 : 1110
- Dirección servidor de configuración : 127.0.0.1
- Puerto de configuración : 9090
- Nombre del directorio de mail : Maildir
- Comando de transformación : 'cat'
- En cuanto a usuarios, no hay usuarios predeterminados y deberán ser ingresados manualmente con la opción -u

#### Conectarse al servidor POP3

Para establecer una conexión con el servidor POP3 se puede conectar vía netcat

```bash
nc -C <dirección_server_pop3> <puerto_pop3>
```

por ejemplo

```bash
nc -C 0.0.0.0 1110
```

Luego se verá un mensaje de éxito y podrá probar el servidor con los comando específicos del servidor explicados en el informe.

#### Conectarse al servidor POP3

Para establecer una conexión con el servidor de configuración se puede conectar vía netcat

```bash
nc -u <dirección_server_config> <puerto_config>
```

por ejemplo

```bash
nc -u 127.0.0.1 9090
```

No se verá ningún mensaje pero se podrán ingresar los comandos específicos definidos en el RFC de CONF1 expuesto en el informe.

### Ejemplos de uso

```bash

# syscall tracking

make clean ; make all; strace -ff ./pop3d -u testuser:testuser

# pop3 server (IPV4)

nc -C localhost 1110

# pop3 server (IPV6)

nc -6C ::1 1110

# configuration server (IPV6)

nc -u -C localhost 9090 #and run commands

# transformation check (initially turned off)

make clean; make all;strace -ff ./pop3d -u testuser:testuser -t 'sed s/[Aa]/4/g|sed s/[Ee]/3/g|sed s/[iI]/1/g|sed s/[Oo]/0/g'

```
