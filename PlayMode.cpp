#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > factory_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("factory.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > factory_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("factory.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = factory_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});


Load< Sound::Sample > good_song_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("GoodSong.wav"));
});

Load< Sound::Sample > bad_song_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("BadSong.wav"));
	});

PlayMode::PlayMode() : scene(*factory_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Body") player = &transform;
		else if (transform.name == "Wheel_FR") wheel_fr = &transform;
		else if (transform.name == "Wheel_FL") wheel_fl = &transform;
		else if (transform.name == "Wheel_BR") wheel_br = &transform;
		else if (transform.name == "Wheel_BL") wheel_bl = &transform;
		else if (transform.name == "Searchlight_L") searchlight_l = &transform;
		else if (transform.name == "Searchlight_R") searchlight_r = &transform;
	}
	if (player == nullptr) throw std::runtime_error("player not found.");
	if (wheel_fr == nullptr) throw std::runtime_error("wheel_fr not found.");
	if (wheel_fl == nullptr) throw std::runtime_error("wheel_fl not found.");
	if (wheel_br == nullptr) throw std::runtime_error("wheel_br not found.");
	if (wheel_fl == nullptr) throw std::runtime_error("wheel_bl not found.");
	if (searchlight_l == nullptr) throw std::runtime_error("Left light not found.");
	if (searchlight_r == nullptr) throw std::runtime_error("Right light not found.");

	searchlight_l_original_rotation = searchlight_l->rotation;
	searchlight_r_original_rotation = searchlight_r->rotation;
	


	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	search_l_offset = searchlight_l->position - camera->transform->position;
	search_r_offset = searchlight_r->position - camera->transform->position;

	//start music loop playing:
	// (note: position will be over-ridden in update())
	good_loop = Sound::loop(*good_song_sample, 1.0f, 10.0f);
	bad_loop = Sound::loop(*bad_song_sample, 0.0f, 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEMOTION) {
		/*
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}*/
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (gameLost)
	{
		//Update sound changing
		good_loop->set_volume(0.f);
		bad_loop->set_volume(1.f);
	}
	else if (gameWon)
	{
		good_loop->set_volume(1.f);
		bad_loop->set_volume(0.f);

		//Move player offscreen in victory!
		player->position.y += PlayerSpeed * elapsed;
	}
	else
	{

		secondsTillNextChange -= elapsed;

		if (isSafe)
		{
			if (secondsTillNextChange <= 0)
			{
				secondsTillNextChange = 0;
			}

			if (secondsTillNextChange < timeToLerp)
			{
				lightTrackingLerp = 1.1f - (secondsTillNextChange / timeToLerp);
			}
			else
			{
				//Lerp back?
				lightTrackingLerp -= elapsed / timeToLerp;
				if (lightTrackingLerp <= 0)
				{
					lightTrackingLerp = 0;
				}
			}

			if (secondsTillNextChange < musicTimeToLerp)
			{
				musicTrackingLerp = 1.0f - (secondsTillNextChange / musicTimeToLerp);
			}
			else
			{
				musicTrackingLerp -= elapsed / timeToLerp;
				if (musicTrackingLerp <= 0)
				{
					musicTrackingLerp = 0;
				}
			}

			if (secondsTillNextChange <= 0)
			{
				isSafe = false;
				secondsTillNextChange = durationOfBad;
			}
		}
		else
		{
			if (secondsTillNextChange <= 0)
			{
				isSafe = true;
				secondsTillNextChange = (float)((rand() % (maxSecondsBetweenChange - minSecondsBetweenChange)) + minSecondsBetweenChange);
			}
		}

		//Update sound changing
		good_loop->set_volume(1.f - musicTrackingLerp);
		bad_loop->set_volume(musicTrackingLerp);

		//slowly rotates through [0,1):
		wobble += elapsed / 10.0f;
		wobble2 += elapsed / 8.f;
		wobble -= std::floor(wobble);
		wobble2 -= std::floor(wobble2);

		{
			glm::quat searchlight_l_rand = searchlight_l_original_rotation * glm::angleAxis(
				glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
				glm::vec3(1.0f, 1.0f, 0.0f)
			);

			glm::quat searchlight_r_rand = searchlight_r_original_rotation * glm::angleAxis(
				glm::radians(5.0f * std::sin(wobble2 * 2.0f * float(M_PI))),
				glm::vec3(1.0f, 1.0f, 0.0f)
			);

			glm::quat searchlight_l_look = point_at(searchlight_l->position, player->position);
			glm::quat searchlight_r_look = point_at(searchlight_r->position, player->position);

			searchlight_l->rotation = glm::mix(searchlight_l_rand, searchlight_l_look, lightTrackingLerp);

			searchlight_r->rotation = glm::mix(searchlight_r_rand, searchlight_r_look, lightTrackingLerp);

			searchlight_l->position = camera->transform->position + search_l_offset;
			searchlight_r->position = camera->transform->position + search_r_offset;
		}

		/*hip->rotation = hip_base_rotation * glm::angleAxis(
			glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
			glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
			glm::vec3(0.0f, 0.0f, 1.0f)
		);
		lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
			glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
			glm::vec3(0.0f, 0.0f, 1.0f)
		);*/

		//move sound to follow leg tip position:
		//leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

		//move camera:
		{

			//combine inputs into a move:
			float wheelHeight = .3f;

			float wheelCircumference = 2 * wheelHeight * float(M_PI);

			glm::vec2 move = glm::vec2(0.0f);
			if (left.pressed && !right.pressed) move.x = -1.0f;
			if (!left.pressed && right.pressed) move.x = 1.0f;
			if (down.pressed && !up.pressed) move.y = -1.0f;
			if (!down.pressed && up.pressed) move.y = 1.0f;

			//make it so that moving diagonally doesn't go faster:
			//if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

			if (space.pressed)
			{
				if (!isSafe)
				{
					gameLost = true;
				}

				float dist = PlayerSpeed * elapsed;
				player->position.y += PlayerSpeed * elapsed;
				camera->transform->position.y += PlayerSpeed * elapsed;
				if (camera->transform->position.y > maxY)
				{
					camera->transform->position.y = maxY;
				}

				if (player->position.y > winY && !gameLost)
				{
					gameWon = true;
				}
			}

			/*
			glm::mat4x3 frame = camera->transform->make_local_to_parent();
			glm::vec3 right = frame[0];
			//glm::vec3 up = frame[1];
			glm::vec3 forward = -frame[2];

			camera->transform->position += move.x * right + move.y * forward;
			*/
		}

		{ //update listener to camera position:
			glm::mat4x3 frame = camera->transform->make_local_to_parent();
			glm::vec3 right = frame[0];
			glm::vec3 at = frame[3];
			Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
		}

		//reset button press counters:
		left.downs = 0;
		right.downs = 0;
		up.downs = 0;
		down.downs = 0;
	}
}

void PlayMode::draw(glm::uvec2 const& drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));


		if (gameLost)
		{
			constexpr float H = 0.09f;
			lines.draw_text("You were caught... Game over!",
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			/*float ofs = 2.0f / drawable_size.y;
			lines.draw_text("You were caught... Game over!",
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00)); */
		}
		else if (gameWon)
		{
			constexpr float H = 0.09f;
			lines.draw_text("You have escaped! Congratulations!",
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			/*float ofs = 2.0f / drawable_size.y;
			lines.draw_text("You have escaped! Congratulations!",
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));*/
		}
	}
	GL_ERRORS();
}

glm::quat PlayMode::point_at(glm::vec3 pos, glm::vec3 target)
{
	return glm::quatLookAt(glm::normalize(target - pos), glm::vec3(0.0f, 0.0f, 1.0f));
}
