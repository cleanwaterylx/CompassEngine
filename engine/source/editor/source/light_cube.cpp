#include "editor/include/light_cube.h"

namespace Compass
{
    EditorLightCube::EditorLightCube()
    {
        // create translation light cube render mesh
        float vertices[] = {
            // λ��              // ����            // ����            // UV ����
            // ����
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // ���º�
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // ���º�
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // ���Ϻ�
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // ���Ϻ�

            // ǰ��
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // ����ǰ
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // ����ǰ
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // ����ǰ
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // ����ǰ

            // ����
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // ���º�
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // ���Ϻ�
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // ����ǰ
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // ����ǰ

            // ����
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // ���º�
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // ���Ϻ�
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // ����ǰ
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // ����ǰ

            // ����
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // ���º�
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // ���º�
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // ����ǰ
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // ����ǰ

            // ����
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // ���Ϻ�
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // ���Ϻ�
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // ����ǰ
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f   // ����ǰ
        };

        unsigned int indices[] =
            {
                // ����
                0, 1, 2, 2, 3, 0,
                // ǰ��
                4, 5, 6, 6, 7, 4,
                // ����
                8, 9, 10, 10, 11, 8,
                // ����
                12, 13, 14, 14, 15, 12,
                // ����
                16, 17, 18, 18, 19, 16,
                // ����
                20, 21, 22, 22, 23, 20};

        size_t vertex_data_size = sizeof(vertices);
        m_mesh_data.m_static_mesh_data.m_vertex_buffer = std::make_shared<BufferData>(vertex_data_size);

        size_t index_data_size = sizeof(indices);
        m_mesh_data.m_static_mesh_data.m_index_buffer = std::make_shared<BufferData>(index_data_size);

        memcpy(m_mesh_data.m_static_mesh_data.m_vertex_buffer->m_data, vertices, vertex_data_size);
        memcpy(m_mesh_data.m_static_mesh_data.m_index_buffer->m_data, indices, index_data_size);
    }
} // namespace Compass
