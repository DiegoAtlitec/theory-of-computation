#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <string>

// Definir implementaciones para las librerias stb
#include "stb_image.h"

#include "stb_image_write.h"

using namespace std;

// Estructura para un pixel o un centroide
struct Color {
    double r, g, b;
};

// Calcular distancia euclidiana al cuadrado (más rápido que sacar raíz cuadrada)
double getDistanceSq(const Color& a, const Color& b) {
    return (a.r - b.r)*(a.r - b.r) + (a.g - b.g)*(a.g - b.g) + (a.b - b.b)*(a.b - b.b);
}

int main() {
    // 1. Obtener datos del usuario
    string inputPath;
    int k;

    cout << "--- K-Means Image Segmenter (Sin OpenCV) ---" << endl;
    cout << "Arrastra tu imagen desde el Finder aqui y presiona Enter: ";
    // Usamos getline para capturar espacios si la ruta los tiene
    getline(cin, inputPath);
    
    // Limpiar comillas si el Finder las agregó
    if (!inputPath.empty() && inputPath.front() == '\'') inputPath.erase(0, 1);
    if (!inputPath.empty() && inputPath.back() == '\'') inputPath.pop_back();
    // Eliminar espacio al final si existe (común en terminal)
    if (!inputPath.empty() && inputPath.back() == ' ') inputPath.pop_back();

    cout << "Ingresa el valor de K (numero de colores): ";
    cin >> k;

    // 2. Cargar Imagen
    int width, height, channels;
    unsigned char* img = stbi_load(inputPath.c_str(), &width, &height, &channels, 3); // Forzar 3 canales (RGB)

    if (img == NULL) {
        cerr << "Error: No se pudo cargar la imagen. Revisa la ruta." << endl;
        return -1;
    }

    int numPixels = width * height;
    vector<Color> pixels(numPixels);

    // Convertir array de bytes a vector de nuestra estructura Color
    for (int i = 0; i < numPixels; ++i) {
        pixels[i].r = img[i * 3 + 0];
        pixels[i].g = img[i * 3 + 1];
        pixels[i].b = img[i * 3 + 2];
    }

    // 3. Inicializar Centroides (K-Means++)
    // Para simplificar, elegimos K pixeles aleatorios de la imagen original
    srand(time(0));
    vector<Color> centroids(k);
    for (int i = 0; i < k; ++i) {
        centroids[i] = pixels[rand() % numPixels];
    }

    vector<int> assignments(numPixels);
    bool changed = true;
    int maxIterations = 50; // Limite de seguridad para evitar loops infinitos
    int iter = 0;

    cout << "Procesando... (Esto puede tardar dependiendo del tamaño)" << endl;

    // 4. Algoritmo K-Means
    while (changed && iter < maxIterations) {
        changed = false;
        
        // Paso A: Asignar cada pixel al centroide más cercano
        for (int i = 0; i < numPixels; ++i) {
            double minDist = numeric_limits<double>::max();
            int bestCluster = 0;
            
            for (int c = 0; c < k; ++c) {
                double dist = getDistanceSq(pixels[i], centroids[c]);
                if (dist < minDist) {
                    minDist = dist;
                    bestCluster = c;
                }
            }
            
            if (assignments[i] != bestCluster) {
                assignments[i] = bestCluster;
                changed = true;
            }
        }

        // Paso B: Recalcular centroides
        vector<Color> newCentroids(k, {0, 0, 0});
        vector<int> counts(k, 0);

        for (int i = 0; i < numPixels; ++i) {
            int clusterId = assignments[i];
            newCentroids[clusterId].r += pixels[i].r;
            newCentroids[clusterId].g += pixels[i].g;
            newCentroids[clusterId].b += pixels[i].b;
            counts[clusterId]++;
        }

        for (int c = 0; c < k; ++c) {
            if (counts[c] > 0) {
                centroids[c].r = newCentroids[c].r / counts[c];
                centroids[c].g = newCentroids[c].g / counts[c];
                centroids[c].b = newCentroids[c].b / counts[c];
            } else {
                // Si un cluster queda vacio, reinicializar aleatoriamente
                 centroids[c] = pixels[rand() % numPixels];
            }
        }
        
        iter++;
        cout << "Iteracion " << iter << " completada." << endl;
    }

    // 5. Reconstruir la imagen con los colores segmentados
    unsigned char* outImg = new unsigned char[numPixels * 3];
    for (int i = 0; i < numPixels; ++i) {
        int clusterId = assignments[i];
        outImg[i * 3 + 0] = (unsigned char)centroids[clusterId].r;
        outImg[i * 3 + 1] = (unsigned char)centroids[clusterId].g;
        outImg[i * 3 + 2] = (unsigned char)centroids[clusterId].b;
    }

    // 6. Guardar resultado
    string outputPath = "resultado_kmeans.jpg";
    stbi_write_jpg(outputPath.c_str(), width, height, 3, outImg, 100);

    cout << "Listo! Imagen guardada como: " << outputPath << endl;

    // Liberar memoria
    stbi_image_free(img);
    delete[] outImg;

    // 7. Mostrar resultado usando comandos de sistema de Mac
    // El comando 'open' en Mac abre el archivo con la app predeterminada (Vista Previa)
    string cmdOriginal = "open \"" + inputPath + "\"";
    string cmdResult = "open " + outputPath;
    
    system(cmdOriginal.c_str());
    system(cmdResult.c_str());

    return 0;
}