// Mesh.cpp
#include "engine/Mesh.hpp"
#include "engine/Texture.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <cfloat>
#include <cmath>

namespace fs = std::filesystem;

namespace engine {

// Directório base opcional, definido pelo utilizador (para resolver paths relativos).
static std::string g_baseDirPath = "";

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Resolve paths relativos ao directório do .obj/.mtl (ou a base dir se definida).
static fs::path resolvePath(const fs::path& baseDir, const std::string& p) {
    fs::path in(p);
    if (in.is_absolute()) return in;
    return baseDir / in;
}

// Material lido do .mtl (cor difusa Kd + textura map_Kd).
struct MTLMat {
    float kd[3] = {1,1,1};
    std::string mapKd; // filename (relativo)
};

// Último token da linha (útil porque map_Kd pode ter flags antes do filename).
static std::string lastToken(const std::string& line) {
    std::istringstream ss(line);
    std::string tok, last;
    while (ss >> tok) last = tok;
    return last;
}

// Leitura simples de .mtl: suporta newmtl, Kd, map_Kd (ignorando flags no map_Kd).
static std::unordered_map<std::string, MTLMat> loadMTL(const fs::path& mtlPath) {
    std::ifstream f(mtlPath);
    if (!f) throw std::runtime_error("Can't open MTL: " + mtlPath.string());

    std::unordered_map<std::string, MTLMat> mats;
    std::string line;
    std::string currentName;
    MTLMat current;

    auto flush = [&]() {
        if (!currentName.empty()) mats[currentName] = current;
    };

    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "newmtl") {
            flush();
            current = MTLMat{};
            ss >> currentName;

        } else if (key == "Kd") {
            ss >> current.kd[0] >> current.kd[1] >> current.kd[2];

        } else if (key == "map_Kd") {
            // Lê o resto da linha e apanha só o último token (normalmente o caminho do ficheiro).
            std::string rest;
            std::getline(ss, rest);
            rest = trim(rest);
            current.mapKd = lastToken(rest);
        }
    }

    flush();
    return mats;
}

// Vertex final: posição + normal + UV (o teu shader base espera isto).
struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

struct IdxTriple {
    int vi = -1; // index posição
    int ti = -1; // index uv
    int ni = -1; // index normal
};

// Faz parse de "v/t/n", "v//n", "v/t" ou "v".
static IdxTriple parseTriple(const std::string& tok) {
    IdxTriple out;

    size_t p1 = tok.find('/');
    if (p1 == std::string::npos) {
        out.vi = std::stoi(tok) - 1;
        return out;
    }

    size_t p2 = tok.find('/', p1 + 1);

    std::string s1 = tok.substr(0, p1);
    std::string s2 = (p2 == std::string::npos)
        ? tok.substr(p1 + 1)
        : tok.substr(p1 + 1, p2 - (p1 + 1));
    std::string s3 = (p2 == std::string::npos) ? "" : tok.substr(p2 + 1);

    if (!s1.empty()) out.vi = std::stoi(s1) - 1;
    if (!s2.empty()) out.ti = std::stoi(s2) - 1;
    if (!s3.empty()) out.ni = std::stoi(s3) - 1;

    return out;
}

// ---------------- Normalização ----------------
//
// Objectivo: meter o mesh num “cubo unitário” (aprox [-0.5..0.5] em cada eixo),
// mantendo-o centrado. Isto facilita reusar modelos com escalas diferentes.
//
// Nota: como há escalas diferentes por eixo (sx/sy/sz), as normais ficam erradas
// se não forem ajustadas. Aqui faz-se um ajuste simples e renormaliza.

static inline float safeInv(float v) {
    return (std::fabs(v) > 1e-8f) ? (1.0f / v) : 1.0f;
}

static inline void normalize3(float& x, float& y, float& z) {
    float len = std::sqrt(x*x + y*y + z*z);
    if (len > 1e-8f) { x /= len; y /= len; z /= len; }
}

