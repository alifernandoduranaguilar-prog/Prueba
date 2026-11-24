// Sistema de registro estudiantil
// Practica de evaluacion de entrada y salida de datos en C++.
//
// Compilar:  g++ main.cpp -o registro -std=c++17
// Ejecutar:  ./registro        (Windows: registro.exe)
//
// Los datos se guardan en estudiantes.txt con el formato:
//   cedula|nombres|apellidos|edad|correo|carrera|nota
// y los profesores en profesores.txt:
//   cedula|nombres|apellidos|especialidad
//
// El acceso requiere iniciar sesion. Las cuentas se guardan en usuarios.txt
// con sal aleatoria y hash SHA-256 iterado. En el primer arranque se crea el
// usuario admin con la contrasena admin123 (cambiable desde el menu).

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#endif

using namespace std;

const string ARCHIVO = "estudiantes.txt";
const string ARCHIVO_PROFESORES = "profesores.txt";
const string ARCHIVO_USUARIOS = "usuarios.txt";
const int INTENTOS_MAXIMOS = 3;
const int ITERACIONES_HASH = 10000;
const string USUARIO_DEFECTO = "admin";
const string CLAVE_DEFECTO = "admin123";

struct Estudiante {
    string cedula;
    string nombres;
    string apellidos;
    string correo;
    string carrera;
    int edad = 0;
    double nota = 0.0;
};

struct Usuario {
    string nombre;
    string sal;   // sal aleatoria en hexadecimal (16 bytes)
    string hash;  // hash SHA-256 en hexadecimal
};

