#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>

// asset dimensions
#define SCREEN_R 50
#define SCREEN_C 150
#define LOADING_R 6
#define LOADING1_C 41
#define LOADING2_C 43
#define LOADING3_C 45
#define PWON_R 5
#define P1WON_C 36
#define P2WON_C 39
#define BOTHWON_R 10
#define BOTHWON_C 59
#define GAMEOVER_R 5
#define GAMEOVER_C 52
#define FISH_R 1
#define FISH_C 8
#define SHARK_R 9
#define SHARK_C 21
#define CAN_R 6
#define CAN_C 14
#define BAG_R 9
#define BAG_C 20
#define BOTTLE_R 9
#define BOTTLE_C 11
#define GUEST_R 13
#define WHALE_C 107
#define DOLPHIN_C 60
#define TURTLE_C 33
#define SCENERY_R 16
#define CORAL_C 31 
#define REEF_C 33
#define BUSH_C 24
#define ROCK_C 50
#define WEED1_C 29
#define WEED2_C 18
#define WEED3_C 24
#define STARFISH_C 28

// game constraints
#define TRASH_LIMIT 10
#define SCENERY_LIMIT 10
#define MAX_SCORE 10

// enumerates the the type of game objects
typedef enum {
        FISH, SHARK, CAN, BAG, BOTTLE, TURTLE, DOLPHIN, WHALE, CORAL, REEF, 
        BUSH, ROCK, WEED1, WEED2, WEED3, STARFISH, TRASH, SCENERY
} Type;

// holds top left coordinate of something
typedef struct {
        int x;
        int y;
        Type type;
} Object;

// game assets (2d but stored in 1d character arrays)
char *fish1, *fish2, *shark, *can, *bag, *bottle, *whale, *dolphin;
char *turtle, *coral, *reef, *bush, *rock, *weed1, *weed2, *weed3, *starfish;
char *starfish, *homePage, *loading1, *loading2, *loading3, *p1Won, *p2Won;
char *bothWon, *gameOver;

// players start in the middle of screen height and left third of screen width
Object p1 = {40, 16, FISH};
Object p2 = {50, 25, FISH}; 

// 47 lines of gameplay + 150 char per line (without 3 line header)
char scene[SCREEN_R-3][SCREEN_C];

// keeps track of scene view 
int sceneX;

// total amount of trash dodged (score)
int p1TrashEvaded, p2TrashEvaded;

// true while players have not been eaten
_Bool p1IsAlive, p2IsAlive;

// trackers for players' dazed status and when to end it
_Bool p1IsDazed, p2IsDazed;
int p1DazedCount, p2DazedCount;

// player records
int p1Highest, p2Highest, encyclopediaLVL;
_Bool encyclopediaLeveledUp;

// indicates whether the game is running
_Bool running = 0;

// trash & scenery will be created when needed and removed when off screen
int numOfTrash, numOfScenery;
Object trash[TRASH_LIMIT], scenery[SCENERY_LIMIT];

// only one endangered species shows up per game (encourages replaying)
Object guest; 

char* loadArt(int rows, int cols, char *fileName); // stores specified ascii art asset in the program's memory
void loadAssets(void); // loads all the ascii art into the game by using loadArt()
void freeLoadingAssets(void); // frees the loading screen assets
void freeAssets(void); // frees all the remaining dyncamically allocated asset
void loadInfo(void); // loads all recorded player information
void freeAll(void); // frees the allocated memory for all the ascii art assets and game objects
void printLine(int row); // prints a line of dashes on the given row of the window
void wipeWindow(void); // replaces all characters in the window with a space
void wipeScene(void); // replaces all characters in the scene with a space
void waitFor(unsigned int seconds, unsigned long nanoseconds); // delays processes for the given amount of time
void drawLoading(char *loading, int columns); // draws the corresponding loading screen
void drawIntro(void); // draws the intro (screen calibration and loading screen)
void drawHomePage(void); // draws the Waste Management loading screen
void setHomePage(void); // allows user to start game or read intructions
void chooseGuest(void); // based on encyclopedia level, chooses an endangered species to swim over players
void generateNewObjects(Type type); // if the current number of objects is less than the threshold, generates more
void removeOldObjects(Type type); // if objects have gone off the left side of the screen, they are removed
void manageObjects(void); // generates and removes any extra trash and scenery objects 
void drawScore(int score, int column); // draws the score (with a leading zero if necessary)
void drawHeader(void); // draws the header on the window (including scores, encyclopedia lvl, and endangered species found)
void updateHeader(void); // updates encyclopedia lvl if an endangered species has emerged
void drawShark(void); // draws the shark onto the scene
_Bool hitTrash(int playerNo); // checks and dazes given player if they hit a trash object
_Bool hitFish(void); // checks and dazes both players if they hit each other
void moveFish(void); // moves both players according to WASD or IJKL inputs, player statuses, and game bounds
void updateFish(void); // moves players and then updates their status if needed
void drawFish(void); // draws the fishes onto the scene
void drawGuest(void); // draws the guest if they are visible on the scene
void drawScene(void); // draws the scene onto the window
void moveScene(void); // moves the scene forward

