#include "black-border.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-auto-crop", "en-US")

namespace {

constexpr uint32_t max_capture_size = 1280;

struct Selection {
	obs_sceneitem_t *item = nullptr;
	int count = 0;
};

bool collect_selection(obs_scene_t *, obs_sceneitem_t *item, void *parameter)
{
	auto *selection = static_cast<Selection *>(parameter);
	if (obs_sceneitem_selected(item)) {
		if (++selection->count == 1)
			selection->item = item;
	}
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, collect_selection, parameter);
	return true;
}

obs_source_t *get_edit_scene()
{
	return obs_frontend_preview_program_mode_active() ? obs_frontend_get_current_preview_scene()
							 : obs_frontend_get_current_scene();
}

bool scene_contains_item(obs_scene_t *scene, obs_sceneitem_t *item)
{
	struct Search {
		obs_sceneitem_t *wanted;
		bool found;
	} search{item, false};
	obs_scene_enum_items(
		scene,
		[](obs_scene_t *, obs_sceneitem_t *candidate, void *parameter) {
			auto *search = static_cast<Search *>(parameter);
			if (candidate == search->wanted) {
				search->found = true;
				return false;
			}
			return true;
		},
		&search);
	return search.found;
}

struct CaptureJob {
	obs_sceneitem_t *item = nullptr;
	obs_source_t *source = nullptr;
	obs_source_t *scene_source = nullptr;
	obs_source_t *item_scene_source = nullptr;
	uint32_t source_width = 0;
	uint32_t source_height = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	gs_texrender_t *render = nullptr;
	gs_stagesurf_t *surface = nullptr;
	std::vector<uint8_t> pixels;
	bool success = false;
	int stage = 0;
};

void destroy_graphics(CaptureJob *job)
{
	obs_enter_graphics();
	gs_stagesurface_destroy(job->surface);
	gs_texrender_destroy(job->render);
	obs_leave_graphics();
	job->surface = nullptr;
	job->render = nullptr;
}

class AutoCropPlugin final : public QObject {
public:
	AutoCropPlugin()
	{
		qApp->installEventFilter(this);
		if (auto *window = static_cast<QWidget *>(obs_frontend_get_main_window())) {
			main_window = window;
			for (QMenu *menu : window->findChildren<QMenu *>("transformMenu"))
				attach_menu(menu);
		}
	}

	~AutoCropPlugin() override
	{
		qApp->removeEventFilter(this);
		delete action;
		if (job) {
			obs_remove_tick_callback(capture_tick, this);
			destroy_graphics(job);
			release_job();
		}
	}

protected:
	bool eventFilter(QObject *object, QEvent *event) override
	{
		if (event->type() == QEvent::Show && object->objectName() == "transformMenu") {
			if (auto *menu = qobject_cast<QMenu *>(object)) {
				attach_menu(menu);
				update_enabled();
			}
		}
		return QObject::eventFilter(object, event);
	}

private:
	QPointer<QWidget> main_window;
	QPointer<QAction> action;
	CaptureJob *job = nullptr;

	void attach_menu(QMenu *menu)
	{
		if (action && menu->actions().contains(action))
			return;
		if (action)
			delete action;

		action = new QAction(QString::fromUtf8(obs_module_text("AutoCrop")), menu);
		action->setObjectName("obsAutoCropAction");
		QObject::connect(action, &QAction::triggered, this, [this] { start_capture(); });

		const auto actions = menu->actions();
		for (int i = 0; i < actions.size(); ++i) {
			if (actions[i]->objectName() == "actionResetTransform") {
				menu->insertAction(i + 1 < actions.size() ? actions[i + 1] : nullptr, action);
				blog(LOG_INFO, "OBS Auto Crop added to Transform menu");
				return;
			}
		}
		menu->addAction(action);
		blog(LOG_INFO, "OBS Auto Crop added to Transform menu");
	}

