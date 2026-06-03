#ifndef COVER_FLOW_MENU_H
#define COVER_FLOW_MENU_H

#include <Arduino.h>
#include <M5StickCPlus.h>

struct MenuItem;

typedef void (*MenuItemAction)(const MenuItem& item);

struct MenuItem {
  uint8_t id;
  const char* title;
  const uint32_t* icon;
  uint8_t iconWidth;
  uint8_t iconHeight;
  uint16_t color;
  MenuItemAction callback;
};

struct CoverFlowInputState {
  bool nextPressed;
  bool previousPressed;
  bool selectPressed;
  int8_t tiltDirection;
};

class CoverFlowMenu {
 public:
  CoverFlowMenu(const MenuItem* items, uint8_t itemCount);
  ~CoverFlowMenu();

  CoverFlowMenu(const CoverFlowMenu&) = delete;
  CoverFlowMenu& operator=(const CoverFlowMenu&) = delete;

  bool begin(TFT_eSPI& display, int16_t width, int16_t height);
  void update(const CoverFlowInputState& inputState, uint32_t deltaMs);
  void draw(TFT_eSPI& display);

  void next();
  void previous();
  void select();

  uint8_t selectedIndex() const;
  float animationProgress() const;
  bool isAnimating() const;

 private:
  struct ItemPose {
    uint8_t index;
    int16_t offsetQ8;
    int16_t absOffsetQ8;
    int16_t centerX;
    int16_t topY;
    int16_t scaleXQ8;
    int16_t scaleYQ8;
    int16_t skewQ8;
    uint8_t brightness;
  };

  void startTransition(int8_t direction);
  void updateAnimation(uint32_t deltaMs);
  void drawScene(TFT_eSPI& gfx);
  void drawItem(TFT_eSPI& gfx, const ItemPose& pose);
  void drawIcon(TFT_eSPI& gfx, const MenuItem& item, int16_t centerX,
                int16_t topY, int16_t scaleXQ8, int16_t scaleYQ8,
                int16_t skewQ8, uint16_t color, bool reflected,
                uint8_t maxRows);
  void drawCoverBack(TFT_eSPI& gfx, const ItemPose& pose, int16_t width,
                     int16_t height, uint16_t color);
  void drawSelectedLabel(TFT_eSPI& gfx);
  void drawPageDots(TFT_eSPI& gfx);
  ItemPose makePose(uint8_t index) const;
  int16_t signedDistanceToSelected(uint8_t index) const;
  uint8_t activeIndexForLabel() const;

  const MenuItem* _items;
  uint8_t _itemCount;
  uint8_t _selectedIndex;
  uint8_t _targetIndex;
  int8_t _transitionDirection;
  uint16_t _transitionMs;
  uint16_t _animationElapsedMs;
  float _animationProgress;
  bool _isAnimating;
  int16_t _width;
  int16_t _height;
  bool _frameReady;
  TFT_eSprite* _frame;
  alignas(TFT_eSprite) uint8_t _frameStorage[sizeof(TFT_eSprite)];
};

#endif
