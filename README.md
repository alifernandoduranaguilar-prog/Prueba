# Sistema de registro estudiantil (C++)

Programa de consola en un solo archivo (`main.cpp`) que practica la **evaluación de
entrada y salida de datos**: valida cada campo antes de aceptarlo, muestra los datos en
una tabla formateada y los conserva en un archivo de texto.

## Compilar y ejecutar

```bash
g++ -std=c++17 -Wall -Wextra -o registro main.cpp
./registro          # Windows: registro.exe
```

Requiere un compilador C++17 (probado con MinGW g++ 6.3). Sin dependencias externas.

## Funciones

| Opción | Acción |
| --- | --- |
| 1 | Registrar estudiante (la cédula no se puede repetir) |
| 2 | Listar estudiantes en tabla alineada |
| 3 | Buscar por cédula |
| 4 | Actualizar: dejar un campo vacío conserva su valor |
| 5 | Eliminar con confirmación |
| 6 | Estadísticas: total, promedio, aprobados, reprobados, nota máxima y mínima |
| 7 | Guardar cambios |
| 0 | Salir (guarda automáticamente) |

Los cambios se guardan también de forma automática al registrar, actualizar o eliminar.

## Validaciones de entrada

- **Cédula**: solo dígitos, 6 a 10 caracteres, sin duplicados.
- **Nombres y apellidos**: letras (incluye acentos), espacios, guiones y apóstrofos, 3 a 60 caracteres.
- **Edad**: entero entre 5 y 100.
- **Correo**: una sola `@`, dominio con punto y extensión de al menos 2 letras.
- **Carrera**: 3 a 40 caracteres, sin el separador `|`.
- **Nota**: número entre 0 y 20, admite coma o punto decimal.

Cada error muestra un mensaje y vuelve a pedir el dato, sin descartar el formulario.
Al cargar el archivo, las líneas corruptas o duplicadas se descartan con aviso.

## Formato de `estudiantes.txt`

```text
# cedula|nombres|apellidos|edad|correo|carrera|nota
12345678|Ana María|Pérez Gómez|20|ana.perez@correo.com|Ingeniería Civil|17.50
```

El archivo se crea en la primera ejecución y está excluido de git (`.gitignore`).
