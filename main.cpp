// Sistema de registro estudiantil
// Practica de evaluacion de entrada y salida de datos en C++.
//
// Compilar:  g++ main.cpp -o registro -std=c++17
// Ejecutar:  ./registro        (Windows: registro.exe)
//
// Los datos se guardan en estudiantes.txt con el formato:
//   cedula|nombres|apellidos|edad|correo|carrera|nota

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const string ARCHIVO = "estudiantes.txt";

struct Estudiante {
    string cedula;
    string nombres;
    string apellidos;
    string correo;
    string carrera;
    int edad = 0;
    double nota = 0.0;
};

// ---------------------------------------------------------------------------
// Utilidades de texto y numeros
// ---------------------------------------------------------------------------

string trim(const string& texto) {
    size_t inicio = 0;
    size_t fin = texto.size();
    while (inicio < fin && isspace(static_cast<unsigned char>(texto[inicio]))) ++inicio;
    while (fin > inicio && isspace(static_cast<unsigned char>(texto[fin - 1]))) --fin;
    return texto.substr(inicio, fin - inicio);
}

vector<string> separar(const string& texto, char separador) {
    vector<string> partes;
    string parte;
    istringstream flujo(texto);
    while (getline(flujo, parte, separador)) partes.push_back(parte);
    return partes;
}

bool parsearEntero(const string& texto, int& valor) {
    string limpio = trim(texto);
    if (limpio.empty()) return false;
    size_t inicio = (limpio[0] == '-' || limpio[0] == '+') ? 1 : 0;
    if (inicio >= limpio.size()) return false;
    for (size_t i = inicio; i < limpio.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(limpio[i]))) return false;
    }
    try {
        valor = stoi(limpio);
    } catch (...) {
        return false;
    }
    return true;
}

bool parsearDecimal(const string& texto, double& valor) {
    string limpio = trim(texto);
    if (limpio.empty()) return false;
    replace(limpio.begin(), limpio.end(), ',', '.');
    int puntos = 0;
    int digitos = 0;
    for (char c : limpio) {
        if (isdigit(static_cast<unsigned char>(c))) {
            ++digitos;
        } else if (c == '.') {
            if (++puntos > 1) return false;
        } else {
            return false;
        }
    }
    if (digitos == 0) return false;
    try {
        valor = stod(limpio);
    } catch (...) {
        return false;
    }
    return true;
}

string formatoNota(double nota) {
    ostringstream flujo;
    flujo << fixed << setprecision(2) << nota;
    return flujo.str();
}

// ---------------------------------------------------------------------------
// Validacion de campos (evaluacion de la entrada)
// ---------------------------------------------------------------------------

bool esCedulaValida(const string& texto) {
    if (texto.size() < 6 || texto.size() > 10) return false;
    return all_of(texto.begin(), texto.end(), [](unsigned char c) { return isdigit(c) != 0; });
}

bool esNombreValido(const string& texto) {
    if (texto.size() < 3 || texto.size() > 60) return false;
    int letras = 0;
    for (unsigned char c : texto) {
        if (isalpha(c)) {
            ++letras;
        } else if (c >= 0x80) {
            ++letras;  // letras acentuadas codificadas en UTF-8
        } else if (c != ' ' && c != '\'' && c != '-') {
            return false;
        }
    }
    return letras >= 3;
}

bool esCorreoValido(const string& texto) {
    if (texto.size() < 6 || texto.size() > 60) return false;

    size_t arroba = texto.find('@');
    if (arroba == string::npos || texto.find('@', arroba + 1) != string::npos) return false;

    string local = texto.substr(0, arroba);
    string dominio = texto.substr(arroba + 1);
    if (local.empty() || dominio.empty()) return false;
    if (local.front() == '.' || local.back() == '.') return false;

    const string permitidosLocal = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._%+-";
    for (char c : local) {
        if (permitidosLocal.find(c) == string::npos) return false;
    }

    if (dominio.find('.') == string::npos) return false;
    if (dominio.front() == '.' || dominio.back() == '.' ||
        dominio.front() == '-' || dominio.back() == '-') {
        return false;
    }
    bool puntoAnterior = false;
    for (char c : dominio) {
        if (c == '.') {
            if (puntoAnterior) return false;
            puntoAnterior = true;
            continue;
        }
        puntoAnterior = false;
        if (!isalnum(static_cast<unsigned char>(c)) && c != '-') return false;
    }
    size_t ultimoPunto = dominio.rfind('.');
    return dominio.size() - ultimoPunto - 1 >= 2;
}

