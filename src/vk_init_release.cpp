#include "sys.h"


namespace vk {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR caps;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// Instance
	void initInstance(Vulkan&);

	// Debug Section
	void getInstanceExtensions(Vulkan& v, std::vector<const char*>& ext);
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);
	void initDebugMessenger(Vulkan&);
	// Surface
	void initSurface(Vulkan&);
	// Physical Device
	void pickPhysicalDevice(Vulkan&);
	bool isPhysicalDeviceSuitable(Vulkan& v, VkPhysicalDevice d);
	QueueFamilyIndices findQueueFamilies(Vulkan& v, VkPhysicalDevice d);
	bool checkDeviceExtensionSupport(Vulkan& v, VkPhysicalDevice d);
	SwapChainSupportDetails querySwapChainSupport(Vulkan& v, VkPhysicalDevice d);
	// Device
	void initDevice(Vulkan&);
	// Swapchain
	void initSwapchain(Vulkan&);
	// Render Passes
	void initRenderPasses(Vulkan&);
	// Framebuffers
	void initFramebuffer(Vulkan&);
	// Command Pool
	void initCommandPool(Vulkan&);
	// Semaphore
	void initSemaphore(Vulkan&);
	// Fence
	void initFence(Vulkan&);

	void initVulkan(Vulkan& vulkan) {
		initInstance(vulkan);
		if (vulkan.enableValidationLayers) {
			vulkan.debug.open("debug.txt");
			vulkan.debug << "Opened debug.txt" << std::endl;
			initDebugMessenger(vulkan);
		}
		initSurface(vulkan);
		pickPhysicalDevice(vulkan);
		initDevice(vulkan);
		initSwapchain(vulkan);
		initRenderPasses(vulkan);
		initFramebuffer(vulkan);
		initCommandPool(vulkan);
		initSemaphore(vulkan);
		initFence(vulkan);
	}

	void releaseVulkan(Vulkan& vulkan) {
		for (auto fence : vulkan.inFlight) {
			vkDestroyFence(vulkan.device, fence, nullptr);
		}

		for (auto s : vulkan.submitPresentQueue) {
			vkDestroySemaphore(vulkan.device, s, nullptr);
		}

		for (auto s : vulkan.submitCB) {
			vkDestroySemaphore(vulkan.device, s, nullptr);
		}

		if (vulkan.commandPool) {
			vkDestroyCommandPool(vulkan.device, vulkan.commandPool, nullptr);
		}

		// Draw Framebuffer
		for (auto framebuffer : vulkan.framebuffer) {
			vkDestroyFramebuffer(vulkan.device, framebuffer, nullptr);
		}

		if (vulkan.renderPass) {
			vkDestroyRenderPass(vulkan.device, vulkan.renderPass, nullptr);
		}

		for (auto imageView : vulkan.swapchainImageViews) {
			vkDestroyImageView(vulkan.device, imageView, nullptr);
		}
		if (vulkan.swapchain) {
			vkDestroySwapchainKHR(vulkan.device, vulkan.swapchain, nullptr);
		}

		if (vulkan.device) {
			vkDestroyDevice(vulkan.device, nullptr);
		}

		vulkan.physicalDevice = nullptr;

		if (vulkan.surface) {
			vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, nullptr);
		}

		if (vulkan.enableValidationLayers) {
			if (vulkan.debugMessenger) {
				((PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan.instance, "vkDestroyDebugUtilsMessengerEXT"))(vulkan.instance, vulkan.debugMessenger, nullptr);
			}
		}

		if (vulkan.instance) {
			vkDestroyInstance(vulkan.instance, nullptr);
		}

		if (vulkan.enableValidationLayers) {
			vulkan.debug.close();
		}
	}



	// Instance
	void initInstance(Vulkan& v) {
		// Application Info
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "n/a";
		appInfo.applicationVersion = 1;
		appInfo.pEngineName = "n/a";
		appInfo.engineVersion = 1;
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Instance Create Info
		VkInstanceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CI.pApplicationInfo = &appInfo;
		if (v.enableValidationLayers) {
			CI.enabledLayerCount = v.validationLayers.size();
			CI.ppEnabledLayerNames = v.validationLayers.data();
		}

		std::vector<const char*> ext;
		getInstanceExtensions(v, ext);

		CI.enabledExtensionCount = ext.size();
		CI.ppEnabledExtensionNames = ext.data();

		VkResult res = vkCreateInstance(&CI, nullptr, &v.instance);

		if (res != VK_SUCCESS) {
			std::runtime_error("Failed to create instance!");
		}
		else {
			std::cout << "Create Instance" << std::endl;
		}
	}

	// Debug Section
	void getInstanceExtensions(Vulkan& v, std::vector<const char*>& ext) {
		ext.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		ext.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

		if (v.enableValidationLayers) {
			ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	) {
		Vulkan* v = (Vulkan*)pUserData;

		v->debug << "Validation Layer" << std::endl;
		v->debug << pCallbackData->pMessageIdName << std::endl;
		v->debug << pCallbackData->pMessage << std::endl;
		v->debug << std::endl;

		return VK_FALSE;
	}

	void initDebugMessenger(Vulkan& v) {
		VkDebugUtilsMessengerCreateInfoEXT CI = {};
		CI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		CI.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		CI.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		CI.pUserData = &v;
		CI.pfnUserCallback = debugCallback;

		VkResult res = ((PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(v.instance, "vkCreateDebugUtilsMessengerEXT"))(v.instance, &CI, nullptr, &v.debugMessenger);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create debugMessager");
		}
		else {
			std::cout << "Success: Create DebugMessenger" << std::endl;
		}
	}

	// Surface
	void initSurface(Vulkan& v) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		SDL_GetWindowWMInfo(app::getWindow(), &info);

		VkWin32SurfaceCreateInfoKHR CI = {};
		CI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		CI.hwnd = info.info.win.window;
		CI.hinstance = info.info.win.hinstance;

		VkResult res = vkCreateWin32SurfaceKHR(v.instance, &CI, nullptr, &v.surface);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
		else {
			std::cout << "Success: Created Window Surface" << std::endl;
		}
	}

	// Physical Device
	void pickPhysicalDevice(Vulkan& v) {
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(v.instance, &count, nullptr);
		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(v.instance, &count, devices.data());

		for (const auto& d : devices) {
			if (isPhysicalDeviceSuitable(v, d)) {
				v.physicalDevice = d;
				break;
			}
		}

		if (v.physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
		else {
			std::cout << "Success: Found a physical device" << std::endl;
		}
	}

	bool isPhysicalDeviceSuitable(Vulkan& v, VkPhysicalDevice d) {
		QueueFamilyIndices indices;
		indices = findQueueFamilies(v, d);
		bool extensionSupport = checkDeviceExtensionSupport(v, d);

		bool swapChainAdequate = false;
		if (extensionSupport) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(v, d);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionSupport && swapChainAdequate;
	}

	QueueFamilyIndices findQueueFamilies(Vulkan& v, VkPhysicalDevice d) {
		QueueFamilyIndices indices;

		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(d, &count, nullptr);
		std::vector<VkQueueFamilyProperties> props(count);
		vkGetPhysicalDeviceQueueFamilyProperties(d, &count, props.data());

		int i = 0;
		for (const auto& p : props) {
			if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(d, i, v.surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	bool checkDeviceExtensionSupport(Vulkan& v, VkPhysicalDevice d) {
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(d, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> aExt(count);
		vkEnumerateDeviceExtensionProperties(d, nullptr, &count, aExt.data());

		std::set<std::string> reqExt(v.deviceExtensions.begin(), v.deviceExtensions.end());

		for (const auto& e : aExt) {
			reqExt.erase(e.extensionName);
		}

		return reqExt.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(Vulkan& v, VkPhysicalDevice d) {
		SwapChainSupportDetails details;

		// Capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, v.surface, &details.caps);

		// Formats
		uint32_t count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(d, v.surface, &count, nullptr);
		if (count > 0) {
			details.formats.resize(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(d, v.surface, &count, details.formats.data());
		}

		// Present Modes
		count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(d, v.surface, &count, nullptr);
		if (count > 0) {
			details.presentModes.resize(count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(d, v.surface, &count, details.presentModes.data());
		}

		for (auto p : details.presentModes) {
			if (p == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				std::cout << "Immediate Mode" << std::endl;
			}
			else if (p == VK_PRESENT_MODE_FIFO_KHR) {
				std::cout << "FIFO Mode" << std::endl;
			}
			else if (p == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
				std::cout << "FIFO Relaxed Moded" << std::endl;
			}
			else if (p == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "Mailbox Mode" << std::endl;
			}
		}

		return details;
	}

	// Device
	void initDevice(Vulkan& v) {
		QueueFamilyIndices indices = findQueueFamilies(v, v.physicalDevice);

		float queuePriority = 1.0f;

		VkPhysicalDeviceFeatures deviceFeatures = {};

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamily = {
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
		};

		for (uint32_t q : uniqueQueueFamily) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = q;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		CI.queueCreateInfoCount = queueCreateInfos.size();
		CI.pQueueCreateInfos = queueCreateInfos.data();
		CI.pEnabledFeatures = &deviceFeatures;
		CI.enabledExtensionCount = 0;

		if (v.enableValidationLayers) {
			CI.enabledLayerCount = v.validationLayers.size();
			CI.ppEnabledLayerNames = v.validationLayers.data();
		}

		CI.enabledExtensionCount = v.deviceExtensions.size();
		CI.ppEnabledExtensionNames = v.deviceExtensions.data();

		VkResult res = vkCreateDevice(v.physicalDevice, &CI, nullptr, &v.device);

		if (res != VK_SUCCESS) {
			std::runtime_error("failed to create device");
		}
		else {
			std::cout << "Success: Created Device" << std::endl;
		}

		vkGetDeviceQueue(v.device, indices.graphicsFamily.value(), 0, &v.graphicsQueue);
		vkGetDeviceQueue(v.device, indices.presentFamily.value(), 0, &v.presentQueue);
	}

	// Swapchain
	void initSwapchain(Vulkan& v) {
		// ---- SwapChain ---- //
		SwapChainSupportDetails details = querySwapChainSupport(v, v.physicalDevice);

		// Format
		VkSurfaceFormatKHR format = details.formats[0];

		for (const auto& f : details.formats) {
			if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				format = f;
			}
		}

		// Present Mode
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& p : details.presentModes) {
			if (p == VK_PRESENT_MODE_MAILBOX_KHR) {
				presentMode = p;
				break;
			}
		}

		// Extern 2D
		VkExtent2D extent;
		if (details.caps.currentExtent.width != UINT32_MAX) {
			extent = details.caps.currentExtent;
		}
		else {
			uint32_t width, height;

			width = app::getWidth();
			height = app::getHeight();

			VkExtent2D actualExtent = {
				width,
				height
			};

			actualExtent.width = std::max(
				details.caps.minImageExtent.width,
				std::min(
					details.caps.maxImageExtent.width,
					actualExtent.width
				)
			);

			actualExtent.height = std::max(
				details.caps.minImageExtent.height,
				std::min(
					details.caps.maxImageExtent.height,
					actualExtent.height
				)
			);

			extent = actualExtent;
		}

		uint32_t imageCount = details.caps.minImageCount + 1;

		if (details.caps.maxImageCount > 0 && imageCount > details.caps.maxImageCount) {
			imageCount = details.caps.maxImageCount;
		}

		VkSwapchainCreateInfoKHR CI = {};
		CI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		CI.surface = v.surface;
		CI.minImageCount = imageCount;
		CI.imageFormat = format.format;
		CI.imageColorSpace = format.colorSpace;
		CI.imageExtent = extent;
		CI.imageArrayLayers = 1;
		CI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(v, v.physicalDevice);
		std::vector<uint32_t> queueFamilyIndices = {
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
		};

		if (indices.graphicsFamily != indices.presentFamily) {
			CI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			CI.queueFamilyIndexCount = queueFamilyIndices.size();
			CI.pQueueFamilyIndices = queueFamilyIndices.data();

			std::cout << "SwapChain: in concurrent mode" << std::endl;
		}
		else {
			CI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			CI.queueFamilyIndexCount = 0;
			CI.pQueueFamilyIndices = nullptr;
			std::cout << "SwapChain: in exclusive mode" << std::endl;
		}

		CI.preTransform = details.caps.currentTransform;
		CI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		CI.presentMode = presentMode;
		CI.clipped = VK_TRUE;

		CI.oldSwapchain = VK_NULL_HANDLE;

		VkResult res = vkCreateSwapchainKHR(v.device, &CI, nullptr, &v.swapchain);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create swapchain");
		}
		else {
			std::cout << "Success: Create Swapchain" << std::endl;
		}



		// ---- Image ---- //
		vkGetSwapchainImagesKHR(v.device, v.swapchain, &imageCount, nullptr);
		v.swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(v.device, v.swapchain, &imageCount, v.swapchainImages.data());

		v.swapchainImageFormat = format.format;
		v.swapchainExtent = extent;


		// ---- image views ---- //
		v.swapchainImageViews.resize(v.swapchainImages.size());

		for (size_t i = 0; i < v.swapchainImages.size(); i++) {
			VkImageViewCreateInfo temp = {};
			temp.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			temp.image = v.swapchainImages[i];
			temp.viewType = VK_IMAGE_VIEW_TYPE_2D;
			temp.format = v.swapchainImageFormat;
			temp.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			temp.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			temp.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			temp.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			temp.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			temp.subresourceRange.baseMipLevel = 0;
			temp.subresourceRange.levelCount = 1;
			temp.subresourceRange.baseArrayLayer = 0;
			temp.subresourceRange.layerCount = 1;

			res = vkCreateImageView(v.device, &temp, nullptr, &v.swapchainImageViews[i]);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create image view");
			}
			else {
				std::cout << "Success: Create Image View [" << i << "]" << std::endl;
			}

		}
	}

	// Render Passes
	void initRenderPasses(Vulkan& v) {
		// Color Attachments
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = v.swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Subpass Reference
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Supass
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		// deps
		VkSubpassDependency deps = {};
		deps.srcSubpass = VK_SUBPASS_EXTERNAL;
		deps.dstSubpass = 0;
		deps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps.srcAccessMask = 0;
		deps.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		deps.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		VkRenderPassCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		CI.attachmentCount = 1;
		CI.pAttachments = &colorAttachment;
		CI.subpassCount = 1;
		CI.pSubpasses = &subpass;
		CI.dependencyCount = 1;
		CI.pDependencies = &deps;


		VkResult res = vkCreateRenderPass(v.device, &CI, nullptr, &v.renderPass);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create clearRenderPass");
		}
		else {
			std::cout << "Success: Created Clear Render Pass" << std::endl;
		}
	}

	// Framebuffers
	void initFramebuffer(Vulkan& v) {
		v.framebuffer.resize(v.swapchainImageViews.size());

		for (size_t i = 0; i < v.swapchainImageViews.size(); i++) {
			VkFramebufferCreateInfo CI = {};
			CI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			CI.renderPass = v.renderPass;
			CI.attachmentCount = 1;
			CI.pAttachments = &v.swapchainImageViews[i];
			CI.width = v.swapchainExtent.width;
			CI.height = v.swapchainExtent.height;
			CI.layers = 1;

			VkResult res = vkCreateFramebuffer(v.device, &CI, nullptr, &v.framebuffer[i]);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create clear framebuffer");
			}
			else {
				std::cout << "Success: Create Clear Framebuffer [" << i << "]" << std::endl;
			}
		}
	}

	// Command Pool
	void initCommandPool(Vulkan& v) {
		QueueFamilyIndices family = findQueueFamilies(v, v.physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = family.graphicsFamily.value();
		poolInfo.flags = 0;

		VkResult res = vkCreateCommandPool(v.device, &poolInfo, nullptr, &v.commandPool);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool");
		}
		else {
			std::cout << "Success: Create Command Pool" << std::endl;
		}
	}

	// Semaphore
	void initSemaphore(Vulkan& v) {
		VkSemaphoreCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// SubmitCB
		v.submitCB.resize(v.swapchainImages.size());

		for (size_t i = 0; i < v.swapchainImages.size(); i++) {
			VkResult res = vkCreateSemaphore(v.device, &CI, nullptr, &v.submitCB[i]);
			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create submitCB");
			}
			else {
				std::cout << "Success: Created SubmitCB["<< i <<"]" << std::endl;
			}
		}

		// Submit Present Queue
		v.submitPresentQueue.resize(v.swapchainImages.size());

		for (size_t i = 0; i < v.swapchainImages.size(); i++) {
			VkResult res = vkCreateSemaphore(v.device, &CI, nullptr, &v.submitPresentQueue[i]);
			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create submitPresentQueue");
			}
			else {
				std::cout << "Success: Created SubmitPresentQueue["<< i <<"]" << std::endl;
			}
		}
	}

	// Fence
	void initFence(Vulkan& v) {
		VkFenceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		
		v.inFlight.resize(v.swapchainImages.size());
		v.imageInFlight.resize(v.swapchainImages.size());

		for (size_t i = 0; i < v.inFlight.size(); i++) {
			VkResult res = vkCreateFence(v.device, &CI, nullptr, &v.inFlight[i]);

			if (res != VK_SUCCESS) {
				throw std::runtime_error("failed to create inFlight");
			}
			else {
				std::cout << "Success: Create InFlight["<< i <<"]" << std::endl;
			}
		}
	}
}