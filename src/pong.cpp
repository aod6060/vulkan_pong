#include "pong.h"


namespace pong {
	struct UniformCamera {
		glm::mat4 proj;
		glm::mat4 view;
	};

	struct UniformModel {
		glm::mat4 model;
	};

	struct Paddle {
		glm::vec2 position;
		glm::vec2 size;
		glm::vec2 velocity;
		float speed = 32.0f;
	};

	struct Ball {
		glm::vec2 position;
		glm::vec2 size;
		glm::vec2 velocity;
		glm::vec2 speed;
	};

	vk::Vulkan vulkan;
	// Buffer Section
	std::vector<glm::vec3> verticesList;
	VkBuffer stageVerticesBuffer;
	VkBuffer verticesBuffer;
	std::vector<std::uint32_t> indexList;
	VkBuffer stageIndexBuffer;
	VkBuffer indexBuffer;
	VkDeviceSize verticesOffset;
	VkDeviceSize verticesSize;
	VkDeviceSize indexOffset;
	VkDeviceSize indexSize;
	VkDeviceSize stageMemory;
	VkDeviceMemory bufferMemory;

	// Uniforms
	// Uniform Camera
	UniformCamera camera;
	VkBuffer stageCameraBuffer;
	VkBuffer cameraBuffer;
	VkDeviceMemory stageCameraMemory;
	VkDeviceMemory cameraMemory;

	// Uniform Model
	std::vector<UniformModel> models;
	std::vector<VkBuffer> stageModelsBuffer;
	std::vector<VkBuffer> modelsBuffer;
	std::vector<VkDeviceSize> modelOffsets;
	std::vector<VkDeviceSize> modelSize;
	VkDeviceMemory stageModelMemory;
	VkDeviceMemory modelMemory;

	// Graphics Pipeline
	const uint32_t numUniformModel = 3;

	VkDescriptorSetLayout cameraSetLayout;
	VkDescriptorSetLayout modelSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	//VkDescriptorPool descriptorPool;
	VkDescriptorPool cameraSetPool;
	VkDescriptorSet cameraSet;
	VkDescriptorPool modelSetPool;
	std::vector<VkDescriptorSet> modelSet;

	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentFrame = 0;
	uint32_t nextImage = 0;

	Paddle player;
	Paddle aiPlayer;
	Ball ball;

	std::mt19937 mrand;

	AiPlayerType aiPlayerType;

	void initBuffers();
	void initPipelineLayout();
	void initGraphicsPipeline();
	void initDescriptorPool();
	void initDescriptorSets();
	void initCommandBuffer();

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void toRect(util::Rect& r, Paddle& pad);
	void toRect(util::Rect& r, Ball& ball);

	void updatePaddlePlayer(Paddle& pad, float delta);

	void updatePaddleImpossible(Paddle& pad, float delta);
	void updatePaddleExpert(Paddle& pad, float delta);
	void updatePaddleHard(Paddle& pad, float delta);
	void updatePaddleNormal(Paddle& pad, float delta);
	void updatePaddleEasy(Paddle& pad, float delta);

	std::map<AiPlayerType, std::function<void(Paddle&, float)>> aiUpdates = {
		{AiPlayerType::AI_PLAYER_EASY, updatePaddleEasy},
		{AiPlayerType::AI_PLAYER_NORMAL, updatePaddleNormal},
		{AiPlayerType::AI_PLAYER_HARD, updatePaddleHard},
		{AiPlayerType::AI_PLAYER_EXPERT, updatePaddleExpert},
		{AiPlayerType::AI_PLAYER_IMPOSSIBLE, updatePaddleImpossible}
	};

	void resetBall(Ball& b);
	void updateBall(Ball& b, float delta);

	void init() {
		mrand = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());

		input::init();

		input::createInputMapping("move-up", input::createInputMapKey(input::Keys::KEY_UP));
		input::createInputMapping("move-down", input::createInputMapKey(input::Keys::KEY_DOWN));

		audio::init();
		audio::createSoundFX("ball-hit", "data/ball_hit.wav");
		//audio::createSoundFX("player-score", "data/player_score.wav");
		//audio::createSoundFX("ai-player-score", "data/ai_player_score.wav");
		audio::createSoundFX("spawn-ball", "data/spawn_ball.wav");

		vk::initVulkan(vulkan);

		// Buffers
		initBuffers();

		// Graphics Pipeline
		initPipelineLayout();

		initGraphicsPipeline();

		initDescriptorPool();

		initDescriptorSets();

		initCommandBuffer();

		camera.proj = glm::ortho(0.0f, (float)vulkan.swapchainExtent.width, 0.0f, (float)vulkan.swapchainExtent.height);
		camera.view = glm::mat4(1.0f);


		player.size = glm::vec2(8.0f, 64.0f);
		player.position = glm::vec2(4.0f, (vulkan.swapchainExtent.height * 0.5f - player.size.y * 0.5f));
		player.velocity = glm::vec2(0.0f);
		player.speed = 128.0f;

