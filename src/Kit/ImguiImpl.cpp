#include "Kit/ImguiImpl.hpp"

#include "Kit/IncOpenGL.hpp"
#include "Kit/Window.hpp"
#include "Kit/Texture.hpp"
#include "Kit/imgui/imgui.h"

#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <functional>

kit::Window * kit::UISystem::m_window = nullptr;
uint32_t kit::UISystem::m_instanceCount = 0;

bool kit::UISystem::m_usesKeyboard = false;
bool kit::UISystem::m_usesMouse = false;
bool kit::UISystem::m_usesTextInput = false;

bool kit::UISystem::imgui_mousePressed[3] = {false, false, false};
float kit::UISystem::imgui_mouseWheel = 0.0;
GLuint kit::UISystem::imgui_fontTexture = 0;
int kit::UISystem::imgui_shaderHandle = 0;
int kit::UISystem::imgui_vertHandle = 0;
int kit::UISystem::imgui_fragHandle = 0;
int kit::UISystem::imgui_attribLocationTex = 0;
int kit::UISystem::imgui_attribLocationProjMtx = 0;
int kit::UISystem::imgui_attribLocationPosition = 0;
int kit::UISystem::imgui_attribLocationUV = 0;
int kit::UISystem::imgui_attribLocationColor = 0;
unsigned int kit::UISystem::imgui_vboHandle = 0;
unsigned int kit::UISystem::imgui_vaoHandle = 0;
unsigned int kit::UISystem::imgui_elementsHandle = 0;


kit::UISystem::UISystem()
{
  m_instanceCount++;
  if (m_instanceCount == 1)
  {
    allocateShared();
  }
}

kit::UISystem::~UISystem()
{
  kit::UISystem::m_instanceCount--;
  if (kit::UISystem::m_instanceCount == 0)
  {
    kit::UISystem::releaseShared();
  }
}

void kit::UISystem::allocateShared()
{
  std::cout << "Allocating UI system.." << std::endl;
  ImGuiIO &io = ImGui::GetIO();

  io.KeyMap[ImGuiKey_Tab] = kit::Tab;
  io.KeyMap[ImGuiKey_LeftArrow] = kit::Left;
  io.KeyMap[ImGuiKey_RightArrow] = kit::Right;
  io.KeyMap[ImGuiKey_UpArrow] = kit::Up;
  io.KeyMap[ImGuiKey_DownArrow] = kit::Down;
  io.KeyMap[ImGuiKey_PageUp] = kit::Page_up;
  io.KeyMap[ImGuiKey_PageDown] = kit::Page_down;
  io.KeyMap[ImGuiKey_Home] = kit::Home;
  io.KeyMap[ImGuiKey_End] = kit::End;
  io.KeyMap[ImGuiKey_Delete] = kit::Delete;
  io.KeyMap[ImGuiKey_Backspace] = kit::Backspace;
  io.KeyMap[ImGuiKey_Enter] = kit::Enter;
  io.KeyMap[ImGuiKey_Escape] = kit::Escape;
  io.KeyMap[ImGuiKey_A] = kit::A;
  io.KeyMap[ImGuiKey_C] = kit::C;
  io.KeyMap[ImGuiKey_V] = kit::V;
  io.KeyMap[ImGuiKey_X] = kit::X;
  io.KeyMap[ImGuiKey_Y] = kit::Y;
  io.KeyMap[ImGuiKey_Z] = kit::Z;

  io.RenderDrawListsFn = kit::UISystem::imgui_renderDrawLists;
  io.SetClipboardTextFn = kit::UISystem::imgui_setClipboardText;
  io.GetClipboardTextFn = kit::UISystem::imgui_getClipboardText;
}

void kit::UISystem::releaseShared()
{
  kit::UISystem::invalidateDeviceObjects();
  ImGui::Shutdown();
}

void kit::UISystem::setWindow(kit::Window * window)
{
  m_window = window;
  #ifdef _WIN32
    ImGuiIO &io = ImGui::GetIO();
    if (m_window)
    {
      io.ImeWindowHandle = glfwGetWin32Window(m_window->getGLFWHandle());
    }
  #endif
}


