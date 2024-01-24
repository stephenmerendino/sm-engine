#pragma once

class Window;

class DX12Renderer
{
public:
	DX12Renderer();

	virtual void Init(Window* pWindow) final;
	virtual void Shutdown() final;
};
