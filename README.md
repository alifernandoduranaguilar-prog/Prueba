# Sistema de registro estudiantil y de profesores (C++)

Programa de consola en un solo archivo (`main.cpp`) que practica la **evaluación de
entrada y salida de datos**: el acceso está protegido con login, cada campo se valida
antes de aceptarlo, los datos se muestran en tablas formateadas y se conservan en
archivos de texto. Incluye el registro de estudiantes y un submenú de profesores.

## Compilar y ejecutar

```bash
g++ -std=c++17 -Wall -Wextra -o registro main.cpp
./registro          # Windows: registro.exe
```

Requiere un compilador C++17 (probado con MinGW g++ 6.3). Sin dependencias externas.

## Inicio de sesión

En el primer arranque se crea el archivo `usuarios.txt` con la cuenta:

```text
usuario:     admin
contraseña:  admin123
```

El programa pide usuario y contraseña (oculta con `*` en la consola de Windows) y
permite **3 intentos**; al agotarlos se cierra. Dentro de la sesión, la opción 8 cambia
la contraseña y la opción 9 cierra la sesión para volver al login.

Las cuentas se guardan con **sal aleatoria de 16 bytes** y **SHA-256 iterado 10 000
veces** (implementado dentro de `main.cpp`, sin librerías). Nunca se guarda la
contraseña en claro. Si se borra `usuarios.txt`, se vuelve a crear la cuenta por
defecto. Nota: es un ejercicio didáctico; para producción conviene una función como
Argon2 o bcrypt.

## Funciones

| Opción | Acción |
| --- | --- |
| 1 | Registrar estudiante (la cédula no se puede repetir) |
| 2 | Listar estudiantes en tabla alineada |
| 3 | Buscar por cédula |
| 4 | Actualizar: dejar un campo vacío conserva su valor |
| 5 | Eliminar con confirmación |
| 6 | Estadísticas: total, promedio, aprobados, reprobados, nota máxima y mínima |
| 7 | Profesores: submenú con registrar, listar, buscar, actualizar y eliminar |
| 8 | Guardar cambios (estudiantes y profesores) |
| 9 | Cambiar la contraseña de la sesión |
| 10 | Cerrar sesión |
| 0 | Salir (guarda automáticamente) |

Los cambios de estudiantes se guardan también de forma automática al registrar,
actualizar o eliminar.

## Validaciones de entrada

- **Cédula**: solo dígitos, 6 a 10 caracteres, sin duplicados.
- **Nombres y apellidos**: letras (incluye acentos), espacios, guiones y apóstrofos, 3 a 60 caracteres.
- **Edad**: entero entre 5 y 100.
- **Correo**: una sola `@`, dominio con punto y extensión de al menos 2 letras.
- **Carrera**: 3 a 40 caracteres, sin el separador `|`.
- **Nota**: número entre 0 y 20, admite coma o punto decimal.
- **Contraseña**: 8 a 64 caracteres imprimibles y distinta de la anterior al cambiarla.

En el submenú de profesores se validan:

- **Cédula**: solo dígitos, 6 a 10 caracteres, sin duplicados entre profesores.
- **Nombres y apellidos**: letras (incluye acentos), espacios, guiones y apóstrofos, 3 a 60 caracteres.
- **Especialidad**: 3 a 40 caracteres válidos (letras, números, espacios y `. , - ( ) /`).

Cada error muestra un mensaje y vuelve a pedir el dato, sin descartar el formulario.
Al cargar los archivos, las líneas corruptas o duplicadas se descartan con aviso.

## Archivos de datos

`estudiantes.txt`:

```text
# cedula|nombres|apellidos|edad|correo|carrera|nota
12345678|Ana María|Pérez Gómez|20|ana.perez@correo.com|Ingeniería Civil|17.50
```

`usuarios.txt`:

```text
# usuario|sal|hash_sha256
admin|4bce4d9ae07834d6ebaad362d02bb4fc|375b4672089824ceef6129f376a9c3204d88140e12db434821342f4fbba4c8c2
```

`profesores.txt`:

```text
# cedula|nombres|apellidos|especialidad
55667788|María|Torres|Física
```

Los tres se crean en la primera ejecución y están excluidos de git (`.gitignore`).
