#include "ShapeTool.h"
#include "sauce/ShapeToolInjector.h"
#include "core/App.h"
#include "core/Process.h"
#include "core/Command.h"
#include "voxel/Spiral.h"
#include "video/Shader.h"
#include "video/Color.h"
#include "video/GLDebug.h"

constexpr int MOVERIGHT		=	1 << 0;
constexpr int MOVELEFT		=	1 << 1;
constexpr int MOVEFORWARD	=	1 << 2;
constexpr int MOVEBACKWARD	=	1 << 3;

#define registerMoveCmd(name, flag) \
	core::Command::registerCommand(name, [&] (const core::CmdArgs& args) { \
		if (args.empty()) { \
			return; \
		} \
		if (args[0] == "true") \
			_moveMask |= (flag); \
		else \
			_moveMask &= ~(flag); \
	});

// tool for testing the world createXXX functions without starting the application
ShapeTool::ShapeTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world) :
		ui::UIApp(filesystem, eventBus), _worldRenderer(world), _world(world) {
	init("engine", "shapetool");
}

ShapeTool::~ShapeTool() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

core::AppState ShapeTool::onInit() {
	GLDebug::enable(GLDebug::Medium);

	if (!_worldShader.init()) {
		return core::Cleanup;
	}

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	_world->setSeed(1);
	_worldRenderer.onInit();
	_camera.init(_width, _height);
	_camera.setAngles(-M_PI_2, M_PI);
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));

	_clearColor = video::Color::LightBlue;

	// TODO: replace this with a scripting interface for the World::create* functions
	_worldRenderer.onSpawn(_camera.getPosition());

	return ui::UIApp::onInit();
}

core::AppState ShapeTool::onRunning() {
	core::AppState state = UIApp::onRunning();

	_world->onFrame(_deltaFrame);

	if (_resetTriggered && !_world->isReset()) {
		// todo get the value from 'somewhere' (tm)
		voxel::World::WorldContext ctx;
		ctx.landscapeNoiseOctaves = 1;
		ctx.mountainNoiseOctaves = 1;

		_world->setContext(ctx);
		_worldRenderer.onSpawn(_camera.getPosition());
		_resetTriggered = false;
	}

	const bool left = _moveMask & MOVELEFT;
	const bool right = _moveMask & MOVERIGHT;
	const bool forward = _moveMask & MOVEFORWARD;
	const bool backward = _moveMask & MOVEBACKWARD;
	_camera.updatePosition(_deltaFrame, left, right, forward, backward);
	_camera.updateViewMatrix();

	_worldRenderer.onRunning(_now);

	const glm::mat4& view = _camera.getViewMatrix();
	_worldRenderer.renderWorld(_worldShader, view, _aspect);

	return state;
}

core::AppState ShapeTool::onCleanup() {
	_worldRenderer.onCleanup();
	core::AppState state = UIApp::onCleanup();
	_world->destroy();
	return state;
}

void ShapeTool::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	_camera.onMotion(x, y, relX, relY);
}

bool ShapeTool::onKeyPress(int32_t key, int16_t modifier) {
	const bool handled = UIApp::onKeyPress(key, modifier);
	if (handled) {
		return true;
	}

	if (key == SDLK_r) {
		_worldRenderer.reset();
		_world->reset();
		_resetTriggered = true;
		return true;
	}

	return false;
}

int main(int argc, char *argv[]) {
	return getInjector()->get<ShapeTool>()->startMainLoop(argc, argv);
}
