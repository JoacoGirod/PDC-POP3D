#responseawk.txt
Se puede usar como base fake para testear el parser, tambien se puede correr de forma aislada con
'socat TCP4-LISTEN:1110,crlf,reuseaddr SYSTEM:'./<nombredearchivo>',pty,echo=0'
luego nos podriamos conectar con
'nc -C localhost 1110'
y tirar comandos como USER juan, LIST, CAPA...


#miniDemo.c
Servidor con algunos bugs de esqueleto, que nos dio Coda
