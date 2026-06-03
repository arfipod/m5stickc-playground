#include "CoverFlowMenu.h"

#include <new>
#include <pgmspace.h>

namespace {
const uint16_t kBackground = 0x0084;
const uint16_t kPanelDark = 0x2108;
const uint16_t kText = 0xFFFF;
const uint16_t kMuted = 0x632C;
const uint16_t kShadow = 0x0000;
const uint8_t kMaxVisiblePoses = 7;

uint16_t scaleColor565(uint16_t color, uint8_t brightness) {
  uint16_t r = (color >> 11) & 0x1F;
  uint16_t g = (color >> 5) & 0x3F;
  uint16_t b = color & 0x1F;

  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;

  return (r << 11) | (g << 5) | b;
}

uint16_t blendColor565(uint16_t from, uint16_t to, uint8_t amount) {
  uint16_t inv = 255 - amount;
  uint16_t fr = (from >> 11) & 0x1F;
  uint16_t fg = (from >> 5) & 0x3F;
  uint16_t fb = from & 0x1F;
  uint16_t tr = (to >> 11) & 0x1F;
  uint16_t tg = (to >> 5) & 0x3F;
  uint16_t tb = to & 0x1F;

  uint16_t r = (fr * inv + tr * amount) / 255;
  uint16_t g = (fg * inv + tg * amount) / 255;
  uint16_t b = (fb * inv + tb * amount) / 255;

  return (r << 11) | (g << 5) | b;
}

int16_t min16(int16_t a, int16_t b) {
  return a < b ? a : b;
}

int16_t max16(int16_t a, int16_t b) {
  return a > b ? a : b;
}

int16_t abs16(int16_t value) {
  return value < 0 ? -value : value;
}

int16_t interpolateByDistance(int16_t absOffsetQ8, int16_t centerValue,
                              int16_t sideValue, int16_t farValue) {
  int16_t clamped = min16(absOffsetQ8, 512);
  if (clamped <= 256) {
    return centerValue + ((sideValue - centerValue) * clamped) / 256;
  }

  int16_t t = clamped - 256;
  return sideValue + ((farValue - sideValue) * t) / 256;
}

bool iconPixelSet(const MenuItem& item, uint8_t x, uint8_t y) {
  if (item.icon == nullptr || x >= item.iconWidth || y >= item.iconHeight) {
    return false;
  }

  uint32_t row = pgm_read_dword(&item.icon[y]);
  return (row & (0x80000000UL >> x)) != 0;
}
}  // namespace

CoverFlowMenu::CoverFlowMenu(const MenuItem* items, uint8_t itemCount)
    : _items(items),
      _itemCount(itemCount),
      _selectedIndex(0),
      _targetIndex(0),
      _transitionDirection(0),
      _transitionMs(180),
      _animationElapsedMs(0),
      _animationProgress(0.0f),
      _isAnimating(false),
      _width(240),
      _height(135),
      _frameReady(false),
      _frame(nullptr) {
}

CoverFlowMenu::~CoverFlowMenu() {
  if (_frame != nullptr) {
    _frame->deleteSprite();
    _frame->~TFT_eSprite();
    _frame = nullptr;
  }
}

bool CoverFlowMenu::begin(TFT_eSPI& display, int16_t width, int16_t height) {
  _width = width;
  _height = height;

  if (_frame == nullptr) {
    _frame = new (static_cast<void*>(_frameStorage)) TFT_eSprite(&display);
    _frame->setColorDepth(8);
  }

  if (!_frameReady) {
    _frameReady = _frame->createSprite(_width, _height) != nullptr;
  }

  return _frameReady;
}

void CoverFlowMenu::update(const CoverFlowInputState& inputState,
                           uint32_t deltaMs) {
  if (_itemCount == 0) {
    return;
  }

  if (!_isAnimating) {
    if (inputState.selectPressed) {
      select();
    } else if (inputState.nextPressed || inputState.tiltDirection > 0) {
      next();
    } else if (inputState.previousPressed || inputState.tiltDirection < 0) {
      previous();
    }
  }

  updateAnimation(deltaMs);
}

