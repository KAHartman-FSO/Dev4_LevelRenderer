// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include "FSLogo.h"
#include <vector>
#include <chrono>
#include <string>
#include "level_data.h"
#ifdef _WIN32 // must use MT platform DLL libraries on windows
	#pragma comment(lib, "shaderc_combined.lib") 
#endif
// Creation, Rendering & Cleanup
class Renderer
{
	LevelData data;

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;

	// Device, Buffers and Memory
	VkDevice device = nullptr;
	//		vertex
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	//		index data
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	//		storage buffer data
	std::vector<VkBuffer> storageHandles;
	std::vector<VkDeviceMemory> storageDatas;

	// Struct containing Data for Storage Buffer
#define MAX_SUBMESH_PER_DRAW 1024
	struct SHADER_MODEL_DATA
	{
		GW::MATH::GVECTORF sunDirection, sunColor, sunAmbient, camPosition;
		GW::MATH::GMATRIXF view_matrix, projection_matrix;

		GW::MATH::GMATRIXF world_matrices[MAX_SUBMESH_PER_DRAW];	// World Matrix for Particular Sub-Mesh
		OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];						// Color / Texture of Surface for Particular Sub-Mesh
	};
	SHADER_MODEL_DATA SceneData;

	//	Shader Modules
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;

	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	
	// WVP matrices
	GW::MATH::GMatrix MatrixMath;
	GW::MATH::GMATRIXF world;
	GW::MATH::GMATRIXF view;
	GW::MATH::GMATRIXF projection;
	
	// Descriptors
	VkDescriptorSetLayout desc_set_layout = nullptr;	// "DESCRIPTORS ARE COMING!"
	VkDescriptorPool desc_pool = nullptr;					// How Vulkan Efficently Reserves Space for Descriptor Sets
	std::vector<VkDescriptorSet> descriptor_sets;

	unsigned int frameCount = 0;

	// Load a shader file as a string of characters.
	std::string ShaderAsString(const char* shaderFilePath) {
		std::string output;
		unsigned int stringLength = 0;
		GW::SYSTEM::GFile file; file.Create();
		file.GetFileSize(shaderFilePath, stringLength);
		if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
			output.resize(stringLength);
			file.Read(&output[0], stringLength);
		}
		return output;
	}
	std::string vertexShaderSource;
	std::string pixelShaderSource;

	// Uptate Camera Variables
	GW::INPUT::GInput keyboard_input;
	float FOV = 1.13446;
	float mouse_posX, mouse_posY;

