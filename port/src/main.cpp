/*
 * Galerians PC Port — Background Room Browser
 *
 * Loads all TIM textures from port/assets/textures_real/BGTIM_{A,B,C,D}/ and
 * displays each as a 4:3-centred textured quad on a 1920x1080 window.
 *
 * Controls:
 *   RIGHT / LEFT   : next / prev texture
 *   D / A          : jump +10 / -10
 *   1 2 3 4        : switch BGTIM set (A B C D)
 *   ESC            : quit
 *
 * Title bar: "Galerians Port | BGTIM_X | Room N/Total | WxH | FPS"
 */

#include <glad/glad.h>
#include <SDL.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "port_config.h"
#include "assets/tim_loader.h"

/* ── Shaders ─────────────────────────────────────────────────────────────── */

static const char* kVertSrc =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char* kFragSrc =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "void main() { FragColor = texture(uTex, vUV); }\n";

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::fprintf(stderr, "shader error: %s\n", log);
    }
    return sh;
}

static std::vector<uint8_t> ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto n = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(n));
    f.read(reinterpret_cast<char*>(buf.data()), n);
    return buf;
}

static std::filesystem::path FindBgtimBase(const std::filesystem::path& exe_dir,
                                            const std::filesystem::path& cwd) {
    for (const auto& base : {exe_dir, exe_dir / "../..", exe_dir / "../../..",
                              cwd / "port"}) {
        auto p = base / "assets/textures_real";
        std::error_code ec;
        if (std::filesystem::is_directory(p, ec)) return p;
    }
    return {};
}

static std::vector<std::filesystem::path> ListTimFiles(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> out;
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        for (auto& c : ext) c = char(std::tolower(unsigned char(c)));
        if (ext == ".tim") out.push_back(e.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(int /*argc*/, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* win = SDL_CreateWindow("Galerians Port",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        PORT_TARGET_WIDTH, PORT_TARGET_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) { SDL_Quit(); return 1; }

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        SDL_GL_DeleteContext(ctx); SDL_DestroyWindow(win); SDL_Quit(); return 1;
    }
    SDL_GL_SetSwapInterval(1);

    /* Shader program */
    GLuint vs   = CompileShader(GL_VERTEX_SHADER,   kVertSrc);
    GLuint fs   = CompileShader(GL_FRAGMENT_SHADER, kFragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint uTex = glGetUniformLocation(prog, "uTex");

    /* 4:3 centred quad on 16:9 viewport.
     * 1440 / 1920 = 0.75 → NDC x [-0.75, 0.75], y [-1, 1].
     * v=0 at quad top maps to TIM row 0 (top of image). */
    static const float kQuad[] = {
        -0.75f, -1.0f,  0.0f, 1.0f,
         0.75f, -1.0f,  1.0f, 1.0f,
         0.75f,  1.0f,  1.0f, 0.0f,
        -0.75f, -1.0f,  0.0f, 1.0f,
         0.75f,  1.0f,  1.0f, 0.0f,
        -0.75f,  1.0f,  0.0f, 0.0f,
    };
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glViewport(0, 0, PORT_TARGET_WIDTH, PORT_TARGET_HEIGHT);

    /* Discover TIM files for all four BGTIM sets */
    std::filesystem::path exe_dir =
        std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path();
    std::filesystem::path cwd       = std::filesystem::current_path();
    std::filesystem::path bgtim_base = FindBgtimBase(exe_dir, cwd);

    static const char* const kSetNames[4] = {
        "BGTIM_A", "BGTIM_B", "BGTIM_C", "BGTIM_D"
    };
    std::array<std::vector<std::filesystem::path>, 4> sets;
    for (int i = 0; i < 4; ++i) {
        auto dir = bgtim_base / kSetNames[i];
        std::error_code ec;
        if (std::filesystem::is_directory(dir, ec))
            sets[i] = ListTimFiles(dir);
        std::printf("%s: %zu textures\n", kSetNames[i], sets[i].size());
    }

    int     active_set  = 1;   /* default BGTIM_B */
    int     current_idx = 0;
    TIMImage tex{};

    auto load_current = [&]() {
        const auto& files = sets[active_set];
        if (files.empty()) return;
        TIM_Free(tex);
        auto bytes = ReadFile(files[current_idx]);
        if (!bytes.empty())
            tex = TIM_Load(bytes.data(), bytes.size());
    };
    load_current();

    bool     running  = true;
    uint64_t fps_last = SDL_GetPerformanceCounter();
    uint64_t freq     = SDL_GetPerformanceFrequency();
    int      frames   = 0;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN: {
                const auto& files = sets[active_set];
                const int total   = int(files.size());

                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;

                case SDLK_RIGHT:
                    if (total > 0) {
                        current_idx = (current_idx + 1) % total;
                        load_current();
                    }
                    break;

                case SDLK_LEFT:
                    if (total > 0) {
                        current_idx = (current_idx - 1 + total) % total;
                        load_current();
                    }
                    break;

                case SDLK_d:
                    if (total > 0) {
                        current_idx = std::min(current_idx + 10, total - 1);
                        load_current();
                    }
                    break;

                case SDLK_a:
                    if (total > 0) {
                        current_idx = std::max(current_idx - 10, 0);
                        load_current();
                    }
                    break;

                case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: {
                    int new_set = int(ev.key.keysym.sym - SDLK_1);
                    if (new_set != active_set) {
                        active_set  = new_set;
                        current_idx = 0;
                        load_current();
                    }
                    break;
                }

                default: break;
                }
                break;
            }

            default: break;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (tex.texture_id) {
            glUseProgram(prog);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex.texture_id);
            glUniform1i(uTex, 0);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        SDL_GL_SwapWindow(win);

        ++frames;
        uint64_t now     = SDL_GetPerformanceCounter();
        double   elapsed = double(now - fps_last) / double(freq);
        if (elapsed >= 1.0) {
            const auto& files = sets[active_set];
            char title[256];
            std::snprintf(title, sizeof(title),
                "Galerians Port | %s | Room %d/%d | %dx%d | %.1f FPS",
                kSetNames[active_set],
                files.empty() ? 0 : current_idx + 1,
                int(files.size()),
                tex.width, tex.height,
                frames / elapsed);
            SDL_SetWindowTitle(win, title);
            frames   = 0;
            fps_last = now;
        }
    }

    TIM_Free(tex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
