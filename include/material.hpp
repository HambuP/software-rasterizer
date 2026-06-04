#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>
#include "math.hpp"

struct Col { //estructura de color normalita
    int r, g, b;
};

class Material {
public:

    //cada material va a tener un nombre, su textura, osea una lista de colores, tamaño y sus constantes Ns,Ka,Ks,Ks para luz
    std::string nombre;
    std::vector <Col> texture;
    uint32_t width, height;
    float shininess; //que es Ns
    Vec3 ambient; //Ka
    Vec3 diffuse; //Kd
    Vec3 specular; //Ks

    //que es explicit? resulta que c++ a veces hace cosas extrañas con funciones de la clase, y a veces por razones
    //no muy epicas puede construir un nuevo material desde ellas, el explicit obliga a construir un material con el constructor
    explicit Material(const std::string &nombre, const std::string &mtl, const std::string &base) : nombre(nombre) {

        std::string direccion; //inicializamos la direccion de la imagen del material
        std::ifstream file(mtl); //abrimos el archivo y lo abrimos en file

        if (!file) {
            std::cout << "ERROR: no se pudo abrir mtl: " << mtl << "\n";
            return;
        }

        if (file) {

            std::string line;
            std::string material;

            while(std::getline(file,line)) { //vamos linea por linea

                std::istringstream iss(line); //agarra la linea y la trata como si fuera una linea de tokens
                std::string word;

                iss >> word; //vamos palabra por palabra, word lee un token de iss

                if (word == "Ns") {
                    iss >> word;
                    shininess = std::stof(word); //convertimos el string a float
                }

                if (word == "Ka") {
                    iss >> word;
                    ambient.x = std::stof(word);
                    iss >> word;
                    ambient.y = std::stof(word);
                    iss >> word;
                    ambient.z = std::stof(word);
                }

                if (word == "Kd") {
                    iss >> word;
                    diffuse.x = std::stof(word);
                    iss >> word;
                    diffuse.y = std::stof(word);
                    iss >> word;
                    diffuse.z = std::stof(word);
                }

                if (word == "Ks") {
                    iss >> word;
                    specular.x = std::stof(word);
                    iss >> word;
                    specular.y = std::stof(word);
                    iss >> word;
                    specular.z = std::stof(word);
                }

                if (word == "newmtl") { //recupera el nombre del material
                    iss >> word;
                    material = word;
                }

                if (word == "map_Kd") { //recupera la direccion correspondiente al material que buscamos
                    iss >> word;
                    if (nombre == material) {
                        direccion = word;
                    }
                }
            }
        }

        file.close(); //cerramos el archivo

        std::ifstream textura(base + direccion,std::ifstream::binary);//abrimos la textura en modo binario, ya que es bmp

        if (!textura) {
            std::cout << "ERROR: no se pudo abrir textura: " << direccion << "\n";
            return;
        }

        if (textura) { //si la textura se abre

            textura.seekg(0,textura.end); //ponemos "el cursor" al final del textura
            unsigned int length = textura.tellg(); //guardamos en una variable la longitud, osea, en que posición estamos en este caso
            textura.seekg(0,textura.beg); //volvemos al inicio

            auto buffer = new char[length]; //creamos el buffer donde vamos a guardar la info, debe tener claro, la misma longitud

            textura.read(buffer,length); //leemos y guardamos en el buffer

            if (textura) { //verificamos rapidamente si se puede leer toda el textura, buena práctica para debuggear y identificar errores
                std::cout << "all characters read successfully \n";
            } else {
                std::cout << "error only " << textura.gcount() << " could be read";
            }

            uint32_t start = //sabemos por el signature de los header de los bmp que se localizan ahí estas infos, ahora, para evitar
                                //cualquier error por el hecho de que las cosas son char* usamos el static cast uint8t,
                static_cast<uint8_t>(buffer[10]) |
                (static_cast<uint8_t>(buffer[11]) << 8) |
                (static_cast<uint8_t>(buffer[12]) << 16) |
                (static_cast<uint8_t>(buffer[13]) << 24);

            width =
                static_cast<uint8_t>(buffer[18]) |
                (static_cast<uint8_t>(buffer[19]) << 8) |
                (static_cast<uint8_t>(buffer[20]) << 16) |
                (static_cast<uint8_t>(buffer[21]) << 24);

            height =
                static_cast<uint8_t>(buffer[22]) |
                (static_cast<uint8_t>(buffer[23]) << 8) |
                (static_cast<uint8_t>(buffer[24]) << 16) |
                (static_cast<uint8_t>(buffer[25]) << 24);

            uint16_t bitsPerPixel =
            static_cast<uint8_t>(buffer[28]) |
            (static_cast<uint8_t>(buffer[29]) << 8);

            int bytesPerPixel = bitsPerPixel / 8;
            int stride = width * bytesPerPixel;


            int padding = (bitsPerPixel==24)? (4-((width*3)%4))%4 : 0; //significa, cuanto le falta a la linea de bits para que pueda divirse por 4
            std::vector<Col> colores(width*height,{0,0,0}); //inicializamos la textura

            std::cout << "start: " << start << "\n";
            std::cout << "width: " << width << "\n";
            std::cout << "height: " << height << "\n";
            std::cout << "padding: " << padding << "\n";

            for (int j= height-1; j >= 0; j--){ //recorremos el archivo de abajo hacia arriba por convencion de bmp
                for (int i = 0; i < width; i++){ //recorremos cada tres para guardar el r,g y b

                    int idx = j*(stride + padding) + i*bytesPerPixel+start;

                    //los archivos bmp usan orden bgr por convención, en vez del epico y conocido rgb
                    //aquí esta medio raro, pero cada linea pues mide width*3+padding, pero claro
                    //le sumamos el start que es donde comienza la textura en sí
                    //en la lista de colores, restamos j justamente porque colores va de arriba hacia abajo y bmp al reves
                    colores[(height-1-j)*width + i].b = static_cast<unsigned int>(static_cast<unsigned char>(buffer[idx])); //b
                    colores[(height-1-j)*width + i].g = static_cast<unsigned int>(static_cast<unsigned char>(buffer[idx + 1])); //g
                    colores[(height-1-j)*width + i].r = static_cast<unsigned int>(static_cast<unsigned char>(buffer[idx + 2])); //r

                }
            }

            delete[] buffer; //borramos el buffer para liberar memoria
            texture = colores;//asignamos los colores a la textura

        }

    textura.close(); //cerramos el archivo
    }
};