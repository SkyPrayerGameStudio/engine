#include "EditorScene.h"
#include "core/Common.h"
#include "core/Var.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedScissor.h"
#include "video/ScopedFrameBuffer.h"
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QB2Format.h"
#include "voxel/model/MeshExporter.h"
#include "ui/UIApp.h"
#include "Model.h"

static inline EditorModel& m() {
	static EditorModel editorModel;
	return editorModel;
}

#define VOXELIZER_IMPLEMENTATION
#include "voxelizer.h"

EditorScene::EditorScene() :
		Super(),
		_bitmap((tb::UIRendererGL*) tb::g_renderer) {
	//_rawVolumeRenderer.setAmbientColor(core::Color::White.xyz());
	SetIsFocusable(true);
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	_frameBuffer.shutdown();
	EditorModel& mdl = m();
	mdl.shutdown();
}

void EditorScene::newVolume() {
	const EditorModel& mdl = m();
	const voxel::Region region(glm::ivec3(0), glm::ivec3(mdl.size()));
	setNewVolume(new voxel::RawVolume(region));
}

void EditorScene::addReference(EditorScene* ref) {
	_references.push_back(ref);
	ref->_parent = this;
	ref->resetCamera();
}

void EditorScene::setNewVolume(voxel::RawVolume *volume) {
	EditorModel& mdl = m();
	mdl.setNewVolume(volume);
	resetCamera();
}

void EditorScene::render() {
	core_trace_scoped(EditorSceneRender);
	EditorModel& mdl = m();
	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		mdl.render(_camera);
	}
	{
		video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame);
		mdl.render(_camera);
	}
	if (mdl.renderAxis()) {
		_axis.render(_camera);
	}
}

void EditorScene::setKeyAction(Action action) {
	EditorModel& mdl = m();
	if (action == mdl.keyAction()) {
		return;
	}
	mdl._keyAction = action;
}

void EditorScene::setInternalAction(Action action) {
	EditorModel& mdl = m();
	if (action == mdl.action()) {
		return;
	}
	mdl.setAction(action);
	switch (action) {
	case Action::None:
		Log::debug("Action: None");
		break;
	case Action::PlaceVoxel:
		Log::debug("Action: PlaceVoxel");
		break;
	case Action::CopyVoxel:
		Log::debug("Action: CopyVoxel");
		break;
	case Action::DeleteVoxel:
		Log::debug("Action: DeleteVoxel");
		break;
	case Action::OverrideVoxel:
		Log::debug("Action: OverrideVoxel");
		break;
	case Action::SelectVoxels:
		Log::debug("Action: SelectVoxel");
		break;
	}
}

void EditorScene::setAction(Action action) {
	EditorModel& mdl = m();
	mdl._uiAction = action;
}

void EditorScene::setSelectionType(SelectType type) {
	EditorModel& mdl = m();
	mdl._selectionType = type;
}

SelectType EditorScene::selectionType() const {
	const EditorModel& mdl = m();
	return mdl._selectionType;
}

bool EditorScene::newModel(bool force) {
	core_trace_scoped(EditorSceneNewModel);
	EditorModel& mdl = m();
	if (mdl.dirty() && !force) {
		return false;
	}
	mdl._dirty = false;
	newVolume();
	mdl._result = voxel::PickResult();
	mdl._extract = true;
	mdl.resetLastTrace();
	return true;
}

bool EditorScene::saveModel(std::string_view file) {
	core_trace_scoped(EditorSceneSaveModel);
	EditorModel& mdl = m();
	if (!mdl.dirty()) {
		// nothing to save yet
		return true;
	}
	if (mdl.modelVolume() == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	voxel::VoxFormat f;
	if (f.save(mdl.modelVolume(), filePtr)) {
		mdl._dirty = false;
	}
	return !mdl.dirty();
}

bool EditorScene::voxelizeModel(const video::MeshPtr& meshPtr) {
	const video::Mesh::Vertices& positions = meshPtr->vertices();
	const video::Mesh::Indices& indices = meshPtr->indices();
	vx_mesh_t* mesh = vx_mesh_alloc(positions.size(), indices.size());

	for (size_t f = 0; f < indices.size(); f++) {
		mesh->indices[f] = indices[f];
	}

	for (size_t v = 0; v < positions.size() / 3; v++) {
		const core::Vertex& vertex = positions[v];
		mesh->vertices[v].x = vertex._pos.x;
		mesh->vertices[v].y = vertex._pos.y;
		mesh->vertices[v].z = vertex._pos.z;
	}

	const glm::vec3& maxs = meshPtr->maxs();
	const EditorModel& mdl = m();
	const float size = mdl.size();
	const glm::vec3& scale = maxs / size;
	const float precision = scale.x / 10.0f;

	vx_mesh_t* result = vx_voxelize(mesh, scale.x, scale.y, scale.z, precision);

	Log::info("Number of vertices: %i", (int)result->nvertices);
	Log::info("Number of indices: %i", (int)result->nindices);

	vx_mesh_free(result);
	vx_mesh_free(mesh);
	return false;
}

bool EditorScene::isEmpty() const {
	const EditorModel& mdl = m();
	return mdl.empty();
}

bool EditorScene::exportModel(std::string_view file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}

	const EditorModel& mdl = m();
	return voxel::exportMesh(mdl.rawVolumeRenderer().mesh(), filePtr->getName().c_str());
}

