#include "main.h"


static off_t IEB_DOT[MAX_DOT] = {
	IEB_DOT1,
	IEB_DOT2,
	IEB_DOT3,
	IEB_DOT4,
	IEB_DOT5
};
static off_t IEB_FND[MAX_FND] = {
	IEB_FND0,
	IEB_FND1,
	IEB_FND2,
	IEB_FND3,
	IEB_FND4,
	IEB_FND5,
	IEB_FND6,
	IEB_FND7
};



static int fd;
static int map_counter = 0;
static void * map_data[100];
static selection_t sel;

// for check answer
int call;
int usr_input[8];

int sol[8][8] = {	{0,4,0,-1,-1,-1,-1,-1}, //3
					{0,4,1,0,-1,-1,-1,-1}, //4
					{0,3,4,0,-1,-1,-1,-1}, //4
					{0,3,4,1,0,-1,-1,-1}, //5
					{0,4,1,2,0,-1,-1,-1}, //5
					{0,4,3,2,1,0,-1,-1}, //6
					{0,3,4,3,4,1,0,-1}, //7
					{0,1,4,3,4,1,2,0}, //8
				};

truth_t dot_corr(int inp[], int sol[]) {
	int i;	
	for(i = 0; i < 8; i++) {
		if(inp[i] != sol[i]) {return FALSE; }
	}
	return TRUE;
}

// declare life as a static variable
static int life = 3; // default life = 3
static int level = 1;


static truth_t enter = FALSE;

// for count down thread
static pthread_t p_thread, startgame_th;
static truth_t countEnd = FALSE;
void *count_function(){
	count_down();
	life--;
	// if count down end, stop user input
	pthread_cancel(startgame_th);
}

// for user input thread
void *input_function(){
	while(start_game()==TRUE){}
	enter = TRUE;
}

int main() {
	
	int i;
	short * led, * dot[MAX_DOT], * fnd[MAX_FND];
	short * clcd_cmd, * clcd_data, * keypad_out, * keypad_in, * dots_type;
	
	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd == -1) {
		fprintf(stderr, "Cannot open /dev/mem file");
		exit(EXIT_FAILURE);
	}
	
	led = mapper(IEB_LED, PROT_WRITE);
	for( i=0; i<MAX_DOT; i++ ) {
		dot[i] = mapper(IEB_DOT[i], PROT_WRITE);
	}
	for( i=0; i<MAX_FND; i++ ) {
		fnd[i] = mapper(IEB_FND[i], PROT_WRITE);
	}
	clcd_cmd  = mapper(IEB_CLCD_CMD, PROT_WRITE);
	clcd_data = mapper(IEB_CLCD_DATA, PROT_WRITE);
	dots_type = mapper(IEB_DOT_TYPE, PROT_WRITE);
	
	init_led(led);
	init_dot(dot, dots_type);
	init_fnd(fnd);
	init_clcd(clcd_cmd, clcd_data);

	sel.start == 0;
	
	while( logic() == TRUE ) {}
	
	unmapper();
	close(fd);
	return 0;
}

short * mapper(off_t offset, int prot) {
	map_data[map_counter] = mmap(NULL, sizeof(short), prot, MAP_SHARED, fd, offset);
	if ( map_data[map_counter] == MAP_FAILED ) {
		fprintf(stderr, "Cannot do mmap()");
		emergency_closer();
	}
	return (short *)map_data[map_counter++];
}

void unmapper() {
	int i;
	for( i=0; i<map_counter; i++) {
		munmap(map_data[i], sizeof(short));
	}
	map_counter = 0;
}

void emergency_closer() {
	unmapper();
	close(fd);
	exit(EXIT_FAILURE);
}




truth_t logic(){
	pthread_join(startgame_th, NULL);
	if( sel.start == 0 ) { game_start_screen(); }
	else if( sel.exit == 1 ) { return FALSE; }
	else if( sel.game == 1 ) { game_mode();}
	return TRUE;
}


void game_start_screen() {
	int i;   char buf;

	led_clear();
	dot_clear();
	fnd_clear();
	clcd_clear_display();

	if(life<=0){
		clcd_write_string("GAME OVER");
	}else if(level>8){
		clcd_write_string("CONGRATS!       RESTART: s");
	}else{
		clcd_write_string("Welcome to      Hamburger World");
	}
	

	printf("\n");
	printf("*********** Select device **********\n");
	printf("*       press 's' to start game    *\n");
	printf("*       press 'e' to exit game     *\n");
	printf("************************************\n\n");
	scanf("%c", &buf);

	if( buf == 's' ) { 
		life = 3;
		level = 1;
		sel.game = 1;
	}

	if( buf == 'e'){
		sel.exit = 1;
	}
}

void game_mode(){

	if(life <= 0){
		sel.start = 0;
		return;
	}

	if(level <=8){
		life_count(life);

		// initialize usr_input
		for(int i=0; i<8; i++){
			usr_input[i] = -1;
		}
		call = 0;
		enter = FALSE;

		setup_game();

		int count_down_id = pthread_create(&p_thread, NULL, count_function, NULL);
		int startgame_id = pthread_create(&startgame_th, NULL, input_function, NULL);

		// while(start_game() == TRUE){}
		// wait until user input function end
		pthread_join(startgame_th, NULL);

		// user enter q & time over x
		if(enter){
			pthread_cancel(p_thread);
			fnd_clear();
			dot_clear();
			sel.game = 1;
		}else{
			// time over o
			fnd_clear();
			dot_clear();
			clcd_clear_display();
			clcd_write_string("Time Over!");
			usleep(1000000);
		}

		return;

	}else{
		sel.start = 0;
		return;
		
	}
	
}

// show hamburger display
void setup_game(){
	led_level(level);
	clcd_clear_display();
	clcd_write_string("Ready?");
	usleep(1000000);
	clcd_clear_display();
	clcd_write_string("Go!");
	dot_display(sol[level-1]);

	clcd_clear_display();
	clcd_write_string("1.BR 2.LET      3.TMT 4.CHZ 5.PAT");
}


// for check user input
void right () {
	clcd_clear_display();
	if(level<8){
		clcd_write_string("You're right!      LEVEL UP");
	}
	else{
		clcd_write_string("You're GENIUS");
	}
	
	level++;
	usleep(1000000);
		
}

void wrong () {
	clcd_clear_display();
	clcd_write_string("You're wrong");
	life--;
	usleep(1000000);
}

void wrong_input() {
	clcd_clear_display();
	clcd_write_string("Invalid input");
	usleep(1000000);

}

// get user input
truth_t start_game(){
	
	int key_count, key_value = -1;
	key_count = keyboard_read(&key_value);
	
	if(input_check(key_count, &key_value)) {
		if (call >=8 && key_value == 5){	
			enter = TRUE;		
			if( dot_corr(usr_input, sol[level - 1]) ) {right();}
			else{wrong();}
			return FALSE;

		} else if (call >=8 && key_value != 5) {
			wrong();
			return FALSE;
		}
		if (key_value != 5) {
			usr_input[call] = key_value;
			dot_write(key_value);
			call+=1;
			return TRUE;
		
		} else {
			enter = TRUE;
			if( dot_corr(usr_input, sol[level - 1]) ) {right();}
			else{wrong();}
			return FALSE;
		}		
	}
	else{
		if(key_count >= 2) exit(0);
		wrong_input();
		
		return FALSE;
	}
}