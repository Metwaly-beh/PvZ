#include "Renderer.h"
#include "Assets.h"

#include <glad/glad.h>

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// The shader now supports both plain colors and textures.
// ---------------------------------------------------------------------------

namespace {

#ifdef __APPLE__
const char* const SHADER_VERSION = "#version 410 core\n";
#else
const char* const SHADER_VERSION = "#version 430 core\n";
#endif

const char* const VERTEX_BODY = R"(
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

uniform mat4 u_Projection;
uniform vec4 u_Tint;

out vec4 vColor;
out vec2 vTexCoord;

void main()
{
    gl_Position = u_Projection * vec4(aPos, 1.0);
    vColor = aColor * u_Tint;
    vTexCoord = aTexCoord;
}
)";

const char* const FRAGMENT_BODY = R"(
in vec4 vColor;
in vec2 vTexCoord;
uniform sampler2D u_Texture;
uniform bool u_UseTexture;

out vec4 FragColor;

void main()
{
    if (u_UseTexture) {
        FragColor = texture(u_Texture, vTexCoord) * vColor;
    } else {
        FragColor = vColor;
    }
}
)";

GLuint compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Scene shader compile error ("
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                  << "):\n" << infoLog << std::endl;
    }
    return shader;
}

} // namespace

Renderer::Renderer()
    : m_program(0), m_vao(0), m_vbo(0), m_uProjection(-1), m_uTint(-1), m_tint(1.0f, 1.0f, 1.0f, 1.0f)
{
    for (int i = 0; i < 16; ++i) m_projection[i] = 0.0f;
    m_projection[0] = m_projection[5] = m_projection[10] = m_projection[15] = 1.0f;
}

bool Renderer::init()
{
    if (m_program != 0) return true;

    std::string vertexSource   = std::string(SHADER_VERSION) + VERTEX_BODY;
    std::string fragmentSource = std::string(SHADER_VERSION) + FRAGMENT_BODY;

    GLuint vertexShader   = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);

    int success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::cerr << "Scene shader link error:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!success) return false;

    m_uProjection = glGetUniformLocation(m_program, "u_Projection");
    m_uTint       = glGetUniformLocation(m_program, "u_Tint");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Renderer::shutdown()
{
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_program != 0) glDeleteProgram(m_program);
    m_vbo = m_vao = m_program = 0;
}

void Renderer::setProjection(const float* matrix16)
{
    for (int i = 0; i < 16; ++i) m_projection[i] = matrix16[i];
}

void Renderer::setTint(Color c)
{
    m_tint = c;
}

void Renderer::bindTexture(unsigned int textureId)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

void Renderer::draw(const std::vector<Vertex>& vertices, BlendMode mode)
{
    if (vertices.empty() || m_program == 0) return;

    glUseProgram(m_program);
    glUniformMatrix4fv(m_uProjection, 1, GL_FALSE, m_projection);
    glUniform4f(m_uTint, m_tint.r, m_tint.g, m_tint.b, m_tint.a);

    if (mode == BlendMode::Additive) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STREAM_DRAW);

    // attribute layout:
    // 0 = vec3 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(0));
    
    // 1 = vec2 texCoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(3 * sizeof(float)));

    // 2 = vec4 colour
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void>(5 * sizeof(float)));

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
