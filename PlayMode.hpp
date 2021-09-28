#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	virtual glm::quat point_at(glm::vec3 pos, glm::vec3 target);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform* player = nullptr;
	Scene::Transform* wheel_fr = nullptr;
	Scene::Transform* wheel_fl = nullptr;
	Scene::Transform* wheel_br = nullptr;
	Scene::Transform* wheel_bl = nullptr;

	Scene::Transform* searchlight_l = nullptr;
	Scene::Transform* searchlight_r = nullptr;

	glm::quat searchlight_l_original_rotation;
	glm::quat searchlight_r_original_rotation;
	glm::vec3 search_l_offset;
	glm::vec3 search_r_offset;

	float wobble = 0.0f;
	float wobble2 = .5f;

	float maxY = 18.f;

	float winY = 20.f;

	float PlayerSpeed = 1.0f;

	float lightTrackingLerp = 0.f;
	float musicTrackingLerp = 0.f;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > good_loop;
	std::shared_ptr< Sound::PlayingSample > bad_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

	int minSecondsBetweenChange = 6;
	int maxSecondsBetweenChange = 12;

	float secondsTillNextChange = 8.f;
	float musicTimeToLerp = 2.f;
	float timeToLerp = .5f;
	float durationOfBad = 4.f;

	bool isSafe = true;

	bool gameWon = false;
	bool gameLost = false;

};
