
#pragma once //esto le dice al compilador que incluya el archivo solo una vez, o si no, se puede morir todito

#include <cmath>

struct Vec2 {
    float x, y;

    Vec2 operator-(const Vec2& other) const {
        return {x - other.x, y - other.y};
    }
    Vec2 operator+(const Vec2& other) const {
        return {x + other.x, y + other.y};
    }
    Vec2 operator*(const float &k) const { return {x * k, y * k}; }
};

struct Vec3 {
    float x, y, z;

    Vec3 operator+(const Vec3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    float length() const { return sqrtf(x * x + y * y + z * z); }
    Vec3 operator*(const float &k) const { return {x * k , y * k , z * k}; }
    float operator*(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }
    Vec3 cross(const Vec3& other) const { return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x}; }
    Vec3 normalize() const { float l = length(); return {x / l, y / l, z / l}; }
};

struct Vec4 {
    float x, y, z, w;

     Vec4 normalize() const {
        const float length = sqrtf(x * x + y * y + z * z + w * w);
        return {x / length, y / length, z / length, w / length};
    }


};

struct Mat4 {
    float m[16]; // es más eficiente usar un flat array para la matriz

    Mat4 operator*(const Mat4& other) const {
        Mat4 result{};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
                }
            }
        }
        return result;
    }

    Vec4 operator*(const Vec4& other) const {
        return {
            m[0] * other.x + m[1] * other.y + m[2] * other.z + m[3] * other.w,
            m[4] * other.x + m[5] * other.y + m[6] * other.z + m[7] * other.w,
            m[8] * other.x + m[9] * other.y + m[10] * other.z + m[11] * other.w,
            m[12] * other.x + m[13] * other.y + m[14] * other.z + m[15] * other.w
        };
    }
};

struct Color {
    int r, g, b;
};

inline float edge_function(const Vec2& a, const Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

inline float radianes(float const grados) {
    return grados * 3.14159265f / 180.0f;
}

inline Mat4 rotx_matrix(float const angle) {
    return {
        1,0,0,0,
        0,cosf(angle),-sinf(angle),0,
        0,sinf(angle),cosf(angle),0,
        0,0,0,1,
    };
}

inline Mat4 roty_matrix(float const angle) {
    return {
        cosf(angle),0,sinf(angle),0,
        0,1,0,0,
        -sinf(angle),0,cosf(angle),0,
        0,0,0,1,
    };
}

inline Mat4 rotz_matrix(float const angle) {
    return {
        cosf(angle),-sinf(angle),0,0,
        sinf(angle),cosf(angle),0,0,
        0,0,1,0,
        0,0,0,1,
    };
}

inline Mat4 scale_matrix(float const k) {
    return {
        k,0,0,0,
        0,k,0,0,
        0,0,k,0,
        0,0,0,1,
    };
}

inline Mat4 move_matrix(float const x, float const y, float const z) {
    return {
        1,0,0,x,
        0,1,0,y,
        0,0,1,z,
        0,0,0,1,
    };
}

inline Mat4 lookAt_matrix(Vec3 const &eye, Vec3 const &center) {
    const Vec3 temp_up = {0, 1, 0}; // Vector de "arriba" temporal, se puede ajustar según la orientación deseada

    Vec3 forward = (center - eye).normalize(); //a donde ve menos en donde esta para calcular forward, y claro, para hacer todos los calculos toca normalizarlos
    Vec3 right = forward.cross(temp_up.normalize()).normalize(); // el cross product te da un vector perpendicular a ambos, perfect para esto
    Vec3 up = right.cross(forward); // y calculamos up real

    return {// esta es la inversa, o traspuesta de la matriz de view matrix, que hace todos los cambios a la camara, nosotros lo que queremos es que el mundo hago exactamente lo contrario
        right.x,   right.y,   right.z,   -(right * eye),
        up.x,      up.y,      up.z,      -(up * eye),
        forward.x, forward.y, forward.z, -(forward * eye),
        0,         0,         0,         1
    };

    // return {   // esta es la inversa, o traspuesta de la matriz de view matrix, que hace todos los cambios a la camara, nosotros lo que queremos es que el mundo hago exactamente lo contrario
    //     right.x, up.x, -forward.x, 0, // porque usamos el dot en la traslación? justamente, porque aquí el orden que estamos usando es traslacion y despues rotacion, pero para la inversa, toca hacer
    //     right.y, up.y, -forward.y, 0, // rotacion y traslacion para no embarrarla, y el problema, es que si intentamos hacer la traslacion despues de la rotacion con las simple coords de cam
    //     right.z, up.z, -forward.z, 0, // nuestro cubo sale en donde no tiene que ser, estos dot products solucionan eso justamente
    //     -(right * eye), -(up * eye), (forward * eye), 1 //forward es negativo por convecion de poner a los objetos en -z
    // };
}

inline Mat4 projection_matrix(float const fov, float const aspect, float const near, float const far) {
    const float focal_l = 1/tanf(radianes(fov) / 2); //usamos el field of view para calcular la distancia focal, por convencion nuestra pantalla la queremos de -1 a 1, y el fov es para la altura
    return {                                            //ahora, dividimos por el aspect ratio porque x es mas grande, así que abarca más, ya después ponemos en relacion haciendo viewport transform
        focal_l/aspect,0,0,0,          // para poder poner ndc de z necesitamos un z clip que nos funcione, queremos que near plane sea -1 y far sea 1, de esta manera logramos eso
        0,focal_l, 0, 0,                // y al final, claro, mandamos la valor de z a w para no perderla y poder hacer el perspective divide justo despues
        0,0,(near+far)/(far-near), 2*(near*far)/(near-far),
        0,0,1,0
    };

}

inline Mat4 ortho_matrix(float const left, float const right, float const bottom, float const top, float const near, float const far) { //matriz de pryeccion ortografica, mucho más simple que la de proyeccion, para las luces
    return {
        2/(right-left),0,0,-(right+left)/(right-left),
        0,2/(top-bottom),0,-(top+bottom)/(top-bottom),
        0,0,2/(far-near),-(far+near)/(far-near),
        0,0,0,1
    };
}