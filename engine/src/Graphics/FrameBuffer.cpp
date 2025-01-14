#include "Graphics/FrameBuffer.h"

// Project includes.
#include "Core/Logs.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
  FrameBuffer::FrameBuffer()
    : depthBuffer(nullptr)
    , width(1)
    , height(1)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    glGenFramebuffers(1, &this->bufferID);
  }

  // Constructors and destructor.
  FrameBuffer::FrameBuffer(uint width, uint height)
    : depthBuffer(nullptr)
    , width(width)
    , height(height)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    glGenFramebuffers(1, &this->bufferID);
    assert(((width != 0 && height != 0), "Framebuffer width and height cannot be zero."));
  }

  FrameBuffer::~FrameBuffer()
  {
    // Delete the texture attachments.
    this->textureAttachments.clear();

    // Actual buffer delete.
    glDeleteFramebuffers(1, &this->bufferID);
  }

  // Bind the buffer.
  void
  FrameBuffer::bind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, this->bufferID);
  }

  // Unbind the buffer.
  void
  FrameBuffer::unbind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // Attach a texture to the framebuffer.
  void
  FrameBuffer::attachTexture2D(const FBOSpecification &spec, const bool &removeTex)
  {
    Logger* logs = Logger::getInstance();

    Texture2DParams newTexParam = spec;

    uint n;
    if (spec.format == TextureFormats::Red || spec.format == TextureFormats::Depth)
      n = 1;
    else if (spec.format == TextureFormats::RG || spec.format == TextureFormats::DepthStencil)
      n = 2;
    else if (spec.format == TextureFormats::RGB)
      n = 3;
    else if (spec.format == TextureFormats::RGBA)
      n = 4;
    else
      assert("Unknown format, failed to attach.");

    Texture2D* newTex = new Texture2D(this->width, this->height, n, newTexParam);
    newTex->initNullTexture();

    // If an attachment target already exists, remove it from the map and add
    // the new target.
    auto targetLoc = this->textureAttachments.find(spec.target);
    if (targetLoc != this->textureAttachments.end())
      this->textureAttachments.erase(targetLoc);

    glBindFramebuffer(GL_FRAMEBUFFER, this->bufferID);

    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<uint>(spec.target), static_cast<uint>(spec.type), newTex->getID(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Setup the clear flags.
    if (spec.target == FBOTargetParam::Depth)
      this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    else if (spec.target == FBOTargetParam::Stencil)
      this->clearFlags |=GL_STENCIL_BUFFER_BIT;
    else if (spec.target == FBOTargetParam::DepthStencil)
      this->clearFlags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    else
      this->clearFlags |= GL_COLOR_BUFFER_BIT;

    this->textureAttachments.insert({ spec.target, { spec, Shared<Texture2D>(newTex) } });
  }

  void
  FrameBuffer::attachTexture2D(const FBOSpecification &spec, Shared<Texture2D> &tex,
                               const bool &removeTex)
  {
    Logger* logs = Logger::getInstance();

    if (tex->width != this->width || tex->height != this->height)
    {
      std::cout << "Cannot attach this texture, dimensions do not agree!" << std::endl;
      return;
    }

    this->bind();

    // If an attachment target already exists, remove it from the map and add
    // the new target.
    auto targetLoc = this->textureAttachments.find(spec.target);
    if (targetLoc != this->textureAttachments.end())
      this->textureAttachments.erase(targetLoc);


    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<uint>(spec.target),
                           static_cast<uint>(spec.type),
                           tex->getID(), 0);

    this->unbind();

    this->clearFlags |= GL_COLOR_BUFFER_BIT;

    this->textureAttachments.insert({ spec.target, { spec, tex } });
  }

  void
  FrameBuffer::attachRenderBuffer(RBOInternalFormat format)
  {
    Logger* logs = Logger::getInstance();

    if (this->depthBuffer != nullptr)
    {
      std::cout << "This framebuffer already has a render buffer" << std::endl;
      return;
    }
    this->bind();

    this->depthBuffer = createShared<RenderBuffer>(this->width, this->height,
                                                   format);
    if (format == RBOInternalFormat::Depth24 || format == RBOInternalFormat::Depth32f)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthBuffer->getID());
    else if (format == RBOInternalFormat::Stencil)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthBuffer->getID());
    else if (format == RBOInternalFormat::DepthStencil)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthBuffer->getID());
    this->unbind();

    this->clearFlags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    this->hasRenderBuffer = true;
  }

  void
  FrameBuffer::detach(const FBOTargetParam &attachment)
  {
    auto& pair = this->textureAttachments.at(attachment);
    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<uint>(pair.first.target),
                           static_cast<uint>(pair.first.type),
                           0, 0);
  }

  void
  FrameBuffer::reattach(const FBOTargetParam &attachment)
  {
    auto& pair = this->textureAttachments.at(attachment);
    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<uint>(pair.first.target),
                           static_cast<uint>(pair.first.type),
                           pair.second->getID(), 0);
  }

  void
  FrameBuffer::setDrawBuffers()
  {
    this->bind();
    std::vector<GLenum> totalAttachments;
    for (auto& pair : this->textureAttachments)
    {
      if (pair.first != FBOTargetParam::Depth && pair.first != FBOTargetParam::Stencil
          && pair.first != FBOTargetParam::DepthStencil)
        totalAttachments.push_back(static_cast<GLenum>(pair.first));
    }
    if (totalAttachments.size() >= 1)
      glDrawBuffers(totalAttachments.size(), totalAttachments.data());
    this->unbind();
  }

  void
  FrameBuffer::blitzToOther(FrameBuffer &target, const FBOTargetParam &type)
  {
    switch (type)
    {
      case FBOTargetParam::Depth:
      {
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        break;
      }

      case FBOTargetParam::Stencil:
      {
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_STENCIL_BUFFER_BIT, GL_NEAREST);
        break;
      }

      case FBOTargetParam::DepthStencil:
      {
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                               GL_NEAREST);
        break;
      }

      default:
      {
        // Default to colour for any other attachment.
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_COLOR_BUFFER_BIT, GL_LINEAR);
        break;
      }
    }
  }

  int
  FrameBuffer::readPixel(const FBOTargetParam &target, const glm::vec2 &mousePos)
  {
    this->bind();
    glReadBuffer(static_cast<GLenum>(target));
    float data;
    glReadPixels((int) mousePos.x, (int) mousePos.y, 1, 1, GL_RED, GL_FLOAT, &data);
    return static_cast<int>(data);
  }

  // Resize the framebuffer.
  void
  FrameBuffer::resize(uint width, uint height)
  {
    this->width = width;
    this->height = height;

    // Update the attached textures.
    for (auto& pair : this->textureAttachments)
    {
      auto spec = pair.second.first;
      auto tex = pair.second.second;

      if (tex == nullptr)
        continue;

      tex->bind();
      glTexImage2D(static_cast<uint>(spec.type), 0,
                   static_cast<uint>(spec.internal), this->width, this->height, 0,
                   static_cast<uint>(spec.format),
                   static_cast<uint>(spec.dataType), nullptr);
    }

    // Update the attached render buffer.
    if (this->depthBuffer != nullptr)
    {
      this->depthBuffer->bind();
      glRenderbufferStorage(GL_RENDERBUFFER,
                            static_cast<uint>(this->depthBuffer->getFormat()),
                            this->width, this->height);
    }
  }

  void
  FrameBuffer::setViewport()
  {
    glViewport(0, 0, this->width, this->height);
  }

  void
  FrameBuffer::setClearColour(const glm::vec4 &clearColour)
  {
    this->clearColour = clearColour;
  }

  void
  FrameBuffer::bindTextureID(const FBOTargetParam &attachment)
  {
    this->textureAttachments.at(attachment).second->bind();
  }

  void
  FrameBuffer::bindTextureID(const FBOTargetParam &attachment, uint bindPoint)
  {
    this->textureAttachments.at(attachment).second->bind(bindPoint);
  }

  void
  FrameBuffer::clear()
  {
    this->bind();
    glClearColor(this->clearColour[0], this->clearColour[1], this->clearColour[2],
                 this->clearColour[3]);
    glClear(this->clearFlags);
    this->unbind();
  }

  bool
  FrameBuffer::isValid()
  {
    this->bind();
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cout << "The framebuffer is not valid for a drawcall." << std::endl;
      return false;
    }
    else
    {
      return true;
    }
    this->unbind();
  }

  FBOSpecification
  FBOCommands::getDefaultColourSpec(const FBOTargetParam &attach)
  {
    FBOSpecification defaultColour = FBOSpecification();
    defaultColour.target = attach;
    defaultColour.type = FBOTex2DParam::Texture2D;
    defaultColour.internal = TextureInternalFormats::RGB;
    defaultColour.format = TextureFormats::RGB;
    defaultColour.dataType = TextureDataType::Bytes;
    defaultColour.sWrap = TextureWrapParams::Repeat;
    defaultColour.tWrap = TextureWrapParams::Repeat;
    defaultColour.minFilter = TextureMinFilterParams::Linear;
    defaultColour.maxFilter = TextureMaxFilterParams::Linear;

    return defaultColour;
  }

  FBOSpecification
  FBOCommands::getFloatColourSpec(const FBOTargetParam &attach)
  {
    FBOSpecification floatColour = FBOSpecification();
    floatColour.target = attach;
    floatColour.type = FBOTex2DParam::Texture2D;
    floatColour.internal = TextureInternalFormats::RGBA16f;
    floatColour.format = TextureFormats::RGBA;
    floatColour.dataType = TextureDataType::Floats;
    floatColour.sWrap = TextureWrapParams::Repeat;
    floatColour.tWrap = TextureWrapParams::Repeat;
    floatColour.minFilter = TextureMinFilterParams::Linear;
    floatColour.maxFilter = TextureMaxFilterParams::Linear;

    return floatColour;
  }

  FBOSpecification
  FBOCommands::getDefaultDepthSpec()
  {
    FBOSpecification defaultDepth = FBOSpecification();
    defaultDepth.target = FBOTargetParam::Depth;
    defaultDepth.type = FBOTex2DParam::Texture2D;
    defaultDepth.internal = TextureInternalFormats::Depth32f;
    defaultDepth.format = TextureFormats::Depth;
    defaultDepth.dataType = TextureDataType::Floats;
    defaultDepth.sWrap = TextureWrapParams::Repeat;
    defaultDepth.tWrap = TextureWrapParams::Repeat;
    defaultDepth.minFilter = TextureMinFilterParams::Nearest;
    defaultDepth.maxFilter = TextureMaxFilterParams::Nearest;

    return defaultDepth;
  }
}
