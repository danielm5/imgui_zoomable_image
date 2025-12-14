

#ifndef IMGUI_ZOOMABLE_IMAGE_H
#define IMGUI_ZOOMABLE_IMAGE_H

#include "imgui.h"

#include <limits>
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace ImGuiImage
{
  struct State
  {
    bool zoomPanEnabled = true;
    bool maintainAspectRatio = false;
    ImVec2 textureSize = ImVec2(0.0f, 0.0f);
    float zoomLevel = 1.0f;
    ImVec2 panOffset = ImVec2(0.0f, 0.0f);
    ImVec2 mousePosition = ImVec2(0.0f, 0.0f);
  };

  constexpr ImVec2 kDefaultUV0(0.0f, 0.0f);
  constexpr ImVec2 kDefaultUV1(1.0f, 1.0f);
  constexpr ImVec4 kDefaultBackgroundColor(0, 0, 0, 0);
  constexpr ImVec4 kDefaultTintColor(1, 1, 1, 1);

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
    State* state = nullptr);

  IMGUI_API void Zoomable(
    ImTextureRef texRef,
    const ImVec2& displaySize,
    const ImVec2& uv0,
    const ImVec2& uv1,
    State* state = nullptr);

  IMGUI_API void Zoomable(
    ImTextureRef texRef,
    const ImVec2& displaySize,
    const ImVec2& uv0 = kDefaultUV0,
    const ImVec2& uv1 = kDefaultUV1,
    const ImVec4& bgColor = kDefaultBackgroundColor,
    const ImVec4& tintColor = kDefaultTintColor,
    State* state = nullptr);
}
// ----------------------------------- Implementation ------------------------
namespace ImGuiImage
{
  inline void Zoomable(
    ImTextureRef texRef,
    const ImVec2& displaySize,
    State* state)
  {
    Zoomable(texRef, displaySize, kDefaultUV0, kDefaultUV1,
      kDefaultBackgroundColor, kDefaultTintColor, state);
  }

  inline void Zoomable(
    ImTextureRef texRef,
    const ImVec2& displaySize,
    const ImVec2& uv0,
    const ImVec2& uv1,
    State* state)
  {
    Zoomable(texRef, displaySize, kDefaultUV0, kDefaultUV1,
      kDefaultBackgroundColor, kDefaultTintColor, state);
  }

