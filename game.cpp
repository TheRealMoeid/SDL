#include "SDL.h"
#include <ctime>
#include <string>

using namespace std;

SDL_Texture* LoadTextureChecked(SDL_Renderer* ren, const char* path) {
	SDL_Texture* tex = IMG_LoadTexture(ren, path);
	if (!tex) {
		SDL_Log("Failed to load texture '%s': %s", path, IMG_GetError());
	}
	return tex;
}


// B6: real rectangle-overlap test, used for spawn placement (and reusable
// for collisions). Replaces the old per-loop checks that only tested
// whether one object's top-left corner fell inside another object's box.
bool Overlaps(const SDL_Rect& a, const SDL_Rect& b) {
	if (a.x + a.w <= b.x || b.x + b.w <= a.x) return false;
	if (a.y + a.h <= b.y || b.y + b.h <= a.y) return false;
	return true;
}

constexpr double HEALTH_GROWTH = 40.0; // px gained per health item eaten
constexpr double FIRE_PENALTY = 40.0;  // px lost per fire item touched

// B10: explicit game state. Replaces the two independent booleans
// gameover/winner, which nothing prevented from both being true at once
// (nothing guarded the collision/eating blocks during an ending) and which
// duplicated the "are we actually playing" check as `!gameover && !winner`
// at every site that needed it.
enum class GameState {
	Playing,
	GameOver,
	Victory
};

// B3: single source of truth for "start a fresh round". Replaces the two
// near-identical restart blocks (Game Over + Win), which reset size,
// position, enemy position and score but left health_counter, fire_visable,
// counter_enemy_direction and enemy_direction untouched. Concrete bug this
// fixes: dying while a fire was on screen used to leave fire_visable true,
// so the very first frame of the new round would spawn another fire
// immediately. Random_Health_i/j and Random_Fire_i/j are not reset here —
// setting sizeHealth/sizeFire to -1 makes the existing per-frame spawn
// checks (B6) pick fresh, validated positions on the next frame, same as
// they already did before.
void ResetGame(
	SDL_Renderer* ren,
	SDL_Texture*& A_Texture, SDL_Texture*& B_Texture, SDL_Texture*& C_Texture,
	SDL_Texture*& Health_Texture, SDL_Texture*& Fire_Texture, SDL_Texture*& Enemy_Texture,
	SDL_Texture*& Background_Texture, SDL_Texture*& GameOver_Texture, SDL_Texture*& Win_Texture,
	int window_size,
	double& size, double& sizeHealth, double& sizeFire,
	int& Character_i, int& Character_j,
	int& Enemy_i, int& Enemy_j,
	int& health_counter, int& score_counter,
	int& counter_enemy_direction, int& enemy_direction,
	GameState& state, bool& fire_visable)
{
	// Freeing textures
	SDL_DestroyTexture(A_Texture);
	SDL_DestroyTexture(B_Texture);
	SDL_DestroyTexture(Health_Texture);
	SDL_DestroyTexture(Fire_Texture);
	SDL_DestroyTexture(Enemy_Texture);
	SDL_DestroyTexture(Background_Texture);
	SDL_DestroyTexture(GameOver_Texture);
	SDL_DestroyTexture(Win_Texture);

	// Re-loading textures
	A_Texture = LoadTextureChecked(ren, "BacheGherti.png");
	B_Texture = LoadTextureChecked(ren, "Hestler.png");
	Health_Texture = LoadTextureChecked(ren, "Health.png");
	Fire_Texture = LoadTextureChecked(ren, "Fire.png");
	Enemy_Texture = LoadTextureChecked(ren, "Enemy.png");
	Background_Texture = LoadTextureChecked(ren, "background.png");
	GameOver_Texture = LoadTextureChecked(ren, "GameOver.png");
	Win_Texture = LoadTextureChecked(ren, "win.png");
	C_Texture = A_Texture; // reset character to original (small) sprite

	// Reset game state
	state = GameState::Playing;
	size = 50;
	sizeHealth = -1;
	sizeFire = -1;
	Character_i = (window_size - (int)size) / 2;
	Character_j = (window_size - (int)size) / 2;
	Enemy_i = 100;
	Enemy_j = 100;
	score_counter = 0;
	health_counter = 0;
	fire_visable = false;
	counter_enemy_direction = 0;
	enemy_direction = 0;
}

