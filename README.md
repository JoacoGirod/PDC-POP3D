# Trabajo Practico - Protocolos de Comunicacion - POP3D

> Integrantes :
> Iñaki Bengolea
> Christian Tomas Ijjas,
> Joaquin Girod,
> Felix Lopez Menardi,

### Useful Commands

##### Compile and Run

```bash
make clean ; make ; ./pop3d -u testuser:testuser
```

#### Testing

```bash
# syscall tracking
make clean ; make all; strace -ff ./pop3d -u testuser:testuser
# pop3 server (IPV4)
nc -C localhost 1110
# pop3 server (IPV6)
nc -6C ::1 1110
# configuration server (IPV6)
nc -u -C localhost 8080 #and run commands
# transformation check (initially turned off)
make clean; make all;strace -ff ../pop3d -u testuser:testuser -t 'sed s/[Aa]/4/g|sed s/[Ee]/3/g|sed s/[iI]/1/g|sed s/[Oo]/0/g'

```
