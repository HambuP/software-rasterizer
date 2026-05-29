#include <SDL3/SDL.h>
#include "include/render.hpp"

constexpr int WIDTH = 800; //usamos constexpr en vez del simple const esto lo evalua en compile time y no en runtime,
constexpr int HEIGHT = 600; //así que el programa reemplaza el valor antes de ejecutarlo y no tener que buscarlo mientras corre
                            //el programa. Es lo más eficiente para variables de este tipo. El programa no busca el valor en memoria, pues ya está escrito

int main(int argc, char* argv[]) { // aquí el argc(numero de argumentos) y el char* argv (array con strings de argumentos)
                                    //son los argumentos que le mandamos al programa y SDL3 por convención lo requiere, aunque no los vayamos
                                    //a usar, pero es necesario para que el programa funcione correctamente

    //utilidad para pintar fps
    Uint64 startTicks = SDL_GetTicks();
    int frameCount = 0;
    float fps = 0.0f;

    //inicializamos herramientas de ventana SDL
    SDL_Init(SDL_INIT_VIDEO); // Entonces esto es para inicializar SDL

    SDL_Window* window = SDL_CreateWindow("Rasterizer",WIDTH,HEIGHT,0);
    // creamos el datatype de window y con el pointer esto nos va a llevar al window que va a estar en SDL_CreateWindow
    // SDL_CreateWindow crea la ventana, pero debido a la complexidad que usa SDL lo maneja internamente, esta función, nos
    // devuelve la direccion en donde se creo la ventana y esa se guarda en window

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    // SDL_CreateRenderer es para crear el renderer, que es el encargado de dibujar en la ventana,
    //y le pasamos la ventana que acabamos de crear, un nullptr para el driver
    //(que es el que se encarga de manejar la renderización) con nullptr elige el mejor, que traduce todito al lenguaje que usa Windows10


    //inicializamos objetos 3D y escena 3D
    Render renderizador(renderer,WIDTH,HEIGHT); //incializamos el render con la clase

    Mesh skull;
    Mesh skull2;
    Mesh fondo;
    Mesh frustum;

    skull.load_obj("obj/skull/12140_Skull_v3_L2.obj"); // cargamos los objetos usando sus respectivos archivos
    skull2.load_obj("obj/skull/12140_Skull_v3_L2.obj");
    fondo.load_obj("obj/fondo/room.obj");
    frustum.load_obj("obj/frustum/frustum.obj");

    //floor.load_obj("obj/floor/floor.obj");

    std::vector<Mesh> meshes; //creamos la lista de objetos y la llenamos
    meshes.push_back(fondo);
    meshes.push_back(skull);
    meshes.push_back(skull2);
    //meshes.push_back(frustum);

    //definimos transformaciones iniciales para los objetos de la escena, estos ya que no entran en el loop solo se aplican pues una vez
    meshes[0].transforms.translation.z = 12 ;
    meshes[0].transforms.scale = 3;
    meshes[0].transforms.translation.x  = -4;
    meshes[0].transforms.rotation.y = -1.5;
    meshes[0].transforms.translation.y  = -7;
    meshes[0].shading_mode  = ShadingMode::PHONG;

    meshes[1].transforms.translation.z = 18;
    meshes[1].transforms.scale = 0.4;
    meshes[1].transforms.translation.y  = -5;
    meshes[1].transforms.translation.x  = -6;
    meshes[1].transforms.rotation.x = -1.5;
    meshes[1].transforms.rotation.y = 1.9;
    meshes[1].shading_mode  = ShadingMode::PHONG;

    meshes[2].transforms.translation.z = 8;
    meshes[2].transforms.scale = 0.4;
    meshes[2].transforms.translation.y  = -5;
    meshes[2].transforms.translation.x  = -6;
    meshes[2].transforms.rotation.x = -1.5;
    meshes[2].transforms.rotation.y = 1.2;
    meshes[2].shading_mode  = ShadingMode::TOON;

    bool running = true; //la variable que va a servir de condición para el loop del juego, cambia cuando nos salimos
    SDL_Event event; //esta es la variable que va a hacerce cargo de todos los eventos de SDL como movimiento mouse

    SDL_SetWindowRelativeMouseMode(window, true);//esconde el mouse y para que de solo deltas al moverse, el que necesitamos

    const float sensitivity = 0.001; //sensibilidad de la camara
    const float speed = 0.5; //velocidad de la camara

    //loop principal
    while (running) {

        while (SDL_PollEvent(&event)) { //el sdl_pollevent saca un evento de la cola de eventos de SDL y lo pone en la variable event
            if (event.type == SDL_EVENT_QUIT) running = false; //si cierras la ventana, running se vuelve falso

            if (event.type == SDL_EVENT_MOUSE_MOTION) { //el coso para identificar si se mueve el mouse
                renderizador.cam.yaw   -= event.motion.xrel * sensitivity; //ambos negativos porque ambos ejes están invertidos
                renderizador.cam.pitch -= event.motion.yrel * sensitivity;
                renderizador.cam.pitch  = std::clamp(renderizador.cam.pitch, -1.5f, 1.5f); // evita voltear
            }

        }

        const bool* keys = SDL_GetKeyboardState(NULL); //revisa el estado del teclado en cada frame

        Vec3 front = Vec3{
            sinf(renderizador.cam.yaw) * cosf(renderizador.cam.pitch),
            sinf(renderizador.cam.pitch),
            cosf(renderizador.cam.yaw) * cosf(renderizador.cam.pitch)
        }.normalize(); //calculamos el vector front de la camara usando las formulas de rotacion, es lo mismo que coordenadas polares en 3D más o menos

        Vec3 right = front.cross(Vec3{0, 1, 0}).normalize(); //calculamos right con el cross product de siempre

        //si keys tiene adentro una de estas, ejecutar, aquí movemos la camara y ya
        if (keys[SDL_SCANCODE_W]) renderizador.cam.position = renderizador.cam.position + front * speed;
        if (keys[SDL_SCANCODE_S]) renderizador.cam.position = renderizador.cam.position - front * speed;
        if (keys[SDL_SCANCODE_A]) renderizador.cam.position = renderizador.cam.position - right * speed;
        if (keys[SDL_SCANCODE_D]) renderizador.cam.position = renderizador.cam.position + right * speed;

        // marcar las sombras como dirty, esto se activa manualmente si sabes que las shadow map tienen que actualizarse a cada frame
        //for (auto& light : renderizador.lights) {
        //    std::visit([](auto& l) { l.dirty = true; }, light);
        //}

        SDL_RenderClear(renderer); // esto limpia el renderer, para que no se queden los pixeles del frame anterior

        renderizador.render_shadows(meshes); //renderizamos las sombras, esto va a llenar el shadow map de cada luz
        renderizador.render_obj(meshes); //renderizamos los meshes, esto va a llenar el framebuffer

        SDL_RenderPresent(renderer); // esto presenta el frame actual en pantalla

        frameCount++; //cada frame le subimos al frame count
        Uint32 currentTicks = SDL_GetTicks(); //obtenemos los ticks actuales

        if (currentTicks - startTicks > 1000) { //si pasó un segundo entre los ticks actuales y  los de inicia

            fps = frameCount * 1000.0f / (currentTicks - startTicks);

            //Mostrar en ventana
            std::string title = "FPS: " + std::to_string(static_cast<int>(fps));
            SDL_SetWindowTitle(window, title.c_str());

            //Resetear para siguiente segundo
            startTicks = currentTicks;
            frameCount = 0;

        }
    }

    //destructores para no dejar nada en memoria
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}