void CoverFlowMenu::draw(TFT_eSPI& display) {
  if (_frameReady && _frame != nullptr) {
    drawScene(*_frame);
    _frame->pushSprite(0, 0);
    return;
  }

  drawScene(display);
}

void CoverFlowMenu::next() {
  startTransition(1);
}

void CoverFlowMenu::previous() {
  startTransition(-1);
}

void CoverFlowMenu::select() {
  if (_itemCount == 0 || _isAnimating) {
    return;
  }

  const MenuItem& item = _items[_selectedIndex];
  if (item.callback != nullptr) {
    item.callback(item);
  }
}

uint8_t CoverFlowMenu::selectedIndex() const {
  return _selectedIndex;
}

float CoverFlowMenu::animationProgress() const {
  return _animationProgress;
}

bool CoverFlowMenu::isAnimating() const {
  return _isAnimating;
}

void CoverFlowMenu::startTransition(int8_t direction) {
  if (_itemCount < 2 || _isAnimating || direction == 0) {
    return;
  }

  _transitionDirection = direction > 0 ? 1 : -1;
  _targetIndex =
      _transitionDirection > 0
          ? (_selectedIndex + 1) % _itemCount
          : (_selectedIndex == 0 ? _itemCount - 1 : _selectedIndex - 1);
  _animationElapsedMs = 0;
  _animationProgress = 0.0f;
  _isAnimating = true;
}

void CoverFlowMenu::updateAnimation(uint32_t deltaMs) {
  if (!_isAnimating) {
    return;
  }

  uint32_t elapsed = _animationElapsedMs + deltaMs;
  if (elapsed >= _transitionMs) {
    _selectedIndex = _targetIndex;
    _transitionDirection = 0;
    _animationElapsedMs = 0;
    _animationProgress = 0.0f;
    _isAnimating = false;
    return;
  }

  _animationElapsedMs = elapsed;
  float t = static_cast<float>(_animationElapsedMs) /
            static_cast<float>(_transitionMs);
  _animationProgress = t * t * (3.0f - 2.0f * t);
}

void CoverFlowMenu::drawScene(TFT_eSPI& gfx) {
  gfx.fillRect(0, 0, _width, _height, kBackground);
  gfx.fillRect(0, 0, _width, 20, 0x0002);
  gfx.drawFastHLine(0, 89, _width, 0x18C3);
  gfx.drawFastHLine(0, 90, _width, 0x0000);

  ItemPose poses[kMaxVisiblePoses];
  uint8_t poseCount = 0;

  for (uint8_t i = 0; i < _itemCount && poseCount < kMaxVisiblePoses; ++i) {
    ItemPose pose = makePose(i);
    if (pose.absOffsetQ8 <= 640) {
      poses[poseCount++] = pose;
    }
  }

  bool drawn[kMaxVisiblePoses] = {false};
  for (uint8_t drawCount = 0; drawCount < poseCount; ++drawCount) {
    int8_t drawIndex = -1;
    int16_t largestDistance = -1;

    for (uint8_t i = 0; i < poseCount; ++i) {
      if (!drawn[i] && poses[i].absOffsetQ8 > largestDistance) {
        largestDistance = poses[i].absOffsetQ8;
        drawIndex = i;
      }
    }

    if (drawIndex >= 0) {
      drawItem(gfx, poses[drawIndex]);
      drawn[drawIndex] = true;
    }
  }

  drawSelectedLabel(gfx);
  drawPageDots(gfx);
}