void endGame(void); // ends game if both players are dead or either (or both) won
void showResult(int playerNo); // displays corresponding result page with prompt to check encyclopedia if it was updated
void saveGame(void); // saves player records to files
void runGame(void); // resets all global variables, load in player records, and runs game

int main(void){
        srand(time(NULL)); // seed the random function with current time
        initscr(); // initializes window/screen
        cbreak(); // puts terminal in cbreak mode (to allow for single char inputs)
        loadAssets(); // loads all ascii art and player records into the game
        drawIntro(); // gets the player ready to start game
        
        setHomePage(); // sets home page
        freeLoadingAssets(); // frees loading screen assets
        runGame(); // starts game

        freeAssets(); // frees the memory holding all remaining assets
        nocbreak(); // cbreak mode is disabled
        endwin(); // window/screen is closed 
        return 0;
}

char* loadArt(int rows, int cols, char *fileName){
        /* 2d art is loaded into 1d character arrays large enough to hod 
           all the characters making up the art */
        FILE *artFile = fopen(fileName, "r");
        char *art = (char*)malloc(rows * cols * sizeof(char));
        if (artFile == NULL){
                wipeWindow();
                mvprintw(0, 0, "Error: failed to load in art assets");
                refresh();
                waitFor(2, 0);
                exit(1);
        }

        char pixel;
        int i = 0, j = 0;
        while ((pixel = fgetc(artFile)) != EOF){
                if (pixel == '\n'){
                        i++;
                        j = 0;
                        continue;
                }
                art[i * cols + j] = pixel;
                j++;
        }
        return art;
}

void loadAssets(void){
        // home page and loading screen art
        homePage = loadArt(SCREEN_R, SCREEN_C, "assets/homePage.txt");
        loading1 = loadArt(LOADING_R, LOADING1_C, "assets/loading1.txt");
        loading2 = loadArt(LOADING_R, LOADING2_C, "assets/loading2.txt");
        loading3 = loadArt(LOADING_R, LOADING3_C, "assets/loading3.txt");
        p1Won = loadArt(PWON_R, P1WON_C, "assets/p1Won.txt");
        p2Won = loadArt(PWON_R, P2WON_C, "assets/p2Won.txt");
        bothWon = loadArt(BOTHWON_R, BOTHWON_C, "assets/bothWon.txt");
        gameOver = loadArt(GAMEOVER_R, GAMEOVER_C, "assets/gameOver.txt");

        // player (fish) art
        fish1 = loadArt(FISH_R, FISH_C, "assets/fish.txt");
        fish2 = loadArt(FISH_R, FISH_C, "assets/fish.txt");

        // shark art
        shark = loadArt(SHARK_R, SHARK_C, "assets/shark.txt");

        // trash art
        can = loadArt(CAN_R, CAN_C, "assets/can.txt");
        bag = loadArt(BAG_R, BAG_C, "assets/bag.txt");
        bottle = loadArt(BOTTLE_R, BOTTLE_C, "assets/bottle.txt");
        
        // endangered species art
        whale = loadArt(GUEST_R, WHALE_C, "assets/whale.txt");
        dolphin = loadArt(GUEST_R, DOLPHIN_C, "assets/dolphin.txt");
        turtle = loadArt(GUEST_R, TURTLE_C, "assets/turtle.txt");

        // scenery art
        coral = loadArt(SCENERY_R, CORAL_C, "assets/coral.txt");
        reef = loadArt(SCENERY_R, REEF_C, "assets/reef.txt");
        bush = loadArt(SCENERY_R, BUSH_C, "assets/bush.txt");
        rock = loadArt(SCENERY_R, ROCK_C, "assets/reef.txt");
        weed1 = loadArt(SCENERY_R, WEED1_C, "assets/weed1.txt");
        weed2 = loadArt(SCENERY_R, WEED2_C, "assets/weed2.txt");
        weed3 = loadArt(SCENERY_R, WEED3_C, "assets/weed3.txt");
        starfish = loadArt(SCENERY_R, STARFISH_C, "assets/starfish.txt");
}

