// Dear ImGui renderer for Project Zomboid's modern OpenGL context.
// Uses a private shader/VBO/EBO/VAO and renders to the window back buffer,
// then restores the offscreen framebuffer and state left by the game.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <GL/gl.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "imgui.h"
#include "imgui_impl_opengl3.h"

namespace {

using gl_sizeiptr = std::ptrdiff_t;
using gl_char = char;

constexpr GLenum gl_active_texture = 0x84E0;
constexpr GLenum gl_texture0 = 0x84C0;
constexpr GLenum gl_current_program = 0x8B8D;
constexpr GLenum gl_shader_type = 0x8B4F;
constexpr GLenum gl_compile_status = 0x8B81;
constexpr GLenum gl_link_status = 0x8B82;
constexpr GLenum gl_info_log_length = 0x8B84;
constexpr GLenum gl_vertex_shader = 0x8B31;
constexpr GLenum gl_fragment_shader = 0x8B30;
constexpr GLenum gl_array_buffer = 0x8892;
constexpr GLenum gl_element_array_buffer = 0x8893;
constexpr GLenum gl_array_buffer_binding = 0x8894;
constexpr GLenum gl_element_array_buffer_binding = 0x8895;
constexpr GLenum gl_stream_draw = 0x88E0;
constexpr GLenum gl_vertex_array_binding = 0x85B5;
constexpr GLenum gl_draw_framebuffer = 0x8CA9;
constexpr GLenum gl_draw_framebuffer_binding = 0x8CA6;
constexpr GLenum gl_sampler_binding = 0x8919;
constexpr GLenum gl_blend_src_rgb = 0x80C9;
constexpr GLenum gl_blend_dst_rgb = 0x80C8;
constexpr GLenum gl_blend_src_alpha = 0x80CB;
constexpr GLenum gl_blend_dst_alpha = 0x80CA;
constexpr GLenum gl_blend_equation_rgb = 0x8009;
constexpr GLenum gl_blend_equation_alpha = 0x883D;
constexpr GLenum gl_func_add = 0x8006;
constexpr GLenum gl_clip_origin = 0x935C;
constexpr GLenum gl_upper_left = 0x8CA2;
constexpr GLenum gl_unpack_row_length = 0x0CF2;
constexpr GLenum gl_draw_buffer = 0x0C01;

using active_texture_fn = void(APIENTRY*)(GLenum);
using use_program_fn = void(APIENTRY*)(GLuint);
using create_shader_fn = GLuint(APIENTRY*)(GLenum);
using shader_source_fn = void(APIENTRY*)(GLuint, GLsizei, const gl_char* const*, const GLint*);
using compile_shader_fn = void(APIENTRY*)(GLuint);
using get_shader_iv_fn = void(APIENTRY*)(GLuint, GLenum, GLint*);
using get_shader_info_log_fn = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, gl_char*);
using delete_shader_fn = void(APIENTRY*)(GLuint);
using create_program_fn = GLuint(APIENTRY*)();
using attach_shader_fn = void(APIENTRY*)(GLuint, GLuint);
using detach_shader_fn = void(APIENTRY*)(GLuint, GLuint);
using link_program_fn = void(APIENTRY*)(GLuint);
using get_program_iv_fn = void(APIENTRY*)(GLuint, GLenum, GLint*);
using get_program_info_log_fn = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, gl_char*);
using delete_program_fn = void(APIENTRY*)(GLuint);
using get_uniform_location_fn = GLint(APIENTRY*)(GLuint, const gl_char*);
using uniform_1i_fn = void(APIENTRY*)(GLint, GLint);
using uniform_matrix_4fv_fn = void(APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat*);
using gen_buffers_fn = void(APIENTRY*)(GLsizei, GLuint*);
using bind_buffer_fn = void(APIENTRY*)(GLenum, GLuint);
using buffer_data_fn = void(APIENTRY*)(GLenum, gl_sizeiptr, const void*, GLenum);
using delete_buffers_fn = void(APIENTRY*)(GLsizei, const GLuint*);
using gen_vertex_arrays_fn = void(APIENTRY*)(GLsizei, GLuint*);
using bind_vertex_array_fn = void(APIENTRY*)(GLuint);
using delete_vertex_arrays_fn = void(APIENTRY*)(GLsizei, const GLuint*);
using enable_vertex_attrib_array_fn = void(APIENTRY*)(GLuint);
using vertex_attrib_pointer_fn = void(APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using bind_sampler_fn = void(APIENTRY*)(GLuint, GLuint);
using blend_equation_separate_fn = void(APIENTRY*)(GLenum, GLenum);
using blend_func_separate_fn = void(APIENTRY*)(GLenum, GLenum, GLenum, GLenum);
using bind_framebuffer_fn = void(APIENTRY*)(GLenum, GLuint);

struct gl_functions {
    active_texture_fn active_texture{};
    use_program_fn use_program{};
    create_shader_fn create_shader{};
    shader_source_fn shader_source{};
    compile_shader_fn compile_shader{};
    get_shader_iv_fn get_shader_iv{};
    get_shader_info_log_fn get_shader_info_log{};
    delete_shader_fn delete_shader{};
    create_program_fn create_program{};
    attach_shader_fn attach_shader{};
    detach_shader_fn detach_shader{};
    link_program_fn link_program{};
    get_program_iv_fn get_program_iv{};
    get_program_info_log_fn get_program_info_log{};
    delete_program_fn delete_program{};
    get_uniform_location_fn get_uniform_location{};
    uniform_1i_fn uniform_1i{};
    uniform_matrix_4fv_fn uniform_matrix_4fv{};
    gen_buffers_fn gen_buffers{};
    bind_buffer_fn bind_buffer{};
    buffer_data_fn buffer_data{};
    delete_buffers_fn delete_buffers{};
    gen_vertex_arrays_fn gen_vertex_arrays{};
    bind_vertex_array_fn bind_vertex_array{};
    delete_vertex_arrays_fn delete_vertex_arrays{};
    enable_vertex_attrib_array_fn enable_vertex_attrib_array{};
    vertex_attrib_pointer_fn vertex_attrib_pointer{};
    bind_sampler_fn bind_sampler{};
    blend_equation_separate_fn blend_equation_separate{};
    blend_func_separate_fn blend_func_separate{};
    bind_framebuffer_fn bind_framebuffer{};
};

gl_functions gl{};
GLuint font_texture{};
GLuint shader_program{};
GLuint vertex_shader_handle{};
GLuint fragment_shader_handle{};
GLuint vertex_buffer{};
GLuint element_buffer{};
GLint texture_location{-1};
GLint projection_location{-1};

void* load_gl_proc(const char* name)
{
    void* proc = reinterpret_cast<void*>(::wglGetProcAddress(name));
    if (proc == nullptr || proc == reinterpret_cast<void*>(1) ||
        proc == reinterpret_cast<void*>(2) || proc == reinterpret_cast<void*>(3) ||
        proc == reinterpret_cast<void*>(static_cast<std::intptr_t>(-1))) {
        const HMODULE module = ::GetModuleHandleW(L"opengl32.dll");
        proc = module ? reinterpret_cast<void*>(::GetProcAddress(module, name)) : nullptr;
    }
    return proc;
}

template <typename Function>
bool load(Function& destination, const char* name)
{
    destination = reinterpret_cast<Function>(load_gl_proc(name));
    return destination != nullptr;
}

bool load_functions()
{
    return load(gl.active_texture, "glActiveTexture") &&
        load(gl.use_program, "glUseProgram") &&
        load(gl.create_shader, "glCreateShader") &&
        load(gl.shader_source, "glShaderSource") &&
        load(gl.compile_shader, "glCompileShader") &&
        load(gl.get_shader_iv, "glGetShaderiv") &&
        load(gl.get_shader_info_log, "glGetShaderInfoLog") &&
        load(gl.delete_shader, "glDeleteShader") &&
        load(gl.create_program, "glCreateProgram") &&
        load(gl.attach_shader, "glAttachShader") &&
        load(gl.detach_shader, "glDetachShader") &&
        load(gl.link_program, "glLinkProgram") &&
        load(gl.get_program_iv, "glGetProgramiv") &&
        load(gl.get_program_info_log, "glGetProgramInfoLog") &&
        load(gl.delete_program, "glDeleteProgram") &&
        load(gl.get_uniform_location, "glGetUniformLocation") &&
        load(gl.uniform_1i, "glUniform1i") &&
        load(gl.uniform_matrix_4fv, "glUniformMatrix4fv") &&
        load(gl.gen_buffers, "glGenBuffers") &&
        load(gl.bind_buffer, "glBindBuffer") &&
        load(gl.buffer_data, "glBufferData") &&
        load(gl.delete_buffers, "glDeleteBuffers") &&
        load(gl.gen_vertex_arrays, "glGenVertexArrays") &&
        load(gl.bind_vertex_array, "glBindVertexArray") &&
        load(gl.delete_vertex_arrays, "glDeleteVertexArrays") &&
        load(gl.enable_vertex_attrib_array, "glEnableVertexAttribArray") &&
        load(gl.vertex_attrib_pointer, "glVertexAttribPointer") &&
        load(gl.bind_sampler, "glBindSampler") &&
        load(gl.blend_equation_separate, "glBlendEquationSeparate") &&
        load(gl.blend_func_separate, "glBlendFuncSeparate") &&
        load(gl.bind_framebuffer, "glBindFramebuffer");
}

bool shader_compiled(GLuint shader)
{
    GLint status{};
    gl.get_shader_iv(shader, gl_compile_status, &status);
    if (status == GL_TRUE)
        return true;

    GLint length{};
    gl.get_shader_iv(shader, gl_info_log_length, &length);
    if (length > 1) {
        ImVector<char> message;
        message.resize(length + 1);
        gl.get_shader_info_log(shader, length, nullptr, message.Data);
        ::OutputDebugStringA(message.Data);
    }
    return false;
}

bool program_linked(GLuint program)
{
    GLint status{};
    gl.get_program_iv(program, gl_link_status, &status);
    if (status == GL_TRUE)
        return true;

    GLint length{};
    gl.get_program_iv(program, gl_info_log_length, &length);
    if (length > 1) {
        ImVector<char> message;
        message.resize(length + 1);
        gl.get_program_info_log(program, length, nullptr, message.Data);
        ::OutputDebugStringA(message.Data);
    }
    return false;
}

} // namespace

