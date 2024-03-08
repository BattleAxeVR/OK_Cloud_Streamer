//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OK_CLOUD_SESSION_H
#define OK_CLOUD_SESSION_H

#include "defines.h"
#include <shell/shared/renderSession/RenderSession.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <igl/IGL.h>
#include <shell/shared/platform/Platform.h>

namespace igl::shell 
{

struct VertexFormat 
{
  glm::mat4 modelMatrix = glm::mat4(1.0);
  glm::mat4 viewProjectionMatrix[2]{};
  float scaleZ{};
};

class OKCloudSession : public RenderSession 
{
 public:
    OKCloudSession(std::shared_ptr<Platform> platform) : RenderSession(std::move(platform)) {}
  void initialize() noexcept override;
  void update(igl::SurfaceTextures surfaceTextures) noexcept override;

 private:
  std::shared_ptr<ICommandQueue> commandQueue_;
  RenderPassDesc renderPass_;
  std::shared_ptr<IRenderPipelineState> pipelineState_;
  std::shared_ptr<IVertexInputState> vertexInput0_;
  std::shared_ptr<IShaderStages> shaderStages_;
  std::shared_ptr<IBuffer> vb0_, ib0_; // Buffers for vertices and indices (or constants)
  std::shared_ptr<ITexture> tex0_;
  std::shared_ptr<ISamplerState> samp0_;
  std::shared_ptr<IFramebuffer> framebuffer_;

  VertexFormat vertexParameters_;

  // utility fns
  void createSamplerAndTextures(const IDevice& /*device*/);
  void setVertexParams();
};

} // namespace igl::shell

#endif // OK_CLOUD_SESSION_H