bool EditorScene::loadModel(std::string_view file) {
	core_trace_scoped(EditorSceneLoadModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		Log::error("Failed to open model file %s", file.data());
		return false;
	}
	voxel::VoxFormat f;
	voxel::RawVolume* newVolume = f.load(filePtr);
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.data());
		return false;
	}
	Log::info("Loaded model file %s", file.data());
	setNewVolume(newVolume);
	return true;
}

void EditorScene::resetCamera() {
	for (EditorScene* ref : _references) {
		ref->resetCamera();
	}
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	const EditorModel& mdl = m();
	const voxel::RawVolume* volume = mdl.modelVolume();
	if (volume == nullptr) {
		return;
	}
	const voxel::Region& region = volume->getEnclosingRegion();
	const glm::ivec3& center = region.getCentre();
	if (_camMode == SceneCameraMode::Free) {
		_camera.setPosition(glm::vec3(-center));
		_camera.lookAt(glm::vec3(0.0001f));
	} else if (_camMode == SceneCameraMode::Top) {
		_camera.setPosition(glm::vec3(center.x, region.getHeightInCells() + center.y, center.z));
		_camera.lookAt(glm::down);
	} else if (_camMode == SceneCameraMode::Left) {
		_camera.setPosition(glm::vec3(region.getWidthInCells() + center.x, center.y, center.z));
		_camera.lookAt(glm::right);
	} else if (_camMode == SceneCameraMode::Front) {
		_camera.setPosition(glm::vec3(center.x, center.y, region.getDepthInCells() + center.z));
		_camera.lookAt(glm::backward);
	}
}

void EditorScene::setVoxelType(voxel::VoxelType type) {
	EditorModel& mdl = m();
	mdl.setVoxelType(type);
}

bool EditorScene::OnEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
	const int x = ev.target_x;
	const int y = ev.target_y;
	ui::UIRect rect = GetRect();
	ConvertToRoot(rect.x, rect.y);
	const int tx = x + rect.x;
	const int ty = y + rect.y;
	const long now = core::App::getInstance()->currentMillis();
	EditorModel& mdl = m();
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		_mouseDown = true;
		if (mdl.keyAction() != Action::None) {
			setInternalAction(mdl.keyAction());
		} else {
			setInternalAction(mdl.uiAction());
		}
		mdl.executeAction(tx, ty, _mouseDown, now);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		_mouseDown = false;
		setInternalAction(Action::None);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.modifierkeys) {
			if (ev.modifierkeys & tb::TB_ALT) {
				setKeyAction(Action::CopyVoxel);
			} else if (ev.modifierkeys & tb::TB_SHIFT) {
				setKeyAction(Action::OverrideVoxel);
			} else if (ev.modifierkeys & tb::TB_CTRL) {
				setKeyAction(Action::DeleteVoxel);
			}
			if (_mouseDown) {
				setInternalAction(mdl.keyAction());
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_UP) {
		if (ev.modifierkeys && mdl.keyAction() != Action::None) {
			mdl._keyAction = Action::None;
			if (_mouseDown) {
				setInternalAction(mdl.uiAction());
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_WHEEL && ev.delta_y != 0) {
		const glm::vec3& moveDelta = glm::backward * mdl._cameraSpeed * (float)(ev.delta_y * 100);
		_camera.move(moveDelta);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = mdl.action() == Action::None && (ev.modifierkeys & tb::TB_ALT);
		if (relative || middle || alt) {
			const float yaw = x - _mouseX;
			const float pitch = y - _mouseY;
			const float s = _rotationSpeed->floatVal();
			if (_camMode == SceneCameraMode::Free) {
				_camera.turn(yaw * s);
				_camera.pitch(pitch * s);
			}
			_mouseX = x;
			_mouseY = y;
			return true;
		}
		_mouseX = x;
		_mouseY = y;
		mdl.executeAction(tx, ty, _mouseDown, now);
		return true;
	}
	return Super::OnEvent(ev);
}

void EditorScene::select(const glm::ivec3& pos) {
	m().select(pos);
}

void EditorScene::OnFocusChanged(bool focused) {
	Super::OnFocusChanged(focused);
}

void EditorScene::OnResized(int oldw, int oldh) {
	core_trace_scoped(EditorSceneOnResized);
	Super::OnResized(oldw, oldh);
	const tb::TBRect& rect = GetRect();
	const glm::ivec2 pos(0, 0);
	const glm::ivec2 dim(rect.w, rect.h);
	_camera.init(pos, dim);
	_frameBuffer.shutdown();
	_frameBuffer.init(dim);
	_bitmap.Init(dim.x, dim.y, _frameBuffer.texture());
	EditorModel& mdl = m();
	mdl.onResize(pos, dim);
}

void EditorScene::OnPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
	Super::OnPaint(paintProps);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::UIRect rect = GetRect();
	// the fbo is flipped in memory, we have to deal with it here
	const tb::TBRect srcRect(0, dimension.y, rect.w, -rect.h);
	tb::g_renderer->DrawBitmap(rect, srcRect, &_bitmap);
}

void EditorScene::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);
	_axis.init();

	const char *cameraMode = info.node->GetValueString("camera", "free");
	if (!strcmp(cameraMode, "top")) {
		_camMode = SceneCameraMode::Top;
		_camera.setMode(video::CameraMode::Orthogonal);
	} else if (!strcmp(cameraMode, "front")) {
		_camMode = SceneCameraMode::Front;
		_camera.setMode(video::CameraMode::Orthogonal);
	} else if (!strcmp(cameraMode, "left")) {
		_camMode = SceneCameraMode::Left;
		_camera.setMode(video::CameraMode::Orthogonal);
	} else {
		_camMode = SceneCameraMode::Free;
		_camera.setMode(video::CameraMode::Perspective);
	}

	EditorModel& mdl = m();
	mdl.init();

	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	resetCamera();
}