void freeLoadingAssets(void){
        free(loading1);
        free(loading2);
        free(loading3);
}

void freeAssets(void){
        free(homePage);
        free(p1Won);
        free(p2Won);
        free(bothWon);
        free(gameOver);
        free(fish1);
        free(fish2);
        free(shark);
        free(can);
        free(bag);
        free(bottle);
        free(whale);
        free(dolphin);
        free(turtle);
        free(coral);
        free(reef);
        free(bush);
        free(rock);
        free(weed1);
        free(weed2);
        free(weed3);
        free(starfish);
}

void loadInfo(void){
        /* Player records are loaded in line by line (first line
           is p1 best, second is p2 best, and third is the encyclopedia lvl) */
        FILE *records = fopen("assets/records.txt", "r");

        if (records == NULL){
                wipeWindow();
                mvprintw(0, 0, "Error: failed to open file records");
                refresh();
                waitFor(2, 0);
                exit(1);
        }
     
        fscanf(records, "%d\n", &p1Highest);
        fscanf(records, "%d\n", &p2Highest);
        fscanf(records, "%d\n", &encyclopediaLVL);

        fclose(records);
}

void printLine(int row){
        // prints a dotted line onto the window
        for (int i = 0; i < 150; i++){
                mvprintw(row, i, "-");
        }
}

void wipeWindow(void){
        // replaces all the characters in the window with spaces
        for (int i = 0; i < SCREEN_R; i++){
                for (int j = 0; j < SCREEN_C; j++){
                        mvprintw(i, j, " ");
                }
        }
}

void wipeScreen(void){
        // replaces all the characters in the scene with spaces
        for (int i = 0; i < SCREEN_R-3; i++){
                for (int j = 0; j < SCREEN_C; j++){
                        scene[i][j] = ' ';
                }
        }
}


void waitFor(unsigned int seconds, unsigned long nanoseconds){
        // creates a delay with the given amount of time
        const struct timespec delay = {seconds, nanoseconds};
        nanosleep(&delay, NULL);
}

void drawLoading(char *loading, int columns){
        /* Changes between the three loading screens to 
           acheive loading screen effect */
        wipeWindow();
        printLine(0);
        for (int i = 1; i < 49; i++){
                mvprintw(i, 0, "|");
                mvprintw(i, 149, "|");
        }

        for (int i = 0; i < LOADING_R; i++){
                for (int j = 0; j < columns; j++){
                        mvprintw(i+23, j+52, "%c", loading[i * columns + j]);
                }
        }
        printLine(49);
        refresh();
        waitFor(1,0);
}

void drawIntro(void){
        // players are given a few seconds to adjust their window size before game starts loading
        mvprintw(0, 0, "Please adjust your window and zoom in/out to see the following box appropriately:");
        refresh();
        waitFor(4, 0);
        drawLoading(loading1, LOADING1_C);
        drawLoading(loading2, LOADING2_C);
        drawLoading(loading3, LOADING3_C);
        drawLoading(loading1, LOADING1_C);
        drawLoading(loading2, LOADING2_C);
        drawLoading(loading3, LOADING3_C);
}

