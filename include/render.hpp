#pragma once
#include "math.hpp"
#include <algorithm>
#include <vector>
#include <SDL3/SDL.h>
#include "mesh.hpp"
#include <complex>
#include <thread>
#include <variant>
#include <array>

#include "material.hpp"

struct DirectionalLight { //luz direccional
    Vec3 direction;
    Vec3 color;
    std::vector<float> shadow_map;
    Mat4 light_vp; //la matriz que vamos a usar para proyectar a los puntos, difiere por luz
    bool dirty = true; //el flag que vamos a usar para no renderizar el shadow map a cada frame si no es necesario
};

struct SpotLight { //spotlight, más complejo que el direccional
    Vec3 position;
    Vec3 direction;
    float cone_angle;
    Vec3 color;
    std::vector<float> shadow_map;
    Mat4 light_vp;
    bool dirty = true;
};

struct Camera {
    Vec3 position = {0, 0, 3};
    float yaw   = 0.0f;
    float pitch = 0.0f;
};

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

struct ShadowTri { //el triangulo de las sombras, para renderizar los shadow maps
    Vec3 p1,p2,p3;
    float ndc1,ndc2,ndc3;
};

class Render {
public:

    //inicializamos renderer, textura, framebuffer y zbuffer
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int WIDTH, HEIGHT;
    std::vector<Uint32> framebuffer;
    std::vector<float> zbuffer;

    std::vector<Triangulo> triangulos;
    std::vector<std::thread> threads_list;

    unsigned int num_cores = std::thread::hardware_concurrency(); //obtenemos el número de núcleos del procesador para dividir el trabajo entre ellos
    std::vector<tuple> threads; //las dos listas que vamos a usar para dividir el trabajo de render en los nucleos
    std::vector<tuple> shadow_threads;

    Camera cam;
    std::vector<std::variant<DirectionalLight,SpotLight>> lights; //usamos std::variant para poder guardar diferentes tipos en una lista

