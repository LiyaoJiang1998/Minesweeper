//Name: LIYAO JIANG

#include <Arduino.h>
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <math.h>

#define TFT_DC 9
#define TFT_CS 10
#define SD_CS 6

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

// touch screen pins, obtained from the documentaion
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM  5  // can be a digital pin
#define XP  4  // can be a digital pin

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000

//initialize the TouchScreen
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//initialize the input of joystick
#define JOY_VERT  A1 // should connect A1 to pin VRx
#define JOY_HORIZ A0 // should connect A0 to pin VRy
#define JOY_SEL   2

#define pushbutton 3 //pushbutton pin

//constants related to joystick
#define JOY_CENTER   512
#define JOY_DEADZONE 64

//define size of cursor
#define CURSOR_SIZE 10

struct block {
  bool open;
  uint8_t flag;
  bool mine;
  uint8_t bombnum;
};

//initialize the cursor position on the display
//and the variables for storing previous postion of cursor
int cursorX=0;
int cursorY=0;
int precursorX=0;
int precursorY=0;
//two booleans will be used for checking if the joystick had moved
bool changeposx =0;
bool changeposy =0;

// forward declaration
void menu();
void redrawCursor();
void grid();
void statedraw(uint8_t x,uint8_t y,uint8_t state,uint8_t bombnum);
void scorepannal(int Mine,int Flag);

int minenum = 0;
int randx = 0;
int randy = 0;
int firstclickx = 0;
int firstclicky = 0;

unsigned long startime = 0;//time varialbe for millis

void setup() {
  //standard initialize process of serial monitor, joystick
  //SD card, rotation of screen
  init();

  Serial.begin(9600);

	pinMode(JOY_SEL, INPUT_PULLUP);
  pinMode(pushbutton,INPUT_PULLUP);
  pinMode(A7,INPUT);//pin for random number
	tft.begin();

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");

	tft.setRotation(3);

  //open main menu first
  menu();

  //game started
  scorepannal(9,0);
  grid();//initialize the grid

  //initially all blocks unopened
  for(int i=0;i<9;i++){
    for(int j=0;j<9;j++){
      statedraw(i,j,0,0);
    }
  }
  redrawCursor();
}

//count the time after first click on a block
//and display on tft. using millis function
int timing(unsigned long startime,int prevsecond){
  unsigned long currtime = millis() - startime;
  int hour =  ((currtime/1000)/60) /60;
  int minute = ((currtime/1000)/60) %60;
  int second = (currtime/1000)%60;
  //only refresh the screen when the second number changed
  if(prevsecond != second){
    // Serial.print(hour);
    // Serial.print(":");
    // Serial.print(minute);
    // Serial.print(":");
    // Serial.println(second);
    tft.fillRect(235,200,90,30,ILI9341_WHITE);
    tft.setCursor(235,200);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_BLACK);
    tft.print(hour);
    tft.print(":");
    tft.print(minute);
    tft.print(":");
    tft.print(second);
  }
  return second;
}

bool checkTouch() {
	TSPoint touch = ts.getPoint();

	if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
		// no touch, just quit
		return 0;
	}

	// get the y coordinate of where the display was touched
	// remember the x-coordinate of touch is really our y-coordinate
	// on the display
	int touchY = map(touch.x, TS_MINX, TS_MAXX, 0, TFT_HEIGHT - 1);

	// need to invert the x-axis, so reverse the
	// range of the display coordinates
	int touchX = map(touch.y, TS_MINY, TS_MAXY, TFT_WIDTH - 1, 0);

  // when face touched
  if( ((265< touchX) && (touchX < 265+40)) && ((10< touchY) && (touchY <10+40))){
    return 1;
  }
  else{
    //no touch on the smile face
    return 0;
  }
}

void(*resetFunc) (void) = 0;

void menu(){
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextWrap(0);
  tft.setCursor(20,20);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_GREEN,ILI9341_BLACK);
  tft.print("Minesweeper!!");
  tft.setTextSize(2);
  tft.setCursor(20,200);
  tft.setTextColor(ILI9341_RED,ILI9341_BLACK);
  tft.print("Touch to start a game...");
  //check for touch
  while(true){
    TSPoint touch = ts.getPoint();
    if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
      // no touch, just quit
      continue;
    }
    else{
      tft.fillScreen(ILI9341_WHITE);
      return;
    }
  }
}

void redrawCursor() {
  tft.drawRect(precursorX*25,precursorY*25,25,25,ILI9341_BLACK);
  tft.drawRect(cursorX*25, cursorY*25,25, 25, ILI9341_RED);
}

