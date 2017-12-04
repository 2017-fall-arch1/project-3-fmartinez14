/** \file Pong.c
 *  This is a simple pong game to be played on the MSP430.
 *  This file creates two paddles and a ball.
 *  Code is explained more in depth for each line. 
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6


AbRect player2 ={abRectGetBounds,abRectCheck,{5,15}}; // * Player 2 Paddle */
AbRect rect10 = {abRectGetBounds, abRectCheck, {5,15}}; /* Player 1 Paddle */
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 0};
unsigned char currentScorePlayer1[] = "0"; //Score for player 1
unsigned char currentScorePlayer2[] = "0"; //Score for player 2
int direction = "0";

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer layer4 = {
  (AbShape *)&rightArrow,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
 // COLOR_PINK,
  0
};
  

Layer layer3 = {		/**< Layer with Player 2 */
  (AbShape *)&player2,
  {15, (screenHeight/2)}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,  
  &layer4,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &layer3,
};


Layer layer1 = {		/**< Layer with player 1*/
  (AbShape *)&rect10,
  {screenWidth-15, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &fieldLayer,  
};

Layer layer0 = {		/**< The little bouncy ball  */
  (AbShape *)&circle8,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE,
  &layer1,  
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */


typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer3, {0,0}, 0 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {0,0}, &ml3}; 
MovLayer ml0 = { &layer0, {1,1}, &ml1 }; 
//MovLayer ml4= { &layer5, {0,0},0};


void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  


void movePaddle(MovLayer *myPlayer,  int myDirection){ //Modes the paddle depending on the button pressed.
  myPlayer-> velocity.axes[1] = 3*(myDirection);
}

void bouncyBall(Region *fence,MovLayer *ball){ //Checks for collision between the ball and paddle.
 int collides = abShapeCheck(ball->layer->abShape, &ball->layer->pos, &ml1.layer->pos);
 int collides2= abShapeCheck(ball->layer->abShape,&ball->layer->pos,&ml3.layer->pos);
 if(collides){ //If the first paddle collides with the ball, throw the ball in the opposite direction.
   int velocityX = ball->velocity.axes[0] = -ball->velocity.axes[0];
   int velocityY = ball->velocity.axes[1] = -ball->velocity.axes[1];
   ball-> velocity.axes[1] = 3*(1);
   ball-> layer -> pos.axes[0] +=  (2*velocityX);
   ball -> layer -> pos.axes[1] += (2*velocityY);
   buzzer_set_period(9000,100); //Make a sound if the ball is hit
   delayPlease(50);
   buzzer_set_period(2,1);
 }
 if(collides2){ //If the second paddle collides with the ball throw the ball in the opposite direction.
   int velocityX = ball->velocity.axes[0] = -ball->velocity.axes[0];
   int velocityY = ball->velocity.axes[1] = -ball->velocity.axes[1];
   ball->velocity.axes[1] = 3*(-1);
   ball-> layer -> pos.axes[0] +=  (2*velocityX);
   ball -> layer -> pos.axes[1] += (2*velocityY);
   buzzer_set_period(2000,3);
   delayPlease(50); //Makes a sound if the ball is hit
   buzzer_set_period(2,1);
 }
 
}


//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if (shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]){
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
        buzzer_set_period(6000,9);
        delayPlease(50);
        buzzer_set_period(2,1);
        if(shapeBoundary.topLeft.axes[0] < fence-> topLeft.axes[0])
          currentScorePlayer2[0]++; //Adds to score if the ball hits the wall
      }
	if (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]){
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
        buzzer_set_period(6000,9);
        delayPlease(50);
        buzzer_set_period(2,1);
        if(shapeBoundary.botRight.axes[0] > fence->botRight.axes[0])
	  currentScorePlayer1[0]++; //Adds to score if the ball hit the wall.
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


u_int bgColor = COLOR_BLUE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  buzzer_init();
  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  drawString5x7(screenWidth/2 - 10,0,"PONG", COLOR_YELLOW, COLOR_BLUE);

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  bouncyBall(&fieldFence,&ml0);
  unsigned int mySwitch = p2sw_read();
  if (count == 15) {
    redrawScreen=1; //Redraws the screen with the most updated scores.
    drawString5x7(10,screenHeight-9,"Score: P1-", COLOR_YELLOW, COLOR_BLUE);
    drawString5x7(70,screenHeight-9,currentScorePlayer1,COLOR_RED,COLOR_BLUE);
    drawString5x7(80,screenHeight-9,"P2-", COLOR_YELLOW,COLOR_BLUE);
    drawString5x7(110,screenHeight-9,currentScorePlayer2,COLOR_RED,COLOR_BLUE);
    mlAdvance(&ml0, &fieldFence);
     if (~mySwitch & 1){ //SW 1
      moveUp();
      movePaddle(&ml3,direction);
}
     else if( ~mySwitch & 2){ // SW2
      moveDown();
      movePaddle(&ml3,direction);
}
     else if(~mySwitch & 7){ // SW 3
      moveUp();
      movePaddle(&ml1,direction);
}
     else if(~mySwitch & 8){ //SW 4
      moveDown();
      movePaddle(&ml1,direction);
}
    else{ //If none is pressed, dont move the paddle.
        dontMove();
	movePaddle(&ml3,direction);
        movePaddle(&ml1,direction);
}
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}