    Render(SDL_Renderer* renderer, const int width, const int height) : renderer(renderer), WIDTH(width), HEIGHT(height),
                                                framebuffer(width * height, 0), zbuffer(width * height, HUGE_VALF){

        Vec3 light_dir = Vec3{-1, -1, -1}.normalize(); // apunta derecha-abajo-adelante
        Vec3 light_pos = Vec3{0,0,0} - light_dir * 20;

        //una luz direccional la llenamos
        DirectionalLight l;
        l.shadow_map.assign(1024*1024, HUGE_VALF);
        l.direction = light_dir;
        l.color     = {0.35f, 0.5f, 0.75f};
        l.light_vp = ortho_matrix(-25,5,-15,10,0.1f,80.f) * lookAt_matrix(light_pos, {0,0,0});
        lights.push_back(std::move(l));

        //una spotlight la llenamos
        SpotLight s;
        s.shadow_map.assign(1024*1024, HUGE_VALF);
        s.color      = {1.0f, 0.65f, 0.3f};
        s.position   = {0.0f, 5.0f, 5.0f};
        Vec3 spot_target = {-5.0f, -2.0f, 14.0f}; // centro de la skull en este caso
        s.direction  = (spot_target - s.position).normalize();
        s.cone_angle = 140.0f;
        s.light_vp   = projection_matrix(s.cone_angle, 1.0f, 0.1f, 50.f) * lookAt_matrix(s.position, spot_target);
        lights.push_back(std::move(s));

        int lines_per_t = height / num_cores; //calculamos el numero de filas para cada thread
        int lines_rest = height % num_cores; //calculamos el resto que le vamos a dar al ultimo thread

        for (int i = 0; i < num_cores; i++) { //guardamos el trabajo por thread en la lista de threads
            threads.push_back({i*lines_per_t,(i+1)*lines_per_t - 1}); // guardamos en esta epica lista los valores para cada core
        }

        threads.back().y += lines_rest; //le ponemos al ultimo thread lo que falte

        int lines_per_t_s = 1024 / num_cores; //calculamos el numero de filas para cada thread de sombras
        int lines_rest_s = 1024 % num_cores; //calculamos el resto que le vamos a dar al ultimo thread

        for (int i = 0; i < num_cores; i++) { //guardamos el trabajo por thread para cada shadow map en la lista
            shadow_threads.push_back({i*lines_per_t_s,(i+1)*lines_per_t_s - 1});
        }

        shadow_threads.back().y += lines_rest_s; //igual le damos al ultimo lo que falte

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                            WIDTH,HEIGHT); //creamos la textura, que es lo que nos va a servir para pintar en el renderer

    }


    void Rasterize_threads(const std::vector<Triangulo>& triangulos , const int &miny_t, const int &maxy_t) {

        //rasterizamos cada triangulo
        for (auto& tri : triangulos) {

            ShadingMode shading_mode = tri.shading_mode;

            //obtenemos los 3 vertices en screen space
            Vec3 pA = tri.p1;
            Vec3 pB = tri.p2;
            Vec3 pC = tri.p3;

            //obtenemos las coords uv de los 3 vertices
            Vec2 uvA = tri.uv1;
            Vec2 uvB = tri.uv2;
            Vec2 uvC = tri.uv3;

            //obtenemos las normales de los 3 vertices
            Vec3 norA = tri.n1;
            Vec3 norB = tri.n2;
            Vec3 norC = tri.n3;

            //obtenemos los 3 vertices en world space para la iluminación
            Vec3 realA = tri.real1;
            Vec3 realB = tri.real2;
            Vec3 realC = tri.real3;

            //obtenemos las constantes para la iluminación
            float const shininess = tri.shininess;
            Vec3 const ks = tri.ks;
            Vec3 const ka = tri.ka;
            Vec3 const kd = tri.kd;

            //obtenemos la posicion de la camara
            Vec3 eye = cam.position;

            //obtenemos la textura, por referencia claro y sus dimensiones
            const std::vector<Col> &colores = *tri.texture;
            const uint32_t tex_width = tri.tex_width;
            const uint32_t tex_height = tri.tex_height;

            //definimos el bounding box del triangulo en pantalla
            int minx = static_cast<int>(std::min(pA.x, std::min(pB.x, pC.x)));
            int miny = static_cast<int>(std::min(pA.y, std::min(pB.y, pC.y)));
            int maxx = static_cast<int>(std::max(pA.x, std::max(pB.x, pC.x)));
            int maxy = static_cast<int>(std::max(pA.y, std::max(pB.y, pC.y)));

            //arreglamos el bounding box por si sale de la pantalla
            minx = std::max(minx, 0);
            miny = std::max(miny, 0);
            maxx = std::min(maxx, WIDTH - 1);
            maxy = std::min(maxy, HEIGHT - 1);

            //reducimos aun más el bounding box para corresponder a la zona para el thread
            miny = std::max(miny,miny_t);
            maxy = std::min(maxy,maxy_t);

            //definimos los puntos 2d, los de la pantalla
            const Vec2 pA_2d = {pA.x, pA.y};
            const Vec2 pB_2d = {pB.x, pB.y};
            const Vec2 pC_2d = {pC.x, pC.y};

            //definimos los lados en pantalla
            const Vec2 ladoAB = pB_2d - pA_2d;
            const Vec2 ladoBC = pC_2d - pB_2d;
            const Vec2 ladoCA = pA_2d - pC_2d;

            //recuperamos la valor de la profundidad por cada vertice por separado
            const float w1 = pA.z;
            const float w2 = pB.z;
            const float w3 = pC.z;

            const float inv_w1 = 1.0f / w1;
            const float inv_w2 = 1.0f / w2;
            const float inv_w3 = 1.0f / w3;

            for (int i = minx; i <= maxx; i++) { //para cada pixel de la zona que definimos
                for (int j = miny; j <= maxy; j++) {

                    Vec2 p = {static_cast<float>(i), static_cast<float>(j)}; //el pixel que vamos a analizar

                    //calculamos edge del punto con cada lado, para verificar si esta adentro
                    //adicionalmente los edges son casi las coordenadas baricentricas que vamos a usar para interpolar
                    //los edges en resumen son el cross product, y este calcula el doble del area del triangulo que forman
                    //ambos vectores, además, este area tiene signo, que es lo que usamos para identificar si esta adentro o no

                    float const edge1 = edge_function(p - pA_2d,ladoAB); //usamos para interpolar el punto c
                    float const edge2 = edge_function(p - pB_2d,ladoBC); //usamos para interpolar el punto a
                    float const edge3 = edge_function(p - pC_2d,ladoCA); //usamos para interpolar el punto b

                    //si los tres tienen el mismo signo, dibujamos el pixel
                    if ((edge1 >= 0 && edge2 >= 0 && edge3 >= 0)||(edge1 <= 0 && edge2 <= 0 && edge3 <= 0)) {

                        //el area total, corresponde al doble del area del triangulo
                        float const area_total = edge1 + edge2 + edge3;

                        //calculamos las coordenadas baricentricas, cada una entre 0,1 definiendo cuan cerca estamos de un punto
                        float const e1_norm = edge1 / area_total; //coordenada baricentrica para c
                        float const e2_norm = edge2 / area_total; //baricentrica para a
                        float const e3_norm = edge3 / area_total; //baricentrica para b

                        //interpolamos la profundidad pero con perspective correct interpolation
                        float depth = e1_norm*(inv_w3) + e2_norm*(inv_w1) + e3_norm*(inv_w2);
                        depth = 1/depth;

                        //interpolamos la normal, el punto en world space y las coordenadas uv con perspective correct interpolation igual
                        Vec3 normal = norC*e1_norm*(inv_w3) + norA*e2_norm*(inv_w1) + norB*e3_norm*(inv_w2);
                        normal = normal * depth;
                        Vec3 real_p = realC * e1_norm*(inv_w3) + realA * e2_norm*(inv_w1) + realB * e3_norm*(inv_w2);
                        real_p = real_p * depth;
                        Vec2 uv_coor = uvC*e1_norm*(inv_w3) + uvA*e2_norm*(inv_w1) + uvB*e3_norm*(inv_w2);
                        uv_coor = uv_coor* depth;

                        normal = normal.normalize(); //normalizamos la normal . Importante para el calculo de la iluminacion
                        Vec3 dir_cam = (eye - real_p).normalize(); //calculamos el vector que va del punto al ojo, y lo normalizamos

                        Vec3 color_final = {0,0,0}; //color final inicalizado a 0

                        //test del z-buffer
                        if (depth < zbuffer[j * WIDTH + i]) {
                            zbuffer[j * WIDTH + i] = depth;
                        } else {
                            continue;
                        }

                        const float Ia  = 0.1f; //intensidad de la luz ambiental
                        Vec3 inten_amb = ka * Ia; //intensidad ambiental, es la constante de ambient multiplicada por la intensidad de la luz ambiental
                        Vec3 coloro = {1,1,1};

                        if (!colores.empty()) {

                            //calculamos las coords del pixel en la textura, restamos 1 en y porque uv, 0 abajo
                            int pix_x = static_cast<int>(uv_coor.x * (tex_width-1));
                            int pix_y = static_cast<int>((1-uv_coor.y) * (tex_height-1));

                            //si pixel afuera de la textura, return para salir, porque estamos dentro de std::visit
                            if (pix_x < 0 || pix_x >= static_cast<int>(tex_width) ||
                               pix_y < 0 || pix_y >= static_cast<int>(tex_height)) {return;}

                            //obtenemos el color del pixel
                            Col color = colores[pix_y * tex_width + pix_x];
                            //y lo normalizamos
                            coloro = {color.r/255.0f, color.g/255.0f, color.b/255.0f};
                            inten_amb = Vec3{coloro.x*ka.x, coloro.y*ka.y, coloro.z*ka.z}*Ia;

                        }



                        for (auto& luz : lights) { //recorremos todas las luces

                            std::visit([&](auto& l) { //devolvemos la luz con el tipo correcto

                                //inicializamos atenuacion y factor a 1, solo cambia para el spotlight
                                float atenuacion = 1;
                                float factor = 1;

                                Vec3 aporte = {0,0,0}; // lo que vamos a sumarla al final al color final
                                bool no_aporta = false;

                                using T = std::decay_t<decltype(l)>;

                                Vec3 light_dir;

                                if constexpr (std::is_same_v<T, DirectionalLight>) { //direccional
                                    light_dir = (l.direction * -1).normalize();

                                } else { //spotlight
                                    Vec3 delta = l.position - real_p;
                                    float d = delta.length();
                                    atenuacion = 1 / (1 + 0.05*d + 0.01*d*d); //atenuacion por distancia
                                    light_dir = (delta).normalize();

                                    //calculamos el angulo del cono y su direccion
                                    Vec3 cono_dir = (l.direction*-1).normalize();
                                    float angulo = radianes(l.cone_angle);

                                    //calculamos el coseno de cada angulo, interior, exterior y actual para calcular el factor
                                    float cos_angulo_inner = cosf(radianes(30)/2);
                                    float cos_angulo_outer = cosf(radianes(l.cone_angle)/2);
                                    float cos_actual = light_dir * cono_dir;

                                    //calculamos el factor con un simple lerp y nos aseguramos de que este entre 0 e 1
                                    factor = (cos_actual - cos_angulo_outer) / (cos_angulo_inner - cos_angulo_outer);
                                    factor = std::clamp(factor, 0.0f, 1.0f);

                                    //si el punto afuera del cono, no aporta
                                    if (light_dir * cono_dir < cosf(angulo/2)) {
                                        no_aporta = true;
                                    }
                                }

                                if (!no_aporta) { //si la luz aporta

                                    const Vec3 Iluz = l.color * atenuacion * factor;  // intensidad de la luz normalita

                                    //proyectamos el punto 3D del pixel a la luz para compararlo con el shadow map
                                    Vec4 pos_luz = l.light_vp * Vec4{real_p.x, real_p.y, real_p.z, 1.0f};
                                    Vec3 ndc_luz = {pos_luz.x/pos_luz.w, pos_luz.y/pos_luz.w, pos_luz.z/pos_luz.w};

                                    //calculamos el bias, constante para evitar errores de acne
                                    //mas grande cuanto más perpendicular es la luz a la normal de la superficie
                                    float n_dot_l = normal * light_dir;
                                    float bias = std::max(0.0005f, 0.01f * (1.0f - n_dot_l));

                                    //por defecto, las cosas no estan en la sombra, pera que toda la escena parezca estar iluminada
                                    bool sombreado = true;

                                    //calculamos el punto en el shadow map y comparamos como un z buffer
                                    int sx = static_cast<int>((ndc_luz.x + 1.0f) * 0.5f * 1023);
                                    int sy = static_cast<int>((1.0f - ndc_luz.y) * 0.5f * 1023);
                                    if (sx >= 0 && sx < 1024 && sy >= 0 && sy < 1024) {
                                        float depth_luz = l.shadow_map[sy * 1024 + sx];
                                        sombreado = (ndc_luz.z > depth_luz + bias);
                                    }

                                    //inicializamos la intensidad especular y diffuse
                                    Vec3 inten_spec = {0,0,0};
                                    Vec3 inten_dif = {0,0,0};

                                    if (!sombreado) { //si no está en la sombra, calculamos speculares, diffuses

                                        //calculamos el vector reflejo de la luz en el punto normalizado con la formula de reflexion
                                        Vec3 reflejo = (normal * (2 * (normal * light_dir)) - light_dir).normalize();
                                        float diff_factor = std::max(normal * light_dir, 0.0f); //el dot de la normal con la direccion de la luz
                                        float spec_factor = std::max(reflejo * dir_cam, 0.0f); //el dot del vector de reflejo con la direccion del punto

                                        //si el punto mira a la luz calculamos la especular, si no no
                                        if (normal * light_dir > 0) {
                                            Vec3 ks_luz = {ks.x * Iluz.x, ks.y * Iluz.y, ks.z * Iluz.z};
                                            //intensidad especular, es la constante de specular multiplicada por la intensidad de la luz
                                            //multiplicada por el dot entre el vector de reflejo y el vector que va al ojo
                                            //elevado a la potencia de shininess
                                            inten_spec = ks_luz * powf(spec_factor, shininess);
                                        }

                                        //si no hay textura
                                        if (colores.empty() || tex_width == 0 || tex_height == 0) {

                                            Vec3 kd_luz = {kd.x * Iluz.x, kd.y * Iluz.y, kd.z * Iluz.z};
                                            //intensidad difusa, es la constante de diffuse multiplicada por la intensidad de la luz
                                            //multiplicada por el dot entre la normal y la direccion de la luz
                                            inten_dif = kd_luz * diff_factor;

                                            aporte = aporte + inten_dif + inten_spec;

                                            //si hay toon shading, pues lo aplicamos
                                            if (shading_mode == ShadingMode::TOON) {

                                                //specular sin difuminado
                                                //si el reflejo está casi alineado con la camara y la cara apunta a la luz, blanco puro
                                                if (spec_factor > 0.95f  && normal * light_dir > 0.0f){
                                                   aporte = {1,1,1};
                                                }

                                                //diffuse cuantizado
                                                else {
                                                    float toon;
                                                    if (diff_factor > 0.75f) toon = 1.0f;
                                                    else if (diff_factor > 0.5f) toon = 0.75f;
                                                    else if (diff_factor > 0.25f) toon = 0.5f;
                                                    else toon = 0.25f;
                                                    aporte = aporte + kd_luz * toon; //finalmente definimos el color, aquí es usando kd porque no hay textura
                                                }
                                            }

                                        }

                                        //si sí hay textura
                                        else {



                                            //cuando hay textura, el color actua como el kd
                                            Vec3 colo_luz = {coloro.x* Iluz.x , coloro.y* Iluz.y , coloro.z* Iluz.z };
                                            Vec3 inten_dif = colo_luz * diff_factor;

                                            aporte = aporte + inten_dif + inten_spec;

                                            if (shading_mode == ShadingMode::TOON) {

                                                //specular sin difuminado
                                                if (spec_factor > 0.95f  && normal * light_dir > 0.0f){
                                                    aporte = {1,1,1};
                                                }
                                                //diffuse cuantizado
                                                else {
                                                    float toon;
                                                    if (diff_factor > 0.75f) toon = 1.0f;
                                                    else if (diff_factor > 0.5f) toon = 0.75f;
                                                    else if (diff_factor > 0.25f) toon = 0.5f;
                                                    else toon = 0.25f;
                                                    aporte = colo_luz * toon;
                                                }
                                            }
                                        }
                                    }
                                }

                                //si no aporta, entonces pues es 0
                                if (no_aporta == true) {
                                    aporte = {0,0,0};
                                }

                                //al color final le sumamos el aporte
                                color_final = color_final + aporte;

                            },luz);
                        }


                        //al color final le sumamos la intensidad ambiental
                        color_final = color_final + inten_amb;

                        //le añadimos el outline si es toon
                        if (shading_mode == ShadingMode::TOON) {
                            float view_factor = dir_cam*normal;
                            if (view_factor < 0.3f) {
                                color_final = {0,0,0};
                            }
                        }

                        //hacemos bit manipulation para tener el color en formato ARGB Uint32 que es el que usa el framebuffer
                        //y adicionalmente clampeamos el valor para evitar errores
                        const Uint32 colorin = (255 << 24) |
                            (static_cast<int>(std::min(color_final.x * 255.0f, 255.0f)) << 16) |
                                (static_cast<int>(std::min(color_final.y * 255.0f, 255.0f)) << 8) |
                                    static_cast<int>(std::min(color_final.z * 255.0f, 255.0f));

                        //pintamos
                        framebuffer[j * WIDTH + i] = colorin;

                    }
                }
            }
        }
    }

    void render_obj(const std::vector<Mesh> &meshes) {

        //inicializamos la lista de threads y de triangulos
        triangulos.clear();
        threads_list.clear();

        //cada frame reiniciamos el framebuffer y zbuffer
        std::fill(framebuffer.begin(), framebuffer.end(), 0);
        std::fill(zbuffer.begin(), zbuffer.end(), HUGE_VALF);

        //calculamos el vector front usando las formulas de rotacion
        Vec3 front = Vec3{
            sinf(cam.yaw) * cosf(cam.pitch),
            sinf(cam.pitch),
            cosf(cam.yaw) * cosf(cam.pitch)
        }.normalize();

        const Vec3 eye = cam.position; //posicion de la camara
        const Vec3 center = cam.position + front; //a donde esta viendo la camara

        //por cada mesh
        for (const auto& mesh : meshes) {

            //obtenmos su modo de shading
            ShadingMode shading_mode = mesh.shading_mode;

            //obtenemos sus transformaciones
            Vec3 move = mesh.transforms.translation;
            const float rotx = mesh.transforms.rotation.x;
            const float roty = mesh.transforms.rotation.y;
            const float rotz = mesh.transforms.rotation.z;
            const float scale = mesh.transforms.scale;

            //obtenemos sus valores de vertices, normales, uvs y caras, por referencia claro
            const std::vector<Face>& faces = mesh.faces;
            const std::vector<Vec3>& vertices = mesh.vertices;
            const std::vector<Vec2>& uvs = mesh.uvs;
            const std::vector<Vec3>& normals = mesh.normals;

            //definimos la epica MVP matrix, que va a mandar cada vertice a clip space
            const Mat4 MVP = projection_matrix(50,static_cast<float>(WIDTH)/HEIGHT,0.1,100.0) *
                            lookAt_matrix(eye, center) *
                              move_matrix(move.x, move.y, move.z) *
                                 rotz_matrix(rotz) * roty_matrix(roty) * rotx_matrix(rotx) * scale_matrix(scale);

            //adicionalmente calculamos la modelMatrix y normalMatrix que se usaran para la iluminacion
            //la model transforma los vertices pero quedandose en world space, la normal lo hace sin considerar traslacion ni escala
            const Mat4 modelMatrix = move_matrix(move.x, move.y, move.z) *
                        rotz_matrix(rotz) * roty_matrix(roty) * rotx_matrix(rotx) * scale_matrix(scale);
            const Mat4 normalMatrix = rotz_matrix(rotz) * roty_matrix(roty) * rotx_matrix(rotx);

            //por cada cara
            for (const auto& face : faces) {

                //obtenemos el nombre del material y el material en sí, con puntero
                const std::string nom_mat = face.name;
                const Material* mat = mesh.get_material(nom_mat);

                //hacemos fan triangulation, para soportar caras con más de 3 vertices
                for (int i=1; i < face.v_indices.size()-1;i++) {
                    //obtenemos las normales, restando uno porque obj inicia en 1
                    Vec3 nor1 = normals[face.n_indices[0]-1];
                    Vec3 nor2 = normals[face.n_indices[i]-1];
                    Vec3 nor3 = normals[face.n_indices[i+1]-1];

                    //obtenemos las coords uv
                    Vec2 uv1 = uvs[face.uv_indices[0]-1];
                    Vec2 uv2 = uvs[face.uv_indices[i]-1];
                    Vec2 uv3 = uvs[face.uv_indices[i+1]-1];

                    //obtenemos las coords de vertices
                    Vec3 ver1 = vertices[face.v_indices[0]-1];
                    Vec3 ver2 = vertices[face.v_indices[i]-1];
                    Vec3 ver3 = vertices[face.v_indices[i+1]-1];

                    //calculamos lo que vamos a devolver para la iluminación, puntos movidos y normales rotadas tambien
                    Vec4 nor1real = normalMatrix * Vec4{nor1.x, nor1.y, nor1.z, 0.0f};
                    Vec4 nor2real = normalMatrix * Vec4{nor2.x, nor2.y, nor2.z, 0.0f};
                    Vec4 nor3real = normalMatrix * Vec4{nor3.x, nor3.y, nor3.z, 0.0f};

                    Vec4 ver1real = modelMatrix * Vec4{ver1.x, ver1.y, ver1.z, 1.0f};
                    Vec4 ver2real = modelMatrix * Vec4{ver2.x, ver2.y, ver2.z, 1.0f};
                    Vec4 ver3real = modelMatrix * Vec4{ver3.x, ver3.y, ver3.z, 1.0f};

                    //cada vertice a clip space
                    Vec4 ver1_clip = MVP * Vec4{ver1.x, ver1.y, ver1.z, 1.0f};
                    Vec4 ver2_clip = MVP * Vec4{ver2.x, ver2.y, ver2.z, 1.0f};
                    Vec4 ver3_clip = MVP * Vec4{ver3.x, ver3.y, ver3.z, 1.0f};


                    //frustum culling
                    //si los tres vertices al tiempo afuera de un mismo lado, no renderizar
                    if (ver1_clip.x < -ver1_clip.w && ver2_clip.x < -ver2_clip.w && ver3_clip.x < -ver3_clip.w) continue;
                    if (ver1_clip.x > ver1_clip.w && ver2_clip.x > ver2_clip.w && ver3_clip.x > ver3_clip.w) continue;
                    if (ver1_clip.y < -ver1_clip.w && ver2_clip.y < -ver2_clip.w && ver3_clip.y < -ver3_clip.w) continue;
                    if (ver1_clip.y > ver1_clip.w && ver2_clip.y > ver2_clip.w && ver3_clip.y > ver3_clip.w) continue;
                    if (ver1_clip.z < -ver1_clip.w && ver2_clip.z < -ver2_clip.w && ver3_clip.z < -ver3_clip.w) continue;
                    if (ver1_clip.z > ver1_clip.w && ver2_clip.z > ver2_clip.w && ver3_clip.z > ver3_clip.w) continue;

                    int puntos_afuera = 0;
                    bool case1 = false;
                    bool case2 = false;
                    bool case3 = false;

                    if (ver1_clip.z < -ver1_clip.w) puntos_afuera += 1, case1 = true;
                    if (ver2_clip.z < -ver2_clip.w) puntos_afuera += 1, case2 = true;
                    if (ver3_clip.z < -ver3_clip.w) puntos_afuera += 1, case3 = true;


                    //ahora, el triangle clipping, tres casos
                    //caso 1, solo pasa un vertice el near plane
                    if (puntos_afuera == 1) {

                        std::array<Vec4,3> nor = {nor1real, nor2real, nor3real};
                        std::array<Vec4,3> ver = {ver1real, ver2real, ver3real};
                        std::array<Vec2,3> uv = {uv1, uv2, uv3};
                        std::array<Vec4,3> clips = {ver1_clip, ver2_clip, ver3_clip};

                        int index_bueno1;
                        int index_bueno2;
                        int index_malo;

                        if (case1) index_malo = 0, index_bueno1 = 1, index_bueno2 = 2;
                        if (case2) index_malo = 1, index_bueno1 = 2, index_bueno2 = 0;
                        if (case3) index_malo = 2, index_bueno1 = 0, index_bueno2 = 1;

                        float delta_l = (clips[index_bueno1].z + clips[index_bueno1].w) / ((clips[index_bueno1].z + clips[index_bueno1].w) - (clips[index_malo].z + clips[index_malo].w));
                        float delta_r = (clips[index_bueno2].z + clips[index_bueno2].w) / ((clips[index_bueno2].z + clips[index_bueno2].w) - (clips[index_malo].z + clips[index_malo].w));

                        Vec4 nuevo_1 = clips[index_bueno1]*(1-delta_l) + clips[index_malo]*delta_l;
                        Vec4 nuevo_2 = clips[index_bueno2]*(1-delta_r) + clips[index_malo]*delta_r;

                        Vec2 nuevo_uv_1 = uv[index_bueno1]*(1-delta_l) + uv[index_malo]*delta_l;
                        Vec2 nuevo_uv_2 = uv[index_bueno2]*(1-delta_r) + uv[index_malo]*delta_r;
                        Vec4 nuevo_nor_1 = nor[index_bueno1]*(1-delta_l) + nor[index_malo]*delta_l;
                        Vec4 nuevo_nor_2 = nor[index_bueno2]*(1-delta_r) + nor[index_malo]*delta_r;
                        Vec4 nuevo_rel_1 = ver[index_bueno1]*(1-delta_l) + ver[index_malo]*delta_l;
                        Vec4 nuevo_rel_2 = ver[index_bueno2]*(1-delta_r) + ver[index_malo]*delta_r;

                        //cada vertice a ndc space, normalized device coordinates
                        Vec3 ndc1 = {clips[index_bueno1].x / clips[index_bueno1].w, clips[index_bueno1].y / clips[index_bueno1].w, clips[index_bueno1].z / clips[index_bueno1].w};
                        Vec3 ndc2 = {clips[index_bueno2].x / clips[index_bueno2].w, clips[index_bueno2].y / clips[index_bueno2].w, clips[index_bueno2].z / clips[index_bueno2].w};
                        Vec3 ndc3 = {nuevo_1.x / nuevo_1.w, nuevo_1.y / nuevo_1.w, nuevo_1.z / nuevo_1.w};
                        Vec3 ndc4 = {nuevo_2.x / nuevo_2.w, nuevo_2.y / nuevo_2.w, nuevo_2.z / nuevo_2.w};

                        //cada vertice a screen space
                        //+1 para tenerlo entre 0 y 2, restamos 1 porque pantalla va hasta width-1 y restamos y porque ndc arriba es 1, 0 en screen space
                        Vec2 real1 = {(1 + ndc1.x) * (WIDTH -1)/ 2, (1 - ndc1.y) * (HEIGHT -1)/ 2};
                        Vec2 real2 = {(1 + ndc2.x) * (WIDTH -1)/ 2, (1 - ndc2.y) * (HEIGHT -1)/ 2};
                        Vec2 real3 = {(1 + ndc3.x) * (WIDTH -1)/ 2, (1 - ndc3.y) * (HEIGHT -1)/ 2};
                        Vec2 real4 = {(1 + ndc4.x) * (WIDTH -1)/ 2, (1 - ndc4.y) * (HEIGHT -1)/ 2};

                        //backface culling, todas las areas tienen que tener el mismo signo
                        //float area1 = edge_function(real4-real1, real3-real1);
                        //if (area1 >= 0) continue;
//
                        //float area2 = edge_function(real2-real1, real4-real1);
                        //if (area2 >= 0) continue;

                        //mandamos las coords 2D con su profundidad para interpolar por profundidad
                        Vec3 p1 = {real1.x, real1.y, clips[index_bueno1].w};
                        Vec3 p2 = {real2.x, real2.y, clips[index_bueno2].w};
                        Vec3 p3 = {real3.x, real3.y, nuevo_1.w};
                        Vec3 p4 = {real4.x, real4.y, nuevo_2.w};


                        //si no hay textura, no matar el código con nullptr
                        static const std::vector<Col> empty_texture;

                        triangulos.push_back({p1,p4,p3,
                            Vec3{ver[index_bueno1].x,ver[index_bueno1].y,ver[index_bueno1].z},
                            Vec3{nuevo_rel_2.x,nuevo_rel_2.y,nuevo_rel_2.z},
                            Vec3{nuevo_rel_1.x,nuevo_rel_1.y,nuevo_rel_1.z},
                            uv[index_bueno1],nuevo_uv_2,nuevo_uv_1,
                            Vec3{nor[index_bueno1].x,nor[index_bueno1].y,nor[index_bueno1].z},
                            Vec3{nuevo_nor_2.x,nuevo_nor_2.y,nuevo_nor_2.z},
                            Vec3{nuevo_nor_1.x,nuevo_nor_1.y,nuevo_nor_1.z},
                            mat ? &mat->texture : &empty_texture,
                            mat ? mat->width : 0,
                            mat ? mat->height : 0,
                            mat->shininess,mat->specular, mat->ambient, mat->diffuse,
                            shading_mode}); //mandamos el triangulo a la lista de triangulos epicament

                        triangulos.push_back({p1,p2,p4,
                            Vec3{ver[index_bueno1].x,ver[index_bueno1].y,ver[index_bueno1].z},
                            Vec3{ver[index_bueno2].x,ver[index_bueno2].y,ver[index_bueno2].z},
                            Vec3{nuevo_rel_2.x,nuevo_rel_2.y,nuevo_rel_2.z},
                            uv[index_bueno1],uv[index_bueno2],nuevo_uv_2,
                            Vec3{nor[index_bueno1].x,nor[index_bueno1].y,nor[index_bueno1].z},
                            Vec3{nor[index_bueno2].x,nor[index_bueno2].y,nor[index_bueno2].z},
                            Vec3{nuevo_nor_2.x,nuevo_nor_2.y,nuevo_nor_2.z},
                            mat ? &mat->texture : &empty_texture,
                            mat ? mat->width : 0,
                            mat ? mat->height : 0,
                            mat->shininess,mat->specular, mat->ambient, mat->diffuse,
                            shading_mode}); //mandamos el triangulo a la lista de triangulos epicament

                    }


                    //caso 2, pasan dos vertices el near plane
                    if (puntos_afuera == 2) {

                        std::array<Vec4,3> nor = {nor1real, nor2real, nor3real};
                        std::array<Vec4,3> ver = {ver1real, ver2real, ver3real};
                        std::array<Vec2,3> uv = {uv1, uv2, uv3};
                        std::array<Vec4,3> clips = {ver1_clip, ver2_clip, ver3_clip};

                        int index_bueno;
                        int index_malo1;
                        int index_malo2;

                        if (case1 && case2) index_malo1 = 1, index_malo2 = 0, index_bueno = 2;
                        if (case2 && case3) index_malo1 = 2, index_malo2 = 1, index_bueno = 0;
                        if (case3 && case1) index_malo1 = 0, index_malo2 = 2, index_bueno = 1;

                        float delta_l = (clips[index_bueno].z + clips[index_bueno].w) / ((clips[index_bueno].z + clips[index_bueno].w) - (clips[index_malo1].z + clips[index_malo1].w));
                        float delta_r = (clips[index_bueno].z + clips[index_bueno].w) / ((clips[index_bueno].z + clips[index_bueno].w) - (clips[index_malo2].z + clips[index_malo2].w));

                        Vec4 nuevo_1 = clips[index_bueno]*(1-delta_l) + clips[index_malo1]*delta_l;
                        Vec4 nuevo_2 = clips[index_bueno]*(1-delta_r) + clips[index_malo2]*delta_r;

                        Vec2 nuevo_uv_1 = uv[index_bueno]*(1-delta_l) + uv[index_malo1]*delta_l;
                        Vec2 nuevo_uv_2 = uv[index_bueno]*(1-delta_r) + uv[index_malo2]*delta_r;
                        Vec4 nuevo_nor_1 = nor[index_bueno]*(1-delta_l) + nor[index_malo1]*delta_l;
                        Vec4 nuevo_nor_2 = nor[index_bueno]*(1-delta_r) + nor[index_malo2]*delta_r;
                        Vec4 nuevo_rel_1 = ver[index_bueno]*(1-delta_l) + ver[index_malo1]*delta_l;
                        Vec4 nuevo_rel_2 = ver[index_bueno]*(1-delta_r) + ver[index_malo2]*delta_r;

                         //cada vertice a ndc space, normalized device coordinates
                        Vec3 ndc1 = {clips[index_bueno].x / clips[index_bueno].w, clips[index_bueno].y / clips[index_bueno].w, clips[index_bueno].z / clips[index_bueno].w};
                        Vec3 ndc2 = {nuevo_1.x / nuevo_1.w, nuevo_1.y / nuevo_1.w, nuevo_1.z / nuevo_1.w};
                        Vec3 ndc3 = {nuevo_2.x / nuevo_2.w, nuevo_2.y / nuevo_2.w, nuevo_2.z / nuevo_2.w};

                        //cada vertice a screen space
                        //+1 para tenerlo entre 0 y 2, restamos 1 porque pantalla va hasta width-1 y restamos y porque ndc arriba es 1, 0 en screen space
                        Vec2 real1 = {(1 + ndc1.x) * (WIDTH -1)/ 2, (1 - ndc1.y) * (HEIGHT -1)/ 2};
                        Vec2 real2 = {(1 + ndc2.x) * (WIDTH -1)/ 2, (1 - ndc2.y) * (HEIGHT -1)/ 2};
                        Vec2 real3 = {(1 + ndc3.x) * (WIDTH -1)/ 2, (1 - ndc3.y) * (HEIGHT -1)/ 2};

                        //backface culling, todas las areas tienen que tener el mismo signo
                        //float area = edge_function(real2-real1, real3-real1);
                        //if (area >= 0) continue;

                        //mandamos las coords 2D con su profundidad para interpolar por profundidad
                        Vec3 p1 = {real1.x, real1.y, clips[index_bueno].w};
                        Vec3 p2 = {real2.x, real2.y, nuevo_1.w};
                        Vec3 p3 = {real3.x, real3.y, nuevo_2.w};


                        //si no hay textura, no matar el código con nullptr
                        static const std::vector<Col> empty_texture;

                        triangulos.push_back({p1,p3,p2,
                            Vec3{ver[index_bueno].x,ver[index_bueno].y,ver[index_bueno].z},
                            Vec3{nuevo_rel_2.x,nuevo_rel_2.y,nuevo_rel_2.z},
                            Vec3{nuevo_rel_1.x,nuevo_rel_1.y,nuevo_rel_1.z},
                            uv[index_bueno],nuevo_uv_2,nuevo_uv_1,
                            Vec3{nor[index_bueno].x,nor[index_bueno].y,nor[index_bueno].z},
                            Vec3{nuevo_nor_2.x,nuevo_nor_2.y,nuevo_nor_2.z},
                            Vec3{nuevo_nor_1.x,nuevo_nor_1.y,nuevo_nor_1.z},
                            mat ? &mat->texture : &empty_texture,
                            mat ? mat->width : 0,
                            mat ? mat->height : 0,
                            mat->shininess,mat->specular, mat->ambient, mat->diffuse,
                            shading_mode}); //mandamos el triangulo a la lista de triangulos epicamente
                    }

                    //caso3, pues ninguno pasa el near plane, todito perfect
                    if ( puntos_afuera == 0 ) {

                        //cada vertice a ndc space, normalized device coordinates
                        Vec3 ndc1 = {ver1_clip.x / ver1_clip.w, ver1_clip.y / ver1_clip.w, ver1_clip.z / ver1_clip.w};
                        Vec3 ndc2 = {ver2_clip.x / ver2_clip.w, ver2_clip.y / ver2_clip.w, ver2_clip.z / ver2_clip.w};
                        Vec3 ndc3 = {ver3_clip.x / ver3_clip.w, ver3_clip.y / ver3_clip.w, ver3_clip.z / ver3_clip.w};

                        //cada vertice a screen space
                        //+1 para tenerlo entre 0 y 2, restamos 1 porque pantalla va hasta width-1 y restamos y porque ndc arriba es 1, 0 en screen space
                        Vec2 real1 = {(1 + ndc1.x) * (WIDTH -1)/ 2, (1 - ndc1.y) * (HEIGHT -1)/ 2};
                        Vec2 real2 = {(1 + ndc2.x) * (WIDTH -1)/ 2, (1 - ndc2.y) * (HEIGHT -1)/ 2};
                        Vec2 real3 = {(1 + ndc3.x) * (WIDTH -1)/ 2, (1 - ndc3.y) * (HEIGHT -1)/ 2};

                        //backface culling, todas las areas tienen que tener el mismo signo
                        float area = edge_function(real2-real1, real3-real1);
                        if (area >= 0) continue;

                        //mandamos las coords 2D con su profundidad para interpolar por profundidad
                        Vec3 p1 = {real1.x, real1.y, ver1_clip.w};
                        Vec3 p2 = {real2.x, real2.y, ver2_clip.w};
                        Vec3 p3 = {real3.x, real3.y, ver3_clip.w};

                        //si no hay textura, no matar el código con nullptr
                        static const std::vector<Col> empty_texture;

                        triangulos.push_back({p1,p2,p3,
                            Vec3{ver1real.x,ver1real.y,ver1real.z},
                            Vec3{ver2real.x,ver2real.y,ver2real.z},
                            Vec3{ver3real.x,ver3real.y,ver3real.z},
                            uv1,uv2,uv3,
                            Vec3{nor1real.x,nor1real.y,nor1real.z},
                            Vec3{nor2real.x,nor2real.y,nor2real.z},
                            Vec3{nor3real.x,nor3real.y,nor3real.z},
                            mat ? &mat->texture : &empty_texture,
                            mat ? mat->width : 0,
                            mat ? mat->height : 0,
                            mat->shininess,mat->specular, mat->ambient, mat->diffuse,
                            shading_mode}); //mandamos el triangulo a la lista de triangulos epicamente
                    }
                }
            }
        }

        //mandamos una porcion de la pantalla a cada threat para que rasterize
        for (auto& t : threads) {
            threads_list.push_back(std::thread(&Render::Rasterize_threads, this, std::ref(triangulos), t.x, t.y));
        }

        //esperamos a que todos corran para mandarlos al codigo
        for (auto& t : threads_list) {
            t.join();
        }

        triangulos.clear(); //borramos la lista de triangulos

        //actualizamos la textura y el renderer con la textura actualizada
        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), WIDTH * sizeof(Uint32));
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);

    }

    void Rasterize_shadow_threads(const std::vector<ShadowTri>& triangulos , const int &miny_t, const int &maxy_t, std::vector<float>& shadow_map) {

        //para cada shadow triangulo
        //mismo proceso que en el rasterizador solo que con menos operaciones, pues solo necesitamos rellenar un zbuffer
        for (auto& tri : triangulos) {

            Vec3 pA = tri.p1;
            Vec3 pB = tri.p2;
            Vec3 pC = tri.p3;

            int minx = static_cast<int>(std::min(pA.x, std::min(pB.x, pC.x)));
            int miny = static_cast<int>(std::min(pA.y, std::min(pB.y, pC.y)));
            int maxx = static_cast<int>(std::max(pA.x, std::max(pB.x, pC.x)));
            int maxy = static_cast<int>(std::max(pA.y, std::max(pB.y, pC.y)));

            //esta vez es 1024 porque esas son las dimensiones que definmos para cada luz
            minx = std::max(minx, 0);
            miny = std::max(miny, 0);
            maxx = std::min(maxx, 1024 - 1);
            maxy = std::min(maxy, 1024 - 1);

            miny = std::max(miny,miny_t);
            maxy = std::min(maxy,maxy_t);

            const Vec2 pA_2d = {pA.x, pA.y};
            const Vec2 pB_2d = {pB.x, pB.y};
            const Vec2 pC_2d = {pC.x, pC.y};

            const Vec2 ladoAB = pB_2d - pA_2d;
            const Vec2 ladoBC = pC_2d - pB_2d;
            const Vec2 ladoCA = pA_2d - pC_2d;

            for (int i = minx; i <= maxx; i++) {
                for (int j = miny; j <= maxy; j++) {
                    Vec2 p = {static_cast<float>(i), static_cast<float>(j)};

                    float const edge1 = edge_function(p - pA_2d,ladoAB);
                    float const edge2 = edge_function(p - pB_2d,ladoBC);
                    float const edge3 = edge_function(p - pC_2d,ladoCA);

                    if ((edge1 >= 0 && edge2 >= 0 && edge3 >= 0)||(edge1 <= 0 && edge2 <= 0 && edge3 <= 0)) {

                        float const area_total = edge1 + edge2 + edge3;

                        float const e1_norm = edge1 / area_total;
                        float const e2_norm = edge2 / area_total;
                        float const e3_norm = edge3 / area_total;

                        //no usamos depth correct interpolation porque los ndcs ya son lineales en screen space
                        float depth = e1_norm*tri.ndc3 + e2_norm*tri.ndc1 + e3_norm*tri.ndc2;

                        if (depth < shadow_map[j * 1024 + i]) {
                            shadow_map[j * 1024 + i] = depth;
                        } else {
                            continue;
                        }
                    }
                }
            }
        }
    }

    void shadow_pass(const std::vector<Mesh>& meshes, std::variant<DirectionalLight,SpotLight> &light) {

        //extraemos la luz con su tipo
        std::visit([&](auto& luz) {

            if (luz.dirty){ //si la luz necesita actualizarce, este flag justamente define eso

                std::vector<std::thread> threads_list;
                std::vector<ShadowTri> shadow_triangulos;

                //reiniciamos el shadow map de la luz a cada frame
                std::fill(luz.shadow_map.begin(), luz.shadow_map.end(), HUGE_VALF);

                //para cada mesh, mismo proceso que render_obj pero más simple con menos operaciones,
                //pues las luces solo necesitan de un mapa de profundidad, nada más

                for (const auto& mesh : meshes) {

                    Vec3 move = mesh.transforms.translation;
                    const float rotx = mesh.transforms.rotation.x;
                    const float roty = mesh.transforms.rotation.y;
                    const float rotz = mesh.transforms.rotation.z;
                    const float scale = mesh.transforms.scale;

                    const std::vector<Face>& faces = mesh.faces;
                    const std::vector<Vec3>& vertices = mesh.vertices;
                    const std::vector<Vec2>& uvs = mesh.uvs;
                    const std::vector<Vec3>& normals = mesh.normals;

                    const Mat4 modelMatrix = move_matrix(move.x, move.y, move.z) *
                        rotz_matrix(rotz) * roty_matrix(roty) * rotx_matrix(rotx) *
                            scale_matrix(scale);

                    for (const auto& face : faces) {
                        for (int i=1; i < face.v_indices.size()-1;i++){

                            Vec3 ver1 = vertices[face.v_indices[0]-1];
                            Vec3 ver2 = vertices[face.v_indices[i]-1];
                            Vec3 ver3 = vertices[face.v_indices[i+1]-1];

                            Vec4 ver1real = modelMatrix * Vec4{ver1.x, ver1.y, ver1.z, 1.0f};
                            Vec4 ver2real = modelMatrix * Vec4{ver2.x, ver2.y, ver2.z, 1.0f};
                            Vec4 ver3real = modelMatrix * Vec4{ver3.x, ver3.y, ver3.z, 1.0f};

                            Vec4 ver1_clip = luz.light_vp * Vec4{ver1real.x, ver1real.y, ver1real.z, 1.0f};
                            Vec4 ver2_clip = luz.light_vp * Vec4{ver2real.x, ver2real.y, ver2real.z, 1.0f};
                            Vec4 ver3_clip = luz.light_vp * Vec4{ver3real.x, ver3real.y, ver3real.z, 1.0f};

                            //frustum culling
                            if (ver1_clip.x < -ver1_clip.w && ver2_clip.x < -ver2_clip.w && ver3_clip.x < -ver3_clip.w) continue;
                            if (ver1_clip.x > ver1_clip.w && ver2_clip.x > ver2_clip.w && ver3_clip.x > ver3_clip.w) continue;
                            if (ver1_clip.y < -ver1_clip.w && ver2_clip.y < -ver2_clip.w && ver3_clip.y < -ver3_clip.w) continue;
                            if (ver1_clip.y > ver1_clip.w && ver2_clip.y > ver2_clip.w && ver3_clip.y > ver3_clip.w) continue;
                            if (ver1_clip.z < -ver1_clip.w && ver2_clip.z < -ver2_clip.w && ver3_clip.z < -ver3_clip.w) continue;
                            if (ver1_clip.z > ver1_clip.w && ver2_clip.z > ver2_clip.w && ver3_clip.z > ver3_clip.w) continue;

                            Vec3 ndc1 = {ver1_clip.x / ver1_clip.w, ver1_clip.y / ver1_clip.w, ver1_clip.z / ver1_clip.w};
                            Vec3 ndc2 = {ver2_clip.x / ver2_clip.w, ver2_clip.y / ver2_clip.w, ver2_clip.z / ver2_clip.w};
                            Vec3 ndc3 = {ver3_clip.x / ver3_clip.w, ver3_clip.y / ver3_clip.w, ver3_clip.z / ver3_clip.w};

                            Vec2 real1 = {(1 + ndc1.x) * (1024 -1)/ 2, (1 - ndc1.y) * (1024 -1)/ 2};
                            Vec2 real2 = {(1 + ndc2.x) * (1024 -1)/ 2, (1 - ndc2.y) * (1024 -1)/ 2};
                            Vec2 real3 = {(1 + ndc3.x) * (1024 -1)/ 2, (1 - ndc3.y) * (1024 -1)/ 2};

                            Vec3 p1 = {real1.x, real1.y, ver1_clip.w};
                            Vec3 p2 = {real2.x, real2.y, ver2_clip.w};
                            Vec3 p3 = {real3.x, real3.y, ver3_clip.w};

                            shadow_triangulos.push_back({p1,p2,p3,ndc1.z,ndc2.z,ndc3.z});
                        }
                    }
                }

                for (auto& t : shadow_threads) {//mando la lista de shadow triangulos con sus limites a cada threat
                    threads_list.push_back(std::thread(&Render::Rasterize_shadow_threads, this,
                        std::ref(shadow_triangulos), t.x, t.y, std::ref(luz.shadow_map)));
                }

                for (auto& t : threads_list) { //espero a que todos corran para mandarlos al codigo
                    t.join();
                }
                shadow_triangulos.clear();

            }

            //por defecto, despues de correr una vez establezco el flag a falso, para evitar ejecutar de nuevo esto si no es necesario
            luz.dirty = false;

        },light);
    }

    void render_shadows(const std::vector <Mesh>& meshes) {

        //por cada luz, construyo su shadow map
        for (auto& light: lights) {
            shadow_pass(meshes, light);

        }

    }

    ~Render() {
        SDL_DestroyTexture(texture); //destruimos la textura al terminar
    }

};