void scorepannal(int Mine,int Flag){
  //fill the rightmost 80 columns to be white
  tft.fillRect(240,0,80,190,ILI9341_WHITE);
  //draw the rating selector buttons
  tft.drawRect(265,10,40,40,ILI9341_GREEN);
  tft.drawRect(265,85,40,30,ILI9341_RED);
  tft.drawRect(265,160,40,30,ILI9341_RED);

  tft.setTextColor(ILI9341_BLACK, 0xFFFF);
  tft.setTextWrap(false);
  //display the selected star level
  tft.setTextSize(2);
  tft.setCursor(267,20);
  tft.print("-_-"); // wining: "^_^" losing "~_~"
  tft.setTextSize(3);
  tft.setCursor(267,90);
  tft.print(Mine);
  tft.setCursor(267,165);
  tft.print(Flag);

  tft.setTextSize(2);
  tft.setCursor(265,60);
  tft.print("Mine");
  tft.setCursor(265,135);
  tft.print("Flag");
}

//draw the black gridlines on white background
void grid(){
  tft.fillRect(0,0,240,240,ILI9341_WHITE);
  for(int i=0; i<9;i++){
    for(int j=0;j<9;j++){
      tft.drawRect(i*25,j*25,25,25,ILI9341_BLACK);
    }
  }
}

//block coordinates (x,y)
//mode: 0-unoppened 1-mine 2-flag 3-unflag 4-safe
//bombnum: number of bombs near the block
void statedraw(uint8_t x,uint8_t y,uint8_t mode,uint8_t bombnum){
  //mode 1: unopended block
  if(mode == 0){
    tft.fillRect(1+x*25,1+y*25,23,23,ILI9341_WHITE);
  }
  //block opended contains mine
  if(mode == 1){
    tft.fillRect(x*25+1,y*25+1,23,23,ILI9341_BLACK);
    tft.setCursor(25*x+1,25*y+1);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print((char)88);
  }
  //flag unopended block
  else if(mode == 2){
    tft.fillRect(x*25+1,y*25+1,23,23,ILI9341_RED);
    tft.setCursor(25*x+1,25*y+1);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_BLACK, ILI9341_RED);
    tft.print((char)33);
  }
  //empty a questionmarked block
  else if(mode == 3){
    tft.fillRect(1+x*25,1+y*25,23,23,ILI9341_WHITE);
  }
  //opended safe block with nearby bombnum of bombs
  else if(mode == 4){
    tft.fillRect(x*25+1,y*25+1,23,23,ILI9341_BLACK);
    tft.setCursor(25*x+1,25*y+1);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    //leave it blank if bombnum =0
    if(bombnum !=0){
      tft.print(bombnum);
    }
  }
  //mode 5: question mark a flagged block
  else if(mode == 5){
    tft.fillRect(x*25+1,y*25+1,23,23,ILI9341_BLUE);
    tft.setCursor(25*x+1,25*y+1);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLUE);
    tft.print((char)63);
  }
}

void minegenerate(block data[9][9]){
  randomSeed(analogRead(A7));
  randx = random(0,9); //random number from 0 to 8
  randy = random(0,9); //random number from 0 to 8
  if((randx != firstclickx) && (randy != firstclicky) && (data[randx][randy].mine != 1)){
    data[randx][randy].mine = 1;
    minenum ++;
  }
}

//show all mines when lose
void showmine(block data[9][9]){
  for(int i=0;i<9;i++){
    for(int j=0;j<9;j++){
      if(data[i][j].mine == 1){
        statedraw(i,j,1,0);
      }
    }
  }
}

//calculate the number of mine around given block
uint8_t around(int i,int j,block data[9][9]){
  uint8_t num = 0;
  if( ((i-1) >= 0)&&((i-1) <= 8) && ((j-1) >= 0)&&((j-1) <= 8) && data[i-1][j-1].mine == 1){
    num ++;
  }
  if( ((i) >= 0)&&((i) <= 8) && ((j-1) >= 0)&&((j-1) <= 8) && data[i][j-1].mine == 1){
    num ++;
  }
  if( ((i+1) >= 0)&&((i+1) <= 8) && ((j-1) >= 0)&&((j-1) <= 8) && data[i+1][j-1].mine == 1){
    num ++;
  }
  if( ((i+1) >= 0)&&((i+1) <= 8) && ((j) >= 0)&&((j) <= 8) && data[i+1][j].mine == 1){
    num ++;
  }
  if( ((i+1) >= 0)&&((i+1) <= 8) && ((j+1) >= 0)&&((j+1) <= 8) && data[i+1][j+1].mine == 1){
    num ++;
  }
  if( ((i) >= 0)&&((i) <= 8) && ((j+1) >= 0)&&((j+1) <= 8) && data[i][j+1].mine == 1){
    num ++;
  }
  if( ((i-1) >= 0)&&((i-1) <= 8) && ((j+1) >= 0)&&((j+1) <= 8) && data[i-1][j+1].mine == 1){
    num ++;
  }
  if( ((i-1) >= 0)&&((i-1) <= 8) && ((j) >= 0)&&((j) <= 8) && data[i-1][j].mine == 1){
    num ++;
  }
  return num;
}

