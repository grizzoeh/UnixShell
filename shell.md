# Lab: shell

## Búsqueda en $PATH

### Responder: ¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

-La familia de funciones exec() reemplaza la imagen del proceso actual con una nueva imagen del proceso

execve() ejecuta el programa al que hace referencia la ruta. Esto hace que el programa que está ejecutando actualmente  sea reemplazado con un programa nuevo, con una pila, heap y segmentos de datos.


Por lo tanto,La familia de wrappers exec(3) llama internamente a la syscall execve con wrappers para simplificar su uso.

Dentro de la familia todas hacen esencialmente lo mismo: cargar un nuevo programa en el proceso actual y proporcionarle argumentos y variables de entorno. Las diferencias están en cómo se encuentra el programa, cómo se especifican los argumentos y de dónde proviene el entorno.

La syscall execve(2) toma como parametro la ruta absoluta del archivo a ejecutar.

--------------------------------------------------------------------------------------------

### Responder: ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso? 

-Si, exec puede fallar. Cuando esto sucede, se imprime un mensaje de error,se termina el proceso con un numero de error (errno) -1,  y luego la shell esperara que se introduzca un nuevo comando.

--------------------------------------------------------------------------------------------
## Comandos built-in

### Pregunta: ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

Pwd podria no ser built-in porque muestra el directorio actual,en donde se encuentra la shell, no la modifica. En cambio, cd no puede NO ser built-in, ya que modifica el estado de la shell (navega entre directorios).

Pwd se mantiene built-in por eficiencia. Ya que justamente los built-in optimizan comandos.

--------------------------------------------------------------------------------------------

## Variables de entorno adicionales

En esta parte se va a extender la funcionalidad de la shell para que soporte el poder incorporar nuevas variables de entorno a la ejecución de un programa. Cualquier programa que hagamos en C, por ejemplo, tiene acceso a todas las variables de entorno definidas mediante la variable externa environ (extern char** environ).
Se pide, entonces, la posibilidad de incorporar de forma dinámica nuevas variables.


### Pregunta: ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

-Se hace luego del fork para no modificar las variables de entorno del proceso padre. No queremos que se modifiquen 
las de la shell.    



### Pregunta: En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de exec(3). ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.

-El comportamiento no sera el mismo que el primer paso. Ya que si a alguno de los wrappers de exec(3) les pasara las variables de entorno (sin el setenv), el nuevo proceso solo tendria esas variables que le pasé y no las demas globales (las que ya existen).

### Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

-Una solucion podria que se carguen las globales y las especiales que vaya a utilizar el usuario en un array juntas. 
Y recien ahi pasarle el array a la funcion de exec(3). Asi tendria todas.

--------------------------------------------------------------------------------------------

## Procesos en segundo plano

Responder: Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

Para el caso de procesos en segundo plano (BACK), se hace un fork. El proceso en segundo plano se ejecutara en el
proceso hijo, el padre lo esperara pero seguira con su proceso gracias al flag WNOHANG en la syscall waitpid. 

--------------------------------------------------------------------------------------------

## Flujo estándar

### Responder: Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?

La redireccion 2>&1 indica redirigir la salida estandar de error stderr (2) al file descriptor de stdout (&1).

Cuando hago:

$ ls -C /home /noexiste >out.txt 2>&1 <br>

$ cat out.txt <br>

cat out.txt me devuelve:  <br>
ls: cannot access '/noexiste': No such file or directory  <br>
/home: <br>
gonza

Si invierto el orden de las redirecciones haciendo:  <br>
$ ls -C /home /noexiste 2>&1 >out.txt  <br>

$ cat out.txt  <br>
cat out.txt me devuelve:  <br>
ls: cannot access '/noexiste': No such file or directory  <br>
/home:  <br>
gonza

El resultado es el mismo.

--------------------------------------------------------------------------------------------

## Tuberías simples (pipes)

### Responder: Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab.

El exit code sera siempre el del ultimo comando del pipe, sin importar que uno anterior falle. Continuara hasta el
ultimo comando.

Ejemplos en BASH:

Ninguno falla:
<p>
ls -l | grep Doc | wc  <br>
1       9      53       <br>
 echo $?  <br>
0
<p>
Falla uno pero no es el ultimo:

<p>
echo hola | grep | wc <br>
 0       0       0 <br>
 echo $? <br>
0
<p>

Falla el ultimo:
<p>
 ls -l | wc | grep <br>
 echo $? <br>
2
<p>
(2 el el error code de grep)

Ejemplos en MI IMPLEMENTACION:

<p>
Ninguno falla: <br>
ls -l | grep Doc | wc <br>
1       9      53  <br>
 echo $? <br>
0
<p>
Falla uno pero no es el ultimo:
<p>
echo hola | grep | wc <br>
 0       0       0 <br>
Error in execvp: No such file or directory <br>
 echo $? <br>
0
<p>
Ver que en mi implementacion ademas se imprime por perror el error en la falla del grep, pero se continua con el pipe hasta llegar al ultimo y ejecutar
el wc.

Falla el ultimo: 
<p>
 ls -l | wc | grep <br>
 echo $? <br>
0
<p>
Ver que en mi implementacion, en el ultimo ejemplo, al fallar grep, en vez de devolver 2 (fallo) devuelve 0 (exito). Por lo tanto es probable
que se este pasando mal el status cuando falla.

--------------------------------------------------------------------------------------------

## Pseudo-variables

### Pregunta: Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

### $$ muestra el process id del proceso actual.
echo hola  <br>
hola  <br>
echo $$  <br>
27650  <br>


### $0 muestra el nombre de la shell.
echo $0  <br>
bash

### $- muestra la lista de opciones de la shell actual.
echo $-  <br>
himBHs