static void normalizeToUnitCube(std::vector<Vertex>& verts) {
    if (verts.empty()) return;

    float minX =  FLT_MAX, minY =  FLT_MAX, minZ =  FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

    for (const auto& v : verts) {
        minX = std::min(minX, v.px); minY = std::min(minY, v.py); minZ = std::min(minZ, v.pz);
        maxX = std::max(maxX, v.px); maxY = std::max(maxY, v.py); maxZ = std::max(maxZ, v.pz);
    }

    const float cx = (minX + maxX) * 0.5f;
    const float cy = (minY + maxY) * 0.5f;
    const float cz = (minZ + maxZ) * 0.5f;

    const float ex = (maxX - minX);
    const float ey = (maxY - minY);
    const float ez = (maxZ - minZ);

    const float sx = safeInv(ex);
    const float sy = safeInv(ey);
    const float sz = safeInv(ez);

    for (auto& v : verts) {
        // Centragem + escala por eixo (fica normalizado mas “não uniforme” por eixo).
        v.px = (v.px - cx) * sx;
        v.py = (v.py - cy) * sy;
        v.pz = (v.pz - cz) * sz;

        // Ajuste rápido de normais para compensar escalas diferentes (aproximação).
        // Idealmente seria usar inverse-transpose do M, mas como já “cozinhamos” aqui:
        v.nx *= ex;
        v.ny *= ey;
        v.nz *= ez;
        normalize3(v.nx, v.ny, v.nz);
    }
}

// ------------------------------------------------------

void Mesh::setBaseDirPath(const std::string& baseDirPath) {
    g_baseDirPath = baseDirPath;
}

void Mesh::destroy() {
    // Destrói recursos OpenGL associados ao mesh.
    if (textureId) glDeleteTextures(1, &textureId);
    textureId = 0;

    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    ebo = vbo = vao = 0;
    indexCount = 0;
}

