// g++ -std=c++11 markov.cpp -o markov

// ./markov

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <random>
#include <map>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace std;

// Función simple para determinar si es vocal
bool esVocal(char c) {
    c = tolower(c);
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

// Estructura para guardar las probabilidades de transición
struct MarkovProps {
    // Probabilidad de ir a V dado que estoy en V
    double v_to_v; 
    // Probabilidad de ir a V dado que estoy en C
    double c_to_v;
    // (Los opuestos se calculan restando de 1)
};

int main() {
    string filename = "txt.txt";
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: No se encuentra 'texto.txt'. Ejecuta el comando curl primero." << endl;
        return 1;
    }

    string sequence = "";
    char c;
    
    // 1. Lectura y Transformación (v/c)
    //cout << "Leyendo archivo y transformando a 'cvcv'..." << endl;
    while (file.get(c)) {
        if (isalpha(c)) {
            if (esVocal(c)) sequence += 'v';
            else sequence += 'c';
        }
    }
    file.close();

    if (sequence.length() < 2) {
        cerr << "El texto es muy corto." << endl;
        return 1;
    }

    // 2. Análisis de frecuencias (Pares)
    long long total_v = 0, total_c = 0;
    long long vv = 0, vc = 0, cv = 0, cc = 0;

    for (size_t i = 0; i < sequence.length() - 1; ++i) {
        char current = sequence[i];
        char next = sequence[i+1];

        if (current == 'v') {
            total_v++;
            if (next == 'v') vv++;
            else vc++;
        } else { // current == 'c'
            total_c++;
            if (next == 'v') cv++;
            else cc++;
        }
    }

    // Calcular Matriz de Transición (Probabilidades condicionales)
    // P(v|v) = Probabilidad de que la siguiente sea v DADO QUE la actual es v
    double p_v_given_v = (total_v > 0) ? (double)vv / total_v : 0.5;
    double p_c_given_v = 1.0 - p_v_given_v;
    
    double p_v_given_c = (total_c > 0) ? (double)cv / total_c : 0.5;
    double p_c_given_c = 1.0 - p_v_given_c;

    // Estadísticas del texto original
    double total_chars = sequence.length();
    double original_v_ratio = (total_v + total_c > 0) ? (double)(total_v + (sequence.back() == 'v' ? 1 : 0)) / total_chars : 0;

    cout << "\n ESTADISTICAS DEL TEXTO ORIGINAL" << endl;
    cout << "Total letras procesadas: " << total_chars << endl;
    cout << "Proporción Vocales: " << fixed << setprecision(2) << original_v_ratio * 100 << "%" << endl;
    cout << "Proporción Consonantes: " << fixed << setprecision(2) << (1.0 - original_v_ratio) * 100 << "%" << endl;
    
    cout << "\nMATRIZ DE TRANSICION (Probabilidades) " << endl;
    cout << "Si tengo V, siguiente es V: " << p_v_given_v * 100 << "% (vv)" << endl;
    cout << "VC: " << p_c_given_v * 100 << "% (vc)" << endl;
    cout << "CV: " << p_v_given_c * 100 << "% (cv)" << endl;
    cout << "CC: " << p_c_given_c * 100 << "% (cc)" << endl;

    cout << "\nPresiona ENTER para iniciar la generación y ver la gráfica..." << endl;
    cin.ignore();

    // 3. Generación y Visualización
    // Configuración de aleatoriedad
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);

    string generated = "";
    // Empezamos aleatoriamente
    char current_state = (dis(gen) < 0.5) ? 'v' : 'c';
    generated += current_state;

    int gen_v_count = (current_state == 'v') ? 1 : 0;
    int target_length = 2000; // Generaremos 2000 caracteres

    cout << "\nGENERANDO CADENA DE MARKOV" << endl;
    cout << "Objetivo: Ver si la generada se equilibra igual que la original (" 
         << (int)(original_v_ratio*100) << "% Vocales)\n" << endl;

    for (int i = 0; i < target_length; ++i) {
        double r = dis(gen);
        char next_state;

        // Lógica Markov: Decidir siguiente basado en el ACTUAL y las probabilidades calculadas
        if (current_state == 'v') {
            if (r < p_v_given_v) next_state = 'v';
            else next_state = 'c';
        } else { // current_state == 'c'
            if (r < p_v_given_c) next_state = 'v'; // Nota: usamos p_v_given_c aquí
            else next_state = 'c';
        }

        generated += next_state;
        if (next_state == 'v') gen_v_count++;
        current_state = next_state;

        
        if (i % 20 == 0) {
            double current_ratio = (double)gen_v_count / generated.length();
            
            // Limpiar linea actual (retorno de carro)
            cout << "\rGenerando: [";
            
            
            int barWidth = 50;
            int pos = barWidth * current_ratio;
            for (int j = 0; j < barWidth; ++j) {
                if (j < pos) cout << "=";
                else if (j == pos) cout << ">";
                else cout << " ";
            }
            cout << "] " << (int)(current_ratio * 100) << "% Vocales (Meta: " << (int)(original_v_ratio*100) << "%)" << flush;
            
            // Pequeña pausa para ver la animación
            this_thread::sleep_for(chrono::milliseconds(20)); 
        }
    }

    cout << "\n\n RESULTADO FINAL" << endl;
    cout << "Inicio de la cadena generada: " << generated.substr(0, 1000) << endl;
    
    return 0;
}