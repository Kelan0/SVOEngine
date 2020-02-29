#pragma once

#include "core/pch.h"

#define KEYBOARD_SIZE 256
#define MOUSE_SIZE 16

class InputHandler {
public:
	InputHandler(SDL_Window* windowHandle);

	~InputHandler();

	void update();

	void processEvent(SDL_Event event);

	bool keyDown(uint32_t key);

	bool keyPressed(uint32_t key);

	bool keyReleased(uint32_t key);

	bool mouseDown(uint32_t button);

	bool mousePressed(uint32_t button);

	bool mouseReleased(uint32_t button);

	bool mouseDragged(uint32_t button);

	bool isMouseGrabbed() const;

	void setMouseGrabbed(bool grabbed);

	void toggleMouseGrabbed();

	bool setMousePixelCoord(ivec2 coord);

	bool setMouseScreenCoord(dvec2 coord);

	ivec2 getMousePixelCoord();

	ivec2 getLastMousePixelCoord();

	dvec2 getMouseScreenCoord();

	dvec2 getLastMouseScreenCoord();

	ivec2 getMousePixelMotion();

	ivec2 getLastMousePixelMotion();

	ivec2 getRelativeMouseState();

	dvec2 getMouseScreenMotion();

	dvec2 getLastMouseScreenMotion();

	ivec2 getMouseDragPixelOrigin(uint32_t button);

	ivec2 getMouseDragPixelDistance(uint32_t button);

	dvec2 getMouseDragScreenOrigin(uint32_t button);

	dvec2 getMouseDragScreenDistance(uint32_t button);

private:
	SDL_Window* m_windowHandle;

	bool m_keysDown[KEYBOARD_SIZE];
	bool m_keysPressed[KEYBOARD_SIZE];
	bool m_keysReleased[KEYBOARD_SIZE];
	bool m_mouseDown[MOUSE_SIZE];
	bool m_mousePressed[MOUSE_SIZE];
	bool m_mouseReleased[MOUSE_SIZE];
	bool m_mouseDragged[MOUSE_SIZE];
	ivec2 m_mousePressPixelCoord[MOUSE_SIZE];
	ivec2 m_mouseDragPixelOrigin[MOUSE_SIZE];
	ivec2 m_currMousePixelCoord;
	ivec2 m_prevMousePixelCoord;
	ivec2 m_currMousePixelMotion;
	ivec2 m_prevMousePixelMotion;
	bool m_mouseGrabbed;
};