int main() {
	int window_size = 700;

	if (SDL__CreateWindow("Square Baby", window_size, window_size) != 0) {
		SDL_Log("Failed to initialize SDL/window: %s", SDL_GetError());
		return 1;
	}
	Uint64 pasttime = SDL_GetTicks();
	//-------------Textures--------------------------
	SDL_Texture* A_Texture = NULL;
	SDL_Texture* B_Texture = NULL;
	SDL_Texture* C_Texture = NULL;
	SDL_Texture* Health_Texture = NULL;
	SDL_Texture* Fire_Texture = NULL;
	SDL_Texture* Enemy_Texture = NULL;
	SDL_Texture* Background_Texture = NULL;
	SDL_Texture* GameOver_Texture = NULL;
	SDL_Texture* Win_Texture = NULL;
	//---------------Giving Address----------------------
	A_Texture = LoadTextureChecked(renderer, "BacheGherti.png");
	B_Texture = LoadTextureChecked(renderer, "Hestler.png");
	C_Texture = A_Texture;
	Health_Texture = LoadTextureChecked(renderer, "Health.png");
	Fire_Texture = LoadTextureChecked(renderer, "Fire.png");
	Enemy_Texture = LoadTextureChecked(renderer, "Enemy.png");
	Background_Texture = LoadTextureChecked(renderer, "background.png");
	GameOver_Texture = LoadTextureChecked(renderer, "GameOver.png");
	Win_Texture = LoadTextureChecked(renderer, "win.png");


	//----------- Variables --------------

	Events event;
	srand(time(nullptr));
	double size = 50;
	double sizeHealth = 0;
	double sizeFire = 0;
	int Character_i = (window_size - size) / 2;
	int Character_j = (window_size - size) / 2;
	int Enemy_i = 100;
	int Enemy_j = 100;
	int Random_Health_i;
	int Random_Health_j;
	int Random_Fire_i = 0;
	int Random_Fire_j = 0;
	int fairness_health;
	int fairness_fire;
	int health_counter = 0;
	int score_counter = 0;
	int counter_enemy_direction = 0;
	int enemy_direction = 0;
	GameState state = GameState::Playing;
	bool fire_visable = false;

	const char* score = 0;



	SDL_DisplayMode current;
	Uint32 fps = 16; // fallback: ~60fps frame time in ms
	if (SDL_GetCurrentDisplayMode(0, &current) == 0 && current.refresh_rate > 0) {
		fps = 1000 / current.refresh_rate;
	}
	else {
		SDL_Log("Could not get display refresh rate, defaulting to 60fps timing: %s", SDL_GetError());
	}

	while (true) {


		//---- get events and check for exit condition -------------
		event = SDL__Events();
		if (event.exit) {
			break;
		}


		//---- clear screen ----------------------------------------
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, Background_Texture, NULL, NULL);


		//-------------------- delta time --------------------------
		Uint32 deltatime = SDL_GetTicks() - pasttime;
		pasttime = SDL_GetTicks();


		//------------------ DrawCharacter ------------------------
		if (state == GameState::Playing) {
			SDL_Rect rect = { Character_i, Character_j, size, size };
			SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
		}
		if (size > 100) {
			C_Texture = B_Texture;
		}
		else {
			C_Texture = A_Texture;
		}
		//----------------------- ResizeCharacter --------------------------
		if (state == GameState::Playing) {

			size -= 8.3 * log(size) * 0.3 * deltatime / 1000.0;

		}


		//---------------------- Draw Enemy --------------------------------++++++++++++++++++++++++++++++++++++
		if (state == GameState::Playing) {
			SDL_Rect rect = { Enemy_i, Enemy_j, 50, 50 };
			SDL_RenderCopy(renderer, Enemy_Texture, NULL, &rect);
		}
		//---------------- Random movement for enemy -----------------------++++++++++++++++++++++++++++++++++++
		if ((counter_enemy_direction % 70 == 0)) {
			enemy_direction = (rand() % (4)) + 1;//  1 mean UP       2 mean DOWN      3 mean RIGHT      4 mean LEFT
		}

		counter_enemy_direction++;

		if (enemy_direction == 1 && (0 < Enemy_j - 1) && (Enemy_j < (window_size - 50))) {
			Enemy_j -= 1;
		}
		if (enemy_direction == 2 && (0 < Enemy_j) && (Enemy_j + 1 < (window_size - 50))) {
			Enemy_j += 1;
		}
		if (enemy_direction == 3 && (0 < Enemy_i) && (Enemy_i + 1 < (window_size - 50))) {
			Enemy_i += 1;
		}
		if (enemy_direction == 4 && (0 < Enemy_i - 1) && (Enemy_i < (window_size - 50))) {
			Enemy_i -= 1;
		}

		//---- random number for health bar ------------------------
		if (sizeHealth <= 0) {
			SDL_Rect candidate;
			do {
				Random_Health_i = rand() % (window_size - 50);
				Random_Health_j = rand() % (window_size - 50);
				candidate = { Random_Health_i, Random_Health_j, 50, 50 };
			} while (Overlaps(candidate, SDL_Rect{ Character_i, Character_j, (int)size, (int)size }) ||
				Overlaps(candidate, SDL_Rect{ Enemy_i, Enemy_j, 50, 50 }) ||
				(fire_visable && Overlaps(candidate, SDL_Rect{ Random_Fire_i, Random_Fire_j, (int)sizeFire, (int)sizeFire })));

			sizeHealth = 50;
			fairness_health = sqrt(((Random_Health_i - Character_i) * (Random_Health_i - Character_i)) +
				((Random_Health_j - Character_j) * (Random_Health_j - Character_j)));
		}


		//-------------------- DrawHealthBar ----------------------
		if (state == GameState::Playing) {
			SDL_Rect rectHealthBar = { Random_Health_i, Random_Health_j, sizeHealth, sizeHealth };
			SDL_RenderCopy(renderer, Health_Texture, NULL, &rectHealthBar);
		}
		//----------------------- ResizeHealthBar --------------------------
		if (state == GameState::Playing) {

			sizeHealth -= 10 * deltatime / 1000.0;

		}


		//---- random number for fire bar -------------------------
		if (sizeFire <= 0 || !fire_visable) {
			SDL_Rect candidate;
			do {
				Random_Fire_i = rand() % (window_size - 50);
				Random_Fire_j = rand() % (window_size - 50);
				candidate = { Random_Fire_i, Random_Fire_j, 50, 50 };
			} while (Overlaps(candidate, SDL_Rect{ Enemy_i, Enemy_j, 50, 50 }) ||
				Overlaps(candidate, SDL_Rect{ Character_i, Character_j, (int)size, (int)size }) ||
				(sizeHealth > 0 && Overlaps(candidate, SDL_Rect{ Random_Health_i, Random_Health_j, (int)sizeHealth, (int)sizeHealth })));

			sizeFire = 50;
			fairness_fire = sqrt(((Random_Fire_i - Character_i) * (Random_Fire_i - Character_i)) +
				((Random_Fire_j - Character_j) * (Random_Fire_j - Character_j)));
		}


		//-------------------- DrawFireBar ----------------------
		if (fire_visable && state == GameState::Playing) {
			SDL_Rect rectFireBar = { Random_Fire_i,Random_Fire_j, sizeFire, sizeFire };
			SDL_RenderCopy(renderer, Fire_Texture, NULL, &rectFireBar);
		}

		//----------------------- ResizeFireBar --------------------------
		if (fire_visable) {
			sizeFire -= 10 * deltatime / 1000.0;
			if (sizeFire <= 0) {
				fire_visable = false;
				sizeFire = -1;
			}

		}


		//---- movement ---------------------------------------------

		if (state == GameState::Playing) { //allow only game is not over
			if (event.up) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_j -= 3;
			}
			if (event.right) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_i += 3;
			}
			if (event.down) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_j += 3;
			}
			if (event.left) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_i -= 3;
			}

			// B8: clamp after moving instead of only gating the step
			// beforehand — the old `Character_j >= 0` / `Character_i <
			// window_size - size` guards still let a 3px step overshoot
			// by up to 2px on any edge.
			int max_i = window_size - (int)size;
			int max_j = window_size - (int)size;
			if (Character_i < 0) Character_i = 0;
			if (Character_i > max_i) Character_i = max_i;
			if (Character_j < 0) Character_j = 0;
			if (Character_j > max_j) Character_j = max_j;
		}

		//------------ CornersOfCharacterAndHealthBarAndFireBarAndEnemy -------------
		int Cx = Character_i;
		int Cy = Character_j;
		int Cx2 = Character_i + size;
		int Cy2 = Character_j + size;
		int Hx = Random_Health_i;
		int Hy = Random_Health_j;
		int Hx2 = Random_Health_i + sizeHealth;
		int Hy2 = Random_Health_j + sizeHealth;
		int Fx = Random_Fire_i;
		int Fy = Random_Fire_j;
		int Fx2 = Random_Fire_i + sizeFire;
		int Fy2 = Random_Fire_j + sizeFire;
		int Ex = Enemy_i;
		int Ey = Enemy_j;
		int Ex2 = Enemy_i + 50;
		int Ey2 = Enemy_j + 50;


		//------------------- EatingHealthBar ---------------------
		bool collided = true;
		if (Cx2 <= Hx || Hx2 <= Cx) collided = false;
		if (Cy2 <= Hy || Hy2 <= Cy) collided = false;

		if (collided && (size < (window_size - 100))) {
			size += HEALTH_GROWTH;
			sizeHealth = -1;

			// B8: growing can push an edge-hugging player partly outside
			// the window; clamp immediately rather than waiting for next
			// frame's movement-clamp to catch it.
			if (Character_i > window_size - (int)size) Character_i = window_size - (int)size;
			if (Character_j > window_size - (int)size) Character_j = window_size - (int)size;

			if (!fire_visable) {
				health_counter++;

				if (health_counter >= 3) {
					fire_visable = true;
					sizeFire = 50;
					health_counter = 0;
				}
			}
		}


		//------------------- EatingFireBar ---------------------
		bool collided_fire = true;
		if (Cx2 <= Fx || Fx2 <= Cx) collided_fire = false;
		if (Cy2 <= Fy || Fy2 <= Cy) collided_fire = false;

		if (collided_fire && fire_visable && (size < (window_size - 100))) {

			size -= FIRE_PENALTY;
			sizeFire = -1;
			fire_visable = false;

		}

		//------------------- EatingEnemy ---------------------++++++++++++++++++++++++++++++++++++++++
		bool collided_enemy = true;
		if (Cx2 <= Ex || Ex2 <= Cx) collided_enemy = false;
		if (Cy2 <= Ey || Ey2 <= Cy) collided_enemy = false;

		if (collided_enemy && (size < (window_size - 100))) {

			state = GameState::GameOver;
		}


		//--------------------- Score -----------------------------
		if (state == GameState::Playing) {
			string string_score = "score : " + to_string(score_counter / 60);
			const char* score = string_score.c_str();
			SDL__DrawText(score, 0, 0, 30, "#FF0000");
			score_counter += 1;
		}

		//--------------------- GameOver --------------------------
		if (size < 3) {
			state = GameState::GameOver;
		}
		if (state == GameState::GameOver) {
			SDL_RenderCopy(renderer, GameOver_Texture, NULL, NULL);

		}

		if (state == GameState::GameOver && event.space) {
			ResetGame(renderer,
				A_Texture, B_Texture, C_Texture,
				Health_Texture, Fire_Texture, Enemy_Texture,
				Background_Texture, GameOver_Texture, Win_Texture,
				window_size,
				size, sizeHealth, sizeFire,
				Character_i, Character_j,
				Enemy_i, Enemy_j,
				health_counter, score_counter,
				counter_enemy_direction, enemy_direction,
				state, fire_visable);
		}
		//------ WINNER WINNER CHICKEN DINNER ---------------------
		if (size >= (window_size - 100))
			state = GameState::Victory;

		if (state == GameState::Victory) {
			SDL_RenderCopy(renderer, Win_Texture, NULL, NULL);

		}
		if (state == GameState::Victory && event.space) {
			ResetGame(renderer,
				A_Texture, B_Texture, C_Texture,
				Health_Texture, Fire_Texture, Enemy_Texture,
				Background_Texture, GameOver_Texture, Win_Texture,
				window_size,
				size, sizeHealth, sizeFire,
				Character_i, Character_j,
				Enemy_i, Enemy_j,
				health_counter, score_counter,
				counter_enemy_direction, enemy_direction,
				state, fire_visable);
		}


		//-------------------- RenderScreen -----------------------
		SDL_RenderPresent(renderer);


		//----------------- WaitForUselessRender ------------------
		if (deltatime < fps) SDL_Delay(fps - deltatime);

	}

	SDL__Destroy();
	return 0;
}