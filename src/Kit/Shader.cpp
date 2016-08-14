#include "Kit/Shader.hpp"
#include "Kit/Exception.hpp"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

std::string typeToName(kit::Shader::Type type)
{
  switch(type)
  {
    default:
    case kit::Shader::Type::Vertex:
      return "vertex";
      break;
    case kit::Shader::Type::Fragment:
      return "fragment";
      break;
    case kit::Shader::Type::Geometry:
      return "geometry";
      break;
    
  }
}

kit::Shader::Shader(kit::Shader::Type type)
{
  this->m_glHandle = glCreateShader((uint32_t)type);
  this->m_type = type;
  this->m_source = "";
  KIT_ASSERT(this->m_glHandle);
}

kit::Shader::~Shader()
{
  KIT_GL(glDeleteShader(this->m_glHandle));
}

kit::Shader::Ptr kit::Shader::create(kit::Shader::Type type)
{
  return std::make_shared<kit::Shader>(type);
}

bool kit::Shader::sourceFromFile(std::string const & filename)
{
  std::string line;
  std::string source = "";

  std::ifstream handle(filename);

  if(!handle.is_open())
  {
    KIT_ERR("Could not open file");
    return false;
  }

  while(std::getline(handle,line))
  {
    source.append(line);
    source.append("\n");
  }

  this->m_source = source;

  const GLchar * src = (const GLchar *)this->m_source.c_str();
  KIT_GL(glShaderSource(this->m_glHandle, 1, &src, 0));

  return true;
}

void kit::Shader::sourceFromString(std::string const & s)
{
  this->m_source = s;

  const GLchar *source = (const GLchar *)this->m_source.c_str();
  KIT_GL(glShaderSource(this->m_glHandle, 1, &source, 0));
}

void kit::Shader::clearSource()
{
  this->m_source = "";

  std::string emptys = "";
  const GLchar *source = (const GLchar *)emptys.c_str();
  KIT_GL(glShaderSource(this->m_glHandle, 1, &source, 0));
}

bool kit::Shader::compile()
{
  // Attempt to compile the shader
  KIT_GL(glCompileShader(this->m_glHandle));

  // Retrieve the compilation status
  GLint status;
  KIT_GL(glGetShaderiv(this->m_glHandle, GL_COMPILE_STATUS, &status));

  // If compilation failed, dump source and return false
  if (!status)
  {
    GLint blen = 0;
    GLsizei slen = 0;
    KIT_GL(glGetShaderiv(this->m_glHandle, GL_INFO_LOG_LENGTH , &blen));
    if (blen > 1)
    {
      GLchar* compiler_log = new GLchar[blen];
      KIT_GL(glGetShaderInfoLog(this->m_glHandle, blen, &slen, compiler_log));

      std::stringstream ss;
      ss << typeToName(this->m_type) << "-shader compilation failed: " << compiler_log;
      KIT_ERR(ss.str());
      delete[] compiler_log;
      
      std::cout << "Dumping source..." << std::endl;
      std::cout << "------------------------" << std::endl;
      
      std::stringstream sourcestream(this->m_source);
      unsigned int linecount = 1;
      std::string currline;
      while (std::getline(sourcestream, currline))
      {
        std::cout << linecount << ":\t" << currline << std::endl;
        linecount++;
      }

      std::cout << "------------------------" << std::endl;
    }

    return false;
  }

  return true;
}

GLuint kit::Shader::getHandle()
{
  return this->m_glHandle;
}