void EditorScene::OnProcess() {
	Super::OnProcess();
	core_trace_scoped(EditorSceneOnProcess);
	EditorModel& mdl = m();
	const long deltaFrame = core::App::getInstance()->deltaFrame();
	const float speed = mdl._cameraSpeed * static_cast<float>(deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, mdl._moveMask);
	_camera.move(moveDelta);
	_camera.update(deltaFrame);
	if (mdl.modelVolume() == nullptr) {
		return;
	}
	mdl.setAngle(mdl.angle() + deltaFrame * 0.001f);
	const float angle = mdl.angle();
	const glm::vec3 direction(glm::sin(angle), 0.5f, glm::cos(angle));
	mdl.rawVolumeRenderer().setSunDirection(direction);
	const bool skipCursor = isRelativeMouseMode();
	mdl.trace(_mouseX, _mouseY, skipCursor, _camera);

	glClearColor(core::Color::Clear.r, core::Color::Clear.g, core::Color::Clear.b, core::Color::Clear.a);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(false);
	render();
	_frameBuffer.unbind();
}

bool EditorScene::renderAABB() const {
	const EditorModel& mdl = m();
	return mdl.rawVolumeRenderer().renderAABB();
}

void EditorScene::setRenderAABB(bool renderAABB) {
	EditorModel& mdl = m();
	mdl.rawVolumeRenderer().setRenderAABB(renderAABB);
}

bool EditorScene::renderGrid() const {
	const EditorModel& mdl = m();
	return mdl.rawVolumeRenderer().renderGrid();
}

void EditorScene::setRenderGrid(bool renderGrid) {
	EditorModel& mdl = m();
	mdl.rawVolumeRenderer().setRenderGrid(renderGrid);
}

inline long EditorScene::actionExecutionDelay() const {
	const EditorModel& mdl = m();
	return mdl._actionExecutionDelay;
}

void EditorScene::setActionExecutionDelay(long actionExecutionDelay) {
	EditorModel& mdl = m();
	mdl._actionExecutionDelay = actionExecutionDelay;
}

bool EditorScene::renderAxis() const {
	const EditorModel& mdl = m();
	return mdl._renderAxis;
}

void EditorScene::setRenderAxis(bool renderAxis) {
	EditorModel& mdl = m();
	mdl._renderAxis = renderAxis;
}

float EditorScene::cameraSpeed() const {
	const EditorModel& mdl = m();
	return mdl._cameraSpeed;
}

void EditorScene::setCameraSpeed(float cameraSpeed) {
	EditorModel& mdl = m();
	mdl._cameraSpeed = cameraSpeed;
}

bool EditorScene::isDirty() const {
	const EditorModel& mdl = m();
	return mdl.dirty();
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
