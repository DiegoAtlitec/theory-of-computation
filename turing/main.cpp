// g++ -std=c++11 main.cpp -o maquina


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace std;

struct Transicion {
    string estado_siguiente;
    char escribir;
    char direccion; 
};

typedef pair<string, char> Clave;

class MaquinaTuring {
private:
    map<Clave, Transicion> tabla;
    string estado_actual;
    string cinta;
    int cabezal;
    vector<string> estados_finales;

public:
    MaquinaTuring(string archivo, string cinta_input) {
        cargarMaquina(archivo);
        this->cinta = cinta_input;
        // Relleno de seguridad
        this->cinta.insert(0, "BBBBBBBBBB"); 
        this->cinta.append("BBBBBBBBBB");
        this->cabezal = 10; 
    }

    void cargarMaquina(string nombre_archivo) {
        ifstream archivo(nombre_archivo);
        if (!archivo.is_open()) {
            cerr << "Error al abrir el archivo: " << nombre_archivo << endl;
            // No hacemos exit(1) aquí para no matar el programa entero, 
            // pero en un caso real deberíamos manejar el error mejor.
            return;
        }

        string linea;
        archivo >> estado_actual; 

        string linea_finales; 
        archivo >> linea_finales; 
        estados_finales.push_back(linea_finales);

        string q_act, q_sig;
        char leido, escrito, dir;

        while (archivo >> q_act >> leido >> q_sig >> escrito >> dir) {
            tabla[{q_act, leido}] = {q_sig, escrito, dir};
        }
        archivo.close();
    }

    void mostrarCinta() {
        system("clear"); 

        
        cout << "Estado Actual: " << estado_actual << endl;
        cout << "-----------------------------------" << endl;
        
        int ventana_inicio = max(0, cabezal - 15);
        int ventana_fin = min((int)cinta.length(), cabezal + 15);

        for (int i = ventana_inicio; i < ventana_fin; i++) {
            cout << cinta[i];
        }
        cout << endl;

        for (int i = ventana_inicio; i < cabezal; i++) {
            cout << " ";
        }
        cout << "^" << endl;
        cout << "-----------------------------------" << endl;
    }

    void ejecutar() {
        if(tabla.empty()) return; // Si falló la carga

        while (true) {
            mostrarCinta();
            
            // Verificar fin
            for(const string& final : estados_finales) {
                if(estado_actual == final) {
                    cout << "\n EXITO [FIN] Estado final alcanzado: " << final << endl;
                    cout << "Resultado final en cinta: " << cinta << endl;
                    return;
                }
            }

            char caracter_leido = cinta[cabezal];
            // Verificar error/crash
            if (tabla.find({estado_actual, caracter_leido}) == tabla.end()) {
                cout << "\n TRUENA [HALT] La maquina se detuvo (Sin transicion definida)." << endl;
                cout << "Estado: " << estado_actual << " | Leyendo: " << caracter_leido << endl;
                cout << "Cinta final: " << cinta << endl;
                return;
            }

            Transicion t = tabla[{estado_actual, caracter_leido}];
            cinta[cabezal] = t.escribir;
            estado_actual = t.estado_siguiente;

            if (t.direccion == 'R') cabezal++;
            else if (t.direccion == 'L') cabezal--;
            else if (t.direccion == 'S'); // Stay
            
            if (cabezal < 0) {
                cinta.insert(0, "B");
                cabezal = 0;
            }
            if (cabezal >= cinta.length()) {
                cinta.append("B");
            }

            this_thread::sleep_for(chrono::milliseconds(200));
        }
    }
};

void mostrarMenu() {
    system("clear");
    cout << "" << endl;
    
    cout << "Seleccione la maquina a cargar:" << endl;
    cout << "1. Suma Binaria" << endl;
    cout << "2. Inversor de Bits " << endl;
    cout << "3. Detector Palindromos a,b,c" << endl;
    cout << "4. 1" << endl;
    cout << "5. 2" << endl;
    cout << "6. 3" << endl;
    cout << "7. 4" << endl;

    cout << "8. Salir" << endl;
    cout << "Opcion: ";
}

int main() {
    int opcion;
    string archivo, input;
    char continuar;

    do {
        mostrarMenu();
        if (!(cin >> opcion)) { // Validación básica de entrada numérica
            cin.clear();
            cin.ignore(1000, '\n');
            opcion = 0;
        }

        if (opcion == 8) break;

        switch(opcion) {
            case 1: 
                archivo = "suma.txt"; 
                cout << "Ingrese la suma: ";
                break;
            case 2: 
                archivo = "inversor.txt"; 
                cout << "Ingrese binario : ";
                break;
            case 3: 
                archivo = "palindromo.txt"; 
                cout << "Ingrese cadena (usar a,b,c): ";
                break;
            case 4: 
                archivo = "1.txt"; 
                cout << "Ingrese cadena : ";
                break;
            case 5:
                archivo = "2.txt"; 
                cout << "Ingrese cadena : ";
                break;
            case 6:
                archivo = "3.txt"; 
                cout << "Ingrese cadena : ";
                break;
            case 7:
                archivo = "4.txt"; 
                cout << "Ingrese cadena : ";
                break;

            default: 
                cout << "Opcion invalida." << endl;
                this_thread::sleep_for(chrono::milliseconds(1000));
                continue;
        }

        cin >> input;
        
        // Ejecutar máquina
        MaquinaTuring mt(archivo, input);
        mt.ejecutar();

        cout << "\nPresione 's' para menu, para salir: ";
        cin >> continuar;

    } while (continuar == 's' || continuar == 'S');

    
    return 0;
}