		aiPlayer.size = glm::vec2(8.0f, 64.0f);
		aiPlayer.position = glm::vec2(vulkan.swapchainExtent.width - aiPlayer.size.x - 4.0f, (vulkan.swapchainExtent.height * 0.5f - player.size.y * 0.5f));
		aiPlayer.velocity = glm::vec2(0.0f);
		aiPlayer.speed = 128.0f;

		ball.size = glm::vec2(16.0f);
		resetBall(ball);
	}

	void doEvent(SDL_Event& e) {
		input::doEvent(e);
	}

	void update(float delta) {
		updatePaddlePlayer(player, delta);
		aiUpdates[aiPlayerType](aiPlayer, delta);
		updateBall(ball, delta);
	}

	void updateUniforms() {

		// Update Camera
		void* data;

		vkMapMemory(vulkan.device, stageCameraMemory, 0, sizeof(UniformCamera), 0, &data);
		memcpy(data, &camera, sizeof(UniformCamera));
		vkUnmapMemory(vulkan.device, stageCameraMemory);

		copyBuffer(stageCameraBuffer, cameraBuffer, sizeof(UniformCamera));

		// Update Models
		for (size_t i = 0; i < models.size(); i++) {
			vkMapMemory(vulkan.device, stageModelMemory, modelOffsets[i], modelSize[i], 0, &data);
			memcpy(data, &models[i], modelSize[i]);
			vkUnmapMemory(vulkan.device, stageModelMemory);

			copyBuffer(stageModelsBuffer[i], modelsBuffer[i], modelSize[i]);
		}
	}

	void render() {
		vkWaitForFences(vulkan.device, 1, &vulkan.inFlight[currentFrame], VK_TRUE, UINT64_MAX);

		vkAcquireNextImageKHR(vulkan.device, vulkan.swapchain, UINT64_MAX, vulkan.submitCB[currentFrame], VK_NULL_HANDLE, &nextImage);


		updateUniforms();

		if (vulkan.imageInFlight[nextImage] != VK_NULL_HANDLE) {
			vkWaitForFences(vulkan.device, 1, &vulkan.imageInFlight[nextImage], VK_TRUE, UINT64_MAX);
		}

		vulkan.imageInFlight[nextImage] = vulkan.inFlight[nextImage];

		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &vulkan.submitCB[currentFrame];
		submitInfo.pWaitDstStageMask = &waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[nextImage];
		
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &vulkan.submitPresentQueue[currentFrame];

		vkResetFences(vulkan.device, 1, &vulkan.inFlight[currentFrame]);

		if (vkQueueSubmit(vulkan.graphicsQueue, 1, &submitInfo, vulkan.inFlight[currentFrame]) != VK_SUCCESS) {
			std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &vulkan.submitPresentQueue[currentFrame];

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &vulkan.swapchain;

		presentInfo.pImageIndices = &nextImage;

		vkQueuePresentKHR(vulkan.presentQueue, &presentInfo);

		currentFrame = (currentFrame + 1) % vulkan.swapchainImages.size();
	}

	void postUpdate() {
		input::update();
	}

	void release() {
		vkWaitForFences(vulkan.device, 1, &vulkan.inFlight[currentFrame], VK_TRUE, UINT64_MAX);
		if (vulkan.imageInFlight[nextImage] != VK_NULL_HANDLE) {
			vkWaitForFences(vulkan.device, 1, &vulkan.imageInFlight[nextImage], VK_TRUE, UINT64_MAX);
		}

		vulkan.imageInFlight[0] = VK_NULL_HANDLE;
		vulkan.imageInFlight[1] = VK_NULL_HANDLE;

		vkFreeCommandBuffers(vulkan.device, vulkan.commandPool, commandBuffers.size(), commandBuffers.data());
		if (modelSetPool) {
			vkDestroyDescriptorPool(vulkan.device, modelSetPool, nullptr);
		}

		if (cameraSetPool) {
			vkDestroyDescriptorPool(vulkan.device, cameraSetPool, nullptr);
		}

		if (graphicsPipeline) {
			vkDestroyPipeline(vulkan.device, graphicsPipeline, nullptr);
		}

		if (pipelineLayout) {
			vkDestroyPipelineLayout(vulkan.device, pipelineLayout, nullptr);
		}

		if (modelSetLayout) {
			vkDestroyDescriptorSetLayout(vulkan.device, modelSetLayout, nullptr);
		}

		if (cameraSetLayout) {
			vkDestroyDescriptorSetLayout(vulkan.device, cameraSetLayout, nullptr);
		}

		// UniformModel
		if (modelMemory) {
			vkFreeMemory(vulkan.device, modelMemory, nullptr);
		}
		

		for (size_t j = 0; j < modelsBuffer.size(); j++) {
			std::cout << j << std::endl;
			if (modelsBuffer[j] != VK_NULL_HANDLE) {
				vkDestroyBuffer(vulkan.device, modelsBuffer[j], nullptr);
			}
		}

		if (stageModelMemory) {
			vkFreeMemory(vulkan.device, stageModelMemory, nullptr);
		}

		for (size_t j = 0; j < stageModelsBuffer.size(); j++) {
			if (stageModelsBuffer[j] != VK_NULL_HANDLE) {
				vkDestroyBuffer(vulkan.device, stageModelsBuffer[j], nullptr);
			}
		}

		models.clear();
		stageModelsBuffer.clear();
		modelsBuffer.clear();
		modelOffsets.clear();
		modelSize.clear();

		// UniformCamera
		if (cameraMemory) {
			vkFreeMemory(vulkan.device, cameraMemory, nullptr);
		}

		if (cameraBuffer) {
			vkDestroyBuffer(vulkan.device, cameraBuffer, nullptr);
		}

		if (stageCameraMemory) {
			vkFreeMemory(vulkan.device, stageCameraMemory, nullptr);
		}

		if (stageCameraBuffer) {
			vkDestroyBuffer(vulkan.device, stageCameraBuffer, nullptr);
		}

		// Buffer
		if (bufferMemory) {
			vkFreeMemory(vulkan.device, bufferMemory, nullptr);
		}

		if (indexBuffer) {
			vkDestroyBuffer(vulkan.device, indexBuffer, nullptr);
		}

		if (verticesBuffer) {
			vkDestroyBuffer(vulkan.device, verticesBuffer, nullptr);
		}

		if (stageMemory) {
			vkFreeMemory(vulkan.device, stageMemory, nullptr);
		}

		if (stageIndexBuffer) {
			vkDestroyBuffer(vulkan.device, stageIndexBuffer, nullptr);
		}

		if (stageVerticesBuffer) {
			vkDestroyBuffer(vulkan.device, stageVerticesBuffer, nullptr);
		}

		vk::releaseVulkan(vulkan);

		audio::release();

		input::clearInputMaps();
	}

	void setup(app::Config* conf, AiPlayerType type) {

		std::stringstream cap;

		cap << "Vulkan Pong: ";

		switch (type) {
		case AiPlayerType::AI_PLAYER_EASY:
			cap << "(Easy)";
			break;
		case AiPlayerType::AI_PLAYER_NORMAL:
			cap << "(Normal)";
			break;
		case AiPlayerType::AI_PLAYER_HARD:
			cap << "(Hard)";
			break;
		case AiPlayerType::AI_PLAYER_EXPERT:
			cap << "(Expert)";
			break;
		case AiPlayerType::AI_PLAYER_IMPOSSIBLE:
			cap << "(Impossible!!! Are you crazy??? You can't win... :( )";
			break;
		}

		conf->caption = cap.str();
		conf->width = 640;
		conf->height = 480;

		conf->initCB = init;
		conf->doEventCB = doEvent;
		conf->updateCB = update;
		conf->renderCB = render;
		conf->postUpdate = postUpdate;
		conf->releaseCB = release;

		aiPlayerType = type;
	}

	void initBuffers() {
		{
			// Buffer
			// Vertices
			verticesList.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
			verticesList.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
			verticesList.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
			verticesList.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
			// Index
			indexList = {
				0, 1, 2,
				2, 1, 3
			};
			VkBufferCreateInfo vCI = {};
			vCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vCI.size = verticesList.size() * sizeof(glm::vec3);
			vCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			vCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult res = vkCreateBuffer(vulkan.device, &vCI, nullptr, &stageVerticesBuffer);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create vertex buffer");
			}
			else {
				std::cout << "Success: Created Vertex Buffer" << std::endl;
			}

			vCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			res = vkCreateBuffer(vulkan.device, &vCI, nullptr, &verticesBuffer);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create vertex buffer");
			}
			else {
				std::cout << "Success: Created Vertex Buffer" << std::endl;
			}

			VkMemoryRequirements vMemReq;
			vkGetBufferMemoryRequirements(vulkan.device, stageVerticesBuffer, &vMemReq);

			VkBufferCreateInfo iCI = {};
			iCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			iCI.size = indexList.size() * sizeof(uint32_t);
			iCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			iCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			res = vkCreateBuffer(vulkan.device, &iCI, nullptr, &stageIndexBuffer);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create index buffer");
			}
			else {
				std::cout << "Success: Created Index Buffer" << std::endl;
			}

			iCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			res = vkCreateBuffer(vulkan.device, &iCI, nullptr, &indexBuffer);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create index buffer");
			}
			else {
				std::cout << "Success: Created Index Buffer" << std::endl;
			}

			VkMemoryRequirements iMemReq;
			vkGetBufferMemoryRequirements(vulkan.device, stageIndexBuffer, &iMemReq);

			VkMemoryPropertyFlags p =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memProps);

			std::optional<size_t> index;

			for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
				if ((memProps.memoryTypes[i].propertyFlags & p) == p) {
					index = i;
				}
			}

			if (!index.has_value()) {
				throw std::runtime_error("failed to find memory type");
			}

			verticesOffset = 0;
			verticesSize = vMemReq.size;
			
			indexOffset = vMemReq.size;
			if (indexOffset % iMemReq.alignment != 0) {
				indexOffset += indexOffset % iMemReq.alignment;
			}
			indexSize = iMemReq.size;

			VkDeviceSize maxSize = indexOffset + indexSize;

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = maxSize;
			allocInfo.memoryTypeIndex = index.value();

			res = vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &stageMemory);


			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate bufferMemory");
			}
			else {
				std::cout << "Success: Allocate BufferMemory" << std::endl;
			}


			vkBindBufferMemory(vulkan.device, stageVerticesBuffer, stageMemory, verticesOffset);
			vkBindBufferMemory(vulkan.device, stageIndexBuffer, stageMemory, indexOffset);

			void* data = nullptr;
			vkMapMemory(vulkan.device, stageMemory, verticesOffset, verticesSize, 0, &data);
			memcpy(data, verticesList.data(), (uint32_t)verticesSize);
			vkUnmapMemory(vulkan.device, stageMemory);

			vkMapMemory(vulkan.device, stageMemory, indexOffset, indexSize, 0, &data);
			memcpy(data, indexList.data(), (uint32_t)indexSize);
			vkUnmapMemory(vulkan.device, stageMemory);




			// Create Device Local Memory
			p = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
				if ((memProps.memoryTypes[i].propertyFlags & p) == p) {
					index = i;
				}
			}

			if (!index.has_value()) {
				throw std::runtime_error("failed to find memory type");
			}

			/*
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = maxSize;
			*/
			allocInfo.memoryTypeIndex = index.value();

			res = vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &bufferMemory);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate bufferMemory");
			}
			else {
				std::cout << "Success: Allocate BufferMemory" << std::endl;
			}

			vkBindBufferMemory(vulkan.device, verticesBuffer, bufferMemory, verticesOffset);
			vkBindBufferMemory(vulkan.device, indexBuffer, bufferMemory, indexOffset);

			// Copy Vertex Buffer
			copyBuffer(stageVerticesBuffer, verticesBuffer, verticesSize);

			// Copy Index Buffer
			copyBuffer(stageIndexBuffer, indexBuffer, indexSize);

		}

		// Uniform Camera

		{
			VkBufferCreateInfo ucCI = {};
			ucCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			ucCI.size = sizeof(UniformCamera);
			ucCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			ucCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult res = vkCreateBuffer(vulkan.device, &ucCI, nullptr, &stageCameraBuffer);

			if (res != VK_SUCCESS) {
				std::runtime_error("failed to create cameraBuffer");
			}
			else {
				std::cout << "Success: Create CameraBuffer" << std::endl;
			}

			ucCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			res = vkCreateBuffer(vulkan.device, &ucCI, nullptr, &cameraBuffer);

			if (res != VK_SUCCESS) {
				std::runtime_error("failed to create cameraBuffer");
			}
			else {
				std::cout << "Success: Create CameraBuffer" << std::endl;
			}

			VkMemoryRequirements ucMemReq;
			vkGetBufferMemoryRequirements(vulkan.device, cameraBuffer, &ucMemReq);

			VkMemoryPropertyFlags p =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memProps);

			std::optional<size_t> index;

			for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
				if ((memProps.memoryTypes[i].propertyFlags & p) == p) {
					index = i;
				}
			}

			if (!index.has_value()) {
				throw std::runtime_error("failed to find memory type");
			}

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = ucMemReq.size;
			allocInfo.memoryTypeIndex = index.value();

			res = vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &stageCameraMemory);

			if (res != VK_SUCCESS) {
				std::runtime_error("failed to allocate cameraMemory");
			}
			else {
				std::cout << "Success: Allocate CameraMemory" << std::endl;
			}

			vkBindBufferMemory(vulkan.device, stageCameraBuffer, stageCameraMemory, 0);

			p = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
				if ((memProps.memoryTypes[i].propertyFlags & p) == p) {
					index = i;
				}
			}

			if (!index.has_value()) {
				throw std::runtime_error("failed to find memory type");
			}

			allocInfo.memoryTypeIndex = index.value();

			res = vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &cameraMemory);

			if (res != VK_SUCCESS) {
				std::runtime_error("failed to allocate cameraMemory");
			}
			else {
				std::cout << "Success: Allocate CameraMemory" << std::endl;
			}

			vkBindBufferMemory(vulkan.device, cameraBuffer, cameraMemory, 0);


		}
		// Uniform Model
		{
			models.resize(numUniformModel);
			stageModelsBuffer.resize(numUniformModel);
			modelsBuffer.resize(numUniformModel);
			modelOffsets.resize(numUniformModel);
			modelSize.resize(numUniformModel);

			std::vector<VkMemoryRequirements> reqs(numUniformModel);

			// Create Staging Buffers
			for (size_t i = 0; i < stageModelsBuffer.size(); i++) {
				VkBufferCreateInfo CI = {};
				CI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				CI.size = sizeof(UniformModel);
				CI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				CI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkResult res = vkCreateBuffer(vulkan.device, &CI, nullptr, &stageModelsBuffer[i]);

				if (res != VK_SUCCESS) {
					throw std::runtime_error("failed to modelBuffer");
				}
				else {
					std::cout << "Success: Create modelBuffer["<< i <<"]" << std::endl;
				}
			}

			// Creating Buffers
			for (size_t i = 0; i < modelsBuffer.size(); i++) {
				VkBufferCreateInfo CI = {};
				CI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				CI.size = sizeof(UniformModel);
				CI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				CI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkResult res = vkCreateBuffer(vulkan.device, &CI, nullptr, &modelsBuffer[i]);

				if (res != VK_SUCCESS) {
					throw std::runtime_error("failed to modelBuffer");
				}
				else {
					std::cout << "Success: Create modelBuffer[" << i << "]" << std::endl;
				}
			}
			// Grab memory requirements
			for (size_t i = 0; i < modelsBuffer.size(); i++) {
				vkGetBufferMemoryRequirements(vulkan.device, modelsBuffer[i], &reqs[i]);
			}

			VkDeviceSize maxSize = 0;

			modelOffsets[0] = 0;
			modelSize[0] = reqs[0].size;

			std::cout << modelOffsets[0] << ": " << modelSize[0] << std::endl;
			for (size_t i = 1; i < reqs.size(); i++) {
				modelOffsets[i] = modelOffsets[i - 1] + modelSize[i - 1];
				modelSize[i] = reqs[i].size;

				std::cout << modelOffsets[i] << ": " << modelSize[i] << std::endl;
			}

			maxSize = modelOffsets[modelOffsets.size() - 1] + modelSize[modelSize.size() - 1];

			VkMemoryPropertyFlags p =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memProps);

			std::optional<size_t> index;

			for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
				if ((memProps.memoryTypes[i].propertyFlags & p) == p) {
					index = i;
				}
			}

			if (!index.has_value()) {
				throw std::runtime_error("failed to find memory type");
			}

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = maxSize;
			allocInfo.memoryTypeIndex = index.value();

			VkResult res = vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &stageModelMemory);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate modelMemory");
			}
			else {
				std::cout << "Success: allocate modelMemory" << std::endl;
			}

			p = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			for (size_t i = 0; i < modelsBuffer.size(); i++) {
				vkBindBufferMemory(vulkan.device, stageModelsBuffer[i], stageModelMemory, modelOffsets[i]);
			}

			for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
				if ((memProps.memoryTypes[i].propertyFlags & p) == p) {
					index = i;
				}
			}

			if (!index.has_value()) {
				throw std::runtime_error("failed to find memory type");
			}


			allocInfo.memoryTypeIndex = index.value();

			res = vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &modelMemory);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate modelMemory");
			}
			else {
				std::cout << "Success: allocate modelMemory" << std::endl;
			}

			for (size_t i = 0; i < modelsBuffer.size(); i++) {
				vkBindBufferMemory(vulkan.device, modelsBuffer[i], modelMemory, modelOffsets[i]);
			}

		}
	}

	void initPipelineLayout() {
		{
			VkDescriptorSetLayoutBinding uboCameraBinding = {};
			uboCameraBinding.binding = 0;
			uboCameraBinding.descriptorCount = 1;
			uboCameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboCameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = &uboCameraBinding;
			
			VkResult res = vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, nullptr, &cameraSetLayout);

			//VkResult res = vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, nullptr, &descSetLayout);
			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor set layout");
			}
			else {
				std::cout << "Success: Created Descriptor Set Layout" << std::endl;
			}
		}
		{
			VkDescriptorSetLayoutBinding uboModelBinding = {};
			uboModelBinding.binding = 1;
			uboModelBinding.descriptorCount = numUniformModel;
			uboModelBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboModelBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = &uboModelBinding;

			VkResult res = vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, nullptr, &modelSetLayout);

			//VkResult res = vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, nullptr, &descSetLayout);
			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor set layout");
			}
			else {
				std::cout << "Success: Created Descriptor Set Layout" << std::endl;
			}
		}

		std::vector<VkDescriptorSetLayout> descSetLayouts = {
			cameraSetLayout,
			modelSetLayout
		};

		VkPipelineLayoutCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		CI.setLayoutCount = descSetLayouts.size();
		CI.pSetLayouts = descSetLayouts.data();
		
		VkResult res = vkCreatePipelineLayout(vulkan.device, &CI, nullptr, &pipelineLayout);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout");
		}
		else {
			std::cout << "Success: Created Pipeline Layout" << std::endl;
		}
	}

	void initGraphicsPipeline() {
		// Vertex Shader
		std::vector<char> vertexShaderData;
		util::loadBlob("data/main.vert.spv", vertexShaderData);
		VkShaderModule vertexShaderMod = vk::createShaderModule(vulkan, vertexShaderData);

		// Fragment Shader
		std::vector<char> fragShaderData;
		util::loadBlob("data/main.frag.spv", fragShaderData);
		VkShaderModule fragShaderMod = vk::createShaderModule(vulkan, fragShaderData);

		// Vertex PipelineStage
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderMod;
		vertShaderStageInfo.pName = "main";

		// Fragment PipelineStage
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderMod;
		fragShaderStageInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
			vertShaderStageInfo,
			fragShaderStageInfo
		};

		VkVertexInputBindingDescription bindDesc = {};
		bindDesc.binding = 0;
		bindDesc.stride = sizeof(glm::vec3);
		bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attrDesc = {};
		attrDesc.binding = 0;
		attrDesc.location = 0;
		attrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc.offset = 0;

		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &bindDesc;
		vertexInput.vertexAttributeDescriptionCount = 1;
		vertexInput.pVertexAttributeDescriptions = &attrDesc;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)vulkan.swapchainExtent.width;
		viewport.height = (float)vulkan.swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = vulkan.swapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		//  Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthBiasClamp = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.lineWidth = 1.0f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo ms = {};
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.sampleShadingEnable = VK_FALSE;
		ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		ms.minSampleShading = 1.0f;

		// Blend
		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		blendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo blend = {};
		blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend.logicOpEnable = VK_FALSE;
		blend.logicOp = VK_LOGIC_OP_COPY;
		blend.attachmentCount = 1;
		blend.pAttachments = &blendAttachment;


		// Graphics Pipeline
		VkGraphicsPipelineCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		CI.stageCount = shaderStages.size();
		CI.pStages = shaderStages.data();
		CI.pVertexInputState = &vertexInput;
		CI.pInputAssemblyState = &inputAssembly;
		CI.pViewportState = &viewportState;
		CI.pRasterizationState = &rasterizer;
		CI.pMultisampleState = &ms;
		CI.pDepthStencilState = nullptr;
		CI.pColorBlendState = &blend;
		CI.pDynamicState = nullptr;
		CI.layout = pipelineLayout;
		CI.renderPass = vulkan.renderPass;
		CI.subpass = 0;
		CI.basePipelineHandle = VK_NULL_HANDLE;
		CI.basePipelineIndex = -1;

		VkResult res = vkCreateGraphicsPipelines(
			vulkan.device,
			VK_NULL_HANDLE,
			1,
			&CI,
			nullptr,
			&graphicsPipeline);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline");
		}
		else {
			std::cout << "Success: Create Graphics Pipeline" << std::endl;
		}

		vkDestroyShaderModule(vulkan.device, vertexShaderMod, nullptr);
		vkDestroyShaderModule(vulkan.device, fragShaderMod, nullptr);
	}

	void initDescriptorPool() {
		{
			// Camera Set Pool
			VkDescriptorPoolSize uboSize = {};
			uboSize.descriptorCount = 1;
			uboSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;


			VkDescriptorPoolCreateInfo CI = {};
			CI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			CI.poolSizeCount = 1;
			CI.pPoolSizes = &uboSize;
			CI.maxSets = 1;
			
			VkResult res = vkCreateDescriptorPool(vulkan.device, &CI, nullptr, &cameraSetPool);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor pool");
			}
			else {
				std::cout << "Success: Create Descriptor Pool" << std::endl;
			}
		}
		{
			// Model Set Pool
			VkDescriptorPoolSize uboSize = {};
			uboSize.descriptorCount = sizeof(UniformModel) * numUniformModel;
			uboSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VkDescriptorPoolCreateInfo CI = {};
			CI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			CI.poolSizeCount = 1;
			CI.pPoolSizes = &uboSize;
			CI.maxSets = numUniformModel;

			VkResult res = vkCreateDescriptorPool(vulkan.device, &CI, nullptr, &modelSetPool);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor pool");
			}
			else {
				std::cout << "Success: Create Descriptor Pool" << std::endl;
			}
		}
	}

	void initDescriptorSets() {
		// Camera
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = cameraSetPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &cameraSetLayout;
			
			VkResult res = vkAllocateDescriptorSets(vulkan.device, &allocInfo, &cameraSet);


			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor sets");
			}
			else {
				std::cout << "Success: Allocated Camera Descriptor Set" << std::endl;
			}

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = cameraBuffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformCamera);

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = cameraSet;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			
			vkUpdateDescriptorSets(vulkan.device, 1, &descriptorWrite, 0, nullptr);

		}

		// Models
		{
			std::vector<VkDescriptorSetLayout> layouts(numUniformModel, modelSetLayout);

			std::cout << layouts.size() << std::endl;

			modelSet.resize(numUniformModel);

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = modelSetPool;
			allocInfo.descriptorSetCount = layouts.size();
			allocInfo.pSetLayouts = layouts.data();

			VkResult res = vkAllocateDescriptorSets(vulkan.device, &allocInfo, modelSet.data());

			if (res != VK_SUCCESS) {

				switch (res) {
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					std::cout << "Error out of host memory" << std::endl;
					break;
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					std::cout << "Error out of a device memory" << std::endl;
					break;
				case VK_ERROR_FRAGMENTED_POOL:
					std::cout << "Error fragmented pool" << std::endl;
					break;
				case VK_ERROR_OUT_OF_POOL_MEMORY:
					std::cout << "Error out of pool memory" << std::endl;
					break;
				}
				throw std::runtime_error("failed to create model sets");
			}
			else {
				std::cout << "Success: Allocated Model Descriptor Set" << std::endl;
			}

			std::cout << modelSet.size() << std::endl;
			std::vector<VkDescriptorBufferInfo> bi;
			std::vector<VkWriteDescriptorSet> wds;

			bi.resize(modelSet.size());
			wds.resize(modelSet.size());

			for (size_t i = 0; i < bi.size(); i++) {
				bi[i].buffer = modelsBuffer[i];
				bi[i].offset = 0;
				bi[i].range = sizeof(UniformModel);
			}

			for (size_t i = 0; i < wds.size(); i++) {
				wds[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wds[i].dstSet = modelSet[i];
				wds[i].dstBinding = 1;
				wds[i].dstArrayElement = 0;
				wds[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				wds[i].descriptorCount = 1;
				wds[i].pBufferInfo = &bi[i];
			}

			vkUpdateDescriptorSets(vulkan.device, wds.size(), wds.data(), 0, nullptr);
		}
	}

	void initCommandBuffer() {
		commandBuffers.resize(vulkan.swapchainImages.size());

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = vulkan.commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			VkResult res = vkAllocateCommandBuffers(vulkan.device, &allocInfo, &commandBuffers[i]);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate commandBuffer");
			}
			
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			
			res = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate Begin Command Buffer");
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = vulkan.renderPass;
			renderPassInfo.framebuffer = vulkan.framebuffer[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = vulkan.swapchainExtent;

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;
			
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Do Stuff
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// Draw Quad
			VkBuffer vertexBuffers[] = { verticesBuffer };
			VkDeviceSize offset[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offset);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Model

			std::vector<VkDescriptorSet> descSets = {
				cameraSet,
				modelSet[0]
			};

			vkCmdBindDescriptorSets(
				commandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				descSets.size(),
				descSets.data(),
				0,
				nullptr);

			vkCmdDrawIndexed(commandBuffers[i], indexList.size(), 1, 0, 0, 0);

			
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offset);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Model
			
			std::vector<VkDescriptorSet> descSets2 = {
				cameraSet,
				modelSet[1]
			};

			
			vkCmdBindDescriptorSets(
				commandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				descSets2.size(),
				descSets2.data(),
				0,
				nullptr);

			vkCmdDrawIndexed(commandBuffers[i], indexList.size(), 1, 0, 0, 0);
			

			// Draw Ball
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offset);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Model

			std::vector<VkDescriptorSet> descSets3 = {
				cameraSet,
				modelSet[2]
			};


			vkCmdBindDescriptorSets(
				commandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				descSets3.size(),
				descSets3.data(),
				0,
				nullptr);

			vkCmdDrawIndexed(commandBuffers[i], indexList.size(), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			res = vkEndCommandBuffer(commandBuffers[i]);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to End Command Buffer");
			}

		}
	}

	void toRect(util::Rect& r, Paddle& pad) {
		r.init(pad.position, pad.size);
	}
	
	void toRect(util::Rect& r, Ball& ball) {
		r.init(ball.position, ball.size);
	}

	void updatePaddlePlayer(Paddle& pad, float delta) {
		if (input::isInputMapPress("move-up")) {
			pad.velocity.y = -1.0f;

			if (pad.position.y < 0.0f) {
				pad.velocity.y = 0.0f;
			}
		}
		else if (input::isInputMapPress("move-down")) {
			pad.velocity.y = 1.0f;

			if (pad.position.y + pad.size.y > vulkan.swapchainExtent.height) {
				pad.velocity.y = 0.0f;
			}
		}
		else {
			pad.velocity.y = 0.0f;
		}

		pad.position += pad.velocity * pad.speed * delta;

		models[0].model =
			glm::translate(glm::mat4(1.0f), glm::vec3(pad.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(pad.size, 0.0f));


	}
	
	void updatePaddleImpossible(Paddle& pad, float delta) {

		if (ball.position.y + ball.size.y < pad.position.y) {
			if (pad.position.y > 0.0f)
				pad.velocity.y = -1.0f;
			else
				pad.velocity.y = 0.0f;
		}
		else if (ball.position.y > pad.position.y + pad.size.y) {
			if (pad.position.y + pad.size.y < vulkan.swapchainExtent.height)
				pad.velocity.y = 1.0f;
			else
				pad.velocity.y = 0.0f;
		}
		else {
			pad.velocity.y = 0.0f;
		}

		pad.position += pad.velocity * pad.speed * delta;

		models[1].model = 
			glm::translate(glm::mat4(1.0f), glm::vec3(pad.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(pad.size, 0.0f));
	}
	
	void updatePaddleExpert(Paddle& pad, float delta) {
		if (ball.velocity.x > 0.0f) {
			if (ball.position.y + ball.size.y < pad.position.y) {
				if (pad.position.y > 0.0f)
					pad.velocity.y = -1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else if (ball.position.y > pad.position.y + pad.size.y) {
				if (pad.position.y + pad.size.y < vulkan.swapchainExtent.height)
					pad.velocity.y = 1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else {
				pad.velocity.y = 0.0f;
			}
		}
		else {
			pad.velocity.y = 0.0f;
		}

		pad.position += pad.velocity * pad.speed * delta;

		models[1].model =
			glm::translate(glm::mat4(1.0f), glm::vec3(pad.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(pad.size, 0.0f));
	}

	void updatePaddleHard(Paddle& pad, float delta) {
		if (ball.velocity.x > 0.0f && ball.position.x > vulkan.swapchainExtent.width * 0.25f) {
			if (ball.position.y + ball.size.y < pad.position.y) {
				if (pad.position.y > 0.0f)
					pad.velocity.y = -1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else if (ball.position.y > pad.position.y + pad.size.y) {
				if (pad.position.y + pad.size.y < vulkan.swapchainExtent.height)
					pad.velocity.y = 1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else {
				pad.velocity.y = 0.0f;
			}
		}
		else {
			pad.velocity.y = 0.0f;
		}

		pad.position += pad.velocity * pad.speed * delta;

		models[1].model =
			glm::translate(glm::mat4(1.0f), glm::vec3(pad.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(pad.size, 0.0f));
	}

	void updatePaddleNormal(Paddle& pad, float delta) {
		if (ball.velocity.x > 0.0f && ball.position.x > vulkan.swapchainExtent.width * 0.5f) {
			if (ball.position.y + ball.size.y < pad.position.y) {
				if (pad.position.y > 0.0f)
					pad.velocity.y = -1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else if (ball.position.y > pad.position.y + pad.size.y) {
				if (pad.position.y + pad.size.y < vulkan.swapchainExtent.height)
					pad.velocity.y = 1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else {
				pad.velocity.y = 0.0f;
			}
		}
		else {
			pad.velocity.y = 0.0f;
		}

		pad.position += pad.velocity * pad.speed * delta;

		models[1].model =
			glm::translate(glm::mat4(1.0f), glm::vec3(pad.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(pad.size, 0.0f));
	}

	void updatePaddleEasy(Paddle& pad, float delta) {
		if (ball.velocity.x > 0.0f && ball.position.x > vulkan.swapchainExtent.width * 0.75f) {
			if (ball.position.y + ball.size.y < pad.position.y) {
				if (pad.position.y > 0.0f)
					pad.velocity.y = -1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else if (ball.position.y > pad.position.y + pad.size.y) {
				if (pad.position.y + pad.size.y < vulkan.swapchainExtent.height)
					pad.velocity.y = 1.0f;
				else
					pad.velocity.y = 0.0f;
			}
			else {
				pad.velocity.y = 0.0f;
			}
		}
		else {
			pad.velocity.y = 0.0f;
		}

		pad.position += pad.velocity * pad.speed * delta;

		models[1].model =
			glm::translate(glm::mat4(1.0f), glm::vec3(pad.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(pad.size, 0.0f));
	}

	void resetBall(Ball& b) {
		audio::playSoundFX("spawn-ball");
		b.speed = glm::vec2(32.0f);

		b.position = glm::vec2(
			vulkan.swapchainExtent.width * 0.5f - b.size.x * 0.5f,
			vulkan.swapchainExtent.height * 0.5f - b.size.y * 0.5f
		);

		b.velocity = glm::vec2(
			(mrand() % 2 == 0) ? -1.0f : 1.0f,
			(mrand() % 2 == 0) ? -1.0f : 1.0f
		);

	}

	void updateBall(Ball& b, float delta) {

		if (b.position.y < 0.0f) {
			audio::playSoundFX("ball-hit");
			b.velocity.y = 1.0f;
		}
		else if (b.position.y + b.size.y > vulkan.swapchainExtent.height) {
			audio::playSoundFX("ball-hit");
			b.velocity.y = -1.0f;
		}

		b.position += b.velocity * b.speed * delta;

		if (b.position.x + b.size.x < 0.0f) {
			//audio::playSoundFX("ai-player-score");
			resetBall(b);
		}
		else if (b.position.x > vulkan.swapchainExtent.width) {
			//audio::playSoundFX("player-score");
			resetBall(b);
		}

		util::Rect br, p1r, p2r;

		toRect(br, b);
		toRect(p1r, player);
		toRect(p2r, aiPlayer);

		if (br.isCollide(p1r)) {
			audio::playSoundFX("ball-hit");
			b.velocity.x = 1.0f;
			b.speed.x += b.speed.x * 0.1f;
		}

		if (br.isCollide(p2r)) {
			audio::playSoundFX("ball-hit");
			b.velocity.x = -1.0f;
			b.speed.x += b.speed.x * 0.1f;
		}

		models[2].model =
			glm::translate(glm::mat4(1.0f), glm::vec3(b.position, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(b.size, 0.0f));
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo ai = {};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ai.commandPool = vulkan.commandPool;
		ai.commandBufferCount = 1;

		VkCommandBuffer cb;
		vkAllocateCommandBuffers(vulkan.device, &ai, &cb);

		VkCommandBufferBeginInfo bi = {};
		bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cb, &bi);

		VkBufferCopy cr = {};
		cr.srcOffset = 0;
		cr.dstOffset = 0;
		cr.size = size;
		vkCmdCopyBuffer(cb, srcBuffer, dstBuffer, 1, &cr);

		vkEndCommandBuffer(cb);

		VkSubmitInfo sub = {};
		sub.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sub.commandBufferCount = 1;
		sub.pCommandBuffers = &cb;

		vkQueueSubmit(vulkan.graphicsQueue, 1, &sub, VK_NULL_HANDLE);
		vkQueueWaitIdle(vulkan.graphicsQueue);

		vkFreeCommandBuffers(vulkan.device, vulkan.commandPool, 1, &cb);
	}
}