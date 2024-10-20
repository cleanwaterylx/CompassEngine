#include "editor/include/light_cube.h"

namespace Compass
{
    EditorLightCube::EditorLightCube()
    {
        // create translation light cube render mesh
        float vertices[] = {
            // 位置              // 法线            // 切线            // UV 坐标
            // 后面
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 左下后
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下后
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 右上后
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 左上后

            // 前面
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 左下前
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下前
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 右上前
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 左上前

            // 左面
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // 左下后
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // 左上后
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // 左上前
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // 左下前

            // 右面
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 右下后
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // 右上后
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // 右上前
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // 右下前

            // 底面
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 左下后
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下后
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 右下前
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 左下前

            // 顶面
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 左上后
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右上后
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 右上前
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f   // 左上前
        };

        unsigned int indices[] =
            {
                // 后面
                0, 1, 2, 2, 3, 0,
                // 前面
                4, 5, 6, 6, 7, 4,
                // 左面
                8, 9, 10, 10, 11, 8,
                // 右面
                12, 13, 14, 14, 15, 12,
                // 底面
                16, 17, 18, 18, 19, 16,
                // 顶面
                20, 21, 22, 22, 23, 20};

        size_t vertex_data_size = sizeof(vertices);
        m_mesh_data.m_static_mesh_data.m_vertex_buffer = std::make_shared<BufferData>(vertex_data_size);

        size_t index_data_size = sizeof(indices);
        m_mesh_data.m_static_mesh_data.m_index_buffer = std::make_shared<BufferData>(index_data_size);

        memcpy(m_mesh_data.m_static_mesh_data.m_vertex_buffer->m_data, vertices, vertex_data_size);
        memcpy(m_mesh_data.m_static_mesh_data.m_index_buffer->m_data, indices, index_data_size);
    }
} // namespace Compass