//calculate number of bombnum for every block
void bomb(block data[9][9]){
  uint8_t num;
  for(int i=0;i<9;i++){
    for(int j=0;j<9;j++){
      num = around(i,j,data);
      data[i][j].bombnum = num;
    }
  }
}

//recursively open the eight nearby 0 minenum blocks
void blockopen(block data[9][9],int i,int j){
  //check if the block is valid coordinate and unopended
  if(data[i][j].open == 0 && i >= 0 && i <= 8 && j >= 0 && j<= 8){
    //open the block itself
    data[i][j].open = 1;
    //if it has 0 bomb around it
    //open all 8 adjacent blocks and with the recursion open
    if(data[i][j].bombnum == 0 && data[i][j].mine == 0){
      blockopen(data,i-1,j-1);
      blockopen(data,i,j-1);
      blockopen(data,i+1,j-1);
      blockopen(data,i+1,j);
      blockopen(data,i+1,j+1);
      blockopen(data,i,j+1);
      blockopen(data,i-1,j+1);
      blockopen(data,i-1,j);
    }
  }
}

void processjoystick(block data[9][9]){
  int prebuttonVal = 1;
  int buttonVal = 1;
  int pVal = 1;
  int prepVal =1;
  int flag = 0;
  int mine = 9;
  bool firstclick = 1;
  int prevsecond =0;

  while(true){
    //timing function
    //after first click, the timing starts
    if(startime !=0){
      prevsecond = timing(startime,prevsecond);
    }

    //check touch reset
    bool touch = checkTouch();
    if(touch == 1){
      resetFunc();
      break;
    }

    //Winning condition(all safe block opended)
    bool win = 1;
    for(int i=0;i<9;i++){
      for(int j=0;j<9;j++){
        if(data[i][j].mine == 0 && data[i][j].open == 0){
          //if there is still unopended safe block
          //we still not win
          win = 0;
        }
      }
    }
    //if all safe block opended, win!
    if(win == 1){
      tft.setTextColor(ILI9341_BLACK, 0xFFFF);
      tft.setTextSize(2);
      tft.setCursor(267,20);
      tft.print("^_^");
      return;
    }

    prebuttonVal = buttonVal;
    prepVal = pVal;
    // reading from joystick
    int xVal = analogRead(JOY_HORIZ);
    int yVal = analogRead(JOY_VERT);
    buttonVal = digitalRead(JOY_SEL);//joystick button
    pVal = digitalRead(pushbutton);//pushbutton
    //if joystick pressed down
    //flag/unflag the selecte block
    //it has to be unopended
    if((data[cursorX][cursorY].open != 1)&& buttonVal == 0 && (prebuttonVal != buttonVal)){
      //flag a empty block
      if((flag != 9) && data[cursorX][cursorY].flag == 0){
        data[cursorX][cursorY].flag = 1;
        statedraw(cursorX,cursorY,2,0);
        flag = constrain(flag+1,0,9);
        mine = constrain(mine-1,0,9);
      }
      //question mark a flageed block
      else if(data[cursorX][cursorY].flag == 1){
        data[cursorX][cursorY].flag = 2;
        statedraw(cursorX,cursorY,5,0);
        flag = constrain(flag-1,0,9);
        mine = constrain(mine+1,0,9);
      }
      //empty a questioned marked block
      else if(data[cursorX][cursorY].flag == 2){
        data[cursorX][cursorY].flag = 0;
        statedraw(cursorX,cursorY,3,0);
      }
      //update the scorepannal
      scorepannal(mine,flag);
      continue;
    }

    //if pushbutton pressed down
    //open the selected block
    //it has to be unopended
    if((data[cursorX][cursorY].open != 1)&& pVal == 0 && (prepVal != buttonVal)){
      //first click safe policy
      if(firstclick == 1){
        //start timing after first click!
        startime = millis();
        //firstclick block won't have mine
        firstclickx = cursorX;
        firstclicky = cursorY;
        //generates 9 random mines after first click
        while(minenum < 9){
          minegenerate(data);
        }
        firstclick = 0;
        //calculate the bombnum around every block after firstclick
        bomb(data);
        // //test show all the bombnum:
        // for(int i=0;i<9;i++){
        //   for(int j=0;j<9;j++){
        //     statedraw(i,j,4,data[i][j].bombnum);
        //   }
        // }
        // showmine(data);

      }

      //Recursively open block follow the rule
      blockopen(data,cursorX,cursorY);
      flag =0;
      for(int i=0;i<9;i++){
        for(int j=0;j<9;j++){
          //open a block with mine,losing!
          if((data[i][j].open==1) && (data[i][j].mine==1)){
            //display all mines
            showmine(data);
            tft.setTextColor(ILI9341_BLACK, 0xFFFF);
            tft.setTextSize(2);
            tft.setCursor(267,20);
            tft.print("~_~");
            return;
          }
          if(data[i][j].open == 1 && data[i][j].flag == 0){
            statedraw(i,j,4,data[i][j].bombnum);
          }
          //opended block with flag
          else if(data[i][j].open == 1 && data[i][j].flag == 1){
            data[i][j].flag = 0;
            statedraw(i,j,4,data[i][j].bombnum);
          }
          //opened block with question mark
          else if(data[i][j].open == 1 && data[i][j].flag == 2){
            data[i][j].flag = 0;
            statedraw(i,j,4,data[i][j].bombnum);
          }
          //if block is not opended and with flag, count the flag
          if(data[i][j].flag == 1){
            flag++;
          }
        }
      }
      mine = 9-flag;
      scorepannal(mine,flag);
      continue;
    }

    //control of cursor
    // joystick moved down
    if (yVal < JOY_CENTER - JOY_DEADZONE) {
      //the constain function can make sure the cursor moves with in the screen
      cursorY = constrain(cursorY-1,0,8); // decrease the y coordinate of the cursor
      //the y coordinate changed
      changeposy= 1;
    }
    // joystick moved up
    else if (yVal > JOY_CENTER + JOY_DEADZONE) {
      cursorY = constrain(cursorY+1,0,8);
      changeposy= 1;
    }
    // if the y coordinate was not changed
    else{changeposy= 0;}

    // remember the x-reading increases as we push left
    // joystick pushed to the left
    if (xVal > JOY_CENTER + JOY_DEADZONE) {
      cursorX = constrain(cursorX-1,0,8);
      changeposx= 1;
    }
    // joystick pushed to the right
    else if (xVal < JOY_CENTER - JOY_DEADZONE) {
      cursorX = constrain(cursorX+1,0,8);
      changeposx= 1;
    }
    // if the x coordinate was not changed
    else{changeposx= 0;}

    //using the two boolean variables to determined if the joystick moved the
    //cursor, only redraw when it is moving to avoid flickering
    if(changeposx || changeposy){
      redrawCursor();
      delay(150);
    }
    //save the current cursor position as previous position
    //for redrawing the column
    precursorY = cursorY;
    precursorX = cursorX;
  }
}