public:
	void UpdateCamera()
	{
		static auto clock1 = std::chrono::steady_clock::now();
		auto clock2 = std::chrono::steady_clock::now();
		std::chrono::duration<float> time_bt_frame = clock2 - clock1;
		clock1 = clock2;

		GW::MATH::GMATRIXF temp_view_matrix;
		MatrixMath.IdentityF(temp_view_matrix);
		MatrixMath.InverseF(view, temp_view_matrix); // Store a copy of the inversed camera_wm

		// Adjust Matrix based on User Input
		const float camera_speed = 0.4f;
		const float sensitivity = 2.0f;
		GW::MATH::GVECTORF translationData;
		translationData.x = 0;
		translationData.y = 0;
		translationData.z = 0;
		translationData.w = 0;

		// Adjusting Up and Down of Camera
		float yChange = 0;
		float temp;
		keyboard_input.GetState(G_KEY_SPACE, temp); // UP
		yChange += temp;
		keyboard_input.GetState(G_KEY_LEFTSHIFT, temp); // DOWN
		yChange -= temp;
		yChange = time_bt_frame.count() * yChange * camera_speed;

		// Adjusting Forwards and Back of Camera
		float zChange = 0;
		keyboard_input.GetState(G_KEY_W, temp);
		zChange += temp;
		keyboard_input.GetState(G_KEY_S, temp);
		zChange -= temp;
		zChange = time_bt_frame.count() * zChange * camera_speed;

		// Adjusting Side to Side of Camera
		float xChange = 0;
		keyboard_input.GetState(G_KEY_D, temp);
		xChange += temp;
		keyboard_input.GetState(G_KEY_A, temp);
		xChange -= temp;
		xChange = time_bt_frame.count() * xChange * camera_speed;

		// Rotation!!
		float m2x, m2y = 0;
		keyboard_input.GetMousePosition(m2x, m2y);
		unsigned int height, width;

		//		Up and Down Rotation
		win.GetClientHeight(height);
		float total_pitch = FOV * (m2y - mouse_posY) / static_cast<float>(height);
		MatrixMath.RotateXLocalF(temp_view_matrix, total_pitch * sensitivity, temp_view_matrix);

		//		Left and Right Rotation
		win.GetClientWidth(width);
		float total_yaw = FOV * (m2x - mouse_posX) / static_cast<float>(width);
		MatrixMath.RotateYGlobalF(temp_view_matrix, total_yaw * sensitivity, temp_view_matrix);

		// Store Mouse Position for Next Pass Through
		keyboard_input.GetMousePosition(mouse_posX, mouse_posY);

#pragma region Arrow Keys Rotation
		// Rotation Test! -- With Arrow Keys
		float rotation_speed = 10.0f;

		// Up Down
		float UD_Rot = 0;
		keyboard_input.GetState(G_KEY_DOWN, temp);
		UD_Rot += temp;
		keyboard_input.GetState(G_KEY_UP, temp);
		UD_Rot -= temp;
		UD_Rot = time_bt_frame.count() * UD_Rot * camera_speed;
		MatrixMath.RotateXLocalF(temp_view_matrix, UD_Rot * rotation_speed, temp_view_matrix);

		// Left Right
		float LR_Rot = 0;
		keyboard_input.GetState(G_KEY_RIGHT, temp);
		LR_Rot += temp;
		keyboard_input.GetState(G_KEY_LEFT, temp);
		LR_Rot -= temp;
		LR_Rot = time_bt_frame.count() * LR_Rot * camera_speed;
		MatrixMath.RotateYGlobalF(temp_view_matrix, LR_Rot * rotation_speed, temp_view_matrix);
#pragma endregion

		// Translate x, y, z etc..
		translationData.y = yChange;
		MatrixMath.TranslateGlobalF(temp_view_matrix, translationData, temp_view_matrix); // ytranslation

		translationData.x = xChange;
		translationData.y = 0;
		translationData.z = zChange;
		MatrixMath.TranslateLocalF(temp_view_matrix, translationData, temp_view_matrix); // x and z translation	

		MatrixMath.InverseF(temp_view_matrix, view);
	}
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		// Setting Up Level Data
		data.SetLevel("../../Levels/GameLevel.txt");
		data.LoadLevel();
		
		// Load Shader Sources
		vertexShaderSource = ShaderAsString("../Shaders/VertexShader.hlsl");
		pixelShaderSource = ShaderAsString("../Shaders/PixelShader.hlsl");
	
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
	
		keyboard_input.Create(_win);
		keyboard_input.GetMousePosition(mouse_posX, mouse_posY);
		MatrixMath.Create();
		// ***** Creating Shader Data to Be Passed to Shaders ***** //
		// World Matrix
		MatrixMath.IdentityF(world);
		// View Matrix
		MatrixMath.IdentityF(view);
		GW::MATH::GVECTORF eye;
		eye.x = 0.75f; eye.y = 0.25f; eye.z = -1.5f;
		eye.w = 0;
		GW::MATH::GVECTORF at;
		at.x = 0.15f; at.y = 0.75f; at.z = 0.0f;
		at.w = 0;
		GW::MATH::GVECTORF up;
		up.x = 0; up.y = 1; up.z = 0;
		up.w = 0;
		MatrixMath.LookAtLHF(eye, at, up, view);
		// Projection Matrix
		MatrixMath.IdentityF(projection);
		float aspect_ratio;
		vlk.GetAspectRatio(aspect_ratio);
		MatrixMath.ProjectionVulkanLHF(1.13446, aspect_ratio, 0.1f, 100, projection);
		// Lighting
		GW::MATH::GVECTORF lightDir = { -1.0f, -1.0f, 2.0f, 0 };
		GW::MATH::GVECTORF lightColor = { 0.9f, 0.9f, 1.0f, 1.0f };
		GW::MATH::GVECTORF lightAmbient = { 0.25f, 0.25f, 0.35f, 1.0f };
		// Create a Structure Filled with all Shader Data needed
		SceneData.sunDirection = lightDir;
		SceneData.sunColor = lightColor;
		SceneData.sunAmbient = lightAmbient;
		SceneData.camPosition = eye;
		SceneData.view_matrix = view;
		SceneData.projection_matrix = projection;
		for (int i = 0; i < MAX_SUBMESH_PER_DRAW; i++)
		{
			MatrixMath.IdentityF(SceneData.world_matrices[i]);
		}
		SceneData.materials[0] = FSLogo_materials[0].attrib;
		SceneData.materials[1] = FSLogo_materials[1].attrib;
	
		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_vertices),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, FSLogo_vertices, sizeof(FSLogo_vertices));
		// Transfer FSLogo_Index Data to Index Buffer
		GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_indices),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexHandle, &indexData);
		GvkHelper::write_to_buffer(device, indexData, FSLogo_indices, sizeof(FSLogo_indices));
		// Transfer Shader_Data to StorageBuffer
		vlk.GetSwapchainImageCount(frameCount);
		storageDatas.resize(frameCount);
		storageHandles.resize(frameCount);
		descriptor_sets.resize(frameCount);
		for (int i = 0; i < frameCount; i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(SceneData),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &storageHandles[i], &storageDatas[i]);
			GvkHelper::write_to_buffer(device, storageDatas[i], &SceneData, sizeof(SceneData));
		}

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false); 
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), strlen(vertexShaderSource.c_str()),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource.c_str(), strlen(pixelShaderSource.c_str()),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done
		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
			stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			stage_create_info[0].module = vertexShader;
			stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
			stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stage_create_info[1].module = pixelShader;
			stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
			assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			assembly_create_info.primitiveRestartEnable = false;
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
			vertex_binding_description.binding = 0;
			vertex_binding_description.stride = sizeof(OBJ_VERT);
			vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[3] = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(OBJ_VEC3) },
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(OBJ_VEC3) * 2 }//uv, normal, etc....
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
			input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			input_vertex_info.vertexBindingDescriptionCount = 1;
			input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
			input_vertex_info.vertexAttributeDescriptionCount = 3;
			input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
			viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewport_create_info.viewportCount = 1;
			viewport_create_info.pViewports = &viewport;
			viewport_create_info.scissorCount = 1;
			viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
			rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
			rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
			rasterization_create_info.lineWidth = 1.0f;
			rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterization_create_info.depthClampEnable = VK_FALSE;
			rasterization_create_info.depthBiasEnable = VK_FALSE;
			rasterization_create_info.depthBiasClamp = 0.0f;
			rasterization_create_info.depthBiasConstantFactor = 0.0f;
			rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
			multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisample_create_info.sampleShadingEnable = VK_FALSE;
			multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisample_create_info.minSampleShading = 1.0f;
			multisample_create_info.pSampleMask = VK_NULL_HANDLE;
			multisample_create_info.alphaToCoverageEnable = VK_FALSE;
			multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
			depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depth_stencil_create_info.depthTestEnable = VK_TRUE;
			depth_stencil_create_info.depthWriteEnable = VK_TRUE;
			depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
			depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
			depth_stencil_create_info.minDepthBounds = 0.0f;
			depth_stencil_create_info.maxDepthBounds = 1.0f;
			depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
			color_blend_attachment_state.colorWriteMask = 0xF;
			color_blend_attachment_state.blendEnable = VK_FALSE;
			color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
			color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
			color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
			color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
			color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			color_blend_create_info.logicOpEnable = VK_FALSE;
			color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
			color_blend_create_info.attachmentCount = 1;
			color_blend_create_info.pAttachments = &color_blend_attachment_state;
			color_blend_create_info.blendConstants[0] = 0.0f;
			color_blend_create_info.blendConstants[1] = 0.0f;
			color_blend_create_info.blendConstants[2] = 0.0f;
			color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = { 
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
			dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_create_info.dynamicStateCount = 2;
			dynamic_create_info.pDynamicStates = dynamic_state;

		// Descriptor For Storage Buffer
		VkDescriptorSetLayoutBinding desc_layout_binding = {};
			desc_layout_binding.binding = 0;
			desc_layout_binding.descriptorCount = 1;
			desc_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			desc_layout_binding.pImmutableSamplers = nullptr;
			desc_layout_binding.stageFlags = VK_SHADER_STAGE_ALL;
		VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info = {};
			desc_set_layout_create_info.bindingCount = 1;
			desc_set_layout_create_info.flags = 0;
			desc_set_layout_create_info.pBindings = &desc_layout_binding;
			desc_set_layout_create_info.pNext = nullptr;
			desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		vkCreateDescriptorSetLayout(device, &desc_set_layout_create_info, nullptr, &desc_set_layout);
		VkDescriptorPoolSize desc_pool_size = {};
			desc_pool_size.descriptorCount = frameCount;
			desc_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		VkDescriptorPoolCreateInfo desc_pool_create_info = {};
			desc_pool_create_info.flags = NULL;
			desc_pool_create_info.maxSets = frameCount;
			desc_pool_create_info.pNext = nullptr;
			desc_pool_create_info.poolSizeCount = 1;
			desc_pool_create_info.pPoolSizes = &desc_pool_size;
			desc_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		vkCreateDescriptorPool(device, &desc_pool_create_info, nullptr, &desc_pool);
		VkDescriptorSetAllocateInfo desc_set_allocate_info = {};
			desc_set_allocate_info.descriptorPool = desc_pool;
			desc_set_allocate_info.descriptorSetCount = 1;
			desc_set_allocate_info.pNext = nullptr;
			desc_set_allocate_info.pSetLayouts = &desc_set_layout;
			desc_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		for (int i = 0; i < frameCount; i++)
		{
			vkAllocateDescriptorSets(device, &desc_set_allocate_info, &descriptor_sets[i]);
		}
		VkDescriptorBufferInfo desc_buffer_info = {};
			desc_buffer_info.buffer = storageHandles[0];
			desc_buffer_info.offset = 0;
			desc_buffer_info.range = VK_WHOLE_SIZE;
		VkWriteDescriptorSet write_desc_set = {};
			write_desc_set.descriptorCount = 1;
			write_desc_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write_desc_set.dstArrayElement = 0;
			write_desc_set.dstBinding = 0;
			write_desc_set.dstSet = descriptor_sets[0];
			write_desc_set.pBufferInfo = &desc_buffer_info;
			write_desc_set.pImageInfo = nullptr;
			write_desc_set.pNext = nullptr;
			write_desc_set.pTexelBufferView = nullptr;
			write_desc_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		for (int i = 0; i < frameCount; i++)
		{
			desc_buffer_info.buffer = storageHandles[i];
			write_desc_set.dstSet = descriptor_sets[i];
			vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
		}
		// Push Constant
		VkPushConstantRange push_constant_range = {};
			push_constant_range.offset = 0;
			push_constant_range.size = sizeof(unsigned int);
			push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
			pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeline_layout_create_info.setLayoutCount = 1;
			pipeline_layout_create_info.pSetLayouts = &desc_set_layout;
			pipeline_layout_create_info.pushConstantRangeCount = 1;
			pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
		vkCreatePipelineLayout(device, &pipeline_layout_create_info, 
			nullptr, &pipelineLayout);
	    // Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
			pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_create_info.stageCount = 2;
			pipeline_create_info.pStages = stage_create_info;
			pipeline_create_info.pInputAssemblyState = &assembly_create_info;
			pipeline_create_info.pVertexInputState = &input_vertex_info;
			pipeline_create_info.pViewportState = &viewport_create_info;
			pipeline_create_info.pRasterizationState = &rasterization_create_info;
			pipeline_create_info.pMultisampleState = &multisample_create_info;
			pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
			pipeline_create_info.pColorBlendState = &color_blend_create_info;
			pipeline_create_info.pDynamicState = &dynamic_create_info;
			pipeline_create_info.layout = pipelineLayout;
			pipeline_create_info.renderPass = renderPass;
			pipeline_create_info.subpass = 0;
			pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, 
			&pipeline_create_info, nullptr, &pipeline);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
		});
	}
	void Render()
	{
		// Rotate World Matrix around Y axis over time
		static auto clock1 = std::chrono::steady_clock::now();
		auto clock2 = std::chrono::steady_clock::now();
		std::chrono::duration<float> time_bt_frame = clock2 - clock1;
		clock1 = clock2;
		float rotation_speed = 0.3f; // per second
		float total_rotation = rotation_speed * time_bt_frame.count();
		MatrixMath.RotateYGlobalF(world, total_rotation, world);
		SceneData.world_matrices[1] = world;
		SceneData.view_matrix = view;
		for (int i = 0; i < frameCount; i++)
		{
			GvkHelper::write_to_buffer(device, storageDatas[i], &SceneData, sizeof(SceneData));
		}

		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// setup the pipeline's dynamic settings
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexHandle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			pipelineLayout, 0, 1, &descriptor_sets[currentBuffer], 0, nullptr);

		for (unsigned int i = 0; i < FSLogo_meshcount; i++)
		{
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(unsigned int),
				&FSLogo_meshes[i].materialIndex);
			vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, FSLogo_meshes[i].indexOffset, 0, 0);
		}
	}
	
private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline

		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);

		vkDestroyBuffer(device, indexHandle, nullptr);
		vkFreeMemory(device, indexData, nullptr);

		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyDescriptorSetLayout(device, desc_set_layout, nullptr);
		vkDestroyDescriptorPool(device, desc_pool, nullptr);

		for (int i = 0; i < storageDatas.size(); i++)
		{
			vkDestroyBuffer(device, storageHandles[i], nullptr);
			vkFreeMemory(device, storageDatas[i], nullptr);
		}
	}
};
