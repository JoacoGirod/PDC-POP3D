# Trabajo Practico - Protocolos de Comunicacion - POP3D

>  Integrantes :
Joaquin Girod,
Felix Lopez Menardi,
Christian Tomas Ijjas,
Iñaki Bengolea



### Useful Commands

##### Compile and Run
make clean ; make ; ./pop3d

#### Testing
make ; strace -ff ./pop3d ; make clean
echo -e "hola\r\n" | nc -C localhost 1110
nc -C localhost 1110

