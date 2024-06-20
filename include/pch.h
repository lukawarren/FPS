#pragma once

#include <unordered_map>
#include <stdexcept>
#include <optional>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <array>

#include <glad/glad.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define DBG_MACRO_NO_WARNING
#include "dbg.h"

#define module_private public
#include "csg.hpp"