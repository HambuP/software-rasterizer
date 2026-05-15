#include <SDL3/SDL.h>
#include <ctime>
#include "include/render.hpp"

constexpr int WIDTH = 800; //usamos constexpr en vez del simple const esto lo evalua en compile time y no en runtime, así que el programa reemplaza el valor antes de ejecutarlo y no tener que buscarlo mientras corre
constexpr int HEIGHT = 600; //el programa. Es lo más eficiente para variables de este tipo. El programa no busca el valor en memoria, pues ya está escrito

int main(int argc, char* argv[]) { // aquí el argc(numero de argumentos) y el char* argv (array con strings de argumentos)
                                    //son los argumentos que le mandamos al programa y SDL3 por convención lo requiere, aunque no los vayamos
                                    //a usar, pero es necesario para que el programa funcione correctamente
    //coso para fps
    Uint64 startTicks = SDL_GetTicks();
    int frameCount = 0;
    float fps = 0.0f;


    SDL_Init(SDL_INIT_VIDEO); // Entonces esto es para inicializar SDL

    SDL_Window* window = SDL_CreateWindow("Rasterizer",WIDTH,HEIGHT,0); // creamos el datatype de window y con el pointer esto nos va a llevar al window que va a estar en SDL_CreateWindow
                                                    // SDL_CreateWindow crea la ventana, pero debido a la complexidad que usa SDL lo maneja internamente, esta función, nos
                                                    // devuelve la direccion en donde se creo la ventana y esa se guarda en window

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr); // SDL_CreateRenderer es para crear el renderer, que es el encargado de dibujar en la ventana,
                                                                                        //y le pasamos la ventana que acabamos de crear, un nullptr para el driver
                                                                                        //(que es el que se encarga de manejar la renderización) con nullptr elige el mejor, que traduce todito al lenguaje que usa Windows10

    Render renderizador(renderer,WIDTH,HEIGHT); //incializamos el render con la clase

    Mesh cubo_minecraft;
    Mesh carrito;
    Mesh skull;
    Mesh skull2;
    Mesh fondo;

    cubo_minecraft.load_obj("obj/cubo_mine/sin_nombre.obj"); //cargamos el cubo de minecraft
    carrito.load_obj("obj/Jeep_Renegade_2016_obj/Jeep_Renegade_2016.obj"); //cargamos el carrito
    skull.load_obj("obj/skull/12140_Skull_v3_L2.obj");
    //skull2.load_obj("obj/skull/12140_Skull_v3_L2.obj");
    fondo.load_obj("obj/fondo/room.obj");

    std::vector<Mesh> meshes; //esto es para el caso de que queramos renderizar varios meshes, por ahora solo tenemos uno, pero asi se veria el codigo
    //meshes.push_back(carrito);
    //meshes.push_back(cubo_minecraft);
    meshes.push_back(skull);
    meshes.push_back(fondo);
    //meshes.push_back(skull2);

    meshes[0].transforms.translation.z = 14;
    meshes[0].transforms.scale = 0.4;
    meshes[0].transforms.translation.y  = -5;
    meshes[0].transforms.translation.x  = -6;
    meshes[0].transforms.rotation.x = -1.5;
    meshes[0].shading_mode  = ShadingMode::TOON;

    meshes[1].transforms.translation.z = 12 ;
     meshes[1].transforms.scale = 3;
     meshes[1].transforms.translation.y  = -7;
     meshes[1].transforms.translation.x  = 23;
    meshes[1].transforms.rotation.y = -1.5;
    meshes[1].shading_mode  = ShadingMode::PHONG;

    bool running = true;
    SDL_Event event;

    SDL_SetWindowRelativeMouseMode(window, true);//esconde el mouse y para que de solo deltas al moverse, el que necesitamos

    const float sensitivity = 0.001; //sentibilidad de la camara
    const float speed = 0.5; //velocidad de la camara

    while (running) {

        while (SDL_PollEvent(&event)) { //el sdl_pollevent saca un evento de la cola de eventos de SDL y lo pone en la variable event
            if (event.type == SDL_EVENT_QUIT) running = false; //si cierras la ventana, running se vuelve falso

            if (event.type == SDL_EVENT_MOUSE_MOTION) { //el coso para identificar si se mueve el mouse
                renderizador.cam.yaw   -= event.motion.xrel * sensitivity;
                renderizador.cam.pitch -= event.motion.yrel * sensitivity; // negativo porque Y invertida
                renderizador.cam.pitch  = std::clamp(renderizador.cam.pitch, -1.5f, 1.5f); // evita voltear
            }

        }

        const bool* keys = SDL_GetKeyboardState(NULL);

        Vec3 front = Vec3{
            sinf(renderizador.cam.yaw) * cosf(renderizador.cam.pitch),
            sinf(renderizador.cam.pitch),
            cosf(renderizador.cam.yaw) * cosf(renderizador.cam.pitch)
        }.normalize(); //calculamos el vector front de la camara usando las formulas de rotacion};

        Vec3 right = front.cross(Vec3{0, 1, 0}).normalize(); //calculamos right con el cross product de siempre

        if (keys[SDL_SCANCODE_W]) renderizador.cam.position = renderizador.cam.position + front * speed;
        if (keys[SDL_SCANCODE_S]) renderizador.cam.position = renderizador.cam.position - front * speed;
        if (keys[SDL_SCANCODE_A]) renderizador.cam.position = renderizador.cam.position - right * speed;
        if (keys[SDL_SCANCODE_D]) renderizador.cam.position = renderizador.cam.position + right * speed;




        meshes[0].transforms.rotation.y  = cosf(clock() * 0.0001f) * 5.f;
        //meshes[1].transforms.rotation.y  = cosf(clock() * 0.0001f) * 5.f;
        //meshes[1].transforms.rotation.y  = cosf(clock() * 0.0001f+0.5) * 5.f;

        SDL_RenderClear(renderer); // esto limpia el renderer, para que no se queden los pixeles del frame anterior

        renderizador.render_shadows(meshes); //renderizamos las sombras, esto va a llenar el shadow map de cada luz con la profundidad de los pixeles que corresponden a los objetos en su posición actual, desde la perspectiva de la luz

        renderizador.render_obj(meshes); //renderizamos el cubo, esto va a llenar el framebuffer con los pixeles que corresponden al cubo en su posición actual

        SDL_RenderPresent(renderer); // esto presenta el frame actual en pantalla

        frameCount++;
        Uint32 currentTicks = SDL_GetTicks();

        if (currentTicks - startTicks > 1000) {

            fps = frameCount/((currentTicks - startTicks)/1000);

            //Mostrar en ventana
            std::string title = "FPS: " + std::to_string(static_cast<int>(fps));
            SDL_SetWindowTitle(window, title.c_str());

            //Resetear para siguiente segundo
            startTicks = currentTicks;
            frameCount = 0;

        }

    }

    SDL_DestroyRenderer(renderer); // en c++ toca liberar la memoria manualmente, así se hace
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
