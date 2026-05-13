#pragma once
#include "math.hpp"
#include <algorithm>
#include <vector>
#include <SDL3/SDL.h>
#include "mesh.hpp"
#include <complex>
#include <thread>

#include "material.hpp"


struct tuple {
    int x,y;
};

struct Triangulo {

    Vec3 p1,p2,p3; //los vertices del triangulo en el espacio de pantalla
    Vec3 real1,real2,real3; //vertices originales sin transformar, espacio mundo 3d
    Vec2 uv1,uv2,uv3; //coordenadas de textura de cada vertice
    Vec3 n1,n2,n3; //normales de cada vertice
    const std::vector<Col>* texture; //textura, una lista de colores simplemente
    uint32_t tex_width,tex_height; //ancho y altura de la textura
    float shininess; //constante de shininess del material
    Vec3 ks,ka,kd; //constantes de specular, ambient y diffuse del material
    ShadingMode shading_mode; //modo de sombreado del material

};

class Render {
public:

    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int WIDTH, HEIGHT;
    std::vector<Uint32> framebuffer; //creamos un framebuffer, que es un array de pixeles, con el mismo tamaño que la ventana, y lo inicializamos con 0
    std::vector<float> zbuffer; //creamos un zbuffer, que es un array de floats, y lo inicializamos con el valor más grande posible

    unsigned int num_cores = std::thread::hardware_concurrency(); //obtenemos el número de núcleos del procesador para dividir el trabajo entre ellos

    std::vector<tuple> threads;


    Render(SDL_Renderer* renderer, const int width, const int height) : renderer(renderer), WIDTH(width), HEIGHT(height),
                                                framebuffer(width * height, 0), zbuffer(width * height, HUGE_VALF) {

        int lines_per_t = height / num_cores; //calculamos el numero de filas para cada thread
        int lines_rest = height % num_cores; //calculamos el resto que le vamos a dar al ultimo thread

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                            WIDTH,HEIGHT); //creamos la textura, que es lo que nos va a servir para pintar en el renderer

        for (int i = 0; i < num_cores; i++) {

            threads.push_back({i*lines_per_t,(i+1)*lines_per_t - 1}); // guardamos en esta epica lista los valores para cada core

        }