void drawHomePage(void){
        // the home screen is drawn onto the window
        for (int i = 0; i < SCREEN_R; i++){
                for (int j = 0; j < SCREEN_C; j++){
                        mvprintw(i, j, "%c", homePage[i * SCREEN_C + j]);
                }
        }
        refresh();
}

void setHomePage(void){
        drawHomePage();
        
        /* After the home screen is drawn, players are given the option to read
           the instructions and begin the game */
        char input;
        while (1) {
                input = getc(stdin);
                if (input == '\r'){
                        running = 1;
                        break;
                } 
                else if (input == 'i' || input == 'I'){
                        wipeWindow();
                        printLine(15);
                        mvprintw(16, 0, "\t\t\t\tEvade trash while fleeing from a shark!!");

                        mvprintw(18, 0, "\t\t\t\tPlayer one (top fish) should uses WASD to move");
                        mvprintw(19, 0, "\t\t\t\tPlayer two (bottom fish) should uses IJKL to move");

                        mvprintw(21, 0, "\t\t\t\tIf you get hit by trash or the other player, your fish will be in a momentary state of shock:");
                        mvprintw(23, 0, "\t\t\t\t\t\t\t\t\t><)))@>");
                        mvprintw(25, 0, "\t\t\t\tDuring this time, your fish will be susceptible to getting eaten by your pursuer!");

                        mvprintw(27, 0, "\t\t\t\tThroughout your journey, you may pass by endangered species...");
                        mvprintw(28, 0, "\t\t\t\tIf you happen to find them, information about them will be added to your encyclopedia");

                        mvprintw(30, 0, "\t\t\t\tPress Q to exit:");
                        printLine(31);
                        refresh();

                        while (1) {
                                input = getc(stdin);
                                if (input == 'q' || input == 'Q'){
                                        break;
                                }
                        }
                }
                drawHomePage();
        }
}

void chooseGuest(void){
        /* A guest is chosen based on encyclopedia lvl (random if lvl 3)
           to appear randomly during the game's progression */
        guest.y = 0;
        guest.x = 150 + (rand() % 300);
        if (encyclopediaLVL == 0){
                guest.type = TURTLE;
        }else if (encyclopediaLVL == 1){
                guest.type = DOLPHIN;
        }else if (encyclopediaLVL == 2){
                guest.type = WHALE;
        }else{
                guest.type = TURTLE + (rand() % 3);
        }
}

void generateNewObjects(Type type){
        /* If the current number of objects of the given type are not at
           their threshold, then more are generated and added at the end
           of each array of objects */
        if (type == TRASH){
                while (numOfTrash < TRASH_LIMIT){
                        numOfTrash++;
                        if (numOfTrash == 1){
                                trash[numOfTrash-1].x = sceneX+SCREEN_C;
                        }else{
                                trash[numOfTrash-1].x = trash[numOfTrash-2].x + (rand() % 30) + 15;
                        }
                        trash[numOfTrash-1].y = 13 + (rand() % 9);
                        trash[numOfTrash-1].type = 2 + (rand() % 3);
                }
        }else{
                while (numOfScenery < SCENERY_LIMIT){
                        numOfScenery++;
                        if (numOfScenery == 1){
                                scenery[numOfScenery-1].x = (rand() % 15);
                        }else{
                                scenery[numOfScenery-1].x = scenery[numOfScenery-2].x + 30 + (rand() % 15);
                        }

                        scenery[numOfScenery-1].y = 31;
                        scenery[numOfScenery-1].type = CORAL + (rand() % 8);
                }
        }
}