void CoverFlowMenu::drawItem(TFT_eSPI& gfx, const ItemPose& pose) {
  const MenuItem& item = _items[pose.index];
  int16_t iconW = (static_cast<int32_t>(item.iconWidth) * pose.scaleXQ8) / 256;
  int16_t iconH = (static_cast<int32_t>(item.iconHeight) * pose.scaleYQ8) / 256;
  int16_t panelW = iconW + 13;
  int16_t panelH = iconH + 11;
  bool nearCenter = pose.absOffsetQ8 < 96;

  if (nearCenter) {
    int16_t shadowW = iconW + 20;
    gfx.fillRoundRect(pose.centerX - shadowW / 2, pose.topY + iconH + 11,
                      shadowW, 5, 2, kShadow);
  }

  uint16_t panel =
      blendColor565(kPanelDark, item.color, max16(24, pose.brightness / 4));
  drawCoverBack(gfx, pose, panelW, panelH, panel);

  uint16_t iconColor = scaleColor565(item.color, pose.brightness);
  drawIcon(gfx, item, pose.centerX, pose.topY, pose.scaleXQ8, pose.scaleYQ8,
           pose.skewQ8, iconColor, false, 0);

  if (nearCenter) {
    uint16_t reflectionColor = scaleColor565(item.color, 62);
    drawIcon(gfx, item, pose.centerX, pose.topY + iconH + 5, pose.scaleXQ8,
             pose.scaleYQ8 / 3, -pose.skewQ8 / 2, reflectionColor, true, 12);
  }
}

void CoverFlowMenu::drawIcon(TFT_eSPI& gfx, const MenuItem& item,
                             int16_t centerX, int16_t topY,
                             int16_t scaleXQ8, int16_t scaleYQ8,
                             int16_t skewQ8, uint16_t color, bool reflected,
                             uint8_t maxRows) {
  if (item.icon == nullptr || item.iconWidth == 0 || item.iconHeight == 0) {
    return;
  }

  uint8_t rows = item.iconHeight;
  if (maxRows > 0 && maxRows < rows) {
    rows = maxRows;
  }

  int16_t scaledWidth =
      (static_cast<int32_t>(item.iconWidth) * scaleXQ8 + 128) / 256;
  int16_t baseX = centerX - scaledWidth / 2;

  for (uint8_t sy = 0; sy < rows; ++sy) {
    uint8_t sampleY = reflected ? item.iconHeight - 1 - sy : sy;
    int16_t y0 = topY + (static_cast<int32_t>(sy) * scaleYQ8) / 256;
    int16_t y1 = topY + (static_cast<int32_t>(sy + 1) * scaleYQ8 + 255) / 256;
    int16_t rowHeight = max16(1, y1 - y0);
    int16_t rowSkew =
        ((static_cast<int16_t>(sy) - item.iconHeight / 2) * skewQ8) / 256;
    uint16_t rowColor = color;

    if (reflected) {
      uint8_t fade = max16(18, 125 - sy * 9);
      rowColor = scaleColor565(color, fade);
    }

    for (uint8_t sx = 0; sx < item.iconWidth; ++sx) {
      if (!iconPixelSet(item, sx, sampleY)) {
        continue;
      }

      int16_t x0 =
          baseX + (static_cast<int32_t>(sx) * scaleXQ8) / 256 + rowSkew;
      int16_t x1 =
          baseX + (static_cast<int32_t>(sx + 1) * scaleXQ8 + 255) / 256 +
          rowSkew;
      gfx.fillRect(x0, y0, max16(1, x1 - x0), rowHeight, rowColor);
    }
  }
}

void CoverFlowMenu::drawCoverBack(TFT_eSPI& gfx, const ItemPose& pose,
                                  int16_t width, int16_t height,
                                  uint16_t color) {
  int16_t x0 = pose.centerX - width / 2;
  int16_t x1 = x0 + width;
  int16_t y0 = pose.topY - 5;
  int16_t y1 = y0 + height;
  int16_t topShift = -pose.skewQ8 / 18;
  int16_t bottomShift = pose.skewQ8 / 18;
  uint16_t border = scaleColor565(_items[pose.index].color, pose.brightness);

  gfx.fillTriangle(x0 + topShift, y0, x1 + topShift, y0, x0 + bottomShift, y1,
                   color);
  gfx.fillTriangle(x1 + topShift, y0, x1 + bottomShift, y1,
                   x0 + bottomShift, y1, color);

  gfx.drawLine(x0 + topShift, y0, x1 + topShift, y0, border);
  gfx.drawLine(x1 + topShift, y0, x1 + bottomShift, y1, border);
  gfx.drawLine(x1 + bottomShift, y1, x0 + bottomShift, y1,
               scaleColor565(border, 120));
  gfx.drawLine(x0 + bottomShift, y1, x0 + topShift, y0,
               scaleColor565(border, 120));
}