	void update_enabled()
	{
		if (!action)
			return;
		action->setText(QString::fromUtf8(obs_module_text("AutoCrop")));
		obs_source_t *scene_source = get_edit_scene();
		Selection selection;
		if (scene_source) {
			if (obs_scene_t *scene = obs_scene_from_source(scene_source))
				obs_scene_enum_items(scene, collect_selection, &selection);
		}
		obs_source_t *source = selection.item ? obs_sceneitem_get_source(selection.item) : nullptr;
		const bool supported = selection.count == 1 && source && !obs_sceneitem_is_group(selection.item) &&
				       !obs_sceneitem_locked(selection.item) &&
				       (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) &&
				       obs_source_get_width(source) >= 4 && obs_source_get_height(source) >= 4;
		action->setEnabled(supported && !job);
		obs_source_release(scene_source);
	}

	void start_capture()
	{
		if (job)
			return;
		obs_source_t *scene_source = get_edit_scene();
		if (!scene_source)
			return;
		obs_scene_t *scene = obs_scene_from_source(scene_source);
		Selection selection;
		if (scene)
			obs_scene_enum_items(scene, collect_selection, &selection);
		if (selection.count != 1 || !selection.item || obs_sceneitem_is_group(selection.item) ||
		    obs_sceneitem_locked(selection.item)) {
			obs_source_release(scene_source);
			return;
		}
		obs_source_t *source = obs_sceneitem_get_source(selection.item);
		const uint32_t source_width = obs_source_get_width(source);
		const uint32_t source_height = obs_source_get_height(source);
		if (!(obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) || source_width < 4 || source_height < 4) {
			obs_source_release(scene_source);
			return;
		}

		job = new CaptureJob;
		job->item = selection.item;
		obs_sceneitem_addref(job->item);
		job->source = obs_source_get_ref(source);
		obs_scene_t *item_scene = obs_sceneitem_get_scene(selection.item);
		job->scene_source = scene_source;
		job->item_scene_source = obs_source_get_ref(obs_scene_get_source(item_scene));
		job->source_width = source_width;
		job->source_height = source_height;
		const uint32_t largest = std::max(source_width, source_height);
		job->width = std::max(4u, static_cast<uint32_t>(static_cast<uint64_t>(source_width) *
									std::min(largest, max_capture_size) / largest));
		job->height = std::max(4u, static_cast<uint32_t>(static_cast<uint64_t>(source_height) *
									 std::min(largest, max_capture_size) / largest));
		obs_source_inc_showing(job->source);
		obs_add_tick_callback(capture_tick, this);
		if (action)
			action->setEnabled(false);
	}

	static void capture_tick(void *parameter, float)
	{
		auto *self = static_cast<AutoCropPlugin *>(parameter);
		CaptureJob *capture = self->job;
		if (!capture)
			return;

		obs_enter_graphics();
		if (capture->stage == 0) {
			capture->render = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
			capture->surface = gs_stagesurface_create(capture->width, capture->height, GS_BGRA);
			if (capture->render && capture->surface &&
			    gs_texrender_begin(capture->render, capture->width, capture->height)) {
				vec4 background;
				vec4_zero(&background);
				gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
				gs_viewport_push();
				gs_projection_push();
				gs_set_viewport(0, 0, capture->width, capture->height);
				gs_ortho(0.0f, static_cast<float>(capture->source_width), 0.0f,
					 static_cast<float>(capture->source_height), -100.0f, 100.0f);
				gs_blend_state_push();
				gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
				obs_source_video_render(capture->source);
				gs_blend_state_pop();
				gs_projection_pop();
				gs_viewport_pop();
				gs_texrender_end(capture->render);
				capture->stage = 1;
			} else {
				capture->stage = 3;
			}
		} else if (capture->stage == 1) {
			gs_stage_texture(capture->surface, gs_texrender_get_texture(capture->render));
			capture->stage = 2;
		} else if (capture->stage == 2) {
			uint8_t *mapped = nullptr;
			uint32_t stride = 0;
			if (gs_stagesurface_map(capture->surface, &mapped, &stride) &&
			    stride >= static_cast<uint64_t>(capture->width) * 4) {
				capture->pixels.resize(static_cast<size_t>(capture->width) * capture->height * 4);
				for (uint32_t y = 0; y < capture->height; ++y)
					std::memcpy(capture->pixels.data() + static_cast<size_t>(y) * capture->width * 4,
						    mapped + static_cast<size_t>(y) * stride, capture->width * 4);
				capture->success = true;
				gs_stagesurface_unmap(capture->surface);
			} else if (mapped) {
				gs_stagesurface_unmap(capture->surface);
			}
			capture->stage = 3;
		}
		obs_leave_graphics();

		if (capture->stage == 3) {
			obs_remove_tick_callback(capture_tick, self);
			destroy_graphics(capture);
			capture->stage = 4;
			QMetaObject::invokeMethod(self, [self] { self->capture_finished(); }, Qt::QueuedConnection);
		}
	}