void removeOldObjects(Type type){
        /* Checks if objects of the given type are off the screen. If they are
           the count of objects to remove is increased. Then, the existing objects
           in the corresponding array are shifted over to 'remove' the old objects. */
        int numToRemove = 0;
        int trashWidth;
        if (type == TRASH){
                for (int i = 0; i < numOfTrash; i++){
                        if (trash[i].type == CAN){
                                trashWidth = CAN_C;
                        }else if (trash[i].type == BAG){
                                trashWidth = BAG_C;
                        }else if (trash[i].type == BOTTLE){
                                trashWidth = BOTTLE_C;
                        }

                        if (trash[i].x + trashWidth < sceneX + SHARK_C){
                                numToRemove++;
                                if (p1IsAlive){
                                        p1TrashEvaded++;
                                        if (p1TrashEvaded > p1Highest){
                                                p1Highest++;
                                        }
                                }
                                if (p2IsAlive){
                                        p2TrashEvaded++;
                                        if (p2TrashEvaded > p2Highest){
                                                p2Highest++;
                                        }
                                }
                        }else{
                                break;
                        }
                }
                for (int i = numToRemove; i < numOfTrash; i++){
                        trash[i-numToRemove] = trash[i];
                }
                numOfTrash -= numToRemove;
        }else{
                for (int i = 0; i < numOfScenery; i++){
                        if (scenery[i].x < sceneX - 50){
                                numToRemove++;
                        }else{
                                break;
                        }
                }
                for (int i = numToRemove; i < numOfScenery; i++){
                        scenery[i-numToRemove] = scenery[i];
                }
                numOfScenery -= numToRemove;
        }
}

void manageObjects(void){
        // generates and removes uneeded trash and scenery objects
        generateNewObjects(TRASH);
        removeOldObjects(TRASH);
        generateNewObjects(SCENERY);
        removeOldObjects(SCENERY);
}


void drawScore(int score, int column){
        // score is printed with leading 0 if needed
        if (score < 10){
                mvprintw(1, column, "0%d", score);
        }else{
                mvprintw(1, column, "%d", score);
 
        }
}

void drawHeader(void){
        // header is printed using player recorded data
        printLine(0);
        mvprintw(1, 10, "P1 SCORE:");
        mvprintw(1, 23, "BEST:");
        mvprintw(1, 46, "LVL %d ENCYCLOPEDIA", encyclopediaLVL);
        mvprintw(1, 79, "FOUND ?????? ??????? ?????");
        if (encyclopediaLVL == 3){
                mvprintw(1, 85, "TURTLE DOLPHIN WHALE");
        } else if (encyclopediaLVL == 2){
                mvprintw(1, 85, "TURTLE DOLPHIN");
        } else if (encyclopediaLVL == 1){
                mvprintw(1, 85, "TURTLE");
        }
        mvprintw(1, 120, "P2 SCORE:");
        mvprintw(1, 133, "BEST:");
        drawScore(p1TrashEvaded, 20);
        drawScore(p1Highest, 29);
        drawScore(p2TrashEvaded, 130);
        drawScore(p2Highest, 139);
        printLine(2);
}

void drawShark(void){
        // the shark is drawn onto the scene
        for (int i = 0; i < SHARK_R; i++){
                for (int j = 0; j < SHARK_C; j++){
                        if (shark[i * SHARK_C + j] != ' '){
                                scene[19+i][j] = shark[i * SHARK_C + j]; 
                        }
                }
        }
}

void drawTrash(void){
        /* The array of trash objects is traveresed and all the objects
           the are visible are drawn accordingly */
        int rows, columns;
        char *art;

        for (int i = 0; i < numOfTrash; i++){
                if (trash[i].type == CAN){
                        rows = CAN_R;
                        columns = CAN_C;
                        art = can;
                }else if (trash[i].type == BAG){
                        rows = BAG_R;
                        columns = BAG_C;
                        art = bag;
                }else if (trash[i].type == BOTTLE){
                        rows = BOTTLE_R;
                        columns = BOTTLE_C;
                        art = bottle;
                }

                for (int j = 0; j < rows; j++){
                        for (int k = 0; k < columns; k++){
                                if (trash[i].x+k-sceneX >= SHARK_C && trash[i].x+k+-sceneX < SCREEN_C){
                                        scene[trash[i].y + j][trash[i].x + k - sceneX] = art[j * columns + k];
                                }
                        }
                }
        }
}

void moveScene(void){
        // increments the scene to the right
        sceneX++;
}