  inline void Zoomable(
    ImTextureRef texRef,
    const ImVec2& imageSize,
    const ImVec2& uv0,
    const ImVec2& uv1,
    const ImVec4& bgColor,
    const ImVec4& tintColor,
    State* state)
  {
    // Check image size
    if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
    { // Invalid size, do nothing
      return;
    }

    // Internal state
    State* s{ state };
    if (s == nullptr)
    { // We cannot zoom or pan without a state, just show the image
      ImGui::Image(texRef, imageSize, uv0, uv1, tintColor, bgColor);
      return;
    }

    // Create a child region to limit events to the image area
    // Without the child region, panning the image with the mouse
    // moves the parent window as well.
    ImGui::BeginChild("ImageRegion", ImVec2(0,0), false, ImGuiWindowFlags_NoMove);

    // Get texture size
    ImVec2 textureSize{ s->textureSize };
    if (textureSize.x <= 0.0f || textureSize.y <= 0.0f)
    { // use image size as texture size
      textureSize = ImVec2(
        imageSize.x / std::abs(uv1.x - uv0.x),
        imageSize.y / std::abs(uv1.y - uv0.y));
    }

    // Respect the image aspect ratio
    ImVec2 widgetSize{ ImGui::GetContentRegionAvail() };
    ImVec2 displaySize{ widgetSize };
    if (s->maintainAspectRatio)
    {
      const float aspectRatio{ textureSize.x / textureSize.y };
      if (displaySize.x / displaySize.y > aspectRatio)
      {
        displaySize.x = displaySize.y * aspectRatio;
      }
      else
      {
        displaySize.y = displaySize.x / aspectRatio;
      }
    }

    // Center the image
    ImVec2 displayPos{
      (widgetSize.x - displaySize.x) * 0.5f + ImGui::GetCursorPosX(),
      (widgetSize.y - displaySize.y) * 0.5f + ImGui::GetCursorPosY(),
    };

    // Set the display position
    ImGui::SetCursorPos(displayPos);
    const ImVec2 screenDisplayPos{ ImGui::GetCursorScreenPos() };

    // Apply view setting
    const float zoom{ s->zoomLevel > 1.0f ? s->zoomLevel : 1.0f };
    const float s1{ 1.0f / zoom };
    const ImVec2 t1{ s->panOffset.x, s->panOffset.y };
    const ImVec2 uv0New{ t1.x + uv0.x * s1, t1.y + uv0.y * s1 };
    const ImVec2 uv1New{ t1.x + uv1.x * s1, t1.y + uv1.y * s1 };

    // Display the texture
    ImGui::Image(texRef, displaySize, uv0New, uv1New, tintColor, bgColor);

    // Handle mouse events
    if(ImGui::IsItemHovered())
    {
      auto& io = ImGui::GetIO();

      // update mouse position
      const ImVec2 screenPoint{
        (io.MousePos.x - screenDisplayPos.x) / displaySize.x,
        (io.MousePos.y - screenDisplayPos.y) / displaySize.y,
      };
      const ImVec2 imagePoint{ t1.x + screenPoint.x * s1, t1.y + screenPoint.y * s1 };
      s->mousePosition.x = std::clamp(imagePoint.x * textureSize.x, 0.0f, textureSize.x);
      s->mousePosition.y = std::clamp(imagePoint.y * textureSize.y, 0.0f, textureSize.y);

      if (s->zoomPanEnabled)
      { // handle pan and zoom only if enabled
        if(io.MouseWheel != 0.0f)
        { // update image zoom when mouse wheel is scrolled

          // compute the new scale
          constexpr float maxScale{ 1.0f };
          const float minScale{ 1.0f / std::max(textureSize.x, textureSize.y) };
          const float scaleFactor{ io.MouseWheel < 0 ? 1.1f : 0.9f };
          const float s2{ std::min(maxScale, std::max(minScale, scaleFactor * s1)) };

          // make the image position below the mouse to stay at a fixed point
          // before and after zooming, compute the new translation to keep the
          // fixed point where it was:
          //
          //    screenPoint <-> imagePoint
          //    imagePoint.x = uv0'.x + screenPoint.x * (uv1'.x - uv0'.x)
          //    imagePoint.y = uv0'.y + screenPoint.y * (uv1'.y - uv0'.y)
          //    uv0' = (0,0)*s2 + t2 = t2
          //    uv1' = (1,1)*s2 + t2
          //    uv1'- uv0' = (1,1)*s2 + t2 -t2 = (s2,s2)
          //    -> imagePoint = t2 + screenPoint * s2
          //    -> t2 = imagePoint - screenPoint * s2
          //
          ImVec2 t2{ imagePoint.x - screenPoint.x * s2, imagePoint.y - screenPoint.y * s2 };
          if (t2.x < 0.0f) { t2.x = 0.0f; }
          if (t2.y < 0.0f) { t2.y = 0.0f; }
          if (t2.x > 1.0f - s2) { t2.x = 1.0f - s2; }
          if (t2.y > 1.0f - s2) { t2.y = 1.0f - s2; }

          // update scale and translation
          s->zoomLevel = 1.0f / s2;
          s->panOffset.x = t2.x;
          s->panOffset.y = t2.y;
        }
        else if (io.MouseDoubleClicked[0])
        { // reset view on double click
          s->zoomLevel = 1.0f;
          s->panOffset.x = 0.0f;
          s->panOffset.y = 0.0f;
        }
        else if(io.MouseDown[0])
        { // pan the image if mouse is moved while pressing the left button

          const ImVec2 screenDelta{
            io.MouseDelta.x / displaySize.x,
            io.MouseDelta.y / displaySize.y,
          };
          const ImVec2 imageDelta{ screenDelta.x * s1, screenDelta.y * s1 };

          ImVec2 t2{ t1.x - imageDelta.x, t1.y - imageDelta.y };
          if (t2.x < 0.0f) { t2.x = 0.0f; }
          if (t2.y < 0.0f) { t2.y = 0.0f; }
          if (t2.x > 1.0f - s1) { t2.x = 1.0f - s1; }
          if (t2.y > 1.0f - s1) { t2.y = 1.0f - s1; }

          // update translation
          s->panOffset.x = t2.x;
          s->panOffset.y = t2.y;
        }
      } // if (zoomPanEnabled)
    }
    else
    { // make mouse position invalid if the image is not hovered
      s->mousePosition.x = std::numeric_limits<float>::quiet_NaN();
      s->mousePosition.y = std::numeric_limits<float>::quiet_NaN();
    }

    // End child region
    ImGui::EndChild();
  }
} // namespace ImGuiImage

#endif // IMGUI_ZOOMABLE_IMAGE_H
