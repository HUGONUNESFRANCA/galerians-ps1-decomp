/*
 * Galerians PC Port — main entry point
 *
 * Bootstraps SDL2 + OpenGL 3.3 core, opens a 1920x1080 window with
 * VSync, loads the first .TIM file found in port/assets/textures_real/BGTIM_B/
 * (8bpp with CLUT) and renders it as a textured quad. FPS and current
 * filename are shown in the window title.
 *
 * Quit: ESC or window close (X).
 */

#include <glad/glad.h>
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "port_config.h"
#include "assets/tim_loader.h"

static const char *kVertexShaderSrc =
    "#version 330 core\n"
    "layout(location=0) in vec2 a_pos;\n"
    "layout(location=1) in vec2 a_uv;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "    v_uv = a_uv;\n"
    "    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "}\n";

static const char *kFragmentShaderSrc =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "out vec4 frag_color;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
    "    frag_color = texture(u_tex, v_uv);\n"
    "}\n";

static GLuint CompileShader(GLenum type, const char *src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::fprintf(stderr, "shader compile error: %s\n", log);
    }
    return sh;
}

static std::vector<uint8_t> ReadFileBytes(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return {};
    std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<size_t>(n));
    if (!f.read(reinterpret_cast<char*>(buf.data()), n)) return {};
    return buf;
}

static std::filesystem::path FindAssetsDir(const std::filesystem::path& cwd,
                                           const std::filesystem::path& exe_dir) {
    std::vector<std::filesystem::path> candidates = {
        exe_dir / "assets/textures_real/BGTIM_B",
        exe_dir / "../../assets/textures_real/BGTIM_B",
        exe_dir / "../../../assets/textures_real/BGTIM_B",
        cwd / "port/assets/textures_real/BGTIM_B",
    };
    for (const auto& root : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(root, ec) &&
            std::filesystem::is_directory(root, ec)) {
            return root;
        }
    }
    return {};
}

static std::vector<std::filesystem::path>
ListTimFiles(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> out;
    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
        if (ext == ".tim") out.push_back(entry.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

int main(int /*argc*/, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window *win = SDL_CreateWindow(
        "Galerians PC Port - v0.1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        PORT_TARGET_WIDTH, PORT_TARGET_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        std::fprintf(stderr, "gladLoadGLLoader failed\n");
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1); /* VSync on */

    /* Locate the assets folder, then walk its .tim files trying each in
     * turn until one decodes successfully. */
    std::filesystem::path exe_dir =
        std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path();
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path assets_dir = FindAssetsDir(cwd, exe_dir);

    TIMImage tim{0, 0, 0};
    std::string tim_name = "(no TIM found)";
    if (assets_dir.empty()) {
        std::fprintf(stderr, "assets/textures_real/BGTIM_B/ not found in any candidate path\n");
    } else {
        std::printf("Assets dir: %s\n", assets_dir.string().c_str());
        auto tim_files = ListTimFiles(assets_dir);

        auto try_load = [&](const std::filesystem::path& path) -> bool {
            std::string name = path.filename().string();
            std::error_code ec;
            auto fsize = std::filesystem::file_size(path, ec);
            if (ec || fsize < 32) {
                std::printf("Trying %s: skipped (size %zu < 32 bytes)\n",
                            name.c_str(), size_t(fsize));
                return false;
            }
            auto bytes = ReadFileBytes(path);
            if (bytes.empty()) {
                std::printf("Trying %s: read failed\n", name.c_str());
                return false;
            }
            std::printf("Trying %s:\n", name.c_str());
            std::fflush(stdout);
            TIMImage candidate = TIM_Load(bytes.data(), bytes.size());
            if (candidate.texture_id == 0) {
                std::printf("  -> invalid TIM, trying next\n");
                return false;
            }
            tim = candidate;
            tim_name = name;
            std::printf("Loaded: %s (%dx%d)\n",
                        tim_name.c_str(), tim.width, tim.height);
            return true;
        };

        std::printf("Scanning %zu TIM(s) in BGTIM_B\n", tim_files.size());
        for (const auto& path : tim_files) if (try_load(path)) break;
        if (tim.texture_id == 0) {
            std::fprintf(stderr, "No valid .TIM found under %s\n",
                         assets_dir.string().c_str());
        }
    }

    /* Build pipeline state */
    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertexShaderSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    /* Fit the quad to the texture's aspect ratio, centered in the
     * 16:9 viewport. Falls back to a unit square when no TIM loaded. */
    float qw = 0.8f, qh = 0.8f;
    if (tim.width > 0 && tim.height > 0) {
        float img_aspect = float(tim.width) / float(tim.height);
        float win_aspect = float(PORT_TARGET_WIDTH) / float(PORT_TARGET_HEIGHT);
        if (img_aspect > win_aspect) {
            qw = 0.9f;
            qh = 0.9f * (win_aspect / img_aspect);
        } else {
            qh = 0.9f;
            qw = 0.9f * (img_aspect / win_aspect);
        }
    }

    const float verts[] = {
        /*  x     y     u     v   */
        -qw, -qh,  0.0f, 1.0f,
         qw, -qh,  1.0f, 1.0f,
         qw,  qh,  1.0f, 0.0f,
        -qw, -qh,  0.0f, 1.0f,
         qw,  qh,  1.0f, 0.0f,
        -qw,  qh,  0.0f, 0.0f,
    };
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glViewport(0, 0, PORT_TARGET_WIDTH, PORT_TARGET_HEIGHT);

    GLint u_tex = glGetUniformLocation(prog, "u_tex");

    /* Main loop */
    bool running = true;
    uint64_t fps_last_ticks = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    int frames = 0;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);
        if (tim.texture_id) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tim.texture_id);
            glUniform1i(u_tex, 0);
        }
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        SDL_GL_SwapWindow(win);

        ++frames;
        uint64_t now = SDL_GetPerformanceCounter();
        double elapsed = double(now - fps_last_ticks) / double(freq);
        if (elapsed >= 1.0) {
            char title[256];
            std::snprintf(title, sizeof(title),
                          "Galerians PC Port - v0.1  |  %s  |  %dx%d  |  %.1f FPS",
                          tim_name.c_str(), tim.width, tim.height,
                          frames / elapsed);
            SDL_SetWindowTitle(win, title);
            frames = 0;
            fps_last_ticks = now;
        }
    }

    TIM_Free(tim);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