void kit::UISystem::imgui_renderDrawLists(ImDrawData* draw_data)
{
  
  // Backup GL state
  GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
  GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
  GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
  GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
  GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
  GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
  GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
  GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
  GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
  GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
  GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
  GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glActiveTexture(GL_TEXTURE0);

  // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
  ImGuiIO& io = ImGui::GetIO();
  int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // Setup viewport, orthographic projection matrix
  glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
  const float ortho_projection[4][4] =
  {
    { 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
    { 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
    { 0.0f,                  0.0f,                  -1.0f, 0.0f },
    { -1.0f,                  1.0f,                   0.0f, 1.0f },
  };
  glUseProgram(kit::UISystem::imgui_shaderHandle);
  glUniform1i(kit::UISystem::imgui_attribLocationTex, 0);
  glUniformMatrix4fv(kit::UISystem::imgui_attribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
  glBindVertexArray(kit::UISystem::imgui_vaoHandle);

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawIdx* idx_buffer_offset = 0;

    glBindBuffer(GL_ARRAY_BUFFER, imgui_vboHandle);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imgui_elementsHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

    for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
    {
      if (pcmd->UserCallback)
      {
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else
      {
        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
        glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
      }
      idx_buffer_offset += pcmd->ElemCount;
    }
  }

  // Restore modified GL state
  glUseProgram(last_program);
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
  glBindVertexArray(last_vertex_array);
  glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
  glBlendFunc(last_blend_src, last_blend_dst);
  if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
  if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
  if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
  if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
  glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}


const char* kit::UISystem::imgui_getClipboardText()
{
  if (m_window)
  {
    return glfwGetClipboardString(m_window->getGLFWHandle());
  }
  static const char * woot = "";
  return woot;
}

void kit::UISystem::imgui_setClipboardText(const char * text)
{
  if (m_window)
  {
    glfwSetClipboardString(m_window->getGLFWHandle(), text);
  }
}

bool kit::UISystem::createFontsTexture()
{
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGenTextures(1, &kit::UISystem::imgui_fontTexture);
  glBindTexture(GL_TEXTURE_2D, kit::UISystem::imgui_fontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = (void *)(intptr_t)kit::UISystem::imgui_fontTexture;

  // Restore state
  glBindTexture(GL_TEXTURE_2D, last_texture);

  return true;
}

bool kit::UISystem::createDeviceObjects()
{
  
  // Backup GL state
  GLint last_texture, last_array_buffer, last_vertex_array;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

  const GLchar *vertex_shader =
    "#version 330\n"
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 UV;\n"
    "in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	Frag_UV = UV;\n"
    "	Frag_Color = Color;\n"
    "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

  const GLchar* fragment_shader =
    "#version 330\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main()\n"
    "{\n"
    "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
    "}\n";

  kit::UISystem::imgui_shaderHandle = glCreateProgram();
  kit::UISystem::imgui_vertHandle = glCreateShader(GL_VERTEX_SHADER);
  kit::UISystem::imgui_fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
  void();

  glShaderSource(kit::UISystem::imgui_vertHandle, 1, &vertex_shader, 0);
  glShaderSource(kit::UISystem::imgui_fragHandle, 1, &fragment_shader, 0);
  glCompileShader(kit::UISystem::imgui_vertHandle);
  glCompileShader(kit::UISystem::imgui_fragHandle);
  glAttachShader(kit::UISystem::imgui_shaderHandle, kit::UISystem::imgui_vertHandle);
  glAttachShader(kit::UISystem::imgui_shaderHandle, kit::UISystem::imgui_fragHandle);
  glLinkProgram(kit::UISystem::imgui_shaderHandle);

  kit::UISystem::imgui_attribLocationTex = glGetUniformLocation(kit::UISystem::imgui_shaderHandle, "Texture");
  kit::UISystem::imgui_attribLocationProjMtx = glGetUniformLocation(kit::UISystem::imgui_shaderHandle, "ProjMtx");
  kit::UISystem::imgui_attribLocationPosition = glGetAttribLocation(kit::UISystem::imgui_shaderHandle, "Position");
  kit::UISystem::imgui_attribLocationUV = glGetAttribLocation(kit::UISystem::imgui_shaderHandle, "UV");
  kit::UISystem::imgui_attribLocationColor = glGetAttribLocation(kit::UISystem::imgui_shaderHandle, "Color");

  void();

  glGenBuffers(1, &kit::UISystem::imgui_vboHandle);
  glGenBuffers(1, &kit::UISystem::imgui_elementsHandle);

  glGenVertexArrays(1, &kit::UISystem::imgui_vaoHandle);
  glBindVertexArray(kit::UISystem::imgui_vaoHandle);
  glBindBuffer(GL_ARRAY_BUFFER, kit::UISystem::imgui_vboHandle);
  glEnableVertexAttribArray(kit::UISystem::imgui_attribLocationPosition);
  glEnableVertexAttribArray(kit::UISystem::imgui_attribLocationUV);
  glEnableVertexAttribArray(kit::UISystem::imgui_attribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
  glVertexAttribPointer(kit::UISystem::imgui_attribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
  glVertexAttribPointer(kit::UISystem::imgui_attribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
  glVertexAttribPointer(kit::UISystem::imgui_attribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

  kit::UISystem::createFontsTexture();

  // Restore modified GL state
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
  glBindVertexArray(last_vertex_array);

  return true;
}

void kit::UISystem::invalidateDeviceObjects()
{
  
  if (kit::UISystem::imgui_vaoHandle) glDeleteVertexArrays(1, &kit::UISystem::imgui_vaoHandle);
  if (kit::UISystem::imgui_vboHandle) glDeleteBuffers(1, &kit::UISystem::imgui_vboHandle);
  if (kit::UISystem::imgui_elementsHandle) glDeleteBuffers(1, &kit::UISystem::imgui_elementsHandle);
  kit::UISystem::imgui_vaoHandle = kit::UISystem::imgui_vboHandle = kit::UISystem::imgui_elementsHandle = 0;

  if (kit::UISystem::imgui_vertHandle != 0)
  {
    glDetachShader(kit::UISystem::imgui_shaderHandle, kit::UISystem::imgui_vertHandle);
    glDeleteShader(kit::UISystem::imgui_vertHandle);
  }
  kit::UISystem::imgui_vertHandle = 0;

  if (kit::UISystem::imgui_fragHandle != 0)
  {
    glDetachShader(kit::UISystem::imgui_shaderHandle, kit::UISystem::imgui_fragHandle);
    glDeleteShader(kit::UISystem::imgui_fragHandle);
  }
  kit::UISystem::imgui_fragHandle = 0;

  if (kit::UISystem::imgui_shaderHandle != 0)
  {
    glDeleteProgram(kit::UISystem::imgui_shaderHandle);
  }
  kit::UISystem::imgui_shaderHandle = 0;

  if (kit::UISystem::imgui_fontTexture)
  {
    glDeleteTextures(1, &kit::UISystem::imgui_fontTexture);
    ImGui::GetIO().Fonts->TexID = 0;
    kit::UISystem::imgui_fontTexture = 0;
  }
}

void kit::UISystem::handleEvent(kit::WindowEvent const & evt)
{
  ImGuiIO& io = ImGui::GetIO();
  if (evt.type == kit::WindowEvent::MouseButtonPressed)
  {
    if (evt.mouse.button >= 0 && evt.mouse.button < 3)
    {
      kit::UISystem::imgui_mousePressed[evt.mouse.button] = true;
    }
  }

  if (evt.type == kit::WindowEvent::MouseScrolled)
  {
    kit::UISystem::imgui_mouseWheel += (float)evt.mouse.scrollOffset.y;
  }

  if (evt.type == kit::WindowEvent::KeyPressed)
  {
    io.KeysDown[evt.keyboard.key] = true;

    if (evt.keyboard.key == kit::LeftControl || evt.keyboard.key == kit::RightControl)
    {
      io.KeyCtrl = true;
    }
    if (evt.keyboard.key == kit::LeftShift || evt.keyboard.key == kit::RightShift)
    {
      io.KeyShift = true;
    }
    if (evt.keyboard.key == kit::LeftAlt || evt.keyboard.key == kit::RightAlt)
    {
      io.KeyAlt = true;
    }
  }

  if (evt.type == kit::WindowEvent::KeyReleased)
  {
    io.KeysDown[evt.keyboard.key] = false;

    if (evt.keyboard.key == kit::LeftControl || evt.keyboard.key == kit::RightControl)
    {
      io.KeyCtrl = false;
    }
    if (evt.keyboard.key == kit::LeftShift || evt.keyboard.key == kit::RightShift)
    {
      io.KeyShift = false;
    }
    if (evt.keyboard.key == kit::LeftAlt || evt.keyboard.key == kit::RightAlt)
    {
      io.KeyAlt = false;
    }
  }

  if (evt.type == kit::WindowEvent::TextEntered)
  {
    if (evt.keyboard.unicode > 0 && evt.keyboard.unicode < 0x10000)
    {
      io.AddInputCharacter((unsigned short)evt.keyboard.unicode);
    }
  }
}

void kit::UISystem::prepareFrame(double const & ms)
{
  if (!m_window)
  {
    return;
  }

  if (!kit::UISystem::imgui_fontTexture)
  {
    kit::UISystem::createDeviceObjects();
  }

  ImGuiIO& io = ImGui::GetIO();

  // Setup display size (every frame to accommodate for window resizing)
  int w, h;
  int display_w, display_h;
  glfwGetWindowSize(m_window->getGLFWHandle(), &w, &h);
  glfwGetFramebufferSize(m_window->getGLFWHandle(), &display_w, &display_h);
  io.DisplaySize = ImVec2((float)w, (float)h);
  io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

  // Setup time step
  io.DeltaTime = float(ms / 1000.0);

  // Setup inputs
  // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
  if (glfwGetWindowAttrib(m_window->getGLFWHandle(), GLFW_FOCUSED))
  {
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_window->getGLFWHandle(), &mouse_x, &mouse_y);
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
  }
  else
  {
    io.MousePos = ImVec2(-1, -1);
  }

  for (int i = 0; i < 3; i++)
  {
    io.MouseDown[i] = kit::UISystem::imgui_mousePressed[i] || glfwGetMouseButton(m_window->getGLFWHandle(), i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    kit::UISystem::imgui_mousePressed[i] = false;
  }

  io.MouseWheel = kit::UISystem::imgui_mouseWheel;
  kit::UISystem::imgui_mouseWheel = 0.0f;

  // Hide OS mouse cursor if ImGui is drawing it
  //TODO: Find out if we need this (I dont want it): glfwSetInputMode(win->getGLFWHandle(), GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

  // Start the frame
  ImGui::NewFrame();
  kit::UISystem::m_usesKeyboard = io.WantCaptureKeyboard;
  kit::UISystem::m_usesMouse = io.WantCaptureMouse;
  kit::UISystem::m_usesTextInput = io.WantTextInput;
}

void kit::UISystem::render()
{
  ImGui::Render();
}

bool kit::UISystem::usesMouse()
{
  return kit::UISystem::m_usesMouse;
}

bool kit::UISystem::usesKeyboard()
{
  return kit::UISystem::m_usesKeyboard;
}

bool kit::UISystem::usesTextInput()
{
  return kit::UISystem::m_usesTextInput;
}

kit::UISystem::SelectionIterator kit::UISystem::select(const std::string& title, kit::UISystem::SelectionIterator currIndex, kit::UISystem::SelectionList& list)
{
  kit::UISystem::SelectionIterator returner = currIndex;
  
  ImGui::PushID(std::string(title + "-selector").c_str());
  
  ImGui::BeginGroup();
  {
    // Selection box
    ImGui::BeginGroup();    
    if(currIndex == list.end())
    {
      if(ImGui::Button(std::string(title + "##button").c_str()))
      {
        ImGui::OpenPopup(title.c_str());
      }
    }
    else
    {
      if ((*currIndex).second != nullptr)
      {
        ImTextureID texId = (ImTextureID)(*currIndex).second->getHandle();
        if (ImGui::ImageButton(texId, ImVec2(92.0f, 92.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
        {
          ImGui::OpenPopup(title.c_str());
        }
        ImGui::Text((*currIndex).first.c_str());
      }
      else
      {
        if (ImGui::Button((*currIndex).first.c_str()))
        {
          ImGui::OpenPopup(title.c_str());
        }
      }

    }
    ImGui::EndGroup();
    
    // Selection list
    if (ImGui::BeginPopup(title.c_str()))
    {
      // Header text

      for(SelectionIterator i = list.begin(); i != list.end(); i++)
      {
        if(i == currIndex)
        {
          continue;
        }
        
        if ((*i).second != nullptr)
        {
          if (ImGui::ImageButton((ImTextureID)(*i).second->getHandle(), ImVec2(32, 32), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0)))
          {

            returner = i;
            ImGui::CloseCurrentPopup();
          }

          ImGui::SameLine();
        }
        
        if(ImGui::Selectable((*i).first.c_str(), false, 0, ImVec2(0, 32)))
        {
          returner = i;
        }
        
      }

      ImGui::EndPopup();
    }
  }
  ImGui::EndGroup();
  
  ImGui::PopID();
  
  return returner;
}


kit::Texture * kit::UISystem::selectTexture(const std::string& name, kit::Texture * currentTexture, bool srgb, bool reload, const std::string& prefix)
{
  kit::Texture * newTexture = currentTexture;
  static std::vector<std::string> textures; 
  static std::vector<kit::Texture *> textureObjs;
  
  if (reload || textures.size() == 0)
  {
    //textures = kit::Texture::getAvailableTextures(prefix, true); //TODO: Fix this (or scrap this impl)
    for(auto currName : textures)
    {
      //textureObjs.push_back(kit::Texture::load(prefix + currName, srgb)); //TODO fix this 
    }
  }

  ImGui::BeginGroup();
  {
    // Set the map texture if available
    if (currentTexture)
    {
      ImGui::BeginGroup();
      ImTextureID texId = (ImTextureID)currentTexture->getHandle();
      if (ImGui::ImageButton(texId, ImVec2(92.0f, 92.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
      {
        ImGui::OpenPopup(name.c_str());
      }
      ImGui::Text(currentTexture->getFilename().c_str());
      ImGui::EndGroup();
    }
    else
    {
      if (ImGui::Button( std::string( std::string("No texture..##notexture") + name ).c_str() ))
      {
        ImGui::OpenPopup(name.c_str());
      }
    }
    // Show the popup
    if (ImGui::BeginPopup(name.c_str()))
    {
      // Header text
      ImGui::Text("Set texture..");

      // Text to unset texture
      if (ImGui::Selectable(std::string(std::string("Unset texture..##unsettexture") + name).c_str()))
      {
        newTexture = nullptr;
      }
      ImGui::Separator();
      for (uint32_t i = 0; i < textures.size(); i++)
      {
        if(ImGui::ImageButton((ImTextureID)textureObjs[i]->getHandle(), ImVec2(32, 32), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0)))
        {
          //TODO newTexture = kit::Texture::load(prefix + textures[i], srgb);
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Selectable(textures[i].c_str(), (textureObjs[i] == currentTexture), 0, ImVec2(0, 32)))
        {
          //TODO newTexture = kit::Texture::load(prefix + textures[i], srgb);
        }
      }
      ImGui::EndPopup();
    }
  }
  ImGui::EndGroup();

  return newTexture;
}