int main() {
  while (true) {
    setup();
    //varaible that stores the data of each block
    block data[9][9];
    //also zero out the data 2d array
    for(int i=0;i<9;i++){
      for(int j=0;j<9;j++){
        data[i][j].open = 0;
        data[i][j].flag = 0;
        data[i][j].mine = 0;
        data[i][j].bombnum = 0;
      }
    }
    processjoystick(data);

    while(true){
      bool touch = checkTouch();
      if(touch == 1){
        resetFunc();
        break;
      }
    }

    // //test the statedraw function
    // statedraw(8,8,0,4);
    // statedraw(7,7,1,4);
    // statedraw(6,6,2,4);
    // delay(1000);
    // statedraw(6,6,3,4);
    // statedraw(5,5,4,4);
    //
    // //test zero out
    // for(int i=0;i<9;i++){
    //   for(int j=0;j<9;j++){
    //     Serial.print("(");
    //     Serial.print(i);
    //     Serial.print(",");
    //     Serial.print(j);
    //     Serial.print(")");
    //     Serial.print(": ");
    //     Serial.print(data[i][j].open);
    //     Serial.print(data[i][j].flag);
    //     Serial.print(data[i][j].mine);
    //     Serial.println(data[i][j].bombnum);
    //   }
    // }

    Serial.end();
    return 0;
  }
}