        threads.back().y += lines_rest; //le ponemos al ultimo thread lo que falte

    }


    void Rasterize_threads(const std::vector<Triangulo>& triangulos , const int &miny_t, const int &maxy_t) {

        //Vec3 const &pA, Vec3 const &pB, Vec3 const &pC, Vec2 const &uvA, Vec2 const &uvB, Vec2 const &uvC, std::vector<Col> const &colores, const
        //uint32_t &tex_width, const uint32_t &tex_height, const int &miny_t, const int &maxy_t

        for (auto& tri : triangulos) {

            ShadingMode shading_mode = tri.shading_mode;

            Vec3 pA = tri.p1;
            Vec3 pB = tri.p2;
            Vec3 pC = tri.p3;

            Vec2 uvA = tri.uv1;
            Vec2 uvB = tri.uv2;
            Vec2 uvC = tri.uv3;

            Vec3 norA = tri.n1;
            Vec3 norB = tri.n2;
            Vec3 norC = tri.n3;

            Vec3 realA = tri.real1;
            Vec3 realB = tri.real2;
            Vec3 realC = tri.real3;

            float const shininess = tri.shininess;
            Vec3 const ks = tri.ks;
            Vec3 const ka = tri.ka;
            Vec3 const kd = tri.kd;

            Vec3 eye = {0,0,3}; //posicion de la camara
            Vec3 light_dir = {-1,-1,-1}; //direccion de la luz, en este caso, una luz direccional que viene de esa direccion

            light_dir = light_dir.normalize() ; //normalizamos la direccion de la luz para que tenga longitud 1, esto es importante para el calculo de la iluminacion

            const float Ia   = 0.01f;  // ambiental muy bajo — solo para que las sombras no sean negras
            const float Iluz = 1.0f;  // luz a full

            const std::vector<Col> &colores = *tri.texture;

            const uint32_t tex_width = tri.tex_width;
            const uint32_t tex_height = tri.tex_height;

            int minx = static_cast<int>(std::min(pA.x, std::min(pB.x, pC.x))); // el static cast es para convertir de float a int
            int miny = static_cast<int>(std::min(pA.y, std::min(pB.y, pC.y)));
            int maxx = static_cast<int>(std::max(pA.x, std::max(pB.x, pC.x)));
            int maxy = static_cast<int>(std::max(pA.y, std::max(pB.y, pC.y)));

            minx = std::max(minx, 0); //asegurarse de mantenerse en pantalla of course
            miny = std::max(miny, 0);
            maxx = std::min(maxx, WIDTH - 1);
            maxy = std::min(maxy, HEIGHT - 1);

            //definir el espacio para el thread
            miny = std::max(miny,miny_t);
            maxy = std::min(maxy,maxy_t);

            //para todos esos pixeleles, usando edge function para determinar si estan o no, pues ponerlos

            const Vec2 pA_2d = {pA.x, pA.y};
            const Vec2 pB_2d = {pB.x, pB.y};
            const Vec2 pC_2d = {pC.x, pC.y};

            const Vec2 ladoAB = pB_2d - pA_2d;
            const Vec2 ladoBC = pC_2d - pB_2d;
            const Vec2 ladoCA = pA_2d - pC_2d;

            const float w1 = pA.z;
            const float w2 = pB.z;
            const float w3 = pC.z;

            for (int i = minx; i <= maxx; i++) {
                for (int j = miny; j <= maxy; j++) {
                    Vec2 p = {static_cast<float>(i), static_cast<float>(j)};

                    //definir si esta o no esta en triangulo

                    float const edge1 = edge_function(p - pA_2d,ladoAB); //usamos para interpolar el punto c
                    float const edge2 = edge_function(p - pB_2d,ladoBC); //usamos para interpolar el punto a
                    float const edge3 = edge_function(p - pC_2d,ladoCA); //usamos para interpolar el punto b

                    if ((edge1 >= 0 && edge2 >= 0 && edge3 >= 0)||(edge1 <= 0 && edge2 <= 0 && edge3 <= 0)) {
                        //pintar pixel

                        float const area_total = edge1 + edge2 + edge3;

                        float const e1_norm = edge1 / area_total;
                        float const e2_norm = edge2 / area_total;
                        float const e3_norm = edge3 / area_total;

                        float depth = e1_norm*(1/w3) + e2_norm*(1/w1) + e3_norm*(1/w2); //interpolamos la profundidad con correcta profundidad
                        depth = 1/depth;

                        Vec3 normal = norC*e1_norm*(1/w3) + norA*e2_norm*(1/w1) + norB*e3_norm*(1/w2); //interpolamos la normal con interpolacion correcta por profundidad
                        Vec3 real_p = realC * e1_norm*(1/w3) + realA * e2_norm*(1/w1) + realB * e3_norm*(1/w2); //interpolamos el punto en el espacio mundo 3d con interpolacion correcta por profundidad
                        Vec2 uv_coor = uvC*e1_norm*(1/w3) + uvA*e2_norm*(1/w1) + uvB*e3_norm*(1/w2); //interpolamos las coordenadas uv con interpolacion correcta por profundidad

                        uv_coor = uv_coor* depth; //volvemos a multiplicar por depth para que nos de bien
                        real_p = real_p * depth; //volvemos a multiplicar por depth para que nos de bien
                        normal = normal * depth; //volvemos a multiplicar por depth para que nos de bien

                        normal = normal.normalize(); //normalizamos la normal para que tenga longitud 1, esto es importante para el calculo de la iluminacion

                        Vec3 reflejo = (normal * (2 * (normal * light_dir)) - light_dir).normalize(); //calculamos el vector de reflejo usando la formula de reflexion, y lo normalizamos
                        Vec3 dir_cam = (eye - real_p).normalize(); //calculamos el vector que va del punto al ojo, y lo normalizamos

                        Vec3 inten_spec = {0,0,0}; //lo inicializamos a cero para despues mirar si el triangulo mira a la luz o no

                        if (normal * light_dir > 0) {
                            //si el triangulo mira a la luz, calculamos la intensidad especular, si no, se queda en cero, esto es para evitar que las partes que no miran a la luz tengan reflejo especular, lo cual no tiene sentido
                            inten_spec = ks * Iluz * powf(std::max(reflejo * dir_cam, 0.0f), shininess); //intensidad especular, es la constante de specular multiplicada por la intensidad de la luz, multiplicada por el coseno del angulo entre el vector de reflejo y el vector que va al ojo, elevado a la potencia de shininess
                        }

                        Vec3 inten_amb = ka * Ia; //intensidad ambiental, es la constante de ambient multiplicada por la intensidad de la luz ambiental
                        //aquí hay una cosita distinta, pues ka normalmente es la constante, pero es muy inconsistente que a veces es mejor simplemente usar difuse

                        if (depth < zbuffer[j * WIDTH + i]) {
                            zbuffer[j * WIDTH + i] = depth;
                        } else {
                            continue; //si el pixel que queremos pintar es más profundo que el que ya está en el zbuffer, no lo pintamos
                        }

                        if (colores.empty() || tex_width == 0 || tex_height == 0) {

                            //si no hay textura, o la textura tiene ancho o alto 0, no pintamos nada, esto es para evitar errores de división por cero o acceso a memoria inválida
                            Vec3 inten_dif = kd * Iluz * std::max(normal * light_dir, 0.0f); //intensidad difusa, es la constante de diffuse multiplicada por la intensidad de la luz, multiplicada por el coseno del angulo entre la normal y la direccion de la luz

                            Vec3 color_final = inten_amb + inten_dif + inten_spec; //calculamos el color final sumando las intensidades ambiental, difusa y especular

                            //aquí definimos si hay toon shading y aplicamos eso
                            if (shading_mode == ShadingMode::TOON) {

                                float diff_factor = std::max(normal * light_dir, 0.0f); //el dot de la normal con la direccion de la luz, con esto vamos a hacer el clamp para el toon shading
                                float view_factor = dir_cam*normal; //el dot de la direccion del punto con la normal del punto, si el dot es casi cero entonces es borde del objeto, esto es para el borde
                                float spec_factor = std::max(reflejo * dir_cam, 0.0f); //el dot del vector de reflejo con la direccion del punto, esto es para el brillo

                                //outline
                                if (view_factor < 0.2f) {//si el dot de la direccion del punto con la normal es menor a 0.3, entonces es borde negro
                                    color_final = {0,0,0};
                                }

                                //specular sin difuminado
                                else if (spec_factor > 0.9f){//si el dot del vector de reflejo con la direccion del punto es mayor a 0.8, entonces es un brillo blanco
                                    color_final = {1,1,1};
                                }

                                //diffuse cuantizado
                                else {

                                    float toon;
                                    if (diff_factor > 0.75f) toon = 1.0f;
                                    else if (diff_factor > 0.5f) toon = 0.75f;
                                    else if (diff_factor > 0.25f) toon = 0.5f;
                                    else toon = 0.25f;

                                    color_final = kd * toon *Iluz; //finalmente definimos el color, aquí es usando kd porque no hay textura

                                }
                            }

                            Uint32 colorin = (255 << 24) | (static_cast<int>(std::min(color_final.x * 255.0f, 255.0f)) << 16) | (static_cast<int>(std::min(color_final.y * 255.0f, 255.0f)) << 8) | static_cast<int>(std::min(color_final.z * 255.0f, 255.0f)); //convertimos el color final a formato ARGB, asegurándonos de que cada componente esté en el rango [0, 255]

                            framebuffer[j * WIDTH + i] = colorin; //esto es para pintar el pixel, el framebuffer es un array de pixeles, y cada pixel es un Uint32, que es un entero de 32 bits, y cada bit representa un canal de color (RGBA)

                        } //si no hay textura, o la textura tiene ancho o alto 0, no pintamos nada, esto es para evitar errores de división por cero o acceso a memoria inválida

                        else {

                            int pix_x = static_cast<int>(uv_coor.x * (tex_width-1)); //calculamos la coordenada x del pixel en la textura, multiplicando la coordenada uv por el ancho de la textura
                            int pix_y = static_cast<int>((1-uv_coor.y) * (tex_height-1)); //calculamos la coordenada y del pixel en la textura, multiplicando la coordenada uv por el alto de la textura,
                            //  y restando por 1 porque en las coordenadas uv, 0 es abajo, pero en la textura, 0 es arriba

                            if (pix_x < 0 || pix_x >= static_cast<int>(tex_width) ||
                                pix_y < 0 || pix_y >= static_cast<int>(tex_height)) {
                                continue;
                            } //esto es para asegurarnos de que las coordenadas del pixel en la textura estén dentro de los límites de la textura, si no, no pintamos nada, esto es para evitar errores de acceso a memoria inválida

                            Col color = colores[pix_y * tex_width + pix_x]; //obtenemos el color del pixel en la textura, usando las coordenadas que acabamos de calcular

                            Vec3 coloro = {color.r/255.0f, color.g/255.0f, color.b/255.0f}; //normalizamos el color para que esté en el rango [0, 1], esto es importante para el calculo de la iluminacion

                            Vec3 inten_dif = coloro * Iluz * std::max(normal * light_dir, 0.0f); //intensidad difusa, es el color de la textura multiplicada por la intensidad de la luz, multiplicada por el coseno del angulo entre la normal y la direccion de la luz

                            Vec3 color_final = inten_amb + inten_dif + inten_spec; //calculamos el color final sumando las intensidades ambiental, difusa y especular

                            //aquí definimos si hay toon shading y aplicamos eso
                            if (shading_mode == ShadingMode::TOON) {

                                float diff_factor = std::max(normal * light_dir, 0.0f); //el dot de la normal con la direccion de la luz, con esto vamos a hacer el clamp para el toon shading
                                float view_factor = dir_cam*normal; //el dot de la direccion del punto con la normal del punto, si el dot es casi cero entonces es borde del objeto, esto es para el borde
                                float spec_factor = std::max(reflejo * dir_cam, 0.0f); //el dot del vector de reflejo con la direccion del punto, esto es para el brillo

                                //outline
                                if (view_factor < 0.3f) {//si el dot de la direccion del punto con la normal es menor a 0.3, entonces es borde negro
                                    color_final = {0,0,0};
                                }

                                //specular sin difuminado
                                else if (spec_factor > 0.8f){//si el dot del vector de reflejo con la direccion del punto es mayor a 0.8, entonces es un brillo blanco
                                    color_final = {1,1,1};
                                }

                                //diffuse cuantizado
                                else {

                                    float toon;
                                    if (diff_factor > 0.75f) toon = 1.0f;
                                    else if (diff_factor > 0.5f) toon = 0.75f;
                                    else if (diff_factor > 0.25f) toon = 0.5f;
                                    else toon = 0.25f;

                                    color_final = coloro * toon *Iluz;

                                }
                            }

                            const Uint32 colorin = (255 << 24) | (static_cast<int>(std::min(color_final.x * 255.0f, 255.0f)) << 16) | (static_cast<int>(std::min(color_final.y * 255.0f, 255.0f)) << 8) | static_cast<int>(std::min(color_final.z * 255.0f, 255.0f)); //esto es bit manipulation Uint tiene 32 bits, 8 para A,R,G,B, lo que hacemos es mandar 255 con << que es un shift left, osea, esos 8 bits,
                            //se van a mover 24 posiciones a la izquierda, osea, van a quedar en la parte de A, luego r se mueve 16 posiciones, g se mueve 8 posiciones
                            //y b se queda en la parte de B, luego hacemos un OR binario que es cada palito para juntar todito en un solo Uint32

                            framebuffer[j * WIDTH + i] = colorin; //esto es para pintar el pixel, el framebuffer es un array de pixeles, y cada pixel es un Uint32, que es un entero de 32 bits, y cada bit representa un canal de color (RGBA)
                        }


                    }
                }
            }
        }
    }


    void render_obj(const std::vector<Mesh> &meshes) {

        std::vector<std::thread> threads_list; //la lista de threads que vamos a usar
        std::vector<Triangulo> triangulos; //la lista de triangulos que vamos a rasterizar

        std::fill(framebuffer.begin(), framebuffer.end(), 0);//reiniciamos el framebuffer cada frame para no tener los pixeles del frame anterior, esto es importante para que no se queden los pixeles pintados de un frame a otro
        std::fill(zbuffer.begin(), zbuffer.end(), HUGE_VALF);//reiniciamos tambien el zbuffer por la misma razon

        const Vec3 eye = {0,0,3}; //posicion de la camara
        const Vec3 center = {0,0,0}; //a donde esta viendo la camara

        for (const auto& mesh : meshes) { //esto es para el caso de que queramos renderizar varios meshes, por ahora solo tenemos uno, pero asi se veria el codigo

            ShadingMode shading_mode = mesh.shading_mode;

            Vec3 move = mesh.transforms.translation; //obtenemos la traslacion del mesh
            const float rotx = mesh.transforms.rotation.x; //obtenemos la rotacion en x del mesh
            const float roty = mesh.transforms.rotation.y; //obtenemos la rotacion en y del mesh
            const float rotz = mesh.transforms.rotation.z; //obtenemos la rotacion en z del mesh
            const float scale = mesh.transforms.scale; //obtenemos la escala del mesh

            const std::vector<Face>& faces = mesh.faces; //obtenemos las caras
            const std::vector<Vec3>& vertices = mesh.vertices; //obtenemos los vertices
            const std::vector<Vec2>& uvs = mesh.uvs; //obtenemos los uvs
            const std::vector<Vec3>& normals = mesh.normals; //obtenemos las normales

            const Mat4 MVP = projection_matrix(90,static_cast<float>(WIDTH)/HEIGHT,0.1,100.0) * lookAt_matrix(eye, center) * move_matrix(move.x, move.y, move.z) * rotz_matrix(rotz)
                            * roty_matrix(roty) * rotx_matrix(rotx) * scale_matrix(scale); //la epica y superpoderosa matriz MVP, que transforma cada vertice
                //a clip space mandando su profundidad con w listo para transformar a NDC y luego a screen space
                //otro detallito es que para hacer division entre dos enteros, toca transformar uno a float antes de hacerlo

            const Mat4 modelMatrix = move_matrix(move.x, move.y, move.z) * rotz_matrix(rotz) * roty_matrix(roty) * rotx_matrix(rotx) * scale_matrix(scale); //matriz de modelo, que transforma cada vertice a espacio mundo, esto es para el calculo de la iluminacion, ya que necesitamos las posiciones en el espacio mundo para eso
            const Mat4 normalMatrix = rotz_matrix(rotz) * roty_matrix(roty) * rotx_matrix(rotx); //matriz para calcular las normales para la iluminación

            for (const auto& face : faces) {

                const std::string nom_mat = face.name; //obtenemos el nombre del material
                const Material* mat = mesh.get_material(nom_mat); //obtenemos el material usando el nombre, con puntero, para no copiarlo cada frame

                for (int i=1; i < face.v_indices.size()-1;i++){ //esto es para hacer triangulación de caras con más de 3 vertices, si la cara tiene 4 vertices, va a hacer un triangulo con los vertices 0,1,2 y otro triangulo con los vertices 0,2,3

                    Vec3 nor1 = normals[face.n_indices[0]-1]; //obtenemos la normal del vertice 1, el menos uno es porque los indices en .obj comienzan en uno
                    Vec3 nor2 = normals[face.n_indices[i]-1]; //obtenemos la normal del vertice 2
                    Vec3 nor3 = normals[face.n_indices[i+1]-1]; //obtenemos la normal del vertice 3

                    Vec2 uv1 = uvs[face.uv_indices[0]-1]; //el menos uno es porque los indices en .obj comienzan en uno
                    Vec2 uv2 = uvs[face.uv_indices[i]-1];
                    Vec2 uv3 = uvs[face.uv_indices[i+1]-1];

                    Vec3 ver1 = vertices[face.v_indices[0]-1];
                    Vec3 ver2 = vertices[face.v_indices[i]-1];
                    Vec3 ver3 = vertices[face.v_indices[i+1]-1];

                    //calculamos lo que vamos a devolver para la iluminación, osea los puntos rotatos, y las normales rotadas tambien
                    Vec4 nor1real = normalMatrix * Vec4{nor1.x, nor1.y, nor1.z, 0.0f}; //las normales rotadas bien
                    Vec4 nor2real = normalMatrix * Vec4{nor2.x, nor2.y, nor2.z, 0.0f};
                    Vec4 nor3real = normalMatrix * Vec4{nor3.x, nor3.y, nor3.z, 0.0f};

                    Vec4 ver1real = modelMatrix * Vec4{ver1.x, ver1.y, ver1.z, 1.0f}; //las coordenadas giradas, escaladas y trasladadas
                    Vec4 ver2real = modelMatrix * Vec4{ver2.x, ver2.y, ver2.z, 1.0f};
                    Vec4 ver3real = modelMatrix * Vec4{ver3.x, ver3.y, ver3.z, 1.0f};

                    //auto& [ver1,ver2,ver3] = face; //esto es una forma de desempaquetar la estructura de face
                    //es mucho más limpio y épico


                    Vec4 ver1_clip = MVP * Vec4{ver1.x, ver1.y, ver1.z, 1.0f}; //transformamos cada vertice a clip space
                    Vec4 ver2_clip = MVP * Vec4{ver2.x, ver2.y, ver2.z, 1.0f};
                    Vec4 ver3_clip = MVP * Vec4{ver3.x, ver3.y, ver3.z, 1.0f};

                    //frustum culling
                    if (ver1_clip.x < -ver1_clip.w && ver2_clip.x < -ver2_clip.w && ver3_clip.x < -ver3_clip.w) continue; //si los tres vertices estan a la izquierda del frustum, no renderizar
                    if (ver1_clip.x > ver1_clip.w && ver2_clip.x > ver2_clip.w && ver3_clip.x > ver3_clip.w) continue; //si los tres vertices estan a la derecha del frustum, no renderizar
                    if (ver1_clip.y < -ver1_clip.w && ver2_clip.y < -ver2_clip.w && ver3_clip.y < -ver3_clip.w) continue; //si los tres vertices estan abajo del frustum, no renderizar
                    if (ver1_clip.y > ver1_clip.w && ver2_clip.y > ver2_clip.w && ver3_clip.y > ver3_clip.w) continue; //si los tres vertices estan arriba del frustum, no renderizar

                    Vec3 ndc1 = {ver1_clip.x / ver1_clip.w, ver1_clip.y / ver1_clip.w, ver1_clip.z / ver1_clip.w}; //transformamos a NDC dividiendo por w
                    Vec3 ndc2 = {ver2_clip.x / ver2_clip.w, ver2_clip.y / ver2_clip.w, ver2_clip.z / ver2_clip.w};
                    Vec3 ndc3 = {ver3_clip.x / ver3_clip.w, ver3_clip.y / ver3_clip.w, ver3_clip.z / ver3_clip.w};

                    Vec2 real1 = {(1 + ndc1.x) * (WIDTH -1)/ 2, (1 - ndc1.y) * (HEIGHT -1)/ 2}; //transformamos a screen space, el +1 es para tener el x entre 0 y 2 y pues por eso toca dividir por 2
                    Vec2 real2 = {(1 + ndc2.x) * (WIDTH -1)/ 2, (1 - ndc2.y) * (HEIGHT -1)/ 2}; //además, restamos por 1 ya que obviamente la pantalla no va a hasta height sino hasta height-1
                    Vec2 real3 = {(1 + ndc3.x) * (WIDTH -1)/ 2, (1 - ndc3.y) * (HEIGHT -1)/ 2}; //y en y restamos ya que en el ndc 1 es arriba, pero en la screen space 0 es arriba

                    //backface culling
                    float area = edge_function(real2-real1, real3-real1);//edge function, si un vector esta a la derecha del otro, te indica el winding
                    if (area <= 0) continue;

                    Vec3 p1 = {real1.x, real1.y, ver1_clip.w}; //y pues ya tenemos los vertices listos para ser rasterizados, con su profundidad en z
                    Vec3 p2 = {real2.x, real2.y, ver2_clip.w}; //y pues mando el w que teníamos antes, porque es el que se usa para interpolar por profundidad
                    Vec3 p3 = {real3.x, real3.y, ver3_clip.w};// no mandamos ndc.z porque este no contiene informacion de profundidad lineal real, que es la que necesitamos

                    static const std::vector<Col> empty_texture; //esto sirve para cuando no hay textura mandar esto y no matar el codigo con el nullptr

                    triangulos.push_back({p1,p2,p3,Vec3{ver1real.x,ver1real.y,ver1real.z},Vec3{ver2real.x,ver2real.y,ver2real.z},Vec3{ver3real.x,ver3real.y,ver3real.z},uv1,uv2,uv3,Vec3{nor1real.x,nor1real.y,nor1real.z},Vec3{nor2real.x,nor2real.y,nor2real.z},Vec3{nor3real.x,nor3real.y,nor3real.z},mat ? &mat->texture : &empty_texture,
                        mat ? mat->width : 0,
                        mat ? mat->height : 0,
                        mat->shininess,mat->specular, mat->ambient, mat->diffuse, shading_mode}); //mandamos el triangulo a la lista de triangulos epicamente

                }

            }
        }

        for (auto& t : threads) {//mando la lista de triangulos con sus limites a cada threat

            threads_list.push_back(std::thread(&Render::Rasterize_threads, this, std::ref(triangulos), t.x, t.y));
            //creamos un thread para cada espacio definido en threads, y le pasamos los parametros necesarios para rasterizar ese espacio

        }

        for (auto& t : threads_list) { //espero a que todos corran para mandarlos al codigo
            t.join();
        }

        triangulos.clear();

        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), WIDTH * sizeof(Uint32)); //esto actualiza la textura con la info del framebuffer
        SDL_RenderTexture(renderer, texture, nullptr, nullptr); // esto dibuja la textura en el renderer, con nullptr para el source y el destination, lo que significa que se va a dibujar toda la textura en toda la ventana

    }

    ~Render() {
        SDL_DestroyTexture(texture); //destruimos la textura al terminar
    }

};