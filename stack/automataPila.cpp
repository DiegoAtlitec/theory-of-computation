// g++ ap.cpp -o ap
// ./ap
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;


struct Transicion {
    string estadoActual;
    char entrada;       
    char cimaPila;      
    string estadoDestino;
    string push;        
};

// Variables globales para el autómata actual
string estadoInicial;
char simboloPilaInicial;
vector<Transicion> listaTransiciones;
bool automataCargado = false;


bool cargarAutomata(string nombreArchivo) {
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        return false;
    }

    
    listaTransiciones.clear();
    
    
    archivo >> estadoInicial >> simboloPilaInicial;

    
    string actual, flecha, sig, push;
    char entrada, cima;
    
    // Formato: q0 a Z -> q0 AZ
    while (archivo >> actual >> entrada >> cima >> flecha >> sig >> push) {
        listaTransiciones.push_back({actual, entrada, cima, sig, push});
    }
    
    archivo.close();
    automataCargado = true;
    return true;
}

// --- MOTOR DE LÓGICA (RECURSIVO) ---
bool esValida(string estadoActual, string cadenaRestante, string pilaActual) {
    
    // 1. CASO DE ÉXITO: Cadena vacía Y Pila vacía
    if (cadenaRestante.empty() && pilaActual.empty()) {
        return true;
    }

    // 2. BUSCAR TRANSICIONES POSIBLES
    for (const auto& t : listaTransiciones) {
        // Coincide estado?
        if (t.estadoActual != estadoActual) continue;

        // Coincide cima de pila? (Si la pila está vacía, no podemos sacar nada)
        if (pilaActual.empty()) continue; 
        if (t.cimaPila != pilaActual.back()) continue; 

        // Coincide entrada? (Es el caracter actual O es epsilon '.')
        if (t.entrada == '.' || (!cadenaRestante.empty() && t.entrada == cadenaRestante[0])) {
            
            // a) Prepara nueva cadena (consumir caracter si no fue epsilon)
            string nuevaCadena = cadenaRestante;
            if (t.entrada != '.') {
                nuevaCadena = cadenaRestante.substr(1);
            }

            // b) Prepara nueva pila
            string nuevaPila = pilaActual;
            nuevaPila.pop_back(); // Sacar cima (pop)
            
            // Meter nuevos símbolos (push)
            if (t.push != ".") {
                // Se meten en orden inverso para que el primero quede arriba
                for (int i = t.push.length() - 1; i >= 0; i--) {
                    nuevaPila.push_back(t.push[i]);
                }
            }

            // c) Llamada recursiva (Backtracking)
            if (esValida(t.estadoDestino, nuevaCadena, nuevaPila)) {
                return true; 
            }
        }
    }
    return false;
}


void probarCadenas(string nombreArchivo) {
    string cadena;
    while (true) {
        cout << "\n--- PROBANDO: " << nombreArchivo << " ---" << endl;
        cout << "Escribe la cadena a analizar (o 'menu' para volver): ";
        cin >> cadena;

        if (cadena == "menu") break;

        // Inicializar pila con el símbolo inicial
        string pila(1, simboloPilaInicial);

        // cout << ">> Analizando... ";
        if (esValida(estadoInicial, cadena, pila)) {
            cout << "\nVALIDA" << endl;
        } else {
            cout << "\nINVALIDA" << endl;
        }
    }
}

int main() {
    int opcion;
    do {
        
        cout << "   INTERPRETE DE AUTOMATAS DE PILA\n" << endl;
        
        cout << "1. Cargar Automata 1" << endl;
        cout << "2. Cargar Automata 2" << endl;
        cout << "3. Cargar Automata 3" << endl;
        cout << "0. Salir" << endl;
        cout << "Elige una opcion: ";
        cin >> opcion;

        string archivoSeleccionado = "";
        
        switch (opcion) {
            case 1: archivoSeleccionado = "uno.txt"; break;
            case 2: archivoSeleccionado = "dos.txt"; break;
            case 3: archivoSeleccionado = "tres.txt"; break;
            case 0: cout << "Saliendo..." << endl; break;
            default: cout << "Opcion no valida." << endl;
        }

        if (opcion != 0 && archivoSeleccionado != "") {
            if (cargarAutomata(archivoSeleccionado)) {
                cout << "\n[OK] Automata cargado correctamente." << endl;
                probarCadenas(archivoSeleccionado);
            } else {
                cout << "\n[ERROR] No se pudo abrir el archivo " << archivoSeleccionado << endl;
                cout << "Asegurate de que el .txt esta en la misma carpeta." << endl;
            }
        }

    } while (opcion != 0);

    return 0;
}