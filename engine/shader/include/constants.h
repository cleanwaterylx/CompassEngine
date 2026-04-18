#define m_max_point_light_count 15
#define m_max_point_light_geom_vertices 90 // 90 = 2 * 3 * m_max_point_light_count
#define m_mesh_per_drawcall_max_instance_count 64
#define m_mesh_vertex_blending_max_joint_count 1024
#define CHAOS_LAYOUT_MAJOR row_major
#define NEAR_PLANE 0.1f // ͶӰ����Ľ�ƽ��
#define FAR_PLANE 1000.0f // ͶӰ�����Զƽ�� 
#define ssao_kernel_size 64
#define ssao_radius 0.5f
#define ssao_bias 0.005f
#define ssao_strength 1.0f
#define ssao_power 1.0f
#define ssr_roughness_threshold 0.2f
#define ssr_max_steps 128
#define ssr_binary_search_steps 5
#define ssr_step_stride 0.1f
#define ssr_ray_bias 0.02f
#define ssr_thickness 0.12f
#define ssr_max_distance 100.0f
layout(CHAOS_LAYOUT_MAJOR) buffer;
layout(CHAOS_LAYOUT_MAJOR) uniform;