void CoverFlowMenu::drawSelectedLabel(TFT_eSPI& gfx) {
  if (_itemCount == 0) {
    return;
  }

  uint8_t index = activeIndexForLabel();
  const MenuItem& item = _items[index];
  gfx.setTextSize(1);
  gfx.setTextColor(kText, kBackground);
  gfx.drawCentreString(item.title, _width / 2, _height - 36, 2);
  gfx.fillRoundRect(_width / 2 - 24, _height - 14, 48, 2, 1,
                    scaleColor565(item.color, 180));
}

void CoverFlowMenu::drawPageDots(TFT_eSPI& gfx) {
  if (_itemCount == 0) {
    return;
  }

  uint8_t active = activeIndexForLabel();
  int16_t spacing = 8;
  int16_t startX = _width / 2 - ((_itemCount - 1) * spacing) / 2;
  int16_t y = _height - 6;

  for (uint8_t i = 0; i < _itemCount; ++i) {
    uint16_t color = i == active ? scaleColor565(_items[i].color, 210) : kMuted;
    gfx.fillCircle(startX + i * spacing, y, i == active ? 2 : 1, color);
  }
}

CoverFlowMenu::ItemPose CoverFlowMenu::makePose(uint8_t index) const {
  int16_t progressQ8 =
      _isAnimating ? static_cast<int16_t>(_animationProgress * 256.0f + 0.5f)
                   : 0;
  int16_t offsetQ8 =
      signedDistanceToSelected(index) * 256 -
      static_cast<int16_t>(_transitionDirection) * progressQ8;
  int16_t absOffsetQ8 = min16(abs16(offsetQ8), 768);
  int16_t clampedAbs = min16(absOffsetQ8, 512);
  int16_t nearQ8 = min16(clampedAbs, 256);
  int16_t farQ8 = max16(0, clampedAbs - 256);
  int8_t sign = offsetQ8 < 0 ? -1 : 1;
  int16_t xOffset = (nearQ8 * 56 + farQ8 * 36) / 256;
  int16_t scaleYQ8 = interpolateByDistance(clampedAbs, 350, 220, 160);
  int16_t squeezeQ8 = interpolateByDistance(clampedAbs, 0, 34, 48);
  int16_t scaleXQ8 = max16(120, scaleYQ8 - squeezeQ8);
  int16_t skewQ8 = 0;

  if (clampedAbs > 24) {
    skewQ8 = sign * interpolateByDistance(clampedAbs, 0, 42, 58);
  }

  uint8_t brightness =
      static_cast<uint8_t>(interpolateByDistance(clampedAbs, 255, 154, 82));
  int16_t iconH =
      (static_cast<int32_t>(_items[index].iconHeight) * scaleYQ8) / 256;
  int16_t centerY = 55 + (clampedAbs * 5) / 512;

  ItemPose pose;
  pose.index = index;
  pose.offsetQ8 = offsetQ8;
  pose.absOffsetQ8 = absOffsetQ8;
  pose.centerX = _width / 2 + sign * xOffset;
  pose.topY = centerY - iconH / 2;
  pose.scaleXQ8 = scaleXQ8;
  pose.scaleYQ8 = scaleYQ8;
  pose.skewQ8 = skewQ8;
  pose.brightness = brightness;
  return pose;
}

int16_t CoverFlowMenu::signedDistanceToSelected(uint8_t index) const {
  if (_itemCount == 0) {
    return 0;
  }

  int16_t forward = index >= _selectedIndex
                        ? index - _selectedIndex
                        : _itemCount - _selectedIndex + index;
  if (forward > _itemCount / 2) {
    return forward - _itemCount;
  }

  return forward;
}

uint8_t CoverFlowMenu::activeIndexForLabel() const {
  if (_isAnimating && _animationProgress >= 0.5f) {
    return _targetIndex;
  }

  return _selectedIndex;
}