	void capture_finished()
	{
		if (!job)
			return;
		CaptureJob *capture = job;
		if (!capture->success) {
			QMessageBox::warning(main_window, QString::fromUtf8(obs_module_text("AutoCrop")),
					     QString::fromUtf8(obs_module_text("CaptureFailed")));
			release_job();
			return;
		}

		const BlackBorderCrop edges = detect_black_borders(capture->pixels.data(), capture->width,
									 capture->height, capture->width * 4);
		if (!edges.valid) {
			QMessageBox::information(main_window, QString::fromUtf8(obs_module_text("AutoCrop")),
						 QString::fromUtf8(obs_module_text("NoBordersFound")));
			release_job();
			return;
		}

		obs_scene_t *scene = obs_scene_from_source(capture->scene_source);
		obs_scene_t *item_scene = obs_sceneitem_get_scene(capture->item);
		if (scene && item_scene && !obs_sceneitem_locked(capture->item) &&
		    scene_contains_item(item_scene, capture->item)) {
			obs_sceneitem_crop crop{};
			crop.left = static_cast<int>(static_cast<uint64_t>(edges.left) * capture->source_width / capture->width);
			crop.right = static_cast<int>(static_cast<uint64_t>(edges.right) * capture->source_width / capture->width);
			crop.top = static_cast<int>(static_cast<uint64_t>(edges.top) * capture->source_height / capture->height);
			crop.bottom = static_cast<int>(static_cast<uint64_t>(edges.bottom) * capture->source_height / capture->height);
			if (crop.left + crop.right < static_cast<int>(capture->source_width) &&
			    crop.top + crop.bottom < static_cast<int>(capture->source_height)) {
				obs_data_t *before = obs_scene_save_transform_states(scene, true);
				const std::string undo = before ? obs_data_get_json(before) : "";
				obs_data_release(before);
				obs_sceneitem_set_crop(capture->item, &crop);
				obs_data_t *after = obs_scene_save_transform_states(scene, true);
				const std::string redo = after ? obs_data_get_json(after) : "";
				obs_data_release(after);
				if (!undo.empty() && !redo.empty() && undo != redo)
					obs_frontend_add_undo_redo_action(obs_module_text("AutoCrop"), obs_scene_load_transform_states,
									  obs_scene_load_transform_states, undo.c_str(), redo.c_str(),
									  false);
			}
		}
		release_job();
	}

	void release_job()
	{
		CaptureJob *capture = job;
		job = nullptr;
		obs_source_dec_showing(capture->source);
		obs_source_release(capture->source);
		obs_sceneitem_release(capture->item);
		obs_source_release(capture->scene_source);
		obs_source_release(capture->item_scene_source);
		delete capture;
		update_enabled();
	}
};

AutoCropPlugin *plugin = nullptr;

} // namespace

extern "C" bool obs_module_load(void)
{
	plugin = new AutoCropPlugin;
	blog(LOG_INFO, "OBS Auto Crop loaded");
	return true;
}

extern "C" void obs_module_unload(void)
{
	delete plugin;
	plugin = nullptr;
}
