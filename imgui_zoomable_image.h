

#ifndef IMGUI_ZOOMABLE_IMAGE_H
#define IMGUI_ZOOMABLE_IMAGE_H

#include "imgui.h"

namespace ImGuiImage
{

  // Function to display a zoomable and pannable image within an ImGui window.
  // Parameters:
  // - texRef: The ImGui texture reference of the image to display.
  //   Read about `ImTextureID/ImTextureRef` here:
  //   https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
  // - displaySize: The size (width and height) to display the image within the
  //   ImGui window.
  // - 'uv0' and 'uv1' are texture coordinates. Read about them from the same
  //   link above.
  // - bgColor: Background color behind the image.
  // - tintColor: Tint color to apply to the image.
  IMGUI_API void Zoomable(
    ImTextureRef texRef,
    const ImVec2& displaySize,
    const ImVec2& uv0 = ImVec2(0, 0),
    const ImVec2& uv1 = ImVec2(1, 1),
    const ImVec4& bgColor = ImVec4(0, 0, 0, 0),
    const ImVec4& tintColor = ImVec4(1, 1, 1, 1));
}
// ----------------------------------- Implementation -----------------------
namespace ImGuiImage
{
  inline void Zoomable(
    ImTextureRef texRef,
    const ImVec2& displaySize,
    const ImVec2& uv0,
    const ImVec2& uv1,
    const ImVec4& bgColor,
    const ImVec4& tintColor)
  {
    //TODO: implement zoomable image
    ImGui::ImageWithBg(texRef, displaySize, uv0, uv1, bgColor, tintColor);
  }
} // namespace ImGuiImage

#endif // IMGUI_ZOOMABLE_IMAGE_H
