#include "sys.h"


namespace vk {


	VkShaderModule createShaderModule(Vulkan& v, const std::vector<char>& code) {
		VkShaderModuleCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		CI.codeSize = code.size();
		CI.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule temp;

		VkResult res = vkCreateShaderModule(v.device, &CI, nullptr, &temp);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module");
		}
		else {
			std::cout << "Success: Create Shader Module" << std::endl;
		}

		return temp;

	}

}