void drawScenery(void){
        /* The array of scenery objects is traveresed and all the objects
           the are visible are drawn accordingly. A line representing the floor is 
           also drawn */
        int rows = SCENERY_R;
        int columns;
        char *art;

        for (int i = 0; i < SCREEN_C; i++){
                scene[38][i] = '~';
        }

        for (int i = 0; i < numOfScenery; i++){
                if (scenery[i].type == CORAL){
                        columns = CORAL_C;
                        art = coral;
                }else if (scenery[i].type == REEF){
                        columns = REEF_C;
                        art = reef;
                }else if (scenery[i].type == BUSH){
                        columns = BUSH_C;
                        art = bush;
                }else if (scenery[i].type == ROCK){
                        columns = ROCK_C;
                        art = rock;
                }else if (scenery[i].type == WEED1){
                        columns = WEED1_C;
                        art = weed1;
                }else if (scenery[i].type == WEED2){
                        columns = WEED2_C;
                        art = weed2;
                }else if (scenery[i].type == WEED3){
                        columns = WEED3_C;
                        art = weed3;
                }else if(scenery[i].type == STARFISH){ 
                        columns = STARFISH_C;
                        art = starfish;
                }

                for (int j = 0; j < rows; j++){
                        for (int k = 0; k < columns; k++){
                                if (scenery[i].x+k-sceneX >= 0 && scenery[i].x+k-sceneX < SCREEN_C && art[j * columns + k] != ' '){
                                        scene[scenery[i].y + j][scenery[i].x + k - sceneX] = art[j * columns + k];
                                }
                        }
                }
        }
}

_Bool hitTrash(int playerNo){
        /* Trash collisions are determined by checking if trash has been drawn onto the scene
           where the player was going be */
        Object player;
        if (playerNo == 1){
                player = p1;
        }else{
                player = p2;
        }

        for (int i = 0; i < FISH_C; i++){
                if (scene[player.y][player.x+i] != ' '){
                        if (playerNo == 1){
                                p1IsDazed = 1;
                        }else{
                                p2IsDazed = 1;
                        }
                        return 1;
                }
        }

        return 0;
}

_Bool hitFish(void){
        // player collision is determined simply from their coordinates
        int diffX = p1.x - p2.x;
        if (diffX < 0){
                diffX *= -1;
        }

        if ((p1.y == p2.y) && diffX < 9){
                p1IsDazed = 1;
                p2IsDazed = 1;
                return 1;
        }else{
                return 0;
        }
}

void moveFish(void){
        // movement input is taken in
        char c = getc(stdin);

        /* If the player can move (alive and not dazed) the are moved up if w/i is pressed, 
           down if s/k is pressed, right if d/l is pressed, and left if a/j is pressed. If they 
           happen to hit each other or a piece of trash, their movement is reversed. */
        if (!p1IsDazed && p1IsAlive){
                if ((c == 'w' || c == 'W')){
                        p1.y--;
                        if (hitFish() || hitTrash(1)){
                                p1.y++;
                        }
                }else if ((c == 's' || c == 'S')){
                        p1.y++;
                        if (hitFish() || hitTrash(1)){
                                p1.y--;
                        }
                }else if ((c == 'a' || c == 'A')){
                        p1.x--;
                        if (hitFish() || hitTrash(1)){
                                p1.x++;
                        }
                }else if ((c == 'd' || c == 'D')){
                        p1.x++;
                        if (hitFish() || hitTrash(1)){
                                p1.x--;
                        }
                }else{
                        hitFish();
                        hitTrash(1);
                }
        }

        if (!p2IsDazed && p2IsAlive){
                if ((c == 'i' || c == 'I')){
                        p2.y--;
                        if (hitFish() || hitTrash(2)){
                                p2.y++;
                        }
                }else if ((c == 'k' || c == 'K')){
                        p2.y++;
                        if (hitFish() || hitTrash(2)){
                                p2.y--;
                        }
                }else if ((c == 'j' || c == 'J')){
                        p2.x--;
                        if (hitFish() || hitTrash(2)){
                                p2.x++;
                        }
                }else if ((c == 'l' || c == 'L')){
                        p2.x++;
                        if (hitFish() || hitTrash(2)){
                                p2.x--;
                        }
                }else{
                        hitFish();
                        hitTrash(2);
                }
        }
}