struct Profesor {
    string cedula;
    string nombres;
    string apellidos;
    string especialidad;
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
// Seguridad: SHA-256 (implementacion propia, sin librerias externas)
// ---------------------------------------------------------------------------

uint32_t rotarDerecha(uint32_t valor, uint32_t bits) {
    return (valor >> bits) | (valor << (32 - bits));
}

string sha256(const string& mensaje) {
    static const uint32_t constantes[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    uint32_t hash[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    vector<uint8_t> datos(mensaje.begin(), mensaje.end());
    uint64_t bits = static_cast<uint64_t>(datos.size()) * 8;

    datos.push_back(0x80);
    while (datos.size() % 64 != 56) datos.push_back(0);
    for (int i = 7; i >= 0; --i) {
        datos.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
    }

    for (size_t bloque = 0; bloque < datos.size(); bloque += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            size_t pos = bloque + static_cast<size_t>(i) * 4;
            w[i] = (static_cast<uint32_t>(datos[pos]) << 24) |
                   (static_cast<uint32_t>(datos[pos + 1]) << 16) |
                   (static_cast<uint32_t>(datos[pos + 2]) << 8) |
                   static_cast<uint32_t>(datos[pos + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotarDerecha(w[i - 15], 7) ^ rotarDerecha(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotarDerecha(w[i - 2], 17) ^ rotarDerecha(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3];
        uint32_t e = hash[4], f = hash[5], g = hash[6], h = hash[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t s1 = rotarDerecha(e, 6) ^ rotarDerecha(e, 11) ^ rotarDerecha(e, 25);
            uint32_t elegido = (e & f) ^ (~e & g);
            uint32_t temp1 = h + s1 + elegido + constantes[i] + w[i];
            uint32_t s0 = rotarDerecha(a, 2) ^ rotarDerecha(a, 13) ^ rotarDerecha(a, 22);
            uint32_t mayoritaria = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = s0 + mayoritaria;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    ostringstream flujo;
    flujo << hex << setfill('0');
    for (uint32_t valor : hash) flujo << setw(8) << valor;
    return flujo.str();
}

string aHex(const vector<uint8_t>& datos) {
    static const char* digitos = "0123456789abcdef";
    string salida;
    salida.reserve(datos.size() * 2);
    for (uint8_t byte : datos) {
        salida.push_back(digitos[byte >> 4]);
        salida.push_back(digitos[byte & 0x0F]);
    }
    return salida;
}

bool esHex(const string& texto, size_t longitud) {
    if (texto.size() != longitud) return false;
    return all_of(texto.begin(), texto.end(), [](unsigned char c) { return isxdigit(c) != 0; });
}

vector<uint8_t> generarSal() {
    uint64_t semilla = static_cast<uint64_t>(
        chrono::high_resolution_clock::now().time_since_epoch().count());
    random_device dispositivo;
    try {
        semilla ^= (static_cast<uint64_t>(dispositivo()) << 32);
        semilla ^= dispositivo();
    } catch (...) {
        // Si el sistema no ofrece entropia se conserva la semilla del reloj.
    }
    mt19937_64 motor(semilla);

    vector<uint8_t> sal(16);
    for (uint8_t& byte : sal) byte = static_cast<uint8_t>(motor() & 0xFF);
    return sal;
}

// Deriva la clave con sal e iteraciones para encarecer un ataque por fuerza bruta.
string hashPassword(const string& sal, const string& clave) {
    string resultado = sha256(sal + clave);
    for (int i = 1; i < ITERACIONES_HASH; ++i) {
        resultado = sha256(resultado + sal + clave);
    }
    return resultado;
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

// Lee una contrasena. En Windows oculta los caracteres cuando hay consola real;
// si la entrada esta redirigida (pruebas) usa una linea normal.
bool leerPassword(const string& etiqueta, string& destino) {
    cout << etiqueta;
#ifdef _WIN32
    if (_isatty(0)) {
        destino.clear();
        while (true) {
            int tecla = _getch();
            if (tecla == 13) break;  // Enter
            if (tecla == 8) {        // Retroceso
                if (!destino.empty()) {
                    destino.pop_back();
                    cout << "\b \b";
                }
            } else if (tecla >= 32 && tecla < 127) {
                destino.push_back(static_cast<char>(tecla));
                cout << '*';
            }
        }
        cout << "\n";
        return true;
    }
#endif
    if (!getline(cin, destino)) {
        cout << "\n";
        return false;
    }
    return true;
}

bool esPasswordValida(const string& clave) {
    if (clave.size() < 8 || clave.size() > 64) return false;
    return all_of(clave.begin(), clave.end(),
                  [](unsigned char c) { return c >= 32 && c < 127; });
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

bool guardarProfesores(const vector<Profesor>& lista) {
    ofstream salida(ARCHIVO_PROFESORES, ios::trunc);
    if (!salida) {
        aviso("No se pudo abrir " + ARCHIVO_PROFESORES + " para guardar los datos.");
        return false;
    }
    salida << "# cedula|nombres|apellidos|especialidad\n";
    for (const Profesor& p : lista) {
        salida << p.cedula << '|' << p.nombres << '|' << p.apellidos << '|' << p.especialidad
               << '\n';
    }
    if (!salida.good()) {
        aviso("Ocurrió un error al escribir " + ARCHIVO_PROFESORES + ".");
        return false;
    }
    return true;
}

vector<Profesor> cargarProfesores() {
    vector<Profesor> lista;
    ifstream entrada(ARCHIVO_PROFESORES);
    if (!entrada) {
        cout << "  [i] No existe " << ARCHIVO_PROFESORES << "; se inicia un registro vacío.\n";
        return lista;
    }

    string linea;
    int descartadas = 0;
    while (getline(entrada, linea)) {
        string limpia = trim(linea);
        if (limpia.empty() || limpia[0] == '#') continue;

        vector<string> campos = separar(limpia, '|');
        if (campos.size() != 4) {
            ++descartadas;
            continue;
        }

        Profesor p;
        p.cedula = trim(campos[0]);
        p.nombres = trim(campos[1]);
        p.apellidos = trim(campos[2]);
        p.especialidad = trim(campos[3]);

        bool valido = esCedulaValida(p.cedula) && esNombreValido(p.nombres) &&
                      esNombreValido(p.apellidos) && esCarreraValida(p.especialidad);
        bool repetido = false;
        for (const Profesor& otro : lista) {
            if (otro.cedula == p.cedula) repetido = true;
        }
        if (!valido || repetido) {
            ++descartadas;
            continue;
        }
        lista.push_back(p);
    }

    cout << "  [i] " << lista.size() << " profesor(es) cargado(s) desde " << ARCHIVO_PROFESORES
         << ".\n";
    if (descartadas > 0) {
        aviso("Se descartaron " + to_string(descartadas) + " línea(s) inválida(s) o duplicada(s) de " +
              ARCHIVO_PROFESORES + ".");
    }
    return lista;
}

// ---------------------------------------------------------------------------
// Cuentas de acceso
// ---------------------------------------------------------------------------

int buscarUsuario(const vector<Usuario>& usuarios, const string& nombre) {
    for (size_t i = 0; i < usuarios.size(); ++i) {
        if (usuarios[i].nombre == nombre) return static_cast<int>(i);
    }
    return -1;
}

bool guardarUsuarios(const vector<Usuario>& usuarios) {
    ofstream salida(ARCHIVO_USUARIOS, ios::trunc);
    if (!salida) {
        aviso("No se pudo abrir " + ARCHIVO_USUARIOS + " para guardar las cuentas.");
        return false;
    }
    salida << "# usuario|sal|hash_sha256\n";
    for (const Usuario& u : usuarios) {
        salida << u.nombre << '|' << u.sal << '|' << u.hash << '\n';
    }
    if (!salida.good()) {
        aviso("Ocurrió un error al escribir " + ARCHIVO_USUARIOS + ".");
        return false;
    }
    return true;
}

Usuario crearAdminPorDefecto() {
    Usuario u;
    u.nombre = USUARIO_DEFECTO;
    u.sal = aHex(generarSal());
    u.hash = hashPassword(u.sal, CLAVE_DEFECTO);
    return u;
}

vector<Usuario> cargarUsuarios() {
    vector<Usuario> usuarios;

    ifstream entrada(ARCHIVO_USUARIOS);
    if (entrada) {
        string linea;
        int descartadas = 0;
        while (getline(entrada, linea)) {
            string limpia = trim(linea);
            if (limpia.empty() || limpia[0] == '#') continue;

            vector<string> campos = separar(limpia, '|');
            if (campos.size() != 3) {
                ++descartadas;
                continue;
            }

            Usuario u;
            u.nombre = trim(campos[0]);
            u.sal = trim(campos[1]);
            u.hash = trim(campos[2]);
            bool valido = u.nombre.size() >= 3 && u.nombre.size() <= 20 &&
                          esHex(u.sal, 32) && esHex(u.hash, 64);
            if (!valido || buscarUsuario(usuarios, u.nombre) != -1) {
                ++descartadas;
                continue;
            }
            usuarios.push_back(u);
        }
        if (descartadas > 0) {
            aviso("Se descartaron " + to_string(descartadas) + " línea(s) inválida(s) de " +
                  ARCHIVO_USUARIOS + ".");
        }
    }

    if (usuarios.empty()) {
        usuarios.push_back(crearAdminPorDefecto());
        if (guardarUsuarios(usuarios)) {
            cout << "  [i] Se creó el usuario " << USUARIO_DEFECTO << " con la contraseña "
                 << CLAVE_DEFECTO << ".\n";
            cout << "      Cámbiala desde el menú (opción 8) al iniciar sesión.\n";
        }
    }
    return usuarios;
}

bool credencialesValidas(const vector<Usuario>& usuarios, const string& nombre,
                         const string& clave) {
    int indice = buscarUsuario(usuarios, nombre);
    if (indice == -1) return false;
    return hashPassword(usuarios[indice].sal, clave) == usuarios[indice].hash;
}

bool iniciarSesion(const vector<Usuario>& usuarios, string& usuarioActual) {
    cout << "\n--- Inicio de sesión ---\n";
    for (int intento = 1; intento <= INTENTOS_MAXIMOS; ++intento) {
        string nombre;
        if (!leerLinea("  Usuario: ", nombre)) return false;
        string clave;
        if (!leerPassword("  Contraseña: ", clave)) return false;

        if (credencialesValidas(usuarios, nombre, clave)) {
            usuarioActual = nombre;
            cout << "\n";
            exito("Bienvenido, " + nombre + ".");
            return true;
        }
        aviso("Usuario o contraseña incorrectos. Intento " + to_string(intento) + " de " +
              to_string(INTENTOS_MAXIMOS) + ".");
    }
    aviso("Se alcanzó el máximo de intentos. El programa se cerrará.");
    return false;
}

bool cambiarPassword(vector<Usuario>& usuarios, const string& nombre) {
    cout << "\n--- Cambiar contraseña ---\n";
    int indice = buscarUsuario(usuarios, nombre);
    if (indice == -1) {
        aviso("No se encontró la cuenta de la sesión actual.");
        return true;
    }

    string actual;
    if (!leerPassword("  Contraseña actual: ", actual)) return false;
    if (!credencialesValidas(usuarios, nombre, actual)) {
        aviso("La contraseña actual no es correcta. No se hizo ningún cambio.");
        return true;
    }

    string nueva;
    if (!leerPassword("  Nueva contraseña (8-64 caracteres): ", nueva)) return false;
    if (!esPasswordValida(nueva)) {
        aviso("La contraseña debe tener entre 8 y 64 caracteres imprimibles.");
        return true;
    }
    if (nueva == actual) {
        aviso("La nueva contraseña debe ser distinta de la actual.");
        return true;
    }

    string repetida;
    if (!leerPassword("  Repita la nueva contraseña: ", repetida)) return false;
    if (nueva != repetida) {
        aviso("Las contraseñas no coinciden. No se hizo ningún cambio.");
        return true;
    }

    usuarios[indice].sal = aHex(generarSal());
    usuarios[indice].hash = hashPassword(usuarios[indice].sal, nueva);
    if (guardarUsuarios(usuarios)) {
        exito("Contraseña actualizada correctamente.");
    }
    return true;
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
// Registro de profesores
// ---------------------------------------------------------------------------

int buscarProfesorPorCedula(const vector<Profesor>& lista, const string& cedula) {
    for (size_t i = 0; i < lista.size(); ++i) {
        if (lista[i].cedula == cedula) return static_cast<int>(i);
    }
    return -1;
}

bool pedirCedulaProfesor(const vector<Profesor>& lista, const string& actual,
                         bool permitirConservar, string& salida) {
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
        if (texto != actual && buscarProfesorPorCedula(lista, texto) != -1) {
            aviso("Ya existe un profesor registrado con la cédula " + texto + ".");
            continue;
        }
        salida = texto;
        return true;
    }
}

void mostrarDetalleProfesor(const Profesor& p) {
    cout << "  Cédula       : " << p.cedula << "\n";
    cout << "  Nombres      : " << p.nombres << "\n";
    cout << "  Apellidos    : " << p.apellidos << "\n";
    cout << "  Especialidad : " << p.especialidad << "\n";
}

void imprimirTablaProfesores(const vector<Profesor>& lista) {
    const size_t wCedula = 10;
    const size_t wNombre = 35;
    const size_t wEspecialidad = 25;

    cout << "\n  " << ajustar("Cédula", wCedula) << "  " << ajustar("Profesor", wNombre)
         << "  " << ajustar("Especialidad", wEspecialidad) << "\n";
    cout << "  " << string(wCedula, '-') << "  " << string(wNombre, '-') << "  "
         << string(wEspecialidad, '-') << "\n";

    for (const Profesor& p : lista) {
        string nombreCompleto = p.nombres + " " + p.apellidos;
        cout << "  " << ajustar(p.cedula, wCedula) << "  " << ajustar(nombreCompleto, wNombre)
             << "  " << ajustar(p.especialidad, wEspecialidad) << "\n";
    }
    cout << "\n  Total: " << lista.size() << " profesor(es).\n";
}

bool registrarProfesor(vector<Profesor>& lista) {
    cout << "\n--- Registrar profesor ---\n";
    Profesor p;

    if (!pedirCedulaProfesor(lista, "", false, p.cedula)) return false;
    if (!pedirTextoValidado("Nombres", "", false, esNombreValido,
                            "Los nombres solo admiten letras, espacios, guiones y apóstrofos (3-60 caracteres).",
                            p.nombres)) {
        return false;
    }
    if (!pedirTextoValidado("Apellidos", "", false, esNombreValido,
                            "Los apellidos solo admiten letras, espacios, guiones y apóstrofos (3-60 caracteres).",
                            p.apellidos)) {
        return false;
    }
    if (!pedirTextoValidado("Especialidad", "", false, esCarreraValida,
                            "La especialidad debe tener entre 3 y 40 caracteres válidos.",
                            p.especialidad)) {
        return false;
    }

    lista.push_back(p);
    if (guardarProfesores(lista)) {
        exito("Profesor " + p.nombres + " " + p.apellidos + " registrado correctamente.");
    }
    return true;
}

bool buscarProfesor(const vector<Profesor>& lista) {
    cout << "\n--- Buscar profesor ---\n";
    string cedula;
    if (!pedirCedulaSimple(cedula)) return false;

    int indice = buscarProfesorPorCedula(lista, cedula);
    if (indice == -1) {
        aviso("No existe un profesor con la cédula " + cedula + ".");
        return true;
    }
    cout << "\n";
    mostrarDetalleProfesor(lista[indice]);
    return true;
}

bool actualizarProfesor(vector<Profesor>& lista) {
    cout << "\n--- Actualizar profesor ---\n";
    string cedula;
    if (!pedirCedulaSimple(cedula)) return false;

    int indice = buscarProfesorPorCedula(lista, cedula);
    if (indice == -1) {
        aviso("No existe un profesor con la cédula " + cedula + ".");
        return true;
    }

    Profesor& actual = lista[indice];
    cout << "\n  Datos actuales:\n";
    mostrarDetalleProfesor(actual);
    cout << "\n  Deje un campo vacío para conservar el valor actual.\n";

    Profesor nuevo = actual;
    if (!pedirCedulaProfesor(lista, actual.cedula, true, nuevo.cedula)) return false;
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
    if (!pedirTextoValidado("Especialidad", actual.especialidad, true, esCarreraValida,
                            "La especialidad debe tener entre 3 y 40 caracteres válidos.",
                            nuevo.especialidad)) {
        return false;
    }

    actual = nuevo;
    if (guardarProfesores(lista)) {
        exito("Datos del profesor actualizados.");
    }
    return true;
}

bool eliminarProfesor(vector<Profesor>& lista) {
    cout << "\n--- Eliminar profesor ---\n";
    string cedula;
    if (!pedirCedulaSimple(cedula)) return false;

    int indice = buscarProfesorPorCedula(lista, cedula);
    if (indice == -1) {
        aviso("No existe un profesor con la cédula " + cedula + ".");
        return true;
    }

    cout << "\n  Registro a eliminar:\n";
    mostrarDetalleProfesor(lista[indice]);
    if (!confirmar("¿Eliminar este registro?")) {
        cout << "  Operación cancelada.\n";
        return true;
    }

    lista.erase(lista.begin() + indice);
    if (guardarProfesores(lista)) {
        exito("Profesor eliminado del registro.");
    }
    return true;
}

bool menuProfesores(vector<Profesor>& lista) {
    while (true) {
        cout << "\n--- Profesores ---\n";
        cout << "  1. Registrar profesor\n";
        cout << "  2. Listar profesores\n";
        cout << "  3. Buscar por cédula\n";
        cout << "  4. Actualizar profesor\n";
        cout << "  5. Eliminar profesor\n";
        cout << "  0. Volver al menú principal\n";

        string texto;
        if (!leerLinea("  Opción: ", texto)) return false;

        int opcion = 0;
        if (!parsearEntero(texto, opcion)) {
            aviso("Escriba un número de opción válido.");
            continue;
        }

        switch (opcion) {
            case 1:
                if (!registrarProfesor(lista)) return false;
                break;
            case 2:
                cout << "\n--- Lista de profesores ---\n";
                if (lista.empty()) {
                    cout << "  No hay profesores registrados.\n";
                } else {
                    imprimirTablaProfesores(lista);
                }
                break;
            case 3:
                if (!buscarProfesor(lista)) return false;
                break;
            case 4:
                if (!actualizarProfesor(lista)) return false;
                break;
            case 5:
                if (!eliminarProfesor(lista)) return false;
                break;
            case 0:
                return true;
            default:
                aviso("Opción no válida. Elija un número del menú.");
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// Menu principal
// ---------------------------------------------------------------------------

void mostrarMenu(const string& usuario) {
    cout << "\n==========================================\n";
    cout << "    SISTEMA DE REGISTRO ESTUDIANTIL\n";
    cout << "==========================================\n";
    cout << "  Sesión: " << usuario << "\n";
    cout << "------------------------------------------\n";
    cout << "  1. Registrar estudiante\n";
    cout << "  2. Listar estudiantes\n";
    cout << "  3. Buscar por cédula\n";
    cout << "  4. Actualizar estudiante\n";
    cout << "  5. Eliminar estudiante\n";
    cout << "  6. Estadísticas del registro\n";
    cout << "  7. Profesores\n";
    cout << "  8. Guardar cambios\n";
    cout << "  9. Cambiar contraseña\n";
    cout << " 10. Cerrar sesión\n";
    cout << "  0. Salir\n";
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    cout << "\n  Sistema de registro estudiantil (C++)\n";

    vector<Usuario> usuarios = cargarUsuarios();
    vector<Estudiante> lista;
    vector<Profesor> profesores;
    bool datosCargados = false;
    bool ejecutar = true;

    while (ejecutar) {
        string usuarioActual;
        if (!iniciarSesion(usuarios, usuarioActual)) break;

        if (!datosCargados) {
            lista = cargarArchivo();
            profesores = cargarProfesores();
            datosCargados = true;
        }

        bool cerrarSesion = false;
        while (!cerrarSesion && ejecutar) {
            mostrarMenu(usuarioActual);

            string texto;
            if (!leerLinea("  Opción: ", texto)) {
                ejecutar = false;
                break;
            }

            int opcion = 0;
            if (!parsearEntero(texto, opcion)) {
                aviso("Escriba un número de opción válido.");
                continue;
            }

            switch (opcion) {
                case 1:
                    if (!registrarEstudiante(lista)) ejecutar = false;
                    break;
                case 2:
                    listarEstudiantes(lista);
                    break;
                case 3:
                    if (!buscarEstudiante(lista)) ejecutar = false;
                    break;
                case 4:
                    if (!actualizarEstudiante(lista)) ejecutar = false;
                    break;
                case 5:
                    if (!eliminarEstudiante(lista)) ejecutar = false;
                    break;
                case 6:
                    mostrarEstadisticas(lista);
                    break;
                case 7:
                    if (!menuProfesores(profesores)) ejecutar = false;
                    break;
                case 8:
                    if (guardarArchivo(lista) && guardarProfesores(profesores)) {
                        exito("Cambios guardados en " + ARCHIVO + " y " + ARCHIVO_PROFESORES + ".");
                    }
                    break;
                case 9:
                    if (!cambiarPassword(usuarios, usuarioActual)) ejecutar = false;
                    break;
                case 10:
                    cerrarSesion = true;
                    cout << "\n  Sesión cerrada.\n";
                    break;
                case 0:
                    ejecutar = false;
                    break;
                default:
                    aviso("Opción no válida. Elija un número del menú.");
                    break;
            }
        }
    }

    if (datosCargados) {
        bool guardado = guardarArchivo(lista) && guardarProfesores(profesores);
        if (guardado) {
            cout << "\n  Datos guardados en " << ARCHIVO << " y " << ARCHIVO_PROFESORES
                 << ". ¡Hasta pronto!\n";
        } else {
            cout << "\n  No se pudieron guardar todos los datos. ¡Hasta pronto!\n";
        }
    } else {
        cout << "\n  ¡Hasta pronto!\n";
    }
    return 0;
}