bool esCarreraValida(const string& texto) {
    if (texto.size() < 3 || texto.size() > 40) return false;
    for (unsigned char c : texto) {
        if (isalnum(c) || c >= 0x80) continue;
        if (c == ' ' || c == '-' || c == '.' || c == ',' || c == '(' || c == ')' || c == '/') continue;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Ancho visible en consola (cuenta caracteres UTF-8, no bytes)
// ---------------------------------------------------------------------------

size_t anchoTexto(const string& texto) {
    size_t ancho = 0;
    for (unsigned char c : texto) {
        if ((c & 0xC0) != 0x80) ++ancho;
    }
    return ancho;
}

string recortar(const string& texto, size_t maximo) {
    if (anchoTexto(texto) <= maximo) return texto;
    string salida;
    size_t ancho = 0;
    for (size_t i = 0; i < texto.size();) {
        unsigned char c = static_cast<unsigned char>(texto[i]);
        size_t largo = 1;
        if (c >= 0xF0) largo = 4;
        else if (c >= 0xE0) largo = 3;
        else if (c >= 0xC0) largo = 2;
        if (ancho + 1 + 3 > maximo) break;
        salida.append(texto, i, largo);
        i += largo;
        ++ancho;
    }
    return salida + "...";
}

string ajustar(const string& texto, size_t ancho) {
    string salida = recortar(texto, ancho);
    size_t visible = anchoTexto(salida);
    if (visible < ancho) salida.append(ancho - visible, ' ');
    return salida;
}

string ajustarDerecha(const string& texto, size_t ancho) {
    string salida = recortar(texto, ancho);
    size_t visible = anchoTexto(salida);
    if (visible < ancho) salida.insert(0, ancho - visible, ' ');
    return salida;
}

// ---------------------------------------------------------------------------
// Mensajes y entrada de datos
// ---------------------------------------------------------------------------

void aviso(const string& mensaje) { cout << "  [!] " << mensaje << "\n"; }
void exito(const string& mensaje) { cout << "  [OK] " << mensaje << "\n"; }

// Lee una linea completa. Devuelve false si se alcanzo el fin de la entrada.
bool leerLinea(const string& etiqueta, string& destino) {
    cout << etiqueta;
    if (!getline(cin, destino)) {
        cout << "\n";
        return false;
    }
    destino = trim(destino);
    return true;
}

int buscarPorCedula(const vector<Estudiante>& lista, const string& cedula) {
    for (size_t i = 0; i < lista.size(); ++i) {
        if (lista[i].cedula == cedula) return static_cast<int>(i);
    }
    return -1;
}

using Validador = bool (*)(const string&);

bool pedirTextoValidado(const string& etiqueta, const string& actual, bool permitirConservar,
                        Validador validador, const string& mensajeError, string& salida) {
    while (true) {
        string pista;
        if (permitirConservar && !actual.empty()) pista = " [" + actual + "]";
        string texto;
        if (!leerLinea("  " + etiqueta + pista + ": ", texto)) return false;
        if (permitirConservar && texto.empty()) {
            salida = actual;
            return true;
        }
        if (!validador(texto)) {
            aviso(mensajeError);
            continue;
        }
        salida = texto;
        return true;
    }
}

bool pedirCedula(const vector<Estudiante>& lista, const string& actual, bool permitirConservar,
                 string& salida) {
    while (true) {
        string pista = (permitirConservar && !actual.empty()) ? " [" + actual + "]" : "";
        string texto;
        if (!leerLinea("  Cédula (6-10 dígitos)" + pista + ": ", texto)) return false;
        if (permitirConservar && texto.empty()) {
            salida = actual;
            return true;
        }
        if (!esCedulaValida(texto)) {
            aviso("La cédula debe tener entre 6 y 10 dígitos, sin letras ni espacios.");
            continue;
        }
        if (texto != actual && buscarPorCedula(lista, texto) != -1) {
            aviso("Ya existe un estudiante registrado con la cédula " + texto + ".");
            continue;
        }
        salida = texto;
        return true;
    }
}

// Pide una cedula existente o no, sin validar duplicados (para buscar/eliminar).
bool pedirCedulaSimple(string& salida) {
    while (true) {
        if (!leerLinea("  Cédula: ", salida)) return false;
        if (esCedulaValida(salida)) return true;
        aviso("La cédula debe tener entre 6 y 10 dígitos, sin letras ni espacios.");
    }
}

bool pedirEdad(int actual, bool permitirConservar, int& salida) {
    while (true) {
        string pista = permitirConservar ? " [" + to_string(actual) + "]" : "";
        string texto;
        if (!leerLinea("  Edad (5-100)" + pista + ": ", texto)) return false;
        if (permitirConservar && texto.empty()) {
            salida = actual;
            return true;
        }
        int edad = 0;
        if (!parsearEntero(texto, edad) || edad < 5 || edad > 100) {
            aviso("La edad debe ser un número entero entre 5 y 100.");
            continue;
        }
        salida = edad;
        return true;
    }
}

bool pedirNota(double actual, bool permitirConservar, double& salida) {
    while (true) {
        string pista = permitirConservar ? " [" + formatoNota(actual) + "]" : "";
        string texto;
        if (!leerLinea("  Nota (0-20)" + pista + ": ", texto)) return false;
        if (permitirConservar && texto.empty()) {
            salida = actual;
            return true;
        }
        double nota = 0.0;
        if (!parsearDecimal(texto, nota) || nota < 0.0 || nota > 20.0) {
            aviso("La nota debe ser un número entre 0 y 20 (puede usar decimales).");
            continue;
        }
        salida = nota;
        return true;
    }
}

bool confirmar(const string& pregunta) {
    string texto;
    if (!leerLinea("  " + pregunta + " (s/N): ", texto)) return false;
    return !texto.empty() && (texto[0] == 's' || texto[0] == 'S');
}

// ---------------------------------------------------------------------------
// Persistencia
// ---------------------------------------------------------------------------

bool guardarArchivo(const vector<Estudiante>& lista) {
    ofstream salida(ARCHIVO, ios::trunc);
    if (!salida) {
        aviso("No se pudo abrir " + ARCHIVO + " para guardar los datos.");
        return false;
    }
    salida << "# cedula|nombres|apellidos|edad|correo|carrera|nota\n";
    for (const Estudiante& e : lista) {
        salida << e.cedula << '|' << e.nombres << '|' << e.apellidos << '|' << e.edad << '|'
               << e.correo << '|' << e.carrera << '|' << formatoNota(e.nota) << '\n';
    }
    if (!salida.good()) {
        aviso("Ocurrió un error al escribir " + ARCHIVO + ".");
        return false;
    }
    return true;
}

vector<Estudiante> cargarArchivo() {
    vector<Estudiante> lista;
    ifstream entrada(ARCHIVO);
    if (!entrada) {
        cout << "  [i] No existe " << ARCHIVO << "; se inicia un registro vacío.\n";
        return lista;
    }

    string linea;
    int descartadas = 0;
    while (getline(entrada, linea)) {
        string limpia = trim(linea);
        if (limpia.empty() || limpia[0] == '#') continue;

        vector<string> campos = separar(limpia, '|');
        if (campos.size() != 7) {
            ++descartadas;
            continue;
        }

        Estudiante e;
        e.cedula = trim(campos[0]);
        e.nombres = trim(campos[1]);
        e.apellidos = trim(campos[2]);
        e.correo = trim(campos[4]);
        e.carrera = trim(campos[5]);

        int edad = 0;
        double nota = 0.0;
        bool valido = esCedulaValida(e.cedula) && esNombreValido(e.nombres) &&
                      esNombreValido(e.apellidos) && esCorreoValido(e.correo) &&
                      esCarreraValida(e.carrera) && parsearEntero(trim(campos[3]), edad) &&
                      edad >= 5 && edad <= 100 && parsearDecimal(trim(campos[6]), nota) &&
                      nota >= 0.0 && nota <= 20.0;
        if (!valido || buscarPorCedula(lista, e.cedula) != -1) {
            ++descartadas;
            continue;
        }

        e.edad = edad;
        e.nota = nota;
        lista.push_back(e);
    }

    cout << "  [i] " << lista.size() << " estudiante(s) cargado(s) desde " << ARCHIVO << ".\n";
    if (descartadas > 0) {
        aviso("Se descartaron " + to_string(descartadas) + " línea(s) inválida(s) o duplicada(s).");
    }
    return lista;
}

// ---------------------------------------------------------------------------
// Salida de datos
// ---------------------------------------------------------------------------

void mostrarDetalle(const Estudiante& e) {
    cout << "  Cédula     : " << e.cedula << "\n";
    cout << "  Nombres    : " << e.nombres << "\n";
    cout << "  Apellidos  : " << e.apellidos << "\n";
    cout << "  Edad       : " << e.edad << "\n";
    cout << "  Correo     : " << e.correo << "\n";
    cout << "  Carrera    : " << e.carrera << "\n";
    cout << "  Nota       : " << formatoNota(e.nota) << "\n";
}

void imprimirTabla(const vector<Estudiante>& lista) {
    const size_t wCedula = 10;
    const size_t wNombre = 30;
    const size_t wEdad = 4;
    const size_t wCorreo = 28;
    const size_t wCarrera = 18;
    const size_t wNota = 5;

    cout << "\n  " << ajustar("Cédula", wCedula) << "  " << ajustar("Estudiante", wNombre)
         << "  " << ajustarDerecha("Edad", wEdad) << "  " << ajustar("Correo", wCorreo)
         << "  " << ajustar("Carrera", wCarrera) << "  " << ajustarDerecha("Nota", wNota) << "\n";
    cout << "  " << string(wCedula, '-') << "  " << string(wNombre, '-') << "  "
         << string(wEdad, '-') << "  " << string(wCorreo, '-') << "  "
         << string(wCarrera, '-') << "  " << string(wNota, '-') << "\n";

    for (const Estudiante& e : lista) {
        string nombreCompleto = e.nombres + " " + e.apellidos;
        cout << "  " << ajustar(e.cedula, wCedula) << "  " << ajustar(nombreCompleto, wNombre)
             << "  " << ajustarDerecha(to_string(e.edad), wEdad) << "  "
             << ajustar(e.correo, wCorreo) << "  " << ajustar(e.carrera, wCarrera) << "  "
             << ajustarDerecha(formatoNota(e.nota), wNota) << "\n";
    }
    cout << "\n  Total: " << lista.size() << " estudiante(s).\n";
}

// ---------------------------------------------------------------------------
// Operaciones del registro
// ---------------------------------------------------------------------------

bool registrarEstudiante(vector<Estudiante>& lista) {
    cout << "\n--- Registrar estudiante ---\n";
    Estudiante e;

    if (!pedirCedula(lista, "", false, e.cedula)) return false;
    if (!pedirTextoValidado("Nombres", "", false, esNombreValido,
                            "Los nombres solo admiten letras, espacios, guiones y apóstrofos (3-60 caracteres).",
                            e.nombres)) {
        return false;
    }
    if (!pedirTextoValidado("Apellidos", "", false, esNombreValido,
                            "Los apellidos solo admiten letras, espacios, guiones y apóstrofos (3-60 caracteres).",
                            e.apellidos)) {
        return false;
    }
    if (!pedirEdad(0, false, e.edad)) return false;
    if (!pedirTextoValidado("Correo", "", false, esCorreoValido,
                            "El correo no es válido. Ejemplo: ana.perez@correo.com", e.correo)) {
        return false;
    }
    if (!pedirTextoValidado("Carrera", "", false, esCarreraValida,
                            "La carrera debe tener entre 3 y 40 caracteres válidos.", e.carrera)) {
        return false;
    }
    if (!pedirNota(0.0, false, e.nota)) return false;

    lista.push_back(e);
    if (guardarArchivo(lista)) {
        exito("Estudiante " + e.nombres + " " + e.apellidos + " registrado correctamente.");
    }
    return true;
}

void listarEstudiantes(const vector<Estudiante>& lista) {
    cout << "\n--- Lista de estudiantes ---\n";
    if (lista.empty()) {
        cout << "  No hay estudiantes registrados.\n";
        return;
    }
    imprimirTabla(lista);
}

bool buscarEstudiante(const vector<Estudiante>& lista) {
    cout << "\n--- Buscar estudiante ---\n";
    string cedula;
    if (!pedirCedulaSimple(cedula)) return false;

    int indice = buscarPorCedula(lista, cedula);
    if (indice == -1) {
        aviso("No existe un estudiante con la cédula " + cedula + ".");
        return true;
    }
    cout << "\n";
    mostrarDetalle(lista[indice]);
    return true;
}

bool actualizarEstudiante(vector<Estudiante>& lista) {
    cout << "\n--- Actualizar estudiante ---\n";
    string cedula;
    if (!pedirCedulaSimple(cedula)) return false;

    int indice = buscarPorCedula(lista, cedula);
    if (indice == -1) {
        aviso("No existe un estudiante con la cédula " + cedula + ".");
        return true;
    }

    Estudiante& actual = lista[indice];
    cout << "\n  Datos actuales:\n";
    mostrarDetalle(actual);
    cout << "\n  Deje un campo vacío para conservar el valor actual.\n";

    Estudiante nuevo = actual;
    if (!pedirCedula(lista, actual.cedula, true, nuevo.cedula)) return false;
    if (!pedirTextoValidado("Nombres", actual.nombres, true, esNombreValido,
                            "Los nombres solo admiten letras, espacios, guiones y apóstrofos (3-60 caracteres).",
                            nuevo.nombres)) {
        return false;
    }
    if (!pedirTextoValidado("Apellidos", actual.apellidos, true, esNombreValido,
                            "Los apellidos solo admiten letras, espacios, guiones y apóstrofos (3-60 caracteres).",
                            nuevo.apellidos)) {
        return false;
    }
    if (!pedirEdad(actual.edad, true, nuevo.edad)) return false;
    if (!pedirTextoValidado("Correo", actual.correo, true, esCorreoValido,
                            "El correo no es válido. Ejemplo: ana.perez@correo.com", nuevo.correo)) {
        return false;
    }
    if (!pedirTextoValidado("Carrera", actual.carrera, true, esCarreraValida,
                            "La carrera debe tener entre 3 y 40 caracteres válidos.", nuevo.carrera)) {
        return false;
    }
    if (!pedirNota(actual.nota, true, nuevo.nota)) return false;

    actual = nuevo;
    if (guardarArchivo(lista)) {
        exito("Datos del estudiante actualizados.");
    }
    return true;
}

bool eliminarEstudiante(vector<Estudiante>& lista) {
    cout << "\n--- Eliminar estudiante ---\n";
    string cedula;
    if (!pedirCedulaSimple(cedula)) return false;

    int indice = buscarPorCedula(lista, cedula);
    if (indice == -1) {
        aviso("No existe un estudiante con la cédula " + cedula + ".");
        return true;
    }

    cout << "\n  Registro a eliminar:\n";
    mostrarDetalle(lista[indice]);
    if (!confirmar("¿Eliminar este registro?")) {
        cout << "  Operación cancelada.\n";
        return true;
    }

    lista.erase(lista.begin() + indice);
    if (guardarArchivo(lista)) {
        exito("Estudiante eliminado del registro.");
    }
    return true;
}

void mostrarEstadisticas(const vector<Estudiante>& lista) {
    cout << "\n--- Estadísticas del registro ---\n";
    if (lista.empty()) {
        cout << "  No hay estudiantes registrados.\n";
        return;
    }

    double suma = 0.0;
    int aprobados = 0;
    size_t mejor = 0;
    size_t peor = 0;
    for (size_t i = 0; i < lista.size(); ++i) {
        suma += lista[i].nota;
        if (lista[i].nota >= 10.0) ++aprobados;
        if (lista[i].nota > lista[mejor].nota) mejor = i;
        if (lista[i].nota < lista[peor].nota) peor = i;
    }

    double promedio = suma / static_cast<double>(lista.size());
    double porcentaje = 100.0 * aprobados / static_cast<double>(lista.size());
    int reprobados = static_cast<int>(lista.size()) - aprobados;

    cout << fixed << setprecision(2);
    cout << "  Total de estudiantes : " << lista.size() << "\n";
    cout << "  Promedio de notas    : " << promedio << "\n";
    cout << "  Aprobados (>= 10)    : " << aprobados << " (" << porcentaje << " %)\n";
    cout << "  Reprobados           : " << reprobados << "\n";
    cout << "  Nota más alta        : " << formatoNota(lista[mejor].nota) << " ("
         << lista[mejor].nombres << " " << lista[mejor].apellidos << ")\n";
    cout << "  Nota más baja        : " << formatoNota(lista[peor].nota) << " ("
         << lista[peor].nombres << " " << lista[peor].apellidos << ")\n";
    cout.unsetf(ios::floatfield);
    cout << setprecision(6);
}

// ---------------------------------------------------------------------------
// Menu principal
// ---------------------------------------------------------------------------

void mostrarMenu() {
    cout << "\n==========================================\n";
    cout << "    SISTEMA DE REGISTRO ESTUDIANTIL\n";
    cout << "==========================================\n";
    cout << "  1. Registrar estudiante\n";
    cout << "  2. Listar estudiantes\n";
    cout << "  3. Buscar por cédula\n";
    cout << "  4. Actualizar estudiante\n";
    cout << "  5. Eliminar estudiante\n";
    cout << "  6. Estadísticas del registro\n";
    cout << "  7. Guardar cambios\n";
    cout << "  0. Salir\n";
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    cout << "\n  Sistema de registro estudiantil (C++)\n";

    vector<Estudiante> lista = cargarArchivo();
    bool continuar = true;

    while (continuar) {
        mostrarMenu();

        string texto;
        if (!leerLinea("  Opción: ", texto)) break;

        int opcion = 0;
        if (!parsearEntero(texto, opcion)) {
            aviso("Escriba un número de opción válido.");
            continue;
        }

        switch (opcion) {
            case 1:
                if (!registrarEstudiante(lista)) continuar = false;
                break;
            case 2:
                listarEstudiantes(lista);
                break;
            case 3:
                if (!buscarEstudiante(lista)) continuar = false;
                break;
            case 4:
                if (!actualizarEstudiante(lista)) continuar = false;
                break;
            case 5:
                if (!eliminarEstudiante(lista)) continuar = false;
                break;
            case 6:
                mostrarEstadisticas(lista);
                break;
            case 7:
                if (guardarArchivo(lista)) exito("Datos guardados en " + ARCHIVO + ".");
                break;
            case 0:
                continuar = false;
                break;
            default:
                aviso("Opción no válida. Elija un número del menú.");
                break;
        }
    }

    if (guardarArchivo(lista)) {
        cout << "\n  Datos guardados en " << ARCHIVO << ". ¡Hasta pronto!\n";
    } else {
        cout << "\n  No se pudieron guardar los datos. ¡Hasta pronto!\n";
    }
    return 0;
}