bool ImGui_ImplOpenGL3_Init()
{
    return load_functions();
}

bool ImGui_ImplOpenGL3_CreateDeviceObjects()
{
    GLint last_active_texture{};
    GLint last_texture{};
    GLint last_program{};
    GLint last_array_buffer{};
    GLint last_vertex_array{};
    glGetIntegerv(gl_active_texture, &last_active_texture);
    gl.active_texture(gl_texture0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(gl_current_program, &last_program);
    glGetIntegerv(gl_array_buffer_binding, &last_array_buffer);
    glGetIntegerv(gl_vertex_array_binding, &last_vertex_array);

    static constexpr const char* vertex_source =
        "#version 330 core\n"
        "layout(location=0) in vec2 Position;\n"
        "layout(location=1) in vec2 UV;\n"
        "layout(location=2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main(){ Frag_UV=UV; Frag_Color=Color; gl_Position=ProjMtx*vec4(Position,0,1); }\n";
    static constexpr const char* fragment_source =
        "#version 330 core\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout(location=0) out vec4 Out_Color;\n"
        "void main(){ Out_Color=Frag_Color*texture(Texture,Frag_UV); }\n";

    vertex_shader_handle = gl.create_shader(gl_vertex_shader);
    gl.shader_source(vertex_shader_handle, 1, &vertex_source, nullptr);
    gl.compile_shader(vertex_shader_handle);
    fragment_shader_handle = gl.create_shader(gl_fragment_shader);
    gl.shader_source(fragment_shader_handle, 1, &fragment_source, nullptr);
    gl.compile_shader(fragment_shader_handle);

    bool success = shader_compiled(vertex_shader_handle) &&
        shader_compiled(fragment_shader_handle);
    if (success) {
        shader_program = gl.create_program();
        gl.attach_shader(shader_program, vertex_shader_handle);
        gl.attach_shader(shader_program, fragment_shader_handle);
        gl.link_program(shader_program);
        success = program_linked(shader_program);
    }

    if (success) {
        texture_location = gl.get_uniform_location(shader_program, "Texture");
        projection_location = gl.get_uniform_location(shader_program, "ProjMtx");
        gl.gen_buffers(1, &vertex_buffer);
        gl.gen_buffers(1, &element_buffer);

        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels{};
        int width{};
        int height{};
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        GLint last_unpack_row_length{};
        GLint last_unpack_alignment{};
        glGetIntegerv(gl_unpack_row_length, &last_unpack_row_length);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &last_unpack_alignment);
        glPixelStorei(gl_unpack_row_length, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glGenTextures(1, &font_texture);
        glBindTexture(GL_TEXTURE_2D, font_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glPixelStorei(gl_unpack_row_length, last_unpack_row_length);
        glPixelStorei(GL_UNPACK_ALIGNMENT, last_unpack_alignment);
        io.Fonts->TexID = reinterpret_cast<ImTextureID>(
            static_cast<std::intptr_t>(font_texture));
        success = font_texture != 0 && vertex_buffer != 0 && element_buffer != 0;
    }

    gl.use_program(static_cast<GLuint>(last_program));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(last_texture));
    gl.bind_vertex_array(static_cast<GLuint>(last_vertex_array));
    gl.bind_buffer(gl_array_buffer, static_cast<GLuint>(last_array_buffer));
    gl.active_texture(static_cast<GLenum>(last_active_texture));

    if (!success)
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
    return success;
}

void ImGui_ImplOpenGL3_DestroyDeviceObjects()
{
    if (vertex_buffer)
        gl.delete_buffers(1, &vertex_buffer);
    if (element_buffer)
        gl.delete_buffers(1, &element_buffer);
    vertex_buffer = 0;
    element_buffer = 0;

    if (shader_program && vertex_shader_handle)
        gl.detach_shader(shader_program, vertex_shader_handle);
    if (shader_program && fragment_shader_handle)
        gl.detach_shader(shader_program, fragment_shader_handle);
    if (vertex_shader_handle)
        gl.delete_shader(vertex_shader_handle);
    if (fragment_shader_handle)
        gl.delete_shader(fragment_shader_handle);
    if (shader_program)
        gl.delete_program(shader_program);
    vertex_shader_handle = 0;
    fragment_shader_handle = 0;
    shader_program = 0;

    if (font_texture) {
        glDeleteTextures(1, &font_texture);
        ImGui::GetIO().Fonts->TexID = nullptr;
        font_texture = 0;
    }
}

void ImGui_ImplOpenGL3_Shutdown()
{
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
}

void ImGui_ImplOpenGL3_NewFrame()
{
    if (!shader_program)
        ImGui_ImplOpenGL3_CreateDeviceObjects();
}

void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    const int framebuffer_width = static_cast<int>(
        draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const int framebuffer_height = static_cast<int>(
        draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (framebuffer_width <= 0 || framebuffer_height <= 0 || !shader_program)
        return;

    GLint last_active_texture{};
    GLint last_program{};
    GLint last_texture{};
    GLint last_sampler{};
    GLint last_array_buffer{};
    GLint last_element_array_buffer{};
    GLint last_vertex_array{};
    GLint last_draw_framebuffer{};
    GLint last_draw_buffer{};
    GLint last_polygon_mode[2]{};
    GLint last_viewport[4]{};
    GLint last_scissor_box[4]{};
    GLint last_blend_src_rgb{};
    GLint last_blend_dst_rgb{};
    GLint last_blend_src_alpha{};
    GLint last_blend_dst_alpha{};
    GLint last_blend_equation_rgb{};
    GLint last_blend_equation_alpha{};
    GLint clip_origin{};

    glGetIntegerv(gl_active_texture, &last_active_texture);
    gl.active_texture(gl_texture0);
    glGetIntegerv(gl_current_program, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(gl_sampler_binding, &last_sampler);
    glGetIntegerv(gl_array_buffer_binding, &last_array_buffer);
    glGetIntegerv(gl_element_array_buffer_binding, &last_element_array_buffer);
    glGetIntegerv(gl_vertex_array_binding, &last_vertex_array);
    glGetIntegerv(gl_draw_framebuffer_binding, &last_draw_framebuffer);
    glGetIntegerv(gl_draw_buffer, &last_draw_buffer);
    glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    glGetIntegerv(gl_blend_src_rgb, &last_blend_src_rgb);
    glGetIntegerv(gl_blend_dst_rgb, &last_blend_dst_rgb);
    glGetIntegerv(gl_blend_src_alpha, &last_blend_src_alpha);
    glGetIntegerv(gl_blend_dst_alpha, &last_blend_dst_alpha);
    glGetIntegerv(gl_blend_equation_rgb, &last_blend_equation_rgb);
    glGetIntegerv(gl_blend_equation_alpha, &last_blend_equation_alpha);
    glGetIntegerv(gl_clip_origin, &clip_origin);

    const GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    const GLboolean cull_enabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean depth_enabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);

    gl.bind_framebuffer(gl_draw_framebuffer, 0);
    glDrawBuffer(GL_BACK);
    gl.active_texture(gl_texture0);
    gl.use_program(shader_program);
    gl.uniform_1i(texture_location, 0);
    gl.bind_sampler(0, 0);

    GLuint vertex_array{};
    gl.gen_vertex_arrays(1, &vertex_array);
    gl.bind_vertex_array(vertex_array);
    gl.bind_buffer(gl_array_buffer, vertex_buffer);
    gl.bind_buffer(gl_element_array_buffer, element_buffer);
    gl.enable_vertex_attrib_array(0);
    gl.enable_vertex_attrib_array(1);
    gl.enable_vertex_attrib_array(2);
    gl.vertex_attrib_pointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
        reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, pos)));
    gl.vertex_attrib_pointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
        reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, uv)));
    gl.vertex_attrib_pointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
        reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, col)));

    glEnable(GL_BLEND);
    gl.blend_equation_separate(gl_func_add, gl_func_add);
    gl.blend_func_separate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    const float left = draw_data->DisplayPos.x;
    const float right = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    const float top = draw_data->DisplayPos.y;
    const float bottom = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const float projection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { (right + left) / (left - right),
          (top + bottom) / (bottom - top), 0.0f, 1.0f },
    };
    gl.uniform_matrix_4fv(projection_location, 1, GL_FALSE, &projection[0][0]);

    const ImVec2 clip_offset = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;
    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
        const ImDrawList* command_list = draw_data->CmdLists[list_index];
        gl.bind_buffer(gl_array_buffer, vertex_buffer);
        gl.buffer_data(gl_array_buffer,
            static_cast<gl_sizeiptr>(command_list->VtxBuffer.Size) * sizeof(ImDrawVert),
            command_list->VtxBuffer.Data, gl_stream_draw);
        gl.bind_buffer(gl_element_array_buffer, element_buffer);
        gl.buffer_data(gl_element_array_buffer,
            static_cast<gl_sizeiptr>(command_list->IdxBuffer.Size) * sizeof(ImDrawIdx),
            command_list->IdxBuffer.Data, gl_stream_draw);

        std::size_t index_offset{};
        for (int command_index = 0; command_index < command_list->CmdBuffer.Size;
            ++command_index) {
            const ImDrawCmd* command = &command_list->CmdBuffer[command_index];
            if (command->UserCallback) {
                command->UserCallback(command_list, command);
            } else {
                const ImVec4 clip_rect{
                    (command->ClipRect.x - clip_offset.x) * clip_scale.x,
                    (command->ClipRect.y - clip_offset.y) * clip_scale.y,
                    (command->ClipRect.z - clip_offset.x) * clip_scale.x,
                    (command->ClipRect.w - clip_offset.y) * clip_scale.y };
                if (clip_rect.x < framebuffer_width && clip_rect.y < framebuffer_height &&
                    clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    const int scissor_y = clip_origin == static_cast<GLint>(gl_upper_left)
                        ? static_cast<int>(clip_rect.y)
                        : static_cast<int>(framebuffer_height - clip_rect.w);
                    glScissor(static_cast<int>(clip_rect.x), scissor_y,
                        static_cast<int>(clip_rect.z - clip_rect.x),
                        static_cast<int>(clip_rect.w - clip_rect.y));
                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(
                        reinterpret_cast<std::intptr_t>(command->TextureId)));
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(command->ElemCount),
                        sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                        reinterpret_cast<void*>(index_offset));
                }
            }
            index_offset += static_cast<std::size_t>(command->ElemCount) * sizeof(ImDrawIdx);
        }
    }

    gl.delete_vertex_arrays(1, &vertex_array);
    gl.use_program(static_cast<GLuint>(last_program));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(last_texture));
    gl.bind_sampler(0, static_cast<GLuint>(last_sampler));
    gl.bind_vertex_array(static_cast<GLuint>(last_vertex_array));
    gl.bind_buffer(gl_array_buffer, static_cast<GLuint>(last_array_buffer));
    gl.bind_buffer(gl_element_array_buffer, static_cast<GLuint>(last_element_array_buffer));
    gl.bind_framebuffer(gl_draw_framebuffer, static_cast<GLuint>(last_draw_framebuffer));
    glDrawBuffer(static_cast<GLenum>(last_draw_buffer));
    gl.blend_equation_separate(static_cast<GLenum>(last_blend_equation_rgb),
        static_cast<GLenum>(last_blend_equation_alpha));
    gl.blend_func_separate(static_cast<GLenum>(last_blend_src_rgb),
        static_cast<GLenum>(last_blend_dst_rgb),
        static_cast<GLenum>(last_blend_src_alpha),
        static_cast<GLenum>(last_blend_dst_alpha));
    if (blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (cull_enabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (depth_enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (scissor_enabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT, static_cast<GLenum>(last_polygon_mode[0]));
    glPolygonMode(GL_BACK, static_cast<GLenum>(last_polygon_mode[1]));
    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1],
        last_scissor_box[2], last_scissor_box[3]);
    gl.active_texture(static_cast<GLenum>(last_active_texture));
}