void updateFish(void){
        // fishes are moved
        moveFish();

        // updates the each fish's status (and eyes if needed) and keeps them in bounds
        if (p1.x < SHARK_C){
                p1IsAlive = 0;
                p1.x--;
                fish1[5] = 'X';
        }else if (p1IsDazed){
                p1.x--;
                p1DazedCount++;
                fish1[5] = '@';
                if (p1DazedCount == 5){
                        p1IsDazed = 0;
                        p1DazedCount = 0;
                        fish1[5] = 'o';
                }
        }else{
                if (p1.y < 13){
                        p1.y++;
                }else if (p1.y > 31){
                        p1.y--;;
                }else if (p1.x > SCREEN_C - FISH_C){
                        p1.x--;
                }
        }

        if (p2.x < + SHARK_C){
                p2IsAlive = 0;
                p2.x--;
                fish2[5] = 'X';
        }else if (p2IsDazed){
                p2.x--;
                p2DazedCount++;
                fish2[5] = '@';
                if (p2DazedCount == 5){
                        p2IsDazed = 0;
                        p2DazedCount = 0;
                        fish2[5] = 'o';
                }
        }else{
                if (p2.y < 13){
                        p2.y++;
                }else if (p2.y > 31){
                        p2.y--;;
                }else if (p2.x > SCREEN_C - FISH_C){
                        p2.x--;
                }
        }
}

void drawFish(void){
        // Both fish are drawn onto the scene
        for (int i = 0; i < FISH_C; i++){
                if (p1.x+i >= 21) {
                        scene[p1.y][p1.x+i] = fish1[i];
                }
                if (p2.x+i >= 21) {
                        scene[p2.y][p2.x+i] = fish2[i];
                }
        }
}

void drawGuest(void){
        // The appropriate guest is drawn onto the scene with its corresponding parameters 
        int guestWidth; 
        char *art;
        if (guest.type == TURTLE){
                guestWidth = TURTLE_C;
                art = turtle;
        }else if (guest.type == DOLPHIN){
                guestWidth = DOLPHIN_C;
                art = dolphin;
        }else if (guest.type == WHALE){
                guestWidth = WHALE_C;
                art = whale;
        }

        for (int i = 0; i < GUEST_R; i++){
                for (int j = 0; j < guestWidth; j++){
                        if (guest.x + j - sceneX >= 0 && guest.x + j - sceneX < 150){
                                scene[i][guest.x + j - sceneX] = art[i * guestWidth + j];
                        }
                }
        }       
}

void drawScene(void){
        /* The scene is wiped before the trash is drawn. Then the fish are locations updated the trash 
           on the scene to determine collisions. Then, after any fish movement has occured and/or their status
           has changed, the shark is drawn onto the scene. Then the header is updated and drawn as well. After all
           that, the scenery (decoration) and guest are drawn if the are visible on the screen. Finally, everything
           is printed onto the window and it is refreshed. */
        wipeScreen();
        drawTrash();
        updateFish();
        drawFish();
        drawShark();
        updateHeader();
        drawHeader();
        drawScenery();
        drawGuest();
        for (int i = 0; i < SCREEN_R-3; i++){
                for (int j = 0; j < SCREEN_C; j++){
                        mvprintw(i+3, j, "%c", scene[i][j]);
                }
        }
        refresh();
}

void updateHeader(void){
        // if the guest has shown up, the encyclopedia's lvl is incremented
        if (sceneX+SCREEN_C-guest.x == 0 && encyclopediaLVL < 3){
                encyclopediaLVL++;
                encyclopediaLeveledUp = 1;
        }
}

