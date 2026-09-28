#include "SDL.h"
#include <ctime>
#include <string>

using namespace std;

int main() {
	int window_size = 700;

	SDL__CreateWindow("Square Baby", window_size, window_size);
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
	A_Texture = IMG_LoadTexture(renderer, "BacheGherti.png");
	B_Texture = IMG_LoadTexture(renderer, "Hestler.png");
	C_Texture = A_Texture;
	Health_Texture = IMG_LoadTexture(renderer, "Health.png");
	Fire_Texture = IMG_LoadTexture(renderer, "Fire.png");
	Enemy_Texture = IMG_LoadTexture(renderer, "Enemy.png");
	Background_Texture = IMG_LoadTexture(renderer, "background.png");
	GameOver_Texture = IMG_LoadTexture(renderer, "GameOver.png");
	Win_Texture = IMG_LoadTexture(renderer, "win.png");


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
	bool gameover = false;
	bool winner = false;
	bool fire_visable = false;

	const char* score = 0;



	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	Uint32 fps = 1000 / current.refresh_rate;

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
		if (!gameover && !winner) {
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
		if (!gameover && !winner) {

			size -= 8.3 * log(size) * 0.3 * deltatime / 1000.0;

		}


		//---------------------- Draw Enemy --------------------------------++++++++++++++++++++++++++++++++++++
		if (!gameover && !winner) {
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
			do {
				Random_Health_i = rand() % (window_size - 50);
				Random_Health_j = rand() % (window_size - 50);
			} while (((Character_i < Random_Health_i && Random_Health_i < Character_i + size) &&
				(Character_j < Random_Health_j && Random_Health_j < Character_j + size)) ||
				((Enemy_i < Random_Health_i && Random_Health_i < Enemy_i + size) &&
					(Enemy_j < Random_Health_j && Random_Health_j < Enemy_j + size)) ||
				((Random_Fire_i < Random_Health_i && Random_Health_i < Random_Fire_i + sizeFire) &&
					(Random_Fire_j < Random_Health_j && Random_Health_j < Random_Fire_j + sizeFire)));

			sizeHealth = 50;
			fairness_health = sqrt(((Random_Health_i - Character_i) * (Random_Health_i - Character_i)) +
				((Random_Health_j - Character_j) * (Random_Health_j - Character_j)));
		}


		//-------------------- DrawHealthBar ----------------------
		if (!winner && !gameover) {
			SDL_Rect rectHealthBar = { Random_Health_i, Random_Health_j, sizeHealth, sizeHealth };
			SDL_RenderCopy(renderer, Health_Texture, NULL, &rectHealthBar);
		}
		//----------------------- ResizeHealthBar --------------------------
		if (!gameover && !winner) {

			sizeHealth -= 10 * deltatime / 1000.0;

		}


		//---- random number for fire bar -------------------------
		if (sizeFire <= 0 || !fire_visable) {
			do {
				Random_Fire_i = rand() % (window_size - 50);
				Random_Fire_j = rand() % (window_size - 50);
			} while (((Enemy_i < Random_Fire_i && Random_Fire_i < Enemy_i + size) &&
				(Enemy_j < Random_Fire_j && Random_Fire_j < Enemy_j + size)) ||
				((Character_i < Random_Fire_i && Random_Fire_i < Character_i + size) &&
					(Character_j < Random_Fire_j && Random_Fire_j < Character_j + size)) ||
				((Random_Fire_i < Random_Health_i && Random_Health_i < Random_Fire_i + sizeFire) &&
					(Random_Fire_j < Random_Health_j && Random_Health_j < Random_Fire_j + sizeFire)));

			sizeFire = 50;
			fairness_fire = sqrt(((Random_Fire_i - Character_i) * (Random_Fire_i - Character_i)) +
				((Random_Fire_j - Character_j) * (Random_Fire_j - Character_j)));
		}


		//-------------------- DrawFireBar ----------------------
		if (fire_visable && !winner && !gameover) {
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

		if (!gameover && !winner) { //allow only game is not over
			if (event.up && Character_j >= 0) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_j -= 3;
			}
			if (event.right && Character_i < (window_size - size)) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_i += 3;
			}
			if (event.down && Character_j < (window_size - size)) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_j += 3;
			}
			if (event.left && 0 <= Character_i) {
				SDL_Rect rect = { Character_i, Character_j, size, size };
				SDL_RenderCopy(renderer, C_Texture, NULL, &rect);
				Character_i -= 3;
			}
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
			size += sizeHealth * 3 / log(fairness_health);
			sizeHealth = -1;

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

			size -= sizeFire * 2 / log(fairness_fire);
			sizeFire = -1;
			fire_visable = false;

		}

		//------------------- EatingEnemy ---------------------++++++++++++++++++++++++++++++++++++++++
		bool collided_enemy = true;
		if (Cx2 <= Ex || Ex2 <= Cx) collided_enemy = false;
		if (Cy2 <= Ey || Ey2 <= Cy) collided_enemy = false;

		if (collided_enemy && (size < (window_size - 100))) {

			gameover = true;
		}


		//--------------------- Score -----------------------------
		if (!gameover && !winner) {
			string string_score = "score : " + to_string(score_counter / 60);
			const char* score = string_score.c_str();
			SDL__DrawText(score, 0, 0, 30, "#FF0000");
			score_counter += 1;
		}

		//--------------------- GameOver --------------------------
		if (size < 3) {
			gameover = true;
		}
		if (gameover) {
			SDL_RenderCopy(renderer, GameOver_Texture, NULL, NULL);

		}

		if (gameover && event.space) {
			// Freeing textures
			SDL_DestroyTexture(A_Texture);
			SDL_DestroyTexture(B_Texture);
			SDL_DestroyTexture(C_Texture);
			SDL_DestroyTexture(Health_Texture);
			SDL_DestroyTexture(Fire_Texture);
			SDL_DestroyTexture(Enemy_Texture);
			SDL_DestroyTexture(Background_Texture);
			SDL_DestroyTexture(GameOver_Texture);
			SDL_DestroyTexture(Win_Texture);

			// Re-loading textures
			A_Texture = IMG_LoadTexture(renderer, "BacheGherti.png");
			B_Texture = IMG_LoadTexture(renderer, "Hestler.png");
			C_Texture = A_Texture;
			Health_Texture = IMG_LoadTexture(renderer, "Health.png");
			Fire_Texture = IMG_LoadTexture(renderer, "Fire.png");
			Enemy_Texture = IMG_LoadTexture(renderer, "Enemy.png");
			Background_Texture = IMG_LoadTexture(renderer, "background.png");
			GameOver_Texture = IMG_LoadTexture(renderer, "GameOver.png");
			Win_Texture = IMG_LoadTexture(renderer, "win.png");

			// Reset game variables
			gameover = false;
			size = 50;
			sizeHealth = -1;
			sizeFire = -1;
			Character_i = (window_size - size) / 2;
			Character_j = (window_size - size) / 2;
			Enemy_i = 100;
			Enemy_j = 100;
			score_counter = 0;
			C_Texture = A_Texture; // Reset character to original image


		}
		//------ WINNER WINNER CHICKEN DINNER ---------------------
		if (size >= (window_size - 100))
			winner = true;

		if (winner) {
			SDL_RenderCopy(renderer, Win_Texture, NULL, NULL);

		}
		if (winner && event.space) {
			// Freeing textures 
			SDL_DestroyTexture(A_Texture);
			SDL_DestroyTexture(B_Texture);
			SDL_DestroyTexture(C_Texture);
			SDL_DestroyTexture(Health_Texture);
			SDL_DestroyTexture(Fire_Texture);
			SDL_DestroyTexture(Enemy_Texture);
			SDL_DestroyTexture(Background_Texture);
			SDL_DestroyTexture(GameOver_Texture);
			SDL_DestroyTexture(Win_Texture);

			// Re-loading textures
			A_Texture = IMG_LoadTexture(renderer, "BacheGherti.png");
			B_Texture = IMG_LoadTexture(renderer, "Hestler.png");
			C_Texture = A_Texture;
			Health_Texture = IMG_LoadTexture(renderer, "Health.png");
			Fire_Texture = IMG_LoadTexture(renderer, "Fire.png");
			Enemy_Texture = IMG_LoadTexture(renderer, "Enemy.png");
			Background_Texture = IMG_LoadTexture(renderer, "background.png");
			GameOver_Texture = IMG_LoadTexture(renderer, "GameOver.png");
			Win_Texture = IMG_LoadTexture(renderer, "win.png");

			// Reset game variables
			winner = false;
			size = 50;
			sizeHealth = -1;
			sizeFire = -1;
			Character_i = (window_size - size) / 2;
			Character_j = (window_size - size) / 2;
			Enemy_i = 100;
			Enemy_j = 100;
			score_counter = 0;
			C_Texture = A_Texture;  // Reset character to original image
		}


		//-------------------- RenderScreen -----------------------
		SDL_RenderPresent(renderer);


		//----------------- WaitForUselessRender ------------------
		if (deltatime < fps) SDL_Delay(fps - deltatime);

	}

	SDL__Destroy();
	return 0;
}