Mesh Mesh::loadOBJ(const std::string& objRelativeOrFullPath) {
    Mesh mesh;

    // Resolve path (permite paths relativos a um base dir definido pelo jogo).
    fs::path objPath = fs::path(objRelativeOrFullPath);
    if (!objPath.is_absolute() && !g_baseDirPath.empty()) {
        objPath = fs::path(g_baseDirPath) / objPath;
    }
    objPath = fs::absolute(objPath);

    std::ifstream f(objPath);
    if (!f) throw std::runtime_error("Can't open OBJ: " + objPath.string());

    fs::path objDir = objPath.parent_path();

    // Buffers “raw” como vêm do OBJ.
    std::vector<float> positions; // xyz
    std::vector<float> normals;   // xyz
    std::vector<float> texcoords; // uv

    // Buffers finais para GPU.
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(4096);
    indices.reserve(4096);

    // Materiais e estado do material activo.
    std::unordered_map<std::string, MTLMat> mats;
    std::string activeMtlName;
    bool materialApplied = false;

    // Cache de “triples” (v/t/n) -> índice, para não duplicar vértices.
    std::unordered_map<std::string, unsigned int> cache;
    cache.reserve(8192);

    auto keyOf = [](const IdxTriple& t) {
        return std::to_string(t.vi) + "/" + std::to_string(t.ti) + "/" + std::to_string(t.ni);
    };

    auto getPos = [&](int vi, float& x, float& y, float& z) {
        x = positions[vi*3 + 0];
        y = positions[vi*3 + 1];
        z = positions[vi*3 + 2];
    };
    auto getNor = [&](int ni, float& x, float& y, float& z) {
        x = normals[ni*3 + 0];
        y = normals[ni*3 + 1];
        z = normals[ni*3 + 2];
    };
    auto getUV = [&](int ti, float& u, float& v) {
        u = texcoords[ti*2 + 0];
        v = texcoords[ti*2 + 1];
    };

    // Aplica o material activo ao mesh (cor difusa e/ou textura) apenas uma vez.
    // Este loader é “single material”: usa o primeiro material encontrado/activo.
    auto applyMaterialIfNeeded = [&]() {
        if (materialApplied) return;
        if (activeMtlName.empty()) return;

        auto it = mats.find(activeMtlName);
        if (it == mats.end()) return;

        mesh.kd[0] = it->second.kd[0];
        mesh.kd[1] = it->second.kd[1];
        mesh.kd[2] = it->second.kd[2];

        if (!it->second.mapKd.empty()) {
            fs::path texPath = resolvePath(objDir, it->second.mapKd);

            try {
                // flipY=true para alinhar texturas com o teu pipeline (UV vs imagem).
                Texture2D t = Texture2D::loadFromFile(texPath.string(), true);

                // Se já havia textura, remove para evitar leaks (caso o material mude).
                if (mesh.textureId) glDeleteTextures(1, &mesh.textureId);

                mesh.textureId = t.id;
                t.id = 0; // passa ownership para o Mesh
            } catch (const std::exception& e) {
                std::cerr << "[Mesh] texture load failed for " << texPath.string()
                          << " : " << e.what() << "\n";
            }
        }

        materialApplied = true;
    };

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "v") {
            float x,y,z; ss >> x >> y >> z;
            positions.push_back(x); positions.push_back(y); positions.push_back(z);

        } else if (key == "vn") {
            float x,y,z; ss >> x >> y >> z;
            normals.push_back(x); normals.push_back(y); normals.push_back(z);

        } else if (key == "vt") {
            float u,v; ss >> u >> v;
            texcoords.push_back(u); texcoords.push_back(v);

        } else if (key == "mtllib") {
            std::string mtlFile; ss >> mtlFile;
            fs::path mtlPath = resolvePath(objDir, mtlFile);
            mats = loadMTL(mtlPath);

        } else if (key == "usemtl") {
            ss >> activeMtlName;

            // Quando muda o material, permitimos voltar a aplicar.
            materialApplied = false;
            applyMaterialIfNeeded();

        } else if (key == "f") {
            // Faces podem vir trianguladas ou em polygons; triangulamos em “fan”.
            std::vector<IdxTriple> face;
            std::string tok;
            while (ss >> tok) face.push_back(parseTriple(tok));
            if (face.size() < 3) continue;

            applyMaterialIfNeeded();

            auto emitVertexIndex = [&](const IdxTriple& t) -> unsigned int {
                std::string k = keyOf(t);
                auto it = cache.find(k);
                if (it != cache.end()) return it->second;

                Vertex v{};
                if (t.vi >= 0) getPos(t.vi, v.px, v.py, v.pz);

                // Normais/UV podem não existir; mete defaults razoáveis.
                if (t.ni >= 0 && (t.ni*3 + 2) < (int)normals.size()) getNor(t.ni, v.nx, v.ny, v.nz);
                else { v.nx = 0; v.ny = 1; v.nz = 0; }

                if (t.ti >= 0 && (t.ti*2 + 1) < (int)texcoords.size()) getUV(t.ti, v.u, v.v);
                else { v.u = 0; v.v = 0; }

                unsigned int idx = (unsigned int)vertices.size();
                vertices.push_back(v);
                cache[k] = idx;
                return idx;
            };

            // Triangulação em fan: (0,i,i+1)
            for (size_t i = 1; i + 1 < face.size(); i++) {
                unsigned int i0 = emitVertexIndex(face[0]);
                unsigned int i1 = emitVertexIndex(face[i]);
                unsigned int i2 = emitVertexIndex(face[i+1]);
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }
        }
    }

    if (indices.empty() || vertices.empty()) {
        throw std::runtime_error("OBJ has no geometry: " + objPath.string());
    }

    // Normaliza para uma escala consistente (evita “um modelo gigante” vs “um modelo minúsculo”).
    normalizeToUnitCube(vertices);

    // Upload para OpenGL (VAO/VBO/EBO).
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    // Layout de atributos: 0 pos, 1 normal, 2 uv.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, px));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);

    mesh.indexCount = (int)indices.size();
    return mesh;
}
} // namespace engine