void saveGame(void){
        // saves player bests to records file and updates encyclopedia if necessary 
        FILE *records = fopen("assets/records.txt", "w");
        FILE *encyclopedia = fopen("encyclopedia.txt", "w");
        FILE *encyclopediaToCopy;

        // figures out the correct encyclopedia asset file to copy from
        if (encyclopediaLVL == 0){
                encyclopediaToCopy = fopen("assets/encyclopedia_lvl0.txt", "r");
        }else if (encyclopediaLVL == 1){
                encyclopediaToCopy = fopen("assets/encyclopedia_lvl1.txt", "r");
        }else if (encyclopediaLVL == 2){
                encyclopediaToCopy = fopen("assets/encyclopedia_lvl2.txt", "r");
        }else if (encyclopediaLVL == 3){
                encyclopediaToCopy = fopen("assets/encyclopedia_lvl3.txt", "r");
        }

        // checks for erros with file opening
        if (records == NULL || encyclopedia == NULL || encyclopediaToCopy == NULL){
                wipeWindow();
                mvprintw(0, 0, "Error: failed to save game details");
                refresh();
                waitFor(2, 0);
                exit(1);
        }

        // updates encyclopedia
        char c;
        while ((c = fgetc(encyclopediaToCopy)) != EOF){
                fputc(c, encyclopedia);
        }

        // updates player bests and encyclopedia lvl
        fprintf(records, "%d\n", p1Highest);
        fprintf(records, "%d\n", p2Highest);
        fprintf(records, "%d", encyclopediaLVL);

        // closes files
        fclose(records);
        fclose(encyclopedia);
        fclose(encyclopediaToCopy);
}


void endGame(void){
        // shows ending result and saves game if players have died or either/both have won
        if (!p1IsAlive && !p2IsAlive){
                running = 0;
                showResult(0);
                saveGame();
        }else if (p1TrashEvaded == MAX_SCORE || p2TrashEvaded == MAX_SCORE){
                mvprintw(24,10, "X");
                mvprintw(49,148, "");
                refresh();
                running = 0;
                waitFor(3,0);
                if (p1TrashEvaded == MAX_SCORE && p2TrashEvaded == MAX_SCORE){
                        showResult(3);
                }else if (p2TrashEvaded == MAX_SCORE){
                        showResult(2);
                }else{
                        showResult(1);
                }
                saveGame();
        }
}

void showResult(int playerNo){
        /* sets parameters for displaying the screen corresponding 
           to the number of players that won */
        int xOffset, yOffset, rows, cols;
        char *art;
        if (playerNo == 0){
                xOffset = 50;
                yOffset = 21;
                rows = GAMEOVER_R;
                cols = GAMEOVER_C;
                art = gameOver;
        }else if (playerNo == 1){
                xOffset = 58;
                yOffset = 21;
                rows = PWON_R;
                cols = P1WON_C;
                art = p1Won;
        }else if (playerNo == 2){
                xOffset = 56;
                yOffset = 21;
                rows = PWON_R;
                cols = P2WON_C;
                art = p2Won;
        }else{
                xOffset = 45;
                yOffset = 16;
                rows = BOTHWON_R;
                cols = BOTHWON_C;
                art = bothWon;
        }

        // draws the result onto the window
        wipeWindow();
        for (int i = 0; i < rows; i++){
                for (int j = 0; j < cols; j++){
                        mvprintw(i+yOffset, j+xOffset, "%c", art[i * cols + j]);
                }
        }
        
        // displays prompt if players discovered new species
        if (encyclopediaLeveledUp){
                mvprintw(31, 57, "Check Your Encyclopedia For New Entries");
        }
        // provides instructions to play again or quit game
        mvprintw(28, 66, "Press R to Play Again");
        mvprintw(29, 68, "Press Q to Quit");
        refresh();

        // receives input to fulfill user request
        char input;
        while (1){
                input = getc(stdin);
                if (input == 'r' || input == 'R'){
                        runGame();
                }else if (input == 'q' || input == 'Q'){
                        break;
                }
        }
}

void runGame(void){
        // resets game settings and sets home page
        loadInfo();
        chooseGuest();
        setHomePage();

        p1.x = 40;
        p1.y = 16;
        p1TrashEvaded = 0; 
        p1IsAlive = 1;
        p1IsDazed = 0; 
        p1DazedCount = 0;
        fish1[5] = 'o';

        p2.x = 50;
        p2.y = 25;
        p2TrashEvaded = 0;
        p2IsAlive = 1;
        p2IsDazed = 0;
        p2DazedCount = 0;
        fish2[5] = 'o';

        encyclopediaLeveledUp = 0;
        numOfScenery = 0;
        numOfTrash = 0;
        sceneX = 0;

        // runs all the required processes for the game until it ends
        while(running){
                manageObjects();
                drawScene();
                moveScene();
                endGame();
